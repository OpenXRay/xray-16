#include "stdafx.h"
#include "VisibilityPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/GPUCullingManager.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct VisibilityPassData {
    VirtualResourceHandle depth;
    VirtualResourceHandle visId;
    VirtualResourceHandle drawArgsBuffer;
    VirtualResourceHandle skinnedDrawArgs;
    fg::RenderDevice* device = nullptr;
    MaterialCache* materialCache = nullptr;
    GPUCullingManager* gpuCulling = nullptr;
    VisibilityPassState* state = nullptr;
    ClusterDrawConfig config;
    bool retest = false;
};

struct alignas(16) SkinnedVisParams {
    u32 entryBase;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};

struct VisDebugViewData {
    VirtualResourceHandle visId;
    VirtualResourceHandle motion;
    VirtualResourceHandle view;
    fg::RenderDevice* device = nullptr;
    VisibilityPassState* state = nullptr;
    u32 width = 0;
    u32 height = 0;
    u32 mode = 1;
};

struct alignas(16) VisDebugParams {
    u32 mode;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};

void renderVisibilityRaster(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    nvrhi::ITexture* depthRT,
    nvrhi::ITexture* visRT,
    const ClusterDrawConfig& config,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    VisibilityPassState& state,
    bool retest)
{
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!retest) {
        cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 0.0f, false, 0);
        cmdList->clearTextureUInt(visRT, nvrhi::AllSubresources, 0);
    }

    if (!config.UseMegaBuffers())
        return;

    if (materialCache && !retest) {
        materialCache->FinalizePendingMaterials(ctx);
        materialCache->FinalizePendingTerrainMaterials(ctx);
    }
    auto& matBuffer = bindless::MaterialBuffer::Instance();
    if (!retest)
        matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto& cache = GetPassResourceCache();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(visRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = cache.GetOrCreateFramebuffer("VisibilityRaster", fbDesc, nvDevice);
    if (!framebuffer)
        return;

    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), device);
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("cluster_vis", ".vs");
    auto* atRefl = shaderLoader->GetCachedReflection("cluster_vis_at", ".ps");
    auto* fadeRefl = shaderLoader->GetCachedReflection("cluster_vis_fade", ".ps");
    if (!vsRefl || !atRefl || !fadeRefl)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

    const auto& rtDesc = depthRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
    nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

    auto draw = [&](nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingSet* bindingSet, nvrhi::IBuffer* args) {
        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        if (bindlessTable)
            gs.addBindingSet(bindlessTable);
        gs.indirectParams = args;
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(0, 1);
    };

    if (config.IsValid()) {
        BindingSetBuilder bsb(*vsRefl, *atRefl, nvDevice, "VisibilityRaster.Cluster");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
        bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        bsb.BufferSRV("g_InstanceData", config.instanceBuffer);
        bsb.BufferSRV("g_DynamicInstanceData", config.dynamicInstanceBuffer ? config.dynamicInstanceBuffer : config.instanceBuffer);
        bsb.BufferSRV("g_VisibleEntries", config.visibleEntryBuffer);
        bsb.BufferSRV("g_Entries", config.entryBuffer);
        bsb.BufferSRV("g_MegaVB", config.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", config.megaIndexBuffer);
        bsb.BufferSRV("g_DrawFades", config.fadeBuffer);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.layout, nvDevice))
            draw(state.pipeline, bindingSet, config.argsBuffer);
    }

    if (config.TerrainValid()) {
        BindingSetBuilder bsb(*vsRefl, *fadeRefl, nvDevice, "VisibilityRaster.ClusterTerrain");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
        bsb.BufferSRV("g_InstanceData", config.terrainInstanceBuffer);
        bsb.BufferSRV("g_DynamicInstanceData", config.dynamicInstanceBuffer ? config.dynamicInstanceBuffer : config.terrainInstanceBuffer);
        bsb.BufferSRV("g_VisibleEntries", config.terrainVisibleEntryBuffer);
        bsb.BufferSRV("g_Entries", config.entryBuffer);
        bsb.BufferSRV("g_MegaVB", config.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", config.megaIndexBuffer);
        bsb.BufferSRV("g_DrawFades", config.terrainFadeBuffer);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.terrainLayout, nvDevice))
            draw(state.terrainPipeline, bindingSet, config.terrainArgsBuffer);
    }

    const u32 skinnedEntries = (gpuCulling && !retest) ? gpuCulling->GetSkinnedVisibleEntryCount() : 0u;
    if (skinnedEntries > 0 && state.skinnedPipeline) {
        auto* skinnedVsRefl = shaderLoader->GetCachedReflection("cluster_vis_skinned", ".vs");
        nvrhi::IBuffer* preVB = gpuCulling->GetSkinnedPreVertexBuffer();
        nvrhi::IBuffer* skinnedIB = gpuCulling->GetSkinnedPools().GetCombinedIndexBuffer();
        nvrhi::IBuffer* entries = gpuCulling->GetSkinnedEntryBuffer();
        if (skinnedVsRefl && preVB && skinnedIB && entries) {
            auto paramsCB = cache.GetOrCreateVolatileCB("VisibilityRaster", "SkinnedVisParams", sizeof(SkinnedVisParams), device, 16);
            SkinnedVisParams params = {};
            params.entryBase = gpuCulling->GetClusterEntryCapacity();
            cmdList->writeBuffer(paramsCB, &params, sizeof(params));

            BindingSetBuilder bsb(*skinnedVsRefl, *atRefl, nvDevice, "VisibilityRaster.Skinned");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.ConstantBuffer("SkinnedVisParams", paramsCB);
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
            bsb.BufferSRV("g_SkinnedEntries", entries);
            bsb.BufferSRV("g_SkinnedVB", preVB);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_DrawFades", gpuCulling->GetNeutralFadeBuffer());
            if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.skinnedLayout, nvDevice)) {
                nvrhi::GraphicsState gs;
                gs.pipeline = state.skinnedPipeline;
                gs.framebuffer = framebuffer;
                gs.bindings = { bindingSet };
                if (bindlessTable)
                    gs.addBindingSet(bindlessTable);
                gs.viewport.addViewport(viewport);
                gs.viewport.addScissorRect(scissor);
                cmdList->setGraphicsState(gs);
                cmdList->draw(nvrhi::DrawArguments().setVertexCount(GPUCullingManager::SKINNED_ENTRY_INDICES).setInstanceCount(skinnedEntries));
            }
        }
    }
}

}

