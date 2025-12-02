#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Bindless/TextureAtlas.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "xrCore/FMesh.hpp"

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

enum class RenderPhase { None, Opaque, AlphaTested, Transparent };

void renderForwardGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    nvrhi::IBuffer* drawArgsBuffer)
{
    using RENDER_NAMESPACE::CSkeletonX_ST;
    using RENDER_NAMESPACE::CSkeletonX_PM;

    if (!geometry || !materialCache || !device) {
        return;
    }

    const auto& batches = geometry->GetBatches();
    if (batches.empty()) {
        return;
    }

    const bool useIndirectDraw = (drawArgsBuffer != nullptr);

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    SunLightData sunData;
    GetSunLightData(sunData, 2.0f);
    FillSunConstants(staticGlobalsCB, sunData);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            MaterialPSO* matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg, RenderPassType::ForwardColor);
            if (matPSO) {
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.nvrhiBuffer) {
                        if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                            if (cbInfo.name == "static_globals") {
                                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            } else if (cbInfo.name == "dynamic_transforms") {
                                u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicTransformsCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            }
                        }
                    }
                }
            }
        }
    }

    ng::RenderPassDesc passDesc;
    passDesc.passName = "Forward+ Color Pass";
    passDesc.renderTargets[0] = colorRT;
    passDesc.numRenderTargets = 1;
    passDesc.depthStencil = depthRT;

    passDesc.clearColor = false;

    passDesc.clearDepth = false;
    passDesc.clearStencil = false;

    ctx->BeginRenderPass(passDesc);

    const auto& rtDesc = colorRT->getDesc();
    ctx->SetViewport(0, 0,
        static_cast<float>(rtDesc.width),
        static_cast<float>(rtDesc.height));

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = rtDesc.width;
    scissor.height = rtDesc.height;
    ctx->SetScissor(scissor);

    u32 numDraws = 0;
    u32 numTriangles = 0;
    u32 numCulled = 0;

    ng::PipelineState* currentPipeline = nullptr;

    auto renderBatch = [&](const GeometryBatch& batch, u32 batchIndex) -> bool {
        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg, RenderPassType::ForwardColor);

            if (matPSO && matPSO->pso) {
                pipelineToUse = matPSO->pso;
            }
        }

        if (!pipelineToUse) {
            return false;
        }

        if (pipelineToUse != currentPipeline) {
            ctx->SetPipeline(pipelineToUse->GetNativePipeline());
            currentPipeline = pipelineToUse;
        }

        if (matPSO) {
            fgconstants::FGConstantSystem constants(matPSO, materialCache->GetVCBPool());

            Fvector4 dt_params(
                matPSO->detail_scale,
                matPSO->detail_scale,
                matPSO->detail_scale,
                1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range
            );
            constants.Set("dt_params", dt_params);

            constants.CommitMaterial(ctx);

            bool isSkeleton = false;
            u32 visualType = 0;
            if (batch.visual && batch.renderable) {
                visualType = batch.visual->getType();
                isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
            }

            if (isSkeleton) {
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB, batch.worldMatrix);
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                RENDER_NAMESPACE::CKinematics* Parent = nullptr;

                if (visualType == MT_SKELETON_GEOMDEF_ST) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_ST*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                }
                else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_PM*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                }

                if (Parent) {
                    Parent->CalculateBones(TRUE);

                    u32 count = Parent->LL_BoneCount();

                    constexpr u32 MAX_BONES = 78;
                    Fmatrix boneMatrices[MAX_BONES];
                    u32 bonesToUpload = std::min(count, MAX_BONES);

                    for (u32 i = 0; i < bonesToUpload; i++) {
                        boneMatrices[i] = Parent->LL_GetTransform_R(u16(i));
                    }

                    constants.SetArray("sbones_array", boneMatrices, bonesToUpload);
                }
            }
            else {
                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                xform_v.mul_43(Device.mView, batch.worldMatrix);

                constants.Set("m_xform", xform);
                constants.Set("m_xform_v", xform_v);

                using namespace xray::render::RENDER_NAMESPACE;
                float tree_scale = 1.0f / float(FTreeVisual_quant);
                Fvector4 consts(tree_scale, tree_scale, 0.0f, 0.0f);
                constants.Set("consts", consts);
            }

            constants.CommitInstance(ctx);

            materialCache->GetOrCreateBindingSet(matPSO);

            ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
            ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
        }

        nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
        nvrhi::IBuffer* ib = batch.indexBuffer.Get();

        if (!vb || !ib) {
            return false;
        }

        ctx->SetVertexBuffer(0, vb, 0);
        ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

        if (useIndirectDraw && drawArgsBuffer) {
            u32 argsOffset = batchIndex * sizeof(IndirectDrawArgs);
            ctx->DrawIndexedIndirect(drawArgsBuffer, argsOffset);
        } else {
            ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
        }

        numDraws++;
        numTriangles += batch.indexCount / 3;
        return true;
    };

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    RenderPhase currentPhase = RenderPhase::None;

    for (u32 batchIndex = 0; batchIndex < batches.size(); batchIndex++) {
        const auto& batch = batches[batchIndex];

        RenderPhase batchPhase;
        if (batch.IsStrictB2F()) {
            batchPhase = RenderPhase::Transparent;
        } else if (batch.IsAlphaTested()) {
            batchPhase = RenderPhase::AlphaTested;
        } else {
            batchPhase = RenderPhase::Opaque;
        }

        if (batchPhase != currentPhase) {
            if (currentPhase != RenderPhase::None) {
                cmdList->endMarker();
            }

            switch (batchPhase) {
                case RenderPhase::Opaque:
                    cmdList->beginMarker("Forward Opaque (Priority 0)");
                    break;
                case RenderPhase::AlphaTested:
                    cmdList->beginMarker("Forward Alpha-Tested (Priority 1)");
                    break;
                case RenderPhase::Transparent:
                    cmdList->beginMarker("Forward Transparent (bStrictB2F)");
                    break;
                default:
                    break;
            }
            currentPhase = batchPhase;
        }

        renderBatch(batch, batchIndex);
    }

    if (currentPhase != RenderPhase::None) {
        cmdList->endMarker();
    }

    ctx->EndRenderPass();

}

