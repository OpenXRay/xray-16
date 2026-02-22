// xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"  // For loading bindless shaders
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"  // For IndirectDrawArgs struct
#include "Layers/xrRender/Backend/D3D12Backend.h"  // For SM6 bindless descriptor heap
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"  // For terrain rendering
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"  // For variant textures
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "PassCommon.h"
#include "xrCore/FMesh.hpp"  // For MT_NORMAL, MT_TREE, etc.

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

void InitializeForwardResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, ForwardColorPassState& state)
{
    if (state.bindlessInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

    if (!vsResult.handle || !psResult.handle) {
        Msg("! [BindlessForward] Failed to load shaders");
        return;
    }

    state.bindlessVS = vsResult.handle;
    state.bindlessPS = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    state.linearSampler = nvDevice->createSampler(samplerDesc);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(10),
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),
    };
    state.bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.bindlessInputLayout = nvDevice->createInputLayout(attrs, attrCount, state.bindlessVS);

    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);
    if (!drawIndexBuffer) {
        Msg("! [BindlessForward] Failed to create draw index buffer");
        return;
    }

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.bindlessVS;
    pipeDesc.PS = state.bindlessPS;
    pipeDesc.inputLayout = state.bindlessInputLayout;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    if (bindlessLayout) {
        pipeDesc.bindingLayouts = { state.bindlessLayout, bindlessLayout };
    } else {
        pipeDesc.bindingLayouts = { state.bindlessLayout };
    }

    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    state.bindlessPipeline = cache.GetOrCreatePipeline("ForwardColor", pipeDesc, framebuffer->getFramebufferInfo(), nvDevice);
    if (!state.bindlessPipeline) {
        Msg("! [BindlessForward] Failed to create pipeline");
        return;
    }

    QueryBindingLayoutFromPipeline(state.bindlessPipeline, state.bindlessLayout);

    state.bindlessInitialized = true;
    Msg("* [BindlessForward] Pipeline initialized");
}