bool EnsureVisibilityResources(fg::RenderDevice* device, VisibilityPassState& state)
{
    if (state.initialized)
        return true;
    if (state.failed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader) {
        state.failed = true;
        return false;
    }

    auto vsResult = shaderLoader->LoadVertexShader("cluster_vis", "main");
    auto skinnedVsResult = shaderLoader->LoadVertexShader("cluster_vis_skinned", "main");
    auto atResult = shaderLoader->LoadPixelShader("cluster_vis_at", "main");
    auto fadeResult = shaderLoader->LoadPixelShader("cluster_vis_fade", "main");
    if (!vsResult.handle || !vsResult.reflection || !skinnedVsResult.handle || !skinnedVsResult.reflection
        || !atResult.handle || !atResult.reflection || !fadeResult.handle || !fadeResult.reflection) {
        Msg("! [VisibilityRaster] Failed to load cluster visibility shaders");
        state.failed = true;
        return false;
    }
    state.vs = vsResult.handle;
    state.skinnedVS = skinnedVsResult.handle;
    state.psAlphaTest = atResult.handle;
    state.psFade = fadeResult.handle;

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("VisibilityRaster_Cluster", *vsResult.reflection, *atResult.reflection, nvDevice);
    state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection("VisibilityRaster_ClusterTerrain", *vsResult.reflection, *fadeResult.reflection, nvDevice);
    state.skinnedLayout = cache.GetOrCreateBindingLayoutFromReflection("VisibilityRaster_Skinned", *skinnedVsResult.reflection, *atResult.reflection, nvDevice);
    if (!state.layout || !state.terrainLayout || !state.skinnedLayout) {
        Msg("! [VisibilityRaster] Failed to create binding layouts");
        state.failed = true;
        return false;
    }

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::R32_UINT);
    fbInfo.depthFormat = nvrhi::Format::D32;

    auto makeDesc = [&](nvrhi::IShader* pixelShader, nvrhi::IBindingLayout* layout, nvrhi::IShader* vertexShader = nullptr) {
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = vertexShader ? nvrhi::ShaderHandle(vertexShader) : state.vs;
        desc.PS = pixelShader;
        desc.inputLayout = nullptr;
        if (bindlessLayout)
            desc.bindingLayouts = { layout, bindlessLayout };
        else
            desc.bindingLayouts = { layout };
        desc.primType = nvrhi::PrimitiveType::TriangleList;
        desc.renderState.depthStencilState.depthTestEnable = true;
        desc.renderState.depthStencilState.depthWriteEnable = true;
        desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        desc.renderState.rasterState.frontCounterClockwise = false;
        desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
        return desc;
    };

    state.pipeline = cache.GetOrCreatePipeline("VisibilityRaster_Cluster", makeDesc(state.psAlphaTest, state.layout), fbInfo, nvDevice);
    state.terrainPipeline = cache.GetOrCreatePipeline("VisibilityRaster_ClusterTerrain", makeDesc(state.psFade, state.terrainLayout), fbInfo, nvDevice);
    state.skinnedPipeline = cache.GetOrCreatePipeline("VisibilityRaster_Skinned", makeDesc(state.psAlphaTest, state.skinnedLayout, state.skinnedVS), fbInfo, nvDevice);
    if (!state.pipeline || !state.terrainPipeline || !state.skinnedPipeline) {
        Msg("! [VisibilityRaster] Failed to create pipelines");
        state.failed = true;
        return false;
    }

    state.initialized = true;
    Msg("* [VisibilityRaster] Cluster visibility pipelines initialized");
    return true;
}

