#include "stdafx.h"
#include <algorithm>
#include "LocalShadowPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/ResourceManager/NativeRTFactory.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{

void InitializeLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state)
{
    if (!device || state.allocFailed)
        return;

    const u32 res = xray::render::fg::LocalShadowAtlasSize();
    const u32 pages = xray::render::fg::LocalShadowPageCount();
    constexpr u32 kLocalCascadeCBVersions = 8192u;

    if (state.initialized && state.enabled && state.resolution == res &&
        state.sliceCount == pages && state.atlas && state.staticCacheTex &&
        state.cascadeCB && state.cascadeCBMaxVersions >= kLocalCascadeCBVersions &&
        state.clearDepthPipeline && state.localSkinIndirectArgs && state.esmCB)
        return;

    if (state.initialized)
        ShutdownLocalShadowPass(device, state);

    state.opaquePipeline = nullptr;
    state.foliagePipeline = nullptr;
    state.esmConvertPipeline = nullptr;
    state.esmBlurPipeline = nullptr;
    state.esmConvertLayout = nullptr;
    state.esmBlurLayout = nullptr;
    state.skin1w = {};
    state.skinHq = {};
    state.skin2w = {};
    state.skin3w = {};
    state.skin4w = {};
    state.localSkinInstanceSSBO = nullptr;
    state.localSkinInstanceCapacity = 0;
    state.localSkinIndirectArgs = nullptr;
    state.localSkinIndirectArgsCapacity = 0;
    state.cullDoneMarker = nullptr;
    state.clearDepthPipeline = nullptr;
    state.staticCache = nullptr;
    state.staticCacheTex = nullptr;
    state.esmAtlas = nullptr;
    state.esmAtlasTex = nullptr;
    state.esmTemp = nullptr;
    state.esmTempTex = nullptr;
    state.esmCB = nullptr;
    for (u32 i = 0; i < MAX_LOCAL_SHADOW_PAGES; ++i)
        state.pageFramebuffers[i] = nullptr;

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (!nv)
    {
        state.initialized = true;
        state.enabled = false;
        state.allocFailed = true;
        return;
    }

    state.resolution = res;
    state.sliceCount = pages;
    {
        nvrhi::TextureDesc td;
        td.width = res;
        td.height = res;
        td.depth = 1;
        td.arraySize = pages;
        td.mipLevels = 1;
        td.format = nvrhi::Format::D32;
        td.dimension = nvrhi::TextureDimension::Texture2DArray;
        td.isRenderTarget = true;
        td.isTypeless = true;
        td.isShaderResource = true;
        td.useClearValue = true;
        td.clearValue = nvrhi::Color(1.0f);
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = "rt_LocalShadowAtlas";
        state.atlasOwned = nv->createTexture(td);
        state.atlas = state.atlasOwned;
        state.atlasHandle = {};
    }

    {
        nvrhi::TextureDesc td;
        td.width = res;
        td.height = res;
        td.depth = 1;
        td.arraySize = pages;
        td.mipLevels = 1;
        td.format = nvrhi::Format::D32;
        td.dimension = nvrhi::TextureDimension::Texture2DArray;
        td.isRenderTarget = false;
        td.isTypeless = false;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = "rt_LocalShadowStaticCache";
        td.isUAV = false;
        state.staticCache = nv->createTexture(td);
        state.staticCacheTex = state.staticCache;
    }
    {
        nvrhi::TextureDesc td;
        td.width = res;
        td.height = res;
        td.depth = 1;
        td.arraySize = pages;
        td.mipLevels = 1;
        td.format = nvrhi::Format::R16_FLOAT;
        td.dimension = nvrhi::TextureDimension::Texture2DArray;
        td.isUAV = true;
        td.initialState = nvrhi::ResourceStates::UnorderedAccess;
        td.keepInitialState = true;
        td.debugName = "rt_LocalShadowESM";
        state.esmAtlas = nv->createTexture(td);
        state.esmAtlasTex = state.esmAtlas;
        td.debugName = "rt_LocalShadowESMTemp";
        state.esmTemp = nv->createTexture(td);
        state.esmTempTex = state.esmTemp;
    }

    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(ShadowCascadeCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = kLocalCascadeCBVersions;
        cbDesc.debugName = "LocalShadowCascadeCB";
        state.cascadeCB = nv->createBuffer(cbDesc);
        state.cascadeCBMaxVersions = state.cascadeCB ? kLocalCascadeCBVersions : 0;
    }
    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = 32;
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = false;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        cbDesc.keepInitialState = true;
        cbDesc.debugName = "LocalShadowEsmCB";
        state.esmCB = nv->createBuffer(cbDesc);
    }
    {
        nvrhi::BufferDesc md;
        md.byteSize = sizeof(u32);
        md.canHaveUAVs = true;
        md.canHaveRawViews = true;
        md.initialState = nvrhi::ResourceStates::UnorderedAccess;
        md.keepInitialState = true;
        md.debugName = "LocalShadowCullDone";
        state.cullDoneMarker = nv->createBuffer(md);
    }
    {
        constexpr u32 kLocalSkinIndirectCap = 4096u;
        nvrhi::BufferDesc desc;
        desc.byteSize = u64(kLocalSkinIndirectCap) * sizeof(IndirectDrawArgs);
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::IndirectArgument;
        desc.keepInitialState = true;
        desc.debugName = "LocalSkinIndirectArgs";
        state.localSkinIndirectArgs = nv->createBuffer(desc);
        state.localSkinIndirectArgsCapacity =
            state.localSkinIndirectArgs ? kLocalSkinIndirectCap : 0;
    }

    {
        auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
        if (shaderLoader)
        {
            auto vsClear = shaderLoader->LoadVertexShader("shadow\\local_shadow_clear", "main");
            if (vsClear.handle)
            {
                nvrhi::GraphicsPipelineDesc d;
                d.VS = vsClear.handle;
                d.PS = nullptr;
                d.primType = nvrhi::PrimitiveType::TriangleList;
                d.renderState.depthStencilState.setDepthTestEnable(true);
                d.renderState.depthStencilState.setDepthWriteEnable(true);
                d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::Always);
                d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                nvrhi::FramebufferInfoEx fbInfo;
                fbInfo.depthFormat = nvrhi::Format::D32;
                state.clearDepthPipeline = GetPassResourceCache().GetOrCreatePipeline(
                    "LocalShadowClearDepth_v2", d, fbInfo, nv);
                if (!state.clearDepthPipeline)
                    Msg("! [LocalShadow] clearDepth pipeline create failed");
            }
            else
            {
                Msg("! [LocalShadow] shadow\\local_shadow_clear.vs missing");
            }

            auto& cache = GetPassResourceCache();
            auto makeEsmLayout = [&](const ExtractedReflection& refl, const char* name) {
                nvrhi::BindingLayoutDesc desc;
                desc.visibility = nvrhi::ShaderType::Compute;
                for (const auto& cb : refl.constantLayout.constantBuffers.buffers)
                    desc.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(cb.slot));
                for (const auto& tex : refl.rtBindings.inputTextures)
                    desc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(tex.slot));
                for (const auto& uav : refl.rtBindings.uavBindings)
                    desc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_UAV(uav.slot));
                return cache.GetOrCreateBindingLayout(name, desc, nv);
            };

            auto esmConv = shaderLoader->LoadComputeShader("local_shadow_esm_convert", "main");
            if (esmConv.handle && esmConv.reflection)
            {
                state.esmConvertLayout = makeEsmLayout(*esmConv.reflection, "LocalShadowEsmConvert_v3");
                if (state.esmConvertLayout)
                {
                    nvrhi::ComputePipelineDesc pd;
                    pd.CS = esmConv.handle;
                    pd.bindingLayouts = {state.esmConvertLayout};
                    state.esmConvertPipeline = cache.GetOrCreateComputePipeline(
                        "LocalShadowEsmConvert_v3", pd, nv);
                }
            }
            else
            {
                Msg("! [LocalShadow] local_shadow_esm_convert.cs failed to load");
            }
            auto esmBlur = shaderLoader->LoadComputeShader("local_shadow_esm_blur", "main");
            if (esmBlur.handle && esmBlur.reflection)
            {
                state.esmBlurLayout = makeEsmLayout(*esmBlur.reflection, "LocalShadowEsmBlur_v3");
                if (state.esmBlurLayout)
                {
                    nvrhi::ComputePipelineDesc pd;
                    pd.CS = esmBlur.handle;
                    pd.bindingLayouts = {state.esmBlurLayout};
                    state.esmBlurPipeline = cache.GetOrCreateComputePipeline(
                        "LocalShadowEsmBlur_v3", pd, nv);
                }
            }
            else
            {
                Msg("! [LocalShadow] local_shadow_esm_blur.cs failed to load");
            }
        }
    }

    if (state.esmAtlasTex && nv)
    {
        nvrhi::CommandListHandle clearCmd = nv->createCommandList();
        if (clearCmd)
        {
            clearCmd->open();
            clearCmd->clearTextureFloat(
                state.esmAtlasTex, nvrhi::AllSubresources, nvrhi::Color(1.f));
            if (state.esmTempTex)
                clearCmd->clearTextureFloat(
                    state.esmTempTex, nvrhi::AllSubresources, nvrhi::Color(1.f));
            clearCmd->close();
            nv->executeCommandList(clearCmd);
        }
    }

    state.initialized = true;
    state.enabled = (state.atlas != nullptr && state.cascadeCB != nullptr && state.cullDoneMarker != nullptr &&
        state.clearDepthPipeline != nullptr && state.staticCacheTex != nullptr);
    if (state.enabled)
    {
        if (nvrhi::IDevice* nvDev = device ? device->GetNVRHIDevice() : nullptr)
            framegraph::GetPassResourceCache().EnsureShadowBindDummies(nvDev);
        Msg("* [LocalShadow] Init: OK (%ux%u × %u pages, CB versions %u, clear=%d, static=%d, esmTex=%d, esmPipe=%d/%d)",
            res, res, pages, state.cascadeCBMaxVersions, state.clearDepthPipeline ? 1 : 0,
            state.staticCacheTex ? 1 : 0, state.esmAtlasTex ? 1 : 0,
            state.esmConvertPipeline ? 1 : 0, state.esmBlurPipeline ? 1 : 0);
    }
    else
    {
        Msg("! [LocalShadow] Failed to create local shadow atlas/CB — lights stay unshadowed");
        state.allocFailed = true;
        state.atlasOwned = nullptr;
        state.atlas = nullptr;
    }
}

void ShutdownLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state)
{
    if (device && device->GetFGResourceManager() && device->GetFGResourceManager()->GetRTFactory())
    {
        if (state.atlasHandle.IsValid())
            device->GetFGResourceManager()->GetRTFactory()->ReleaseRenderTarget(state.atlasHandle);
    }
    state.atlasHandle = {};
    state.atlasOwned = nullptr;
    state.atlas = nullptr;
    state.allocFailed = false;
    state.staticCache = nullptr;
    state.staticCacheTex = nullptr;
    state.esmAtlas = nullptr;
    state.esmAtlasTex = nullptr;
    state.esmTemp = nullptr;
    state.esmTempTex = nullptr;
    state.cascadeCB = nullptr;
    state.esmCB = nullptr;
    state.cascadeCBMaxVersions = 0;
    state.cullDoneMarker = nullptr;
    state.opaquePipeline = nullptr;
    state.foliagePipeline = nullptr;
    state.esmConvertPipeline = nullptr;
    state.esmBlurPipeline = nullptr;
    state.esmConvertLayout = nullptr;
    state.esmBlurLayout = nullptr;
    state.skin1w = {};
    state.skinHq = {};
    state.skin2w = {};
    state.skin3w = {};
    state.skin4w = {};
    state.localSkinInstanceSSBO = nullptr;
    state.localSkinInstanceCapacity = 0;
    state.localSkinIndirectArgs = nullptr;
    state.localSkinIndirectArgsCapacity = 0;
    for (u32 i = 0; i < MAX_LOCAL_SHADOW_PAGES; ++i)
        state.pageFramebuffers[i] = nullptr;
    state.clearDepthPipeline = nullptr;
    state.resolution = 0;
    state.sliceCount = 0;
    state.initialized = false;
    state.enabled = false;
}