static void renderBindlessForward(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* normalRT,
    nvrhi::ITexture* baseColorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache,
    ForwardColorPassState& ps)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!config.UseGPUCulling() || !geometry || geometry->GetBatches().empty())
        return;

    // Finalize any pending materials (register textures to bindless descriptor heap)
    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
        materialCache->FinalizePendingTerrainMaterials(ctx);  // Terrain materials (4-layer detail blending)
    }

    // Upload material buffer to GPU
    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList)
        return;

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAMEBUFFER AND RENDER STATE
    // ═══════════════════════════════════════════════════════
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    if (normalRT)
        fbDesc.addColorAttachment(normalRT);
    if (baseColorRT)
        fbDesc.addColorAttachment(baseColorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto& cache = framegraph::GetPassResourceCache();
    auto framebuffer = cache.GetOrCreateFramebuffer("ForwardColor", fbDesc, nvDevice);

    InitializeForwardResources(device, framebuffer, ps);

    auto lightingCB = cache.GetOrCreateVolatileCB("ForwardColor", "LightingCB", sizeof(LightingConstants), 16, nvDevice);
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("ForwardColor", "StaticGlobalsCB", sizeof(StaticGlobals), 16, nvDevice);
    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);

    auto lightingData = FillLightingConstants();
    cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

    auto staticGlobals = BuildStaticGlobals();

    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();

    auto createBindingSetForSet = [&](const BindlessDrawSet& set) -> nvrhi::BindingSetHandle {
        nvrhi::BindingSetDesc bindDesc;
        bindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
            nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(10, variantTexBuffer.GetBuffer()),
            nvrhi::BindingSetItem::Sampler(0, ps.linearSampler),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(14, set.instanceBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(15, set.compactBatchIndicesBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(16, set.compactMaterialIDBuffer),
        };

        return framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, ps.bindlessLayout, nvDevice);
    };

    // ═══════════════════════════════════════════════════════
    //  RENDER VISIBLE BATCHES
    // ═══════════════════════════════════════════════════════
    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    // ═══════════════════════════════════════════════════════
    //  GPU-DRIVEN CULLED RENDERING
    // ═══════════════════════════════════════════════════════
    // Only visible batches are drawn using indirect draw commands
    // Draw args come from GPU culling's compact buffer
    if (!config.UseGPUCulling() || !config.UseMegaBuffers()) {
        // GPU culling not available - skip bindless forward
        // (regular deferred path will handle rendering)
        return;
    }

    // Validate all required resources before draw loop
    if (!ps.bindlessPipeline || !framebuffer) {
        return;
    }

    nvrhi::GraphicsState state;
    state.pipeline = ps.bindlessPipeline;
    state.framebuffer = framebuffer;

    // SM6.6 bindless: Add the descriptor table from D3D12 backend
    // IDescriptorTable derives from IBindingSet, so we add it to bindings
    auto* backend = device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = nullptr;
    if (backend) {
        bindlessTable = backend->GetBindlessDescriptorTable();
    }

    // Validate draw index buffer exists
    if (!drawIndexBuffer) {
        Msg("! [BindlessForward] Draw index buffer not initialized!");
        return;
    }

    state.vertexBuffers = {
        {config.megaVertexBuffer, 0, 0},    // Slot 0: Per-vertex geometry (stride 48)
        {drawIndexBuffer, 1, 0}           // Slot 1: Per-instance draw indices (stride 4)
    };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    auto drawSet = [&](const BindlessDrawSet& set) {
        if (!set.IsValid())
            return;

        auto bindingSet = createBindingSetForSet(set);
        R_ASSERT2(bindingSet, "Bindless forward binding set creation failed");

        state.bindings = { bindingSet };
        if (bindlessTable) {
            state.addBindingSet(bindlessTable);
        }
        state.indirectParams = set.compactDrawArgsBuffer;
        state.indirectCountBuffer = set.compactCountBuffer;

        cmdList->setGraphicsState(state);
        cmdList->drawIndexedIndirectCount(0, 0, set.totalObjectCount);
    };

    if (config.variantPartition.Enabled()) {
        auto* backendDev = device->GetBackend();
        VariantPartitionDrawConfig vpCfg;
        vpCfg.defaultPipeline = ps.bindlessPipeline.Get();
        vpCfg.inputLayout = ps.bindlessInputLayout;
        vpCfg.passLayout = ps.bindlessLayout;
        vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
        vpCfg.bindlessTable = bindlessTable;
        vpCfg.sampler = ps.linearSampler;
        vpCfg.staticGlobalsCB = staticGlobalsCB;
        vpCfg.lightingCB = lightingCB;
        vpCfg.materialBuffer = matBuffer.GetBuffer();
        vpCfg.variantTexBuffer = variantTexBuffer.GetBuffer();
        vpCfg.instanceBuffer = config.staticSet.instanceBuffer;
        vpCfg.megaVertexBuffer = config.megaVertexBuffer;
        vpCfg.partition = config.variantPartition;
        vpCfg.selectTransparent = false;

        DrawVariantPartition(cmdList, nvDevice, framebuffer, state, vpCfg);

        state.pipeline = ps.bindlessPipeline;
        state.vertexBuffers = {
            {config.megaVertexBuffer, 0, 0},
            {drawIndexBuffer, 1, 0}
        };
    } else {
        drawSet(config.staticSet);
    }
    drawSet(config.dynamicSet);

    // ═══════════════════════════════════════════════════════
    //  TERRAIN RENDERING (4-layer detail blending)
    // ═══════════════════════════════════════════════════════
    // Terrain uses separate pipeline and TerrainMaterialBuffer
    if (config.HasTerrain() && config.UseMegaBuffers()) {
        R_ASSERT2(config.UseTerrainCompaction(), "Terrain compaction buffers missing");

        // Lazy init terrain pipeline
        if (!ps.terrainInitialized) {
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            if (shaderLoader) {
                auto terrainPsResult = shaderLoader->LoadPixelShader("bindless_terrain", "main");
                if (terrainPsResult.handle) {
                    ps.terrainPS = terrainPsResult.handle;

                    nvrhi::BindingLayoutDesc terrainLayoutDesc;
                    terrainLayoutDesc.visibility = nvrhi::ShaderType::All;
                    terrainLayoutDesc.bindings = {
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(9),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(10),
                        nvrhi::BindingLayoutItem::Sampler(0),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),
                    };
                    ps.terrainLayout = nvDevice->createBindingLayout(terrainLayoutDesc);

                    if (ps.terrainLayout && ps.bindlessVS) {
                        nvrhi::GraphicsPipelineDesc terrainPipeDesc;
                        terrainPipeDesc.VS = ps.bindlessVS;
                        terrainPipeDesc.PS = ps.terrainPS;
                        terrainPipeDesc.inputLayout = ps.bindlessInputLayout;

                        auto* backendDevice = device->GetBackend();
                        nvrhi::IBindingLayout* bindlessLayout = backendDevice ? backendDevice->GetBindlessLayout() : nullptr;
                        if (bindlessLayout) {
                            terrainPipeDesc.bindingLayouts = { ps.terrainLayout, bindlessLayout };
                            Msg("* [TerrainForward] Pipeline created WITH bindless layout");
                        } else {
                            terrainPipeDesc.bindingLayouts = { ps.terrainLayout };
                            Msg("! [TerrainForward] Pipeline created WITHOUT bindless layout - textures will be black!");
                        }

                        terrainPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                        terrainPipeDesc.renderState.depthStencilState.depthTestEnable = true;
                        terrainPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
                        terrainPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                        terrainPipeDesc.renderState.rasterState.frontCounterClockwise = false;
                        terrainPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                        ps.terrainPipeline = cache.GetOrCreatePipeline("ForwardColor_Terrain", terrainPipeDesc, framebuffer->getFramebufferInfo(), nvDevice);
                        if (ps.terrainPipeline) {
                            ps.terrainInitialized = true;
                            Msg("* [BindlessForward] Terrain pipeline initialized");
                        }
                    }
                }
            }
        }

        // Render terrain if pipeline is ready
        R_ASSERT2(ps.terrainInitialized && ps.terrainPipeline, "Terrain pipeline not initialized");
        if (ps.terrainInitialized && ps.terrainPipeline) {
            // Upload terrain materials
            auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
            terrainMatBuffer.Upload(ctx);

            // Validate all required terrain-specific buffers are available
            R_ASSERT2(terrainMatBuffer.GetBuffer() && config.terrainInstanceBuffer &&
                          config.terrainCompactBatchIndicesBuffer && config.terrainCompactMaterialIDBuffer &&
                          config.terrainCompactDrawArgsBuffer && config.terrainCompactCountBuffer,
                "Terrain buffers not ready for compaction rendering");

            // Create terrain binding set (includes TerrainMaterialBuffer at t9)
            // NOTE: Terrain uses its own instance/batch buffers, not the regular ones
            nvrhi::BindingSetDesc terrainBindDesc;
            terrainBindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),   // b2 - static_globals
                nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),        // b4 - lighting
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),  // t8 - regular materials (unused)
                nvrhi::BindingSetItem::StructuredBuffer_SRV(9, terrainMatBuffer.GetBuffer()),  // t9 - terrain materials
                nvrhi::BindingSetItem::StructuredBuffer_SRV(10, variantTexBuffer.GetBuffer()), // t10 - variant textures
                nvrhi::BindingSetItem::Sampler(0, ps.linearSampler),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(14, config.terrainInstanceBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(15, config.terrainCompactBatchIndicesBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(16, config.terrainCompactMaterialIDBuffer),
            };

            auto terrainBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainBindDesc, ps.terrainLayout, nvDevice);
            R_ASSERT2(terrainBindingSet, "Terrain binding set creation failed");

            // Set up terrain graphics state
            nvrhi::GraphicsState terrainState;
            terrainState.pipeline = ps.terrainPipeline;
            terrainState.framebuffer = framebuffer;
            terrainState.bindings = { terrainBindingSet };

            // Add bindless descriptor table
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    terrainState.addBindingSet(bindlessTable);
            }

            terrainState.vertexBuffers = {
                {config.megaVertexBuffer, 0, 0},
                {drawIndexBuffer, 1, 0}
            };
            terrainState.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
            terrainState.indirectParams = config.terrainCompactDrawArgsBuffer;
            terrainState.indirectCountBuffer = config.terrainCompactCountBuffer;
            terrainState.viewport.addViewport(viewport);
            terrainState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

            cmdList->setGraphicsState(terrainState);
            cmdList->drawIndexedIndirectCount(0, 0, config.terrainObjectCount);

        }
    }

    // Transparent geometry is rendered in a separate TransparentPass (after Detail pass)
    // Skinned meshes are rendered in the SkinningPass (see SkinningPassSetup.cpp)
}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle normalInput,
    framegraph::VirtualResourceHandle baseColorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsInput,
    const BindlessForwardConfig& bindlessConfig,
    ForwardColorPassState* state)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height, colorInput, normalInput, baseColorInput, drawArgsInput, bindlessConfig, state](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;
            data.passState = state;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);
            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
            data.normal = passBuilder.write(normalInput, ResourceState::RenderTarget);
            if (baseColorInput.is_valid())
                data.baseColor = passBuilder.write(baseColorInput, ResourceState::RenderTarget);

            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            data.outputs.albedo = data.color;
            data.outputs.normal = data.normal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA (Renders geometry)
        // ═══════════════════════════════════════════════════════
        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;

            if (!depthRT || !colorRT)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (cmdList) {
                cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 1.0f, false, 0);
                if (normalRT)
                    cmdList->clearTextureFloat(normalRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
                if (baseColorRT)
                    cmdList->clearTextureFloat(baseColorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            }

            // Check if we have geometry to render
            if (!data.geometry)
                return;

            // Get draw args buffer through framegraph (proper dependency tracking)
            // This ensures state transition UAV -> IndirectArgument happened
            nvrhi::IBuffer* drawArgsBuffer = nullptr;
            if (data.drawArgsBuffer.is_valid()) {
                drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
            }

            // ═══════════════════════════════════════════════════════
            //  BINDLESS RENDERING PATH (GPU-DRIVEN MULTI-DRAW)
            // ═══════════════════════════════════════════════════════
            // Render static geometry with GPU-driven multi-draw
            // NOTE: Skinned meshes are rendered in SkinningPass (see SkinningPassSetup.cpp)
            renderBindlessForward(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                normalRT,
                baseColorRT,
                depthRT,
                data.bindlessConfig,
                data.materialCache,
                *data.passState
            );
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    return outputs;
}
} // namespace xray::render::RENDER_NAMESPACE::passes