VisibilityPassOutput setupVisibilityPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthTarget,
    VirtualResourceHandle visIdTarget,
    VirtualResourceHandle drawArgsBuffer,
    VirtualResourceHandle skinnedDrawArgs,
    const ClusterDrawConfig& config,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    VisibilityPassState* state,
    bool retest)
{
    auto& passData = fg.addCallbackPass<VisibilityPassData>(
        retest ? "Visibility Retest" : "Visibility Raster",
        [&, depthTarget, visIdTarget, drawArgsBuffer, skinnedDrawArgs, config, materialCache, gpuCulling, state, retest](FrameGraph& builder, PassHandle passHandle, VisibilityPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.materialCache = materialCache;
            data.gpuCulling = gpuCulling;
            data.state = state;
            data.config = config;
            data.retest = retest;
            if (retest) {
                data.depth = passBuilder.readWrite(depthTarget, ResourceState::DepthStencilWrite);
                data.visId = passBuilder.readWrite(visIdTarget, ResourceState::RenderTarget);
            } else {
                data.depth = passBuilder.write(depthTarget, ResourceState::DepthStencilWrite);
                data.visId = passBuilder.write(visIdTarget, ResourceState::RenderTarget);
            }
            if (drawArgsBuffer.is_valid())
                data.drawArgsBuffer = passBuilder.read(drawArgsBuffer, ResourceState::IndirectArgument);
            if (skinnedDrawArgs.is_valid())
                data.skinnedDrawArgs = passBuilder.read(skinnedDrawArgs, ResourceState::ShaderResource);
        },
        [](const VisibilityPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* visRT = fg.GetPhysicalTexture(data.visId);
            if (!depthRT || !visRT || !ctx->GetCommandList())
                return;
            renderVisibilityRaster(ctx, data.device, depthRT, visRT, data.config, data.materialCache,
                data.skinnedDrawArgs.is_valid() ? data.gpuCulling : nullptr, *data.state, data.retest);
        });

    VisibilityPassOutput out;
    out.depth = passData.depth;
    out.visId = passData.visId;
    return out;
}