static nvrhi::GraphicsPipelineHandle s_bindlessPipeline;
static nvrhi::BindingLayoutHandle s_bindlessLayout;
static nvrhi::InputLayoutHandle s_bindlessInputLayout;
static nvrhi::SamplerHandle s_linearSampler;
static nvrhi::ShaderHandle s_bindlessVS;
static nvrhi::ShaderHandle s_bindlessPS;
static nvrhi::TextureHandle s_placeholderTexture;
static bool s_bindlessInitialized = false;

void renderBindlessForward(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!config.IsValid() || !geometry || geometry->GetBatches().empty())
        return;

    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
    }

    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (!s_bindlessInitialized) {
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        if (!shaderLoader) {
            Msg("! [BindlessForward] ShaderLoader not available");
            return;
        }

        auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
        auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

        if (!vsResult.handle || !psResult.handle) {
            Msg("! [BindlessForward] Failed to load bindless shaders");
            return;
        }

        s_bindlessVS = vsResult.handle;
        s_bindlessPS = psResult.handle;

        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);

        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(2),
            nvrhi::BindingLayoutItem::ConstantBuffer(4),
            nvrhi::BindingLayoutItem::ConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::Texture_SRV(10),
            nvrhi::BindingLayoutItem::Texture_SRV(11),
            nvrhi::BindingLayoutItem::Texture_SRV(12),
            nvrhi::BindingLayoutItem::Texture_SRV(13),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),
        };
        s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

        constexpr u32 vertexStride = 48;
        nvrhi::VertexAttributeDesc vertexAttribs[] = {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RGB32_FLOAT)
                .setBufferIndex(0)
                .setOffset(0)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("NORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(12)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TANGENT")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(16)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("BINORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(20)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)
                .setArraySize(2)
                .setBufferIndex(0)
                .setOffset(24)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(40)
                .setElementStride(vertexStride),
        };
        s_bindlessInputLayout = nvDevice->createInputLayout(vertexAttribs, 6, s_bindlessVS);

        nvrhi::TextureDesc texDesc;
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.arraySize = 1;
        texDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
        texDesc.format = nvrhi::Format::RGBA8_UNORM;
        texDesc.isShaderResource = true;
        texDesc.debugName = "BindlessPlaceholder";
        texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        texDesc.keepInitialState = true;
        s_placeholderTexture = nvDevice->createTexture(texDesc);

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_bindlessVS;
        pipeDesc.PS = s_bindlessPS;
        pipeDesc.inputLayout = s_bindlessInputLayout;
        pipeDesc.bindingLayouts = { s_bindlessLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
        pipeDesc.renderState.rasterState.frontCounterClockwise = false;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

        nvrhi::FramebufferDesc fbDesc;
        fbDesc.addColorAttachment(colorRT);
        fbDesc.setDepthAttachment(depthRT);
        auto framebuffer = nvDevice->createFramebuffer(fbDesc);

        s_bindlessPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!s_bindlessPipeline) {
            Msg("! [BindlessForward] Failed to create bindless pipeline");
            return;
        }

        s_bindlessInitialized = true;
        Msg("* [BindlessForward] Bindless rendering initialized");
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    struct LightingConstants {
        Fvector4 sunDirection;
        Fvector4 sunColor;
        Fvector4 ambientColor;
        Fvector4 cameraPosition;
        Fvector4 fogParams;
        Fvector4 fogColor;
    } lightingData;
    static_assert(sizeof(LightingConstants) == 96, "LightingConstants must be 96 bytes");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(LightingConstants);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    auto lightingCB = nvDevice->createBuffer(cbDesc);

    if (g_pGamePersistent) {
        auto& env = g_pGamePersistent->Environment().CurrentEnv;
        lightingData.sunDirection.set(env.sun_dir.x, env.sun_dir.y, env.sun_dir.z, 0.0f);
        lightingData.sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
        lightingData.fogParams.set(env.fog_near, env.fog_far, env.fog_density, 0.0f);
        lightingData.fogColor.set(env.fog_color.x, env.fog_color.y, env.fog_color.z, 1.0f);
    } else {
        lightingData.sunDirection.set(0.5f, -0.7f, 0.5f, 0.0f);
        lightingData.sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
        lightingData.fogParams.set(50.0f, 300.0f, 0.001f, 0.0f);
        lightingData.fogColor.set(0.5f, 0.5f, 0.6f, 1.0f);
    }
    lightingData.ambientColor.set(0.1f, 0.1f, 0.15f, 1.0f);
    lightingData.cameraPosition.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 1.0f);

    cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

    struct PerDrawConstants {
        u32 drawIndex;
        u32 padding[3];
    };
    static_assert(sizeof(PerDrawConstants) == 16, "PerDrawConstants must be 16 bytes");

    cbDesc.byteSize = sizeof(PerDrawConstants);
    auto perDrawCB = nvDevice->createBuffer(cbDesc);

    cbDesc.byteSize = sizeof(StaticGlobals);
    auto staticGlobalsCB = nvDevice->createBuffer(cbDesc);

    StaticGlobals staticGlobals;
    FillGlobalConstants(staticGlobals);
    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    auto& atlasManager = TextureAtlasManager::Instance();

    auto getAtlasOrPlaceholder = [&](AtlasType type) -> nvrhi::ITexture* {
        auto* atlas = atlasManager.GetAtlas(type).GetArray();
        return atlas ? atlas : s_placeholderTexture.Get();
    };

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
        nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),
        nvrhi::BindingSetItem::ConstantBuffer(5, perDrawCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        nvrhi::BindingSetItem::Texture_SRV(10, getAtlasOrPlaceholder(AtlasType::Diffuse)),
        nvrhi::BindingSetItem::Texture_SRV(11, getAtlasOrPlaceholder(AtlasType::Normal)),
        nvrhi::BindingSetItem::Texture_SRV(12, getAtlasOrPlaceholder(AtlasType::Detail)),
        nvrhi::BindingSetItem::Texture_SRV(13, getAtlasOrPlaceholder(AtlasType::PBR)),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(14, config.instanceBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(15, config.compactBatchIndicesBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(16, config.compactMaterialIDBuffer),
    };

    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessLayout);
    if (!bindingSet) {
        Msg("! [BindlessForward] Failed to create binding set");
        return;
    }

    cmdList->beginMarker("Bindless Forward");

    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    static u32 s_gpuDrivenDraws = 0;

    if (!config.UseGPUCulling() || !config.UseMegaBuffers()) {
        cmdList->endMarker();
        return;
    }

    cmdList->beginMarker("GPU-Driven Culled");

    const u32 visibleCount = config.totalObjectCount;

    constexpr u32 INDIRECT_ARGS_SIZE = 20;

    nvrhi::GraphicsState state;
    state.pipeline = s_bindlessPipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };
    state.vertexBuffers = { {config.megaVertexBuffer, 0, 0} };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.indirectParams = config.compactDrawArgsBuffer;
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    for (u32 i = 0; i < visibleCount; i++) {
        struct DrawIndexConstant {
            u32 drawIndex;
            u32 padding[3];
        } perDrawData;
        perDrawData.drawIndex = i;
        perDrawData.padding[0] = perDrawData.padding[1] = perDrawData.padding[2] = 0;
        cmdList->writeBuffer(perDrawCB, &perDrawData, sizeof(perDrawData));

        cmdList->setGraphicsState(state);

        cmdList->drawIndexedIndirect(i * INDIRECT_ARGS_SIZE, 1);
    }

    s_gpuDrivenDraws += visibleCount;
    cmdList->endMarker();
    cmdList->endMarker();
}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsInput,
    const BindlessForwardConfig& bindlessConfig)
{
    using namespace framegraph;

    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;
        VirtualResourceHandle drawArgsBuffer;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        BindlessForwardConfig bindlessConfig;
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        [&, width, height, colorInput, drawArgsInput, bindlessConfig](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);

            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);

            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            data.outputs.albedo = data.color;
            data.outputs.depth = data.depth;
        },

        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

                auto* depthRT = fg.GetPhysicalTexture(data.depth);
                auto* colorRT = fg.GetPhysicalTexture(data.color);

                if (!depthRT || !colorRT) {
                    return;
                }

                if (!data.geometry || data.geometry->GetBatches().empty()) {
                    return;
                }

                nvrhi::IBuffer* drawArgsBuffer = nullptr;
                if (data.drawArgsBuffer.is_valid()) {
                    drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
                }

                if (data.bindlessConfig.IsValid()) {
                    renderBindlessForward(
                        ctx,
                        data.device,
                        data.geometry,
                        colorRT,
                        depthRT,
                        data.bindlessConfig,
                        data.materialCache
                    );
                    return;
                }

                renderForwardGeometry(
                    ctx,
                    data.device,
                    data.geometry,
                    colorRT,
                    depthRT,
                    data.materialCache,
                    data.outputs,
                    fg,
                    drawArgsBuffer
                );
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.color;
    outputs.material = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}
}
