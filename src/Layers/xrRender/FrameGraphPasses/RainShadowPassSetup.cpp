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

extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API int ps_r_path_tracer;
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
    const float e = std::max(extent, 8.f);
    const float texel = (e * 2.f) / float(std::max(smapRes, 1u));

    Fvector eye;
    eye.set(
        floorf(C.x / texel + 0.5f) * texel,
        C.y + height,
        floorf(C.z / texel + 0.5f) * texel);
    Fvector dir(0.f, -1.f, 0.f);
    Fvector up(0.f, 0.f, 1.f);

    Fmatrix view;
    view.build_camera_dir(eye, dir, up);

    Fmatrix proj;
    proj.build_projection_ortho_stdz(e * 2.f, e * 2.f, 1.f, height + 120.f);

    outClipVP.mul(proj, view);

    Fmatrix toUV;
    toUV.identity();
    toUV._11 = 0.5f;
    toUV._22 = -0.5f;
    toUV._33 = 1.f;
    toUV._41 = 0.5f;
    toUV._42 = 0.5f;
    outSampleVP.mul(toUV, outClipVP);
}

} // namespace

void InitializeRainShadowPass(fg::RenderDevice* device, RainShadowPassState& state)
{
    if (!device)
        return;

    const u32 res = std::clamp((u32)ps_r3_dyn_wet_surf_sm_res, 64u, 2048u);
    if (state.initialized && state.enabled && state.resolution == res
        && state.rainSM && state.rainPipeline && state.rainCB)
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

    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (nv && !state.rainCB)
    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(ShadowCascadeCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 64;
        cbDesc.debugName = "RainShadowCB";
        state.rainCB = nv->createBuffer(cbDesc);
    }
    if (loader && nv)
    {
        auto vs = loader->LoadVertexShader("shadow\\shadow_cascade", "main");
        auto ps = loader->LoadPixelShader("shadow\\shadow_cascade_rain", "main");
        if (vs.handle && ps.handle && vs.reflection && ps.reflection)
        {
            auto& cache = GetPassResourceCache();
            state.rainLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "RainShadowSolid", *vs.reflection, *ps.reflection, nv);
            if (state.rainLayout)
            {
                u32 attrCount = 0;
                auto* attrs = GetUnifiedVertexAttributes(attrCount);
                auto inputLayout = nv->createInputLayout(attrs, attrCount, vs.handle);

                nvrhi::GraphicsPipelineDesc pipeDesc;
                pipeDesc.VS = vs.handle;
                pipeDesc.PS = ps.handle;
                pipeDesc.inputLayout = inputLayout;
                pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                pipeDesc.renderState.depthStencilState.setDepthTestEnable(true);
                pipeDesc.renderState.depthStencilState.setDepthWriteEnable(true);
                pipeDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                pipeDesc.renderState.rasterState.depthBias = 2;
                pipeDesc.renderState.rasterState.slopeScaledDepthBias = 1.5f;

                auto* backend = device->GetBackend();
                nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
                if (bindlessLayout)
                    pipeDesc.bindingLayouts = {state.rainLayout, bindlessLayout};
                else
                    pipeDesc.bindingLayouts = {state.rainLayout};

                nvrhi::FramebufferInfoEx fbInfo;
                fbInfo.depthFormat = nvrhi::Format::D32;
                state.rainPipeline = cache.GetOrCreatePipeline("RainShadowSolid", pipeDesc, fbInfo, nv);

                auto terrainPs = loader->LoadPixelShader("shadow\\shadow_cascade_terrain", "main");
                if (terrainPs.handle && terrainPs.reflection)
                {
                    state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection(
                        "RainShadowTerrain", *vs.reflection, *terrainPs.reflection, nv);
                    if (state.terrainLayout)
                    {
                        nvrhi::GraphicsPipelineDesc terrainDesc = pipeDesc;
                        terrainDesc.PS = terrainPs.handle;
                        if (bindlessLayout)
                            terrainDesc.bindingLayouts = {state.terrainLayout, bindlessLayout};
                        else
                            terrainDesc.bindingLayouts = {state.terrainLayout};
                        state.terrainPipeline = cache.GetOrCreatePipeline(
                            "RainShadowTerrain", terrainDesc, fbInfo, nv);
                    }
                    terrainPs.reflection = nullptr;
                }
            }
        }
    }

    state.initialized = true;
    state.enabled = (state.rainSM != nullptr && state.rainPipeline != nullptr && state.rainCB != nullptr);
    if (state.enabled)
        Msg("* [RainShadow] Init: OK (%ux%u)", res, res);
    else
        Msg("! [RainShadow] Failed to create rain SM / solid pipeline");
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
    state.rainPipeline = nullptr;
    state.rainLayout = nullptr;
    state.terrainPipeline = nullptr;
    state.terrainLayout = nullptr;
    state.rainCB = nullptr;
    state.resolution = 0;
    state.initialized = false;
    state.enabled = false;
}