LocalShadowOutputs setupLocalShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    LocalShadowPassState& state,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches,
    fg::GPUCullingManager* gpuCulling,
    framegraph::VirtualResourceHandle gpuCullDrawArgs)
{
    using namespace framegraph;
    LocalShadowOutputs outputs;
    outputs.valid = false;

    if (!device)
        return outputs;

    auto& clm = ClusteredLightManager::Instance();
    const auto& tiles = clm.GetLocalShadowTiles();
    if (ps_r_local_shadows == 0 || tiles.empty())
        return outputs;

    InitializeLocalShadowPass(device, state);
    if (!state.enabled || !state.atlas || !state.cascadeCB || !state.cullDoneMarker ||
        !shadowState.enabled || !shadowState.pipeline)
        return outputs;
    if (!bindlessConfig.UseGPUCulling() || !bindlessConfig.UseMegaBuffers())
        return outputs;

    nvrhi::IDevice* nvInit = device->GetNVRHIDevice();
    if (!gpuCulling || !nvInit || !gpuCulling->EnsureLightShadowCullPipeline(nvInit))
    {
        static bool s_once = false;
        if (!s_once)
        {
            Msg("! [LocalShadow] GPU light_shadow_cull unavailable — skipping local shadows");
            s_once = true;
        }
        return outputs;
    }
    const u32 slotNeed = GPUCullingManager::kLocalShadowCullMaxSlots;
    if (!gpuCulling->EnsureLocalShadowCullSlots(nvInit, slotNeed))
        return outputs;

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = state.resolution;
    desc.height = state.resolution;
    desc.depth = 1;
    desc.arraySize = state.sliceCount;
    desc.format = nvrhi::Format::D32;
    desc.isDepthStencil = true;
    desc.isImported = true;
    desc.debugName = "rt_LocalShadowAtlas";

    auto atlasHandle = fg.ImportTexture("rt_LocalShadowAtlas", state.atlas, desc);

    struct PassData
    {
        VirtualResourceHandle atlas;
        VirtualResourceHandle gpuCullArgs;
        LocalShadowPassState* localState = nullptr;
        ShadowPassState* shadowState = nullptr;
        BindlessForwardConfig bindlessConfig;
        fg::RenderDevice* device = nullptr;
        fg::GPUCullingManager* gpuCulling = nullptr;
        const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches = nullptr;
        const xr_vector<LocalShadowTile>* tiles = nullptr;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "LocalShadowDraw",
        [&, atlasHandle, gpuCullDrawArgs, worldSkinnedBatches, gpuCulling](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.atlas = pb.write(atlasHandle, ResourceState::DepthStencilWrite);
            if (gpuCullDrawArgs.is_valid())
                data.gpuCullArgs = pb.read(gpuCullDrawArgs, ResourceState::IndirectArgument);
            else
                pb.sideEffects();
            data.localState = &state;
            data.shadowState = &shadowState;
            data.bindlessConfig = bindlessConfig;
            data.device = device;
            data.gpuCulling = gpuCulling;
            data.worldSkinnedBatches = worldSkinnedBatches;
            data.tiles = &clm.GetLocalShadowTiles();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            ZoneScopedN("LocalShadowDraw::Execute");
            if (!data.localState || !data.shadowState || !data.tiles || data.tiles->empty())
                return;
            auto* atlasTex = graph.GetPhysicalTexture(data.atlas);
            if (!atlasTex)
                atlasTex = data.localState->atlas;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !atlasTex)
                return;

            ShadowPassState& st = *data.shadowState;
            LocalShadowPassState& ls = *data.localState;

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
            {
                static bool s_warned = false;
                if (!s_warned)
                {
                    Msg("! [LocalShadow] GPU bindless culling unavailable — atlas empty");
                    s_warned = true;
                }
                return;
            }

            const u32 casterCount = data.gpuCulling
                ? (data.gpuCulling->GetStaticObjectCount() + data.gpuCulling->GetDynamicObjectCount() +
                   data.gpuCulling->GetTransparentObjectCount())
                : 0;
            if (casterCount == 0 || !ls.clearDepthPipeline)
                return;

            bool anyRedraw = false;
            for (const LocalShadowTile& tile : *data.tiles)
            {
                if (tile.needsRedraw && tile.size >= 8)
                {
                    anyRedraw = true;
                    break;
                }
            }
            if (!anyRedraw)
                return;

            {
                ZoneScopedN("LocalShadowDraw::LightCull");
                data.gpuCulling->DispatchAllLocalShadowTileCulls(cmd, nv, *data.tiles, 1.35f);
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("LocalShadow", nv);
            if (!vsRefl || !psRefl || !drawIndexBuffer || !st.pipeline || !st.layout || !ls.cascadeCB)
                return;

            {
                ShadowCascadeCB priming{};
                priming.lightVP.identity();
                cmd->writeBuffer(ls.cascadeCB, &priming, sizeof(priming));
            }

            auto& cache = GetPassResourceCache();
            auto& matBuffer = bindless::MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto* backend = GEnv.Backend;
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // Perspective local SMs need far less slope bias than sun ortho CSM —
            // high bias was shrinking bed/table/NPC contact shadows.
            if (!ls.opaquePipeline && st.vs && st.ps && st.inputLayout && st.layout)
            {
                nvrhi::GraphicsPipelineDesc d;
                d.VS = st.vs;
                d.PS = st.ps;
                d.inputLayout = st.inputLayout;
                d.primType = nvrhi::PrimitiveType::TriangleList;
                d.renderState.depthStencilState.setDepthTestEnable(true);
                d.renderState.depthStencilState.setDepthWriteEnable(true);
                d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                d.renderState.rasterState.depthBias = 0;
                d.renderState.rasterState.slopeScaledDepthBias = 0.5f;
                d.renderState.rasterState.depthBiasClamp = 0.001f;
                if (auto* bl = backend ? backend->GetBindlessLayout() : nullptr)
                    d.bindingLayouts = {st.layout, bl};
                else
                    d.bindingLayouts = {st.layout};
                nvrhi::FramebufferInfoEx fbInfo;
                fbInfo.depthFormat = nvrhi::Format::D32;
                ls.opaquePipeline = cache.GetOrCreatePipeline("LocalShadowOpaque", d, fbInfo, nv);
                if (st.foliagePs && st.foliagePipeline)
                {
                    d.PS = st.foliagePs;
                    ls.foliagePipeline = cache.GetOrCreatePipeline("LocalShadowFoliage", d, fbInfo, nv);
                }
            }
            nvrhi::IGraphicsPipeline* opaquePipe =
                ls.opaquePipeline ? ls.opaquePipeline.Get() : st.pipeline.Get();
            nvrhi::IGraphicsPipeline* foliagePipe =
                ls.foliagePipeline ? ls.foliagePipeline.Get() : st.foliagePipeline.Get();

            const u32 cres = ls.resolution;

            const auto& tilesRef = *data.tiles;
            {
                static bool s_once = false;
                if (!s_once)
                {
                    Msg("* [LocalShadow] Drawing %u packed rects @ %ux%u × %u pages (skinned=SSBO)",
                        (u32)tilesRef.size(), cres, cres, ls.sliceCount);
                    s_once = true;
                }
            }

            if (!data.gpuCulling)
                return;
            const u32 maxSlots = data.gpuCulling->GetLocalShadowCullSlotCount();
            const u32 maxTilesThisFrame = static_cast<u32>(tilesRef.size());
            u32 tilesDrawn = 0;

            for (u32 slot = 0; slot < maxSlots; ++slot)
            {
                for (u32 si = 0; si < 3; ++si)
                {
                    if (auto* b = data.gpuCulling->MutLocalShadowSlotDrawBinding(slot, si))
                        *b = nullptr;
                    if (auto* s = data.gpuCulling->MutLocalShadowSlotDrawSrcInstance(slot, si))
                        *s = nullptr;
                }
            }
            ls.skin1w.bindingSet = nullptr;
            ls.skinHq.bindingSet = nullptr;
            ls.skin2w.bindingSet = nullptr;
            ls.skin3w.bindingSet = nullptr;
            ls.skin4w.bindingSet = nullptr;

            struct LocalSkinDraw
            {
                nvrhi::IBuffer* vb = nullptr;
                nvrhi::IBuffer* ib = nullptr;
                u32 indexCount = 0;
                u32 startIndex = 0;
                s32 baseVertex = 0;
                u32 vertexStride = 0;
                u16 skinningRenderMode = 0;
                u32 instanceIndex = 0;
                u32 skinnedPoolFormat = UINT32_MAX;
                s32 skinnedPoolBaseVertex = 0;
                u32 skinnedPoolFirstIndex = 0;
                bool pooled = false;
                Fvector center{};
            };

            static thread_local xr_vector<LocalSkinInstanceGPU> s_skinInstances;
            static thread_local xr_vector<LocalSkinDraw> s_skinDraws;
            auto& skinInstances = s_skinInstances;
            auto& skinDraws = s_skinDraws;
            skinInstances.clear();
            skinDraws.clear();
            nvrhi::IBuffer* skBoneBuf = nullptr;
            auto* skPsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");

            bool anySkinTile = false;
            if ((ps_r_skinned_shadows != 0) && data.worldSkinnedBatches && data.gpuCulling && skPsRefl)
            {
                for (const LocalShadowTile& tile : tilesRef)
                {
                    if (tile.needsRedraw && tile.L)
                    {
                        anySkinTile = true;
                        break;
                    }
                }
            }

            const bool wantWorldSkinned =
                anySkinTile && (ps_r_skinned_shadows != 0) && data.worldSkinnedBatches &&
                data.gpuCulling && skPsRefl;
            if (wantWorldSkinned)
            {
                skBoneBuf = data.gpuCulling->GetGlobalBoneBuffer();
                auto* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
                nvrhi::FramebufferInfoEx skinFbInfo;
                skinFbInfo.depthFormat = nvrhi::Format::D32;

                auto ensureLocalSkinPipe = [&](LocalSkinPipeVariant& var, const char* vsName, const char* cacheName,
                                               const nvrhi::VertexAttributeDesc* baseAttribs, u32 baseAttrCount) {
                    if (var.pipeline || !st.ps)
                        return;
                    auto vsResult = shaderLoader->LoadVertexShader(vsName, "main");
                    if (!vsResult.handle || !vsResult.reflection)
                        return;
                    var.vs = vsResult.handle;
                    var.layout = cache.GetOrCreateBindingLayoutFromReflection(
                        cacheName, *vsResult.reflection, *skPsRefl, nv);
                    if (!var.layout)
                    {
                        vsResult.reflection = nullptr;
                        return;
                    }
                    nvrhi::VertexAttributeDesc attribs[8];
                    for (u32 i = 0; i < baseAttrCount; ++i)
                        attribs[i] = baseAttribs[i];
                    attribs[baseAttrCount] = nvrhi::VertexAttributeDesc()
                        .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
                        .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true);
                    var.inputLayout = nv->createInputLayout(attribs, baseAttrCount + 1, var.vs);
                    nvrhi::GraphicsPipelineDesc d;
                    d.VS = var.vs;
                    d.PS = st.ps;
                    d.inputLayout = var.inputLayout;
                    d.primType = nvrhi::PrimitiveType::TriangleList;
                    d.bindingLayouts = {var.layout};
                    if (bindlessLayout)
                        d.bindingLayouts.push_back(bindlessLayout);
                    d.renderState.depthStencilState.setDepthTestEnable(true);
                    d.renderState.depthStencilState.setDepthWriteEnable(true);
                    d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                    d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                    d.renderState.rasterState.depthBias = 0;
                    d.renderState.rasterState.slopeScaledDepthBias = 0.5f;
                    d.renderState.rasterState.depthBiasClamp = 0.001f;
                    var.pipeline = cache.GetOrCreatePipeline(cacheName, d, skinFbInfo, nv);
                    vsResult.reflection = nullptr;
                };

                {
                    constexpr u32 stride = 24;
                    nvrhi::VertexAttributeDesc a[] = {
                        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA16_SNORM).setOffset(0).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(8).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(12).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG16_SNORM).setOffset(20).setElementStride(stride),
                    };
                    ensureLocalSkinPipe(ls.skin1w, "shadow\\shadow_cascade_skinned_local",
                        "LocalShadowSkin1W", a, 5);
                }
                {
                    constexpr u32 stride = 36;
                    nvrhi::VertexAttributeDesc a[] = {
                        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
                    };
                    ensureLocalSkinPipe(ls.skinHq, "shadow\\shadow_cascade_skinned_hq_local",
                        "LocalShadowSkinHQ", a, 5);
                }
                {
                    constexpr u32 stride = 44;
                    nvrhi::VertexAttributeDesc a[] = {
                        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
                    };
                    ensureLocalSkinPipe(ls.skin2w, "shadow\\shadow_cascade_skinned_2w_local",
                        "LocalShadowSkin2W", a, 5);
                    ensureLocalSkinPipe(ls.skin3w, "shadow\\shadow_cascade_skinned_3w_local",
                        "LocalShadowSkin3W", a, 5);
                }
                {
                    constexpr u32 stride = 40;
                    nvrhi::VertexAttributeDesc a[] = {
                        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
                        nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(36).setElementStride(stride),
                    };
                    ensureLocalSkinPipe(ls.skin4w, "shadow\\shadow_cascade_skinned_4w_local",
                        "LocalShadowSkin4W", a, 6);
                }

                if (skBoneBuf && (ls.skin1w.pipeline || ls.skinHq.pipeline || ls.skin2w.pipeline ||
                                  ls.skin3w.pipeline || ls.skin4w.pipeline))
                {
                    const bool mdiEnabled = data.gpuCulling->IsSkinnedMDIEnabled();
                    skinInstances.reserve(data.worldSkinnedBatches->size());
                    skinDraws.reserve(data.worldSkinnedBatches->size());
                    for (const auto& batch : *data.worldSkinnedBatches)
                    {
                        if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
                            continue;
                        CKinematics* parent = nullptr;
                        const u32 visualType = batch.visual ? batch.visual->getType() : 0;
                        if (visualType == MT_SKELETON_GEOMDEF_ST)
                            parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                        else if (visualType == MT_SKELETON_GEOMDEF_PM)
                            parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                        if (!parent)
                            continue;

                        const u32 boneOffset = data.gpuCulling->GetOrUploadSkeleton(cmd, parent);
                        LocalSkinInstanceGPU inst{};
                        inst.world = batch.worldMatrix;
                        inst.materialID = batch.bindlessMaterialID;
                        inst.boneOffset = boneOffset;
                        const u32 instanceIndex = static_cast<u32>(skinInstances.size());
                        skinInstances.push_back(inst);

                        const u32 variantIdx =
                            bindless::MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
                        LocalSkinDraw draw{};
                        draw.vb = batch.vertexBuffer;
                        draw.ib = batch.indexBuffer;
                        draw.indexCount = batch.indexCount;
                        draw.startIndex = batch.startIndex;
                        draw.baseVertex = batch.baseVertex;
                        draw.vertexStride = batch.vertexStride;
                        draw.skinningRenderMode = batch.skinningRenderMode;
                        draw.instanceIndex = instanceIndex;
                        draw.skinnedPoolFormat = batch.skinnedPoolFormat;
                        draw.skinnedPoolBaseVertex = batch.skinnedPoolBaseVertex;
                        draw.skinnedPoolFirstIndex = batch.skinnedPoolFirstIndex;
                        draw.pooled = mdiEnabled && variantIdx == 0
                            && batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
                            && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
                        draw.center = batch.worldMatrix.c;
                        skinDraws.push_back(draw);
                    }

                    if (!skinInstances.empty())
                    {
                        const u32 needed = static_cast<u32>(skinInstances.size());
                        if (!ls.localSkinInstanceSSBO || ls.localSkinInstanceCapacity < needed)
                        {
                            const u32 capacity = std::max(needed, 256u);
                            nvrhi::BufferDesc ssboDesc;
                            ssboDesc.byteSize = u64(capacity) * sizeof(LocalSkinInstanceGPU);
                            ssboDesc.structStride = sizeof(LocalSkinInstanceGPU);
                            ssboDesc.canHaveUAVs = false;
                            ssboDesc.initialState = nvrhi::ResourceStates::ShaderResource;
                            ssboDesc.keepInitialState = true;
                            ssboDesc.debugName = "LocalSkinInstanceSSBO";
                            ls.localSkinInstanceSSBO = nv->createBuffer(ssboDesc);
                            ls.localSkinInstanceCapacity = ls.localSkinInstanceSSBO ? capacity : 0;
                            ls.skin1w.bindingSet = nullptr;
                            ls.skinHq.bindingSet = nullptr;
                            ls.skin2w.bindingSet = nullptr;
                            ls.skin3w.bindingSet = nullptr;
                            ls.skin4w.bindingSet = nullptr;
                            if (ls.localSkinInstanceSSBO)
                            {
                                static bool s_ssboOnce = false;
                                if (!s_ssboOnce)
                                {
                                    Msg("* [LocalShadow] LocalSkin SSBO ready (cap %u)", capacity);
                                    s_ssboOnce = true;
                                }
                            }
                        }
                        if (ls.localSkinInstanceSSBO)
                        {
                            cmd->writeBuffer(
                                ls.localSkinInstanceSSBO,
                                skinInstances.data(),
                                u64(skinInstances.size()) * sizeof(LocalSkinInstanceGPU));
                        }
                    }
                }
            }
            const bool worldSkinnedReady =
                !skinDraws.empty() && ls.localSkinInstanceSSBO && skBoneBuf;

            if (worldSkinnedReady)
            {
                auto ensureSkinBinding = [&](LocalSkinPipeVariant& var, const char* vsName) {
                    if (!var.pipeline || !var.layout || var.bindingSet)
                        return;
                    auto* vsReflSk = shaderLoader->GetCachedReflection(vsName, ".vs");
                    if (!vsReflSk || !skPsRefl)
                        return;
                    BindingSetBuilder bsb(*vsReflSk, *skPsRefl, nv, "LocalShadow.WorldSkinSSBO");
                    bsb.ConstantBuffer("ShadowCascadeCB", ls.cascadeCB);
                    bsb.BufferSRV("g_BoneMatrices", skBoneBuf);
                    bsb.BufferSRV("g_LocalSkinInstances", ls.localSkinInstanceSSBO);
                    BindBindlessMaterialTables(bsb);
                    var.bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), var.layout, nv);
                };
                ensureSkinBinding(ls.skin1w, "shadow\\shadow_cascade_skinned_local");
                ensureSkinBinding(ls.skinHq, "shadow\\shadow_cascade_skinned_hq_local");
                ensureSkinBinding(ls.skin2w, "shadow\\shadow_cascade_skinned_2w_local");
                ensureSkinBinding(ls.skin3w, "shadow\\shadow_cascade_skinned_3w_local");
                ensureSkinBinding(ls.skin4w, "shadow\\shadow_cascade_skinned_4w_local");
            }

            for (u32 page = 0; page < ls.sliceCount && page < MAX_LOCAL_SHADOW_PAGES; ++page)
            {
                if (ls.pageFramebuffers[page])
                    continue;
                nvrhi::FramebufferDesc fbDesc;
                nvrhi::TextureSubresourceSet depthSub;
                depthSub.baseArraySlice = page;
                depthSub.numArraySlices = 1;
                fbDesc.setDepthAttachment(atlasTex, depthSub);
                ls.pageFramebuffers[page] = cache.GetOrCreateFramebuffer(
                    make_string("LocalShadowP%u_%u", page, cres).c_str(), fbDesc, nv);
            }

            bool needTerrainUpload = false;
            if (st.terrainPipeline && st.terrainLayout && data.bindlessConfig.HasTerrain())
            {
                for (const LocalShadowTile& tile : tilesRef)
                {
                    if (tile.needsRedraw && tile.size >= 128)
                    {
                        needTerrainUpload = true;
                        break;
                    }
                }
            }
            if (needTerrainUpload)
            {
                if (auto* matCache = GEnv.Render ? GEnv.Render->GetMaterialCache() : nullptr)
                    matCache->FinalizePendingTerrainMaterials(ctx);
                bindless::TerrainMaterialBuffer::Instance().Upload(ctx);
            }

            struct SkinCand
            {
                const LocalSkinDraw* draw = nullptr;
                LocalSkinPipeVariant* var = nullptr;
                float dist2 = 0.f;
            };
            struct SkinMdiGroup
            {
                LocalSkinPipeVariant* var = nullptr;
                u32 format = 0;
                u32 start = 0;
                u32 count = 0;
            };
            struct TileSkinMdi
            {
                u32 groupBegin = 0;
                u32 groupCount = 0;
                bool ready = false;
            };
            static thread_local xr_vector<SkinCand> s_skinCands;
            static thread_local xr_vector<SkinCand> s_skinPooled;
            static thread_local xr_vector<SkinCand> s_skinResidual;
            static thread_local xr_vector<IndirectDrawArgs> s_frameSkinArgs;
            static thread_local xr_vector<SkinMdiGroup> s_frameSkinGroups;
            static thread_local xr_vector<TileSkinMdi> s_tileSkinMdi;
            static thread_local xr_vector<xr_vector<SkinCand>> s_tileSkinResidual;
            auto& skinCands = s_skinCands;
            auto& skinPooled = s_skinPooled;
            auto& skinResidual = s_skinResidual;
            auto& frameSkinArgs = s_frameSkinArgs;
            auto& frameSkinGroups = s_frameSkinGroups;
            auto& tileSkinMdi = s_tileSkinMdi;
            auto& tileSkinResidual = s_tileSkinResidual;

            auto pickVar = [&](const LocalSkinDraw& draw) -> LocalSkinPipeVariant* {
                const u16 rm = draw.skinningRenderMode;
                const u32 stride = draw.vertexStride;
                LocalSkinPipeVariant* var = nullptr;
                if (rm == 7 || rm == 8) var = &ls.skin3w;
                else if (rm == 5 || rm == 6) var = &ls.skin2w;
                else if (rm == 9 || rm == 10) var = &ls.skin4w;
                else if (rm == 4 || rm == 2) var = &ls.skinHq;
                else if (rm == 3 || rm == 1) var = &ls.skin1w;
                else if (stride == 36) var = &ls.skinHq;
                else if (stride == 40) var = &ls.skin4w;
                else if (stride == 44) var = &ls.skin2w;
                else var = &ls.skin1w;
                if (!var || !var->pipeline || !var->layout || !var->inputLayout || !var->bindingSet)
                    return nullptr;
                return var;
            };

            frameSkinArgs.clear();
            frameSkinGroups.clear();
            tileSkinMdi.assign(tilesRef.size(), {});
            tileSkinResidual.assign(tilesRef.size(), {});
            const bool skinMdiFrame =
                worldSkinnedReady && ps_r_local_shadow_skinned_max > 0;
            if (skinMdiFrame)
            {
                const u32 skinCap = static_cast<u32>(ps_r_local_shadow_skinned_max);
                for (u32 tileIdx = 0; tileIdx < tilesRef.size() && tileIdx < maxSlots; ++tileIdx)
                {
                    const LocalShadowTile& tile = tilesRef[tileIdx];
                    if (!tile.needsRedraw || !tile.L || tile.size < 8 ||
                        tile.page >= ls.sliceCount ||
                        tile.posX + tile.size > cres || tile.posY + tile.size > cres)
                        continue;

                    const float maxR = std::max(tile.L->range * 2.0f, 4.f);
                    const float maxR2 = maxR * maxR;
                    const Fvector& lp = tile.L->position;
                    skinCands.clear();
                    for (const LocalSkinDraw& draw : skinDraws)
                    {
                        const float d2 = lp.distance_to_sqr(draw.center);
                        if (d2 > maxR2)
                            continue;
                        LocalSkinPipeVariant* var = pickVar(draw);
                        if (!var)
                            continue;
                        skinCands.push_back({&draw, var, d2});
                    }
                    if (skinCands.size() > skinCap)
                    {
                        std::nth_element(
                            skinCands.begin(),
                            skinCands.begin() + skinCap,
                            skinCands.end(),
                            [](const SkinCand& a, const SkinCand& b) { return a.dist2 < b.dist2; });
                        skinCands.resize(skinCap);
                    }

                    skinPooled.clear();
                    skinResidual.clear();
                    for (const SkinCand& cand : skinCands)
                    {
                        if (cand.draw->pooled)
                            skinPooled.push_back(cand);
                        else
                            skinResidual.push_back(cand);
                    }
                    tileSkinResidual[tileIdx] = skinResidual;

                    if (skinPooled.empty())
                        continue;

                    std::sort(
                        skinPooled.begin(),
                        skinPooled.end(),
                        [](const SkinCand& a, const SkinCand& b) {
                            if (a.draw->skinnedPoolFormat != b.draw->skinnedPoolFormat)
                                return a.draw->skinnedPoolFormat < b.draw->skinnedPoolFormat;
                            return a.var < b.var;
                        });

                    TileSkinMdi& tmdi = tileSkinMdi[tileIdx];
                    tmdi.groupBegin = static_cast<u32>(frameSkinGroups.size());
                    for (const SkinCand& cand : skinPooled)
                    {
                        const LocalSkinDraw& draw = *cand.draw;
                        if (frameSkinGroups.size() == tmdi.groupBegin ||
                            frameSkinGroups.back().var != cand.var ||
                            frameSkinGroups.back().format != draw.skinnedPoolFormat)
                        {
                            frameSkinGroups.push_back(
                                {cand.var, draw.skinnedPoolFormat,
                                 static_cast<u32>(frameSkinArgs.size()), 0});
                        }
                        IndirectDrawArgs args{};
                        args.indexCountPerInstance = draw.indexCount;
                        args.instanceCount = 1;
                        args.startIndexLocation = draw.skinnedPoolFirstIndex;
                        args.baseVertexLocation = draw.skinnedPoolBaseVertex;
                        args.startInstanceLocation = draw.instanceIndex;
                        frameSkinArgs.push_back(args);
                        ++frameSkinGroups.back().count;
                    }
                    tmdi.groupCount =
                        static_cast<u32>(frameSkinGroups.size()) - tmdi.groupBegin;
                }

                if (!frameSkinArgs.empty())
                {
                    const u32 needed = static_cast<u32>(frameSkinArgs.size());
                    if (!ls.localSkinIndirectArgs || ls.localSkinIndirectArgsCapacity < needed)
                    {
                        const u32 capacity = std::max(needed, 4096u);
                        nvrhi::BufferDesc desc;
                        desc.byteSize = u64(capacity) * sizeof(IndirectDrawArgs);
                        desc.canHaveRawViews = true;
                        desc.isDrawIndirectArgs = true;
                        desc.initialState = nvrhi::ResourceStates::IndirectArgument;
                        desc.keepInitialState = true;
                        desc.debugName = "LocalSkinIndirectArgs";
                        ls.localSkinIndirectArgs = nv->createBuffer(desc);
                        ls.localSkinIndirectArgsCapacity =
                            ls.localSkinIndirectArgs ? capacity : 0;
                    }
                    if (ls.localSkinIndirectArgs && ls.localSkinIndirectArgsCapacity >= needed)
                    {
                        cmd->writeBuffer(
                            ls.localSkinIndirectArgs,
                            frameSkinArgs.data(),
                            u64(needed) * sizeof(IndirectDrawArgs));
                        cmd->setBufferState(
                            ls.localSkinIndirectArgs,
                            nvrhi::ResourceStates::IndirectArgument);
                        for (TileSkinMdi& tmdi : tileSkinMdi)
                            tmdi.ready = tmdi.groupCount > 0;
                    }
                }
            }

            for (u32 tileIdx = 0; tileIdx < tilesRef.size(); ++tileIdx)
            {
                if (tilesDrawn >= maxTilesThisFrame)
                    break;
                if (tileIdx >= maxSlots)
                    break;
                const LocalShadowTile& tile = tilesRef[tileIdx];
                if (!tile.needsRedraw || tile.size < 8 || tile.page >= ls.sliceCount ||
                    tile.posX + tile.size > cres || tile.posY + tile.size > cres)
                    continue;

                const float x0 = float(tile.posX);
                const float y0 = float(tile.posY);
                const float x1 = float(tile.posX + tile.size);
                const float y1 = float(tile.posY + tile.size);
                nvrhi::Viewport vp(x0, x1, y0, y1, 0.f, 1.f);
                nvrhi::Rect scissor(tile.posX, tile.posX + tile.size, tile.posY, tile.posY + tile.size);

                auto fb = (tile.page < MAX_LOCAL_SHADOW_PAGES) ? ls.pageFramebuffers[tile.page] : nullptr;
                if (!fb)
                    continue;

                if (!ls.clearDepthPipeline)
                    continue;

                {
                    nvrhi::GraphicsState clearGs;
                    clearGs.pipeline = ls.clearDepthPipeline;
                    clearGs.framebuffer = fb;
                    clearGs.viewport.addViewport(vp);
                    clearGs.viewport.addScissorRect(scissor);
                    cmd->setGraphicsState(clearGs);
                    cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
                }

                ShadowCascadeCB cbData{};
                cbData.lightVP = tile.clipVP;
                cmd->writeBuffer(ls.cascadeCB, &cbData, sizeof(cbData));

                auto drawSet = [&](const BindlessDrawSet& set, nvrhi::IGraphicsPipeline* pipe, u32 setIdx) {
                    if (!pipe || !set.instanceBuffer || set.totalObjectCount == 0)
                        return;

                    nvrhi::IBuffer* batchIndices =
                        data.gpuCulling->GetLocalShadowSlotIndices(tileIdx, setIdx);
                    nvrhi::IBuffer* materialIDs =
                        data.gpuCulling->GetLocalShadowSlotMats(tileIdx, setIdx);
                    nvrhi::IBuffer* drawArgs =
                        data.gpuCulling->GetLocalShadowSlotDrawArgs(tileIdx, setIdx);
                    nvrhi::IBuffer* countBuffer =
                        data.gpuCulling->GetLocalShadowSlotCount(tileIdx, setIdx);
                    if (!batchIndices || !materialIDs || !drawArgs || !countBuffer)
                        return;

                    auto* cachedDraw = data.gpuCulling->MutLocalShadowSlotDrawBinding(tileIdx, setIdx);
                    auto* cachedSrc = data.gpuCulling->MutLocalShadowSlotDrawSrcInstance(tileIdx, setIdx);
                    if (!cachedDraw || !cachedSrc)
                        return;

                    nvrhi::IBuffer* inst = set.instanceBuffer;
                    if (!*cachedDraw || *cachedSrc != inst)
                    {
                        BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "LocalShadow");
                        bsb.ConstantBuffer("ShadowCascadeCB", ls.cascadeCB);
                        BindBindlessMaterialTables(bsb);
                        bsb.BufferSRV("g_InstanceData", inst);
                        bsb.BufferSRV("g_CompactBatchIndices", batchIndices);
                        bsb.BufferSRV("g_CompactMaterialIDs", materialIDs);
                        *cachedDraw = cache.GetOrCreateBindingSet(bsb.Build(), st.layout, nv);
                        *cachedSrc = (*cachedDraw) ? inst : nullptr;
                    }
                    if (!*cachedDraw)
                        return;

                    nvrhi::GraphicsState gs;
                    gs.pipeline = pipe;
                    gs.framebuffer = fb;
                    gs.bindings = {*cachedDraw};
                    if (bindlessTable)
                        gs.addBindingSet(bindlessTable);
                    gs.vertexBuffers = {
                        {data.bindlessConfig.megaVertexBuffer, 0, 0},
                        {drawIndexBuffer, 1, 0}};
                    gs.indexBuffer = {data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                    gs.viewport.addViewport(vp);
                    gs.viewport.addScissorRect(scissor);
                    gs.indirectParams = drawArgs;
                    gs.indirectCountBuffer = countBuffer;
                    cmd->setGraphicsState(gs);
                    DrawIndexedIndirectCountOrFallback(
                        cmd, 0, 0, GPUCullingManager::kLocalShadowCullSlotCap);
                };

                drawSet(data.bindlessConfig.staticSet, opaquePipe, 0);

                if (tile.size >= 128 && st.terrainPipeline && st.terrainLayout && data.bindlessConfig.HasTerrain())
                {
                    auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
                    auto* terrainPsRefl = shaderLoader->GetCachedReflection(
                        "shadow\\shadow_cascade_terrain", ".ps");
                    const bool useCompact = data.bindlessConfig.UseTerrainCompaction();
                    nvrhi::IBuffer* tInstance = data.bindlessConfig.terrainInstanceBuffer;
                    nvrhi::IBuffer* tBatch = useCompact
                        ? data.bindlessConfig.terrainCompactBatchIndicesBuffer
                        : data.bindlessConfig.terrainBatchIndicesBuffer;
                    nvrhi::IBuffer* tMat = useCompact
                        ? data.bindlessConfig.terrainCompactMaterialIDBuffer
                        : data.bindlessConfig.terrainMaterialIDBuffer;
                    nvrhi::IBuffer* tArgs = useCompact
                        ? data.bindlessConfig.terrainCompactDrawArgsBuffer
                        : data.bindlessConfig.terrainDrawArgsBuffer;
                    nvrhi::IBuffer* tCount = useCompact
                        ? data.bindlessConfig.terrainCompactCountBuffer
                        : nullptr;
                    if (terrainPsRefl && terrainMatBuffer.GetBuffer() && tInstance && tBatch && tMat && tArgs)
                    {
                        BindingSetBuilder tbsb(*vsRefl, *terrainPsRefl, nv, "LocalShadow.Terrain");
                        tbsb.ConstantBuffer("ShadowCascadeCB", ls.cascadeCB);
                        BindBindlessMaterialTables(tbsb);
                        tbsb.BufferSRV("g_InstanceData", tInstance);
                        tbsb.BufferSRV("g_CompactBatchIndices", tBatch);
                        tbsb.BufferSRV("g_CompactMaterialIDs", tMat);
                        auto terrainSet = cache.GetOrCreateBindingSet(tbsb.Build(), st.terrainLayout, nv);
                        if (terrainSet)
                        {
                            nvrhi::GraphicsState gs;
                            gs.pipeline = st.terrainPipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {terrainSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {data.bindlessConfig.megaVertexBuffer, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {
                                data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                            gs.viewport.addViewport(vp);
                            gs.viewport.addScissorRect(scissor);
                            gs.indirectParams = tArgs;
                            if (tCount)
                                gs.indirectCountBuffer = tCount;
                            cmd->setGraphicsState(gs);
                            DrawIndexedIndirectCountOrFallback(
                                cmd, 0, 0, data.bindlessConfig.terrainObjectCount);
                        }
                    }
                }

                drawSet(data.bindlessConfig.dynamicSet, opaquePipe, 1);
                if (foliagePipe)
                    drawSet(data.bindlessConfig.transparentCasterSet, foliagePipe, 2);

                if (skinMdiFrame && tile.L && tileIdx < tileSkinMdi.size())
                {
                    const TileSkinMdi& tmdi = tileSkinMdi[tileIdx];
                    if (tmdi.ready && ls.localSkinIndirectArgs)
                    {
                        auto& pools = data.gpuCulling->GetSkinnedPools();
                        const u32 groupEnd = tmdi.groupBegin + tmdi.groupCount;
                        for (u32 gi = tmdi.groupBegin; gi < groupEnd && gi < frameSkinGroups.size(); ++gi)
                        {
                            const SkinMdiGroup& g = frameSkinGroups[gi];
                            if (!g.count || !g.var)
                                continue;
                            nvrhi::IBuffer* poolVB = pools.GetVertexBuffer(g.format);
                            nvrhi::IBuffer* poolIB = pools.GetIndexBuffer(g.format);
                            if (!poolVB || !poolIB)
                                continue;

                            nvrhi::GraphicsState gs;
                            gs.pipeline = g.var->pipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {g.var->bindingSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {poolVB, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {poolIB, nvrhi::Format::R16_UINT, 0};
                            gs.viewport.addViewport(vp);
                            gs.viewport.addScissorRect(scissor);
                            gs.indirectParams = ls.localSkinIndirectArgs;
                            cmd->setGraphicsState(gs);
                            cmd->drawIndexedIndirect(
                                g.start * sizeof(IndirectDrawArgs), g.count);
                        }
                    }

                    if (tileIdx < tileSkinResidual.size() && !tileSkinResidual[tileIdx].empty())
                    {
                        auto& residual = tileSkinResidual[tileIdx];
                        std::sort(
                            residual.begin(),
                            residual.end(),
                            [](const SkinCand& a, const SkinCand& b) {
                                if (a.var != b.var)
                                    return a.var < b.var;
                                if (a.draw->vb != b.draw->vb)
                                    return a.draw->vb < b.draw->vb;
                                return a.draw->ib < b.draw->ib;
                            });

                        LocalSkinPipeVariant* curVar = nullptr;
                        nvrhi::IBuffer* curVb = nullptr;
                        nvrhi::IBuffer* curIb = nullptr;
                        for (const SkinCand& cand : residual)
                        {
                            const LocalSkinDraw& draw = *cand.draw;
                            if (cand.var != curVar || draw.vb != curVb || draw.ib != curIb)
                            {
                                curVar = cand.var;
                                curVb = draw.vb;
                                curIb = draw.ib;
                                nvrhi::GraphicsState gs;
                                gs.pipeline = curVar->pipeline;
                                gs.framebuffer = fb;
                                gs.bindings = {curVar->bindingSet};
                                if (bindlessTable)
                                    gs.addBindingSet(bindlessTable);
                                gs.vertexBuffers = {
                                    {draw.vb, 0, 0},
                                    {drawIndexBuffer, 1, 0}};
                                gs.indexBuffer = {draw.ib, nvrhi::Format::R16_UINT, 0};
                                gs.viewport.addViewport(vp);
                                gs.viewport.addScissorRect(scissor);
                                cmd->setGraphicsState(gs);
                            }
                            cmd->drawIndexed(
                                nvrhi::DrawArguments()
                                    .setVertexCount(draw.indexCount)
                                    .setInstanceCount(1)
                                    .setStartIndexLocation(draw.startIndex)
                                    .setStartVertexLocation(draw.baseVertex)
                                    .setStartInstanceLocation(draw.instanceIndex));
                        }
                    }
                }

                ++tilesDrawn;
            }

            if (ps_r_local_shadow_filter != 0 && ls.esmConvertPipeline && ls.esmAtlasTex &&
                ls.esmCB && ls.esmConvertLayout && atlasTex)
            {
                struct EsmCBData
                {
                    u32 page, x0, y0, size;
                    float k;
                    float pad0, pad1, pad2;
                };
                struct EsmBlurCB
                {
                    u32 page, x0, y0, size;
                    u32 horizontal;
                    u32 pad0, pad1, pad2;
                };

                auto* convRefl = shaderLoader->GetCachedReflection("local_shadow_esm_convert", ".cs");
                auto* blurRefl = shaderLoader->GetCachedReflection("local_shadow_esm_blur", ".cs");
                if (convRefl)
                {
                    for (u32 tileIdx = 0; tileIdx < tilesRef.size(); ++tileIdx)
                    {
                        const LocalShadowTile& tile = tilesRef[tileIdx];
                        if (!tile.needsRedraw || tile.size < 8 || tile.page >= ls.sliceCount ||
                            tile.posX + tile.size > cres || tile.posY + tile.size > cres)
                            continue;

                        EsmCBData conv{};
                        conv.page = tile.page;
                        conv.x0 = tile.posX;
                        conv.y0 = tile.posY;
                        conv.size = tile.size;
                        conv.k = 80.f;
                        cmd->writeBuffer(ls.esmCB, &conv, sizeof(conv));

                        BindingSetBuilder cbsb(*convRefl, nv, "LocalShadowEsmConvert");
                        cbsb.ConstantBuffer("LocalShadowEsmCB", ls.esmCB);
                        cbsb.Texture("g_DepthAtlas", atlasTex, nvrhi::Format::R32_FLOAT);
                        cbsb.TextureUAV("g_EsmAtlas", ls.esmAtlasTex);
                        auto convSet = cache.GetOrCreateBindingSet(cbsb.Build(), ls.esmConvertLayout, nv);
                        if (!convSet)
                        {
                            static bool s_once = false;
                            if (!s_once)
                            {
                                Msg("! [LocalShadow] ESM convert binding set failed");
                                s_once = true;
                            }
                            break;
                        }

                        nvrhi::ComputeState cs;
                        cs.pipeline = ls.esmConvertPipeline;
                        cs.bindings = {convSet};
                        cmd->setComputeState(cs);
                        cmd->dispatch((tile.size + 7) / 8, (tile.size + 7) / 8, 1);

                        if (ls.esmBlurPipeline && ls.esmBlurLayout && ls.esmTempTex && blurRefl)
                        {
                            EsmBlurCB blurH{};
                            blurH.page = tile.page;
                            blurH.x0 = tile.posX;
                            blurH.y0 = tile.posY;
                            blurH.size = tile.size;
                            blurH.horizontal = 1;
                            cmd->writeBuffer(ls.esmCB, &blurH, sizeof(blurH));
                            BindingSetBuilder hbsb(*blurRefl, nv, "LocalShadowEsmBlurH");
                            hbsb.ConstantBuffer("LocalShadowEsmBlurCB", ls.esmCB);
                            hbsb.Texture("g_Input", ls.esmAtlasTex);
                            hbsb.TextureUAV("g_Output", ls.esmTempTex);
                            auto hSet = cache.GetOrCreateBindingSet(hbsb.Build(), ls.esmBlurLayout, nv);
                            if (hSet)
                            {
                                nvrhi::ComputeState bcs;
                                bcs.pipeline = ls.esmBlurPipeline;
                                bcs.bindings = {hSet};
                                cmd->setComputeState(bcs);
                                cmd->dispatch((tile.size + 7) / 8, (tile.size + 7) / 8, 1);
                            }

                            EsmBlurCB blurV = blurH;
                            blurV.horizontal = 0;
                            cmd->writeBuffer(ls.esmCB, &blurV, sizeof(blurV));
                            BindingSetBuilder vbsb(*blurRefl, nv, "LocalShadowEsmBlurV");
                            vbsb.ConstantBuffer("LocalShadowEsmBlurCB", ls.esmCB);
                            vbsb.Texture("g_Input", ls.esmTempTex);
                            vbsb.TextureUAV("g_Output", ls.esmAtlasTex);
                            auto vSet = cache.GetOrCreateBindingSet(vbsb.Build(), ls.esmBlurLayout, nv);
                            if (vSet)
                            {
                                nvrhi::ComputeState bcs;
                                bcs.pipeline = ls.esmBlurPipeline;
                                bcs.bindings = {vSet};
                                cmd->setComputeState(bcs);
                                cmd->dispatch((tile.size + 7) / 8, (tile.size + 7) / 8, 1);
                            }
                        }
                    }
                }
            }
        });

    outputs.atlas = passData.atlas;
    outputs.atlasTex = state.atlas;
    outputs.esmTex = state.esmConvertPipeline ? state.esmAtlasTex : nullptr;
    outputs.valid = true;
    return outputs;
}

} // namespace xray::render::fg::passes