VirtualResourceHandle setupVisDebugViewPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle visId,
    VirtualResourceHandle motion,
    u32 width,
    u32 height,
    u32 mode,
    VisibilityPassState* state)
{
    if (!state || state->debugFailed || !visId.is_valid() || !motion.is_valid())
        return {};

    if (!state->debugPipeline) {
        nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        auto csResult = shaderLoader->LoadComputeShader("vis_debug_view", "main");
        if (!csResult.handle || !csResult.reflection) {
            Msg("! [VisibilityRaster] Failed to load vis_debug_view");
            state->debugFailed = true;
            return {};
        }
        state->debugShader = csResult.handle;
        auto& cache = GetPassResourceCache();
        state->debugLayout = cache.GetOrCreateBindingLayoutFromReflection("VisDebugView", *csResult.reflection, nvDevice);
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = state->debugShader;
        pipeDesc.bindingLayouts = { state->debugLayout };
        state->debugPipeline = state->debugLayout ? cache.GetOrCreateComputePipeline("VisDebugView", pipeDesc, nvDevice) : nullptr;
        if (!state->debugPipeline) {
            state->debugFailed = true;
            return {};
        }
    }

    ResourceDesc viewDesc;
    viewDesc.type = ResourceDesc::Type::Texture2D;
    viewDesc.width = width;
    viewDesc.height = height;
    viewDesc.format = nvrhi::Format::RGBA16_FLOAT;
    viewDesc.isUAV = true;
    viewDesc.allowUAV = true;
    viewDesc.isTransient = true;
    viewDesc.debugName = "rt_VisDebug";
    VirtualResourceHandle viewHandle = fg.CreateTexture("rt_VisDebug", viewDesc);

    auto& passData = fg.addCallbackPass<VisDebugViewData>(
        "Vis Debug View",
        [&, visId, motion, viewHandle, width, height, mode, state](FrameGraph& builder, PassHandle passHandle, VisDebugViewData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.state = state;
            data.width = width;
            data.height = height;
            data.mode = mode;
            data.visId = passBuilder.read(visId, ResourceState::ShaderResource);
            data.motion = passBuilder.read(motion, ResourceState::ShaderResource);
            data.view = passBuilder.write(viewHandle, ResourceState::UnorderedAccess);
        },
        [](const VisDebugViewData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* visRT = fg.GetPhysicalTexture(data.visId);
            auto* motionRT = fg.GetPhysicalTexture(data.motion);
            auto* viewRT = fg.GetPhysicalTexture(data.view);
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!visRT || !motionRT || !viewRT || !cmdList)
                return;
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("vis_debug_view", ".cs");
            if (!refl)
                return;
            auto paramsCB = cache.GetOrCreateVolatileCB("VisDebugView", "VisDebugParams", sizeof(VisDebugParams), data.device, 16);
            VisDebugParams params = {};
            params.mode = data.mode;
            cmdList->writeBuffer(paramsCB, &params, sizeof(params));
            BindingSetBuilder bsb(*refl, nvDevice, "VisDebugView");
            bsb.ConstantBuffer("VisDebugParams", paramsCB);
            bsb.Texture("g_VisID", visRT);
            bsb.Texture("g_Motion", motionRT);
            bsb.TextureUAV("g_VisDebug", viewRT);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->debugLayout, nvDevice);
            if (!bindingSet)
                return;
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->debugPipeline;
            cs.bindings = { bindingSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            cmdList->setTextureState(viewRT, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
        });

    return passData.view;
}

}
