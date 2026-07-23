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
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "PassCommon.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "xrCore/FMesh.hpp"

namespace xray::render::fg
{
    extern float r__dtex_range;
}

namespace xray::render::fg::passes {

void InitializeForwardResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ForwardColorPassState& state)
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

    state.bindlessLayout = cache.GetOrCreateBindingLayoutFromReflection("ForwardColor", *vsResult.reflection, *psResult.reflection, nvDevice);

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
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    state.bindlessPipeline = cache.GetOrCreatePipeline("ForwardColor", pipeDesc, fbInfo, nvDevice);
    if (!state.bindlessPipeline) {
        Msg("! [BindlessForward] Failed to create pipeline");
        return;
    }

    QueryBindingLayoutFromPipeline(state.bindlessPipeline, state.bindlessLayout);

    auto terrainPsResult = shaderLoader->LoadPixelShader("bindless_terrain", "main");
    if (terrainPsResult.handle) {
        state.terrainPS = terrainPsResult.handle;
        state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "ForwardColor_Terrain", *vsResult.reflection, *terrainPsResult.reflection, nvDevice);

        if (state.terrainLayout) {
            nvrhi::GraphicsPipelineDesc terrainPipeDesc;
            terrainPipeDesc.VS = state.bindlessVS;
            terrainPipeDesc.PS = state.terrainPS;
            terrainPipeDesc.inputLayout = state.bindlessInputLayout;
            if (bindlessLayout)
                terrainPipeDesc.bindingLayouts = { state.terrainLayout, bindlessLayout };
            else
                terrainPipeDesc.bindingLayouts = { state.terrainLayout };
            terrainPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            terrainPipeDesc.renderState.depthStencilState.depthTestEnable = true;
            terrainPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            terrainPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
            terrainPipeDesc.renderState.rasterState.frontCounterClockwise = false;
            terrainPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
            state.terrainPipeline = cache.GetOrCreatePipeline("ForwardColor_Terrain", terrainPipeDesc, fbInfo, nvDevice);
            if (state.terrainPipeline)
                state.terrainInitialized = true;
        }
    }

    state.bindlessInitialized = true;
    Msg("* [BindlessForward] Pipeline initialized");
}

static void renderBindlessForward(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* normalRT,
    nvrhi::ITexture* baseColorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache,
    ForwardColorPassState& ps)
{
    using namespace fg::bindless;

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

    auto lightingCB = cache.GetOrCreateVolatileCB("ForwardColor", "LightingCB", sizeof(LightingConstants), device);
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), device);
    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);

    auto lightingData = FillLightingConstants();
    cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

    auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
    auto* psReflection = shaderLoader->GetCachedReflection("bindless_forward", ".ps");

    auto& clm = ClusteredLightManager::Instance();

    auto createBindingSetForSet = [&](const BindlessDrawSet& set) -> nvrhi::BindingSetHandle {
        framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "ForwardColor");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
        bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
        bsb.BufferSRV("g_CompactBatchIndices", set.compactBatchIndicesBuffer);
        bsb.BufferSRV("g_CompactMaterialIDs", set.compactMaterialIDBuffer);
        bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
        bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
        bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());

        return framegraph::GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), ps.bindlessLayout, nvDevice);
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
        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, set.totalObjectCount);
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
            auto* terrainVsRefl = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* terrainPsRefl = shaderLoader->GetCachedReflection("bindless_terrain", ".ps");
            framegraph::BindingSetBuilder terrainBsb(*terrainVsRefl, *terrainPsRefl, nvDevice, "ForwardColor.Terrain");
            terrainBsb.ConstantBuffer("static_globals", staticGlobalsCB);
            terrainBsb.BufferSRV("g_TerrainMaterials", terrainMatBuffer.GetBuffer());
            terrainBsb.BufferSRV("g_InstanceData", config.terrainInstanceBuffer);
            terrainBsb.BufferSRV("g_CompactBatchIndices", config.terrainCompactBatchIndicesBuffer);
            terrainBsb.BufferSRV("g_CompactMaterialIDs", config.terrainCompactMaterialIDBuffer);
            terrainBsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            terrainBsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            terrainBsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());

            auto terrainBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainBsb.Build(), ps.terrainLayout, nvDevice);
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
            DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, config.terrainObjectCount);

        }
    }

    // Transparent geometry is rendered in a separate TransparentPass (after Detail pass)
    // Skinned meshes are rendered in the SkinningPass (see SkinningPassSetup.cpp)
}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
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

    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeForwardResources(device, fbInfo, *state);
    }

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
            fg::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;

            if (!depthRT || !colorRT)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (cmdList) {
                cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 0.0f, false, 0);
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
} // namespace xray::render::fg::passes