RainShadowOutputs setupRainShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
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
    if (!state.enabled || !state.rainSM || !state.rainPipeline || !state.rainLayout || !state.rainCB)
        return outputs;
    if (!bindlessConfig.UseGPUCulling() || !bindlessConfig.UseMegaBuffers())
        return outputs;

    const bool rtWet = (ps_r_rt_gi != 0) || (ps_r_path_tracer != 0);
    float requested;
    float maxExtent;
    if (rtWet)
    {
        const float farPlane = g_pGamePersistent
            ? g_pGamePersistent->Environment().CurrentEnv.far_plane
            : 400.f;
        requested = std::max(farPlane * 0.65f, 250.f);
        maxExtent = float(state.resolution) * 0.35f;
    }
    else
    {
        requested = std::max({ps_r3_dyn_wet_surf_far, ps_r3_dyn_wet_surf_near, 40.f});
        maxExtent = float(state.resolution) * 0.05f;
    }
    const float extent = std::min(requested, std::max(maxExtent, 16.f));
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
        BindlessForwardConfig bindlessConfig;
        fg::RenderDevice* device = nullptr;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "RainShadow",
        [&, rainHandle](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.rainSM = pb.write(rainHandle, ResourceState::DepthStencilWrite);
            data.rainState = &state;
            data.bindlessConfig = bindlessConfig;
            data.device = device;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.rainState)
                return;
            auto* rainTex = graph.GetPhysicalTexture(data.rainSM);
            if (!rainTex)
                rainTex = data.rainState->rainSM;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !rainTex)
                return;

            RainShadowPassState& rs = *data.rainState;

            cmd->clearDepthStencilTexture(
                rainTex, nvrhi::TextureSubresourceSet(0, 1, 0, 1), true, 1.0f, false, 0);

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
            {
                static bool s_warned = false;
                if (!s_warned && g_pGameLevel)
                {
                    Msg("! [RainShadow] GPU bindless culling unavailable — rain SM empty (wet cover broken)");
                    s_warned = true;
                }
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_rain", ".ps");
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("RainShadow", nv);
            if (!vsRefl || !psRefl || !drawIndexBuffer || !rs.rainPipeline || !rs.rainLayout || !rs.rainCB)
                return;

            ShadowCascadeCB cbData{};
            cbData.lightVP = rs.clipVP;
            cmd->writeBuffer(rs.rainCB, &cbData, sizeof(cbData));

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

                const bool useLightCull = set.IsLightCullValid();
                const bool castAll = !useLightCull && set.IsCastAllValid();
                if (!useLightCull && !castAll && !set.IsValid())
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
                bsb.ConstantBuffer("ShadowCascadeCB", rs.rainCB);
                BindBindlessMaterialTables(bsb);
                bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
                bsb.BufferSRV("g_CompactBatchIndices", batchIndices);
                bsb.BufferSRV("g_CompactMaterialIDs", materialIDs);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), rs.rainLayout, nv);
                if (!bindingSet)
                    return;

                nvrhi::GraphicsState gs;
                gs.pipeline = rs.rainPipeline;
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

            if (auto* fgr = dynamic_cast<FrameGraphRenderer*>(GEnv.Render))
            {
                if (auto* cull = fgr->GetGPUCullingManager();
                    cull && cull->GetTessObjectCount() > 0)
                {
                    nvrhi::IBuffer* tessInst = cull->GetTessInstanceBuffer();
                    nvrhi::IBuffer* tessMat = cull->GetTessMaterialIDBuffer();
                    nvrhi::IBuffer* tessIdx = cull->GetTessBatchIndicesBuffer();
                    nvrhi::IBuffer* tessArgs = cull->GetTessDrawArgsBuffer();
                    const u32 tessCount = cull->GetTessObjectCount();
                    if (tessInst && tessMat && tessIdx && tessArgs && tessCount > 0)
                    {
                        BindingSetBuilder tsb(*vsRefl, *psRefl, nv, "RainShadow.Tess");
                        tsb.ConstantBuffer("ShadowCascadeCB", rs.rainCB);
                        BindBindlessMaterialTables(tsb);
                        tsb.BufferSRV("g_InstanceData", tessInst);
                        tsb.BufferSRV("g_CompactBatchIndices", tessIdx);
                        tsb.BufferSRV("g_CompactMaterialIDs", tessMat);
                        auto tessSet = cache.GetOrCreateBindingSet(tsb.Build(), rs.rainLayout, nv);
                        if (tessSet)
                        {
                            nvrhi::GraphicsState gs;
                            gs.pipeline = rs.rainPipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {tessSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {data.bindlessConfig.megaVertexBuffer, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {
                                data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                            gs.viewport.addViewport(vp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            gs.indirectParams = tessArgs;
                            cmd->setGraphicsState(gs);
                            cmd->drawIndexedIndirect(0, tessCount);
                        }
                    }
                }
            }

            if (rs.terrainPipeline && rs.terrainLayout && data.bindlessConfig.HasTerrain())
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
                    tbsb.ConstantBuffer("ShadowCascadeCB", rs.rainCB);
                    BindBindlessMaterialTables(tbsb);
                    tbsb.BufferSRV("g_InstanceData", tInstance);
                    tbsb.BufferSRV("g_CompactBatchIndices", tBatch);
                    tbsb.BufferSRV("g_CompactMaterialIDs", tMat);
                    auto terrainSet = cache.GetOrCreateBindingSet(tbsb.Build(), rs.terrainLayout, nv);
                    if (terrainSet)
                    {
                        nvrhi::GraphicsState gs;
                        gs.pipeline = rs.terrainPipeline;
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
