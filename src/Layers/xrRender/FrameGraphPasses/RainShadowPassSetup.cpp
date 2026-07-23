#include "stdafx.h"
#include "RainShadowPassSetup.h"
#include "PassCommon.h"
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
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{

namespace
{

void ComputeTopDownRainMatrices(float extent, float height, u32 smapRes, Fmatrix& outClipVP, Fmatrix& outSampleVP)
{
    const Fvector& C = Device.vCameraPosition;
    Fvector eye;
    eye.set(C.x, C.y + height, C.z);
    Fvector dir(0.f, -1.f, 0.f);
    Fvector up(0.f, 0.f, 1.f);

    Fmatrix view;
    view.build_camera_dir(eye, dir, up);

    const float e = std::max(extent, 8.f);
    Fmatrix proj;
    proj.build_projection_ortho(e * 2.f, e * 2.f, 1.f, height + 120.f);

    outClipVP.mul(proj, view);

    // Clip → UV (same convention as CSM sample VP)
    Fmatrix toUV;
    toUV.identity();
    toUV._11 = 0.5f;
    toUV._22 = -0.5f;
    toUV._33 = 1.f;
    toUV._41 = 0.5f;
    toUV._42 = 0.5f;
    outSampleVP.mul(toUV, outClipVP);

    (void)smapRes;
}

} // namespace

void InitializeRainShadowPass(fg::RenderDevice* device, RainShadowPassState& state)
{
    if (!device)
        return;

    const u32 res = std::clamp((u32)ps_r3_dyn_wet_surf_sm_res, 64u, 2048u);
    if (state.initialized && state.enabled && state.resolution == res && state.rainSM)
        return;

    if (state.initialized)
        ShutdownRainShadowPass(device, state);

    auto* resMgr = device->GetFGResourceManager();
    if (!resMgr || !resMgr->GetRTFactory() || !resMgr->GetTextureManager())
    {
        state.initialized = true;
        state.enabled = false;
        return;
    }

    state.resolution = res;
    state.rainSMHandle = resMgr->GetRTFactory()->CreateShadowMap(res, true, "rt_RainShadow");
    state.rainSM = resMgr->GetTextureManager()->GetNVRHITexture(state.rainSMHandle);
    state.initialized = true;
    state.enabled = (state.rainSM != nullptr);
    if (state.enabled)
        Msg("* [RainShadow] Init: OK (%ux%u)", res, res);
    else
        Msg("! [RainShadow] Failed to create rain SM");
}

void ShutdownRainShadowPass(fg::RenderDevice* device, RainShadowPassState& state)
{
    if (device && device->GetFGResourceManager() && device->GetFGResourceManager()->GetRTFactory())
    {
        if (state.rainSMHandle.IsValid())
            device->GetFGResourceManager()->GetRTFactory()->ReleaseRenderTarget(state.rainSMHandle);
    }
    state.rainSMHandle = {};
    state.rainSM = nullptr;
    state.resolution = 0;
    state.initialized = false;
    state.enabled = false;
}

RainShadowOutputs setupRainShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    RainShadowPassState& state)
{
    using namespace framegraph;
    RainShadowOutputs outputs;
    outputs.valid = false;

    if (!device)
        return outputs;

    const float rainDensity = g_pGamePersistent
        ? g_pGamePersistent->Environment().CurrentEnv.rain_density
        : 0.f;
    const bool wantRain = rainDensity >= 0.001f;
    const bool wantWet = ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF);
    if (!wantRain && !wantWet)
        return outputs;

    InitializeRainShadowPass(device, state);
    if (!state.enabled || !state.rainSM || !shadowState.enabled || !shadowState.pipeline)
        return outputs;

    const float extent = std::max({ps_r3_dyn_wet_surf_far, ps_r3_dyn_wet_surf_near, 40.f});
    // Tall volume so multi-storey roofs above the camera still cast into the map.
    ComputeTopDownRainMatrices(extent, 200.f, state.resolution, state.clipVP, state.sampleVP);

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = state.resolution;
    desc.height = state.resolution;
    desc.depth = 1;
    desc.arraySize = 1;
    desc.format = nvrhi::Format::D32;
    desc.isDepthStencil = true;
    desc.isImported = true;
    desc.debugName = "rt_RainShadow";

    auto rainHandle = fg.ImportTexture("rt_RainShadow", state.rainSM, desc);

    struct PassData
    {
        VirtualResourceHandle rainSM;
        RainShadowPassState* rainState = nullptr;
        ShadowPassState* shadowState = nullptr;
        BindlessForwardConfig bindlessConfig;
        fg::RenderDevice* device = nullptr;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "RainShadow",
        [&, rainHandle](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.rainSM = pb.write(rainHandle, ResourceState::DepthStencilWrite);
            data.rainState = &state;
            data.shadowState = &shadowState;
            data.bindlessConfig = bindlessConfig;
            data.device = device;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.rainState || !data.shadowState)
                return;
            auto* rainTex = graph.GetPhysicalTexture(data.rainSM);
            if (!rainTex)
                rainTex = data.rainState->rainSM;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !rainTex)
                return;

            ShadowPassState& st = *data.shadowState;
            RainShadowPassState& rs = *data.rainState;

            cmd->clearDepthStencilTexture(
                rainTex, nvrhi::TextureSubresourceSet(0, 1, 0, 1), true, 1.0f, false, 0);

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
            {
                static bool s_warned = false;
                if (!s_warned)
                {
                    Msg("! [RainShadow] GPU bindless culling unavailable — rain SM empty (wet cover broken)");
                    s_warned = true;
                }
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("RainShadow", nv);
            if (!vsRefl || !psRefl || !drawIndexBuffer || !st.pipeline || !st.layout || !st.cascadeCB)
                return;

            ShadowCascadeCB cbData{};
            cbData.lightVP = rs.clipVP;
            cmd->writeBuffer(st.cascadeCB, &cbData, sizeof(cbData));

            // Rebuild light-frustum caster lists for the rain ortho volume so roofs
            // outside the camera view still occlude wet surfaces.
            if (auto* fgr = dynamic_cast<FrameGraphRenderer*>(GEnv.Render))
            {
                if (auto* cull = fgr->GetGPUCullingManager())
                    cull->BuildLightFrustumCasters(cmd, nv, rs.clipVP, 1.35f);
            }

            auto& cache = GetPassResourceCache();
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(rainTex);
            auto fb = cache.GetOrCreateFramebuffer(
                make_string("RainShadow_%u", rs.resolution).c_str(), fbDesc, nv);
            if (!fb)
                return;

            auto& matBuffer = bindless::MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto* backend = GEnv.Backend;
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            const u32 cres = rs.resolution;
            nvrhi::Viewport vp(0.f, float(cres), 0.f, float(cres), 0.f, 1.f);

            auto drawSet = [&](const BindlessDrawSet& set) {
                if (!set.IsValid() && !set.IsCastAllValid() && !set.IsLightCullValid())
                    return;
                // Rain/wet SM: prefer cast-all (full caster list) — roofs outside camera
                // compact set must still cast into the top-down rain map.
                const bool castAll = set.IsCastAllValid();
                const bool useLightCull = !castAll && (ps_r_shadow_light_cull != 0) && set.IsLightCullValid();
                if (!castAll && !useLightCull && !set.IsValid())
                    return;

                nvrhi::IBuffer* batchIndices = useLightCull ? set.lightCullBatchIndicesBuffer
                    : (castAll ? set.castAllBatchIndicesBuffer : set.compactBatchIndicesBuffer);
                nvrhi::IBuffer* materialIDs = useLightCull ? set.lightCullMaterialIDBuffer
                    : (castAll ? set.castAllMaterialIDBuffer : set.compactMaterialIDBuffer);
                nvrhi::IBuffer* drawArgs = useLightCull ? set.lightCullDrawArgsBuffer
                    : (castAll ? set.castAllDrawArgsBuffer : set.compactDrawArgsBuffer);
                nvrhi::IBuffer* countBuffer = useLightCull ? set.lightCullCountBuffer
                    : (castAll ? set.castAllCountBuffer : set.compactCountBuffer);

                BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "RainShadow");
                bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
                bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
                bsb.BufferSRV("g_CompactBatchIndices", batchIndices);
                bsb.BufferSRV("g_CompactMaterialIDs", materialIDs);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), st.layout, nv);
                if (!bindingSet)
                    return;

                nvrhi::GraphicsState gs;
                gs.pipeline = st.pipeline;
                gs.framebuffer = fb;
                gs.bindings = {bindingSet};
                if (bindlessTable)
                    gs.addBindingSet(bindlessTable);
                gs.vertexBuffers = {
                    {data.bindlessConfig.megaVertexBuffer, 0, 0},
                    {drawIndexBuffer, 1, 0}};
                gs.indexBuffer = {data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                gs.viewport.addViewport(vp);
                gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                gs.indirectParams = drawArgs;
                gs.indirectCountBuffer = countBuffer;
                cmd->setGraphicsState(gs);
                DrawIndexedIndirectCountOrFallback(cmd, 0, 0, set.totalObjectCount);
            };

            drawSet(data.bindlessConfig.staticSet);
            drawSet(data.bindlessConfig.dynamicSet);
            if (st.foliagePipeline)
                drawSet(data.bindlessConfig.transparentCasterSet);

            // Terrain must cast into rain SM — otherwise hills/roofs of terrain never occlude wet.
            if (st.terrainPipeline && st.terrainLayout && data.bindlessConfig.HasTerrain())
            {
                if (auto* matCache = GEnv.Render ? GEnv.Render->GetMaterialCache() : nullptr)
                    matCache->FinalizePendingTerrainMaterials(ctx);
                auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
                terrainMatBuffer.Upload(ctx);

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
                    BindingSetBuilder tbsb(*vsRefl, *terrainPsRefl, nv, "RainShadow.Terrain");
                    tbsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
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
                        gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                        gs.indirectParams = tArgs;
                        if (tCount)
                            gs.indirectCountBuffer = tCount;
                        cmd->setGraphicsState(gs);
                        DrawIndexedIndirectCountOrFallback(
                            cmd, 0, 0, data.bindlessConfig.terrainObjectCount);
                    }
                }
            }
        });

    outputs.rainSM = passData.rainSM;
    outputs.rainSMTex = state.rainSM;
    outputs.sampleVP = state.sampleVP;
    outputs.valid = true;
    return outputs;
}

} // namespace xray::render::fg::passes
