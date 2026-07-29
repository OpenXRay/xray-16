#include "stdafx.h"
#include "VolumetricPassSetup.h"
#include "ParticlePassSetup.h"
#include "ShaderConstants.h"
#include "TAAPassSetup.h"
#include "SunShaftsPassSetup.h"
#include "Layers/xrRender/Volumetrics/VolumetricRenderer.h"
#include "Layers/xrRender/Volumetrics/IVolumetricSource.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"

extern ENGINE_API int ps_r_upscale;

namespace fg
{
extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes
{

using namespace framegraph;

namespace
{

void FillFroxelParams(FroxelParamsCB& cb, const VolumetricRenderer& vol)
{
    ZeroMemory(&cb, sizeof(cb));
    cb.dims[0] = VolumetricRenderer::kFroxelX;
    cb.dims[1] = VolumetricRenderer::kFroxelY;
    cb.dims[2] = VolumetricRenderer::kFroxelZ;
    cb.nearPlane = VIEWPORT_NEAR;
    cb.farPlane = std::max(g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.f, cb.nearPlane * 2.f);
    const float logRatio = log2f(cb.farPlane / cb.nearPlane);
    cb.invLogFarNear = (logRatio > 1e-5f) ? (1.f / logRatio) : 1.f;

    cb.invVP = Device.mInvFullTransform;
    cb.cameraPos = Device.vCameraPosition;

    const auto& fog = vol.GetWorldFog().GetParams();
    cb.fogDensity = fog.density;
    cb.fogHeightFalloff = fog.heightFalloff;
    cb.fogBaseHeight = fog.baseHeight;
    cb.fogAlbedo = fog.albedo;

    if (g_pGamePersistent)
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        cb.sunDir.set(-env.sun_dir.x, -env.sun_dir.y, -env.sun_dir.z);
        cb.sunDir.normalize_safe();
        cb.sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z);
        cb.sunIntensity = ResolveSunShaftsIntensity();

        const float n = env.fog_near;
        const float f = std::max(env.fog_far, n + 1.0f);
        const float r = 1.0f / (f - n);
        cb.classicFogParams.set(-n * r, r, r, r);
    }
    else
    {
        cb.sunDir.set(0.3f, 0.8f, 0.2f);
        cb.sunColor.set(1.f, 0.95f, 0.85f);
        cb.sunIntensity = 1.0f;
        cb.classicFogParams.set(0.0f, 0.001f, 0.001f, 0.001f);
    }
}

nvrhi::ComputePipelineHandle LoadComputePipe(
    const char* passName,
    const char* shaderName,
    nvrhi::IDevice* device,
    nvrhi::BindingLayoutHandle& outLayout)
{
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
        return nullptr;

    auto cs = loader->LoadComputeShader(shaderName);
    if (!cs.handle || !cs.reflection)
    {
        Msg("! [VolumetricPass] Failed to load compute shader: %s", shaderName);
        return nullptr;
    }

    auto& cache = GetPassResourceCache();
    outLayout = cache.GetOrCreateBindingLayoutFromReflection(passName, *cs.reflection, device);
    if (!outLayout)
        return nullptr;

    nvrhi::ComputePipelineDesc desc;
    desc.CS = cs.handle;
    desc.bindingLayouts = {outLayout};
    return cache.GetOrCreateComputePipeline(passName, desc, device);
}

void BindVolumetricLighting(
    BindingSetBuilder& bsb,
    nvrhi::IDevice* nv,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* paramsCB,
    nvrhi::ITexture* froxel,
    const VolumetricLightingInputs& lighting)
{
    bsb.ConstantBuffer("static_globals", staticGlobalsCB)
        .ConstantBuffer("FroxelParams", paramsCB)
        .TextureUAV("u_FroxelVolume", froxel);

    auto& cache = GetPassResourceCache();
    nvrhi::ITexture* dummy2D = cache.GetDummyShadowMap2D(nv);
    nvrhi::ITexture* dummyHzb = cache.GetDummyContactDepth(nv);
    static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
    static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
    for (u32 i = 0; i < 3; ++i)
    {
        nvrhi::ITexture* t = lighting.shadowCascades[i]
            ? lighting.shadowCascades[i]
            : (i == 0 && lighting.shadowMapArray ? lighting.shadowMapArray : dummy2D);
        if (t)
            bsb.Texture(kNames[i], t);
        nvrhi::ITexture* hz = lighting.shadowHZB[i] ? lighting.shadowHZB[i] : dummyHzb;
        if (hz)
            bsb.Texture(kHzb[i], hz);
    }
    nvrhi::ITexture* contactHist = cache.GetDummyContactHistory(nv);
    if (contactHist)
        bsb.Texture("g_ContactHistory", contactHist);

    if (lighting.lightManager)
    {
        if (auto* lights = lighting.lightManager->GetLightDataBuffer())
            bsb.BufferSRV("g_LightData", lights);
        if (auto* grid = lighting.lightManager->GetClusterGridBuffer())
            bsb.BufferSRV("g_ClusterGrid", grid);
        if (auto* idx = lighting.lightManager->GetLightIndexListBuffer())
            bsb.BufferSRV("g_LightIndexList", idx);
    }
}

} // namespace

void InitializeVolumetricPass(nvrhi::IDevice* device, VolumetricPassState& state)
{
    if (state.initialized || !device)
        return;

    state.clearPipeline = LoadComputePipe("VolClear", "volumetric\\clear_volume", device, state.clearLayout);
    state.fogPipeline = LoadComputePipe("VolFog", "volumetric\\world_fog", device, state.fogLayout);
    state.lightPipeline = LoadComputePipe("VolLight", "volumetric\\apply_lighting", device, state.lightLayout);
    state.particlePipeline = LoadComputePipe("VolParticle", "volumetric\\particle_inject", device, state.particleLayout);
    state.lightShaftPipeline = LoadComputePipe("VolLightShaft", "volumetric\\light_shafts", device, state.lightShaftLayout);
    state.temporalPipeline = LoadComputePipe("VolTemporal", "volumetric\\temporal_reproject", device, state.temporalLayout);

    auto* loader = GEnv.Render->GetShaderLoader();
    auto vs = loader->LoadVertexShader("volumetric\\ray_march");
    auto ps = loader->LoadPixelShader("volumetric\\ray_march");
    if (vs.handle && ps.handle && vs.reflection && ps.reflection)
    {
        auto& cache = GetPassResourceCache();
        state.marchLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "VolMarch", *vs.reflection, *ps.reflection, device);

        if (state.marchLayout)
        {
            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.setVertexShader(vs.handle);
            pipeDesc.setPixelShader(ps.handle);
            pipeDesc.addBindingLayout(state.marchLayout);
            pipeDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
            pipeDesc.renderState.blendState.targets[0].setBlendEnable(false);
            pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
            pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);
            pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            nvrhi::FramebufferInfoEx fbInfo;
            fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
            state.marchPipeline = cache.GetOrCreatePipeline("VolMarch", pipeDesc, fbInfo, device);
        }
    }
    else
    {
        Msg("! [VolumetricPass] Failed to load ray_march shaders");
    }

    {
        nvrhi::BufferDesc cb;
        cb.byteSize = sizeof(FroxelParamsCB);
        cb.isConstantBuffer = true;
        cb.isVolatile = true;
        cb.maxVersions = 16;
        cb.debugName = "FroxelParamsCB";
        state.paramsCB = device->createBuffer(cb);
    }
    {
        nvrhi::BufferDesc cb;
        cb.byteSize = sizeof(TemporalParamsCB);
        cb.isConstantBuffer = true;
        cb.isVolatile = true;
        cb.maxVersions = 16;
        cb.debugName = "VolTemporalCB";
        state.temporalCB = device->createBuffer(cb);
    }
    {
        nvrhi::BufferDesc cb;
        cb.byteSize = sizeof(ParticleInjectParamsCB);
        cb.isConstantBuffer = true;
        cb.isVolatile = true;
        cb.maxVersions = 16;
        cb.debugName = "VolParticleParamsCB";
        state.particleParamsCB = device->createBuffer(cb);
    }
    {
        nvrhi::BufferDesc bd;
        bd.byteSize = sizeof(ParticleInjectPointGPU) * 256;
        bd.structStride = sizeof(ParticleInjectPointGPU);
        bd.canHaveUAVs = false;
        bd.keepInitialState = true;
        bd.initialState = nvrhi::ResourceStates::ShaderResource;
        bd.debugName = "VolParticlePoints";
        state.particlePointsCB = device->createBuffer(bd);
    }
    {
        nvrhi::BufferDesc cb;
        cb.byteSize = sizeof(LightShaftParamsCB);
        cb.isConstantBuffer = true;
        cb.isVolatile = true;
        cb.maxVersions = 16;
        cb.debugName = "VolLightShaftCB";
        state.lightShaftCB = device->createBuffer(cb);
    }

    {
        nvrhi::SamplerDesc s;
        s.setAllFilters(true);
        s.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        state.linearSampler = device->createSampler(s);
    }

    state.computeEnabled = state.clearPipeline && state.fogPipeline && state.lightPipeline &&
        state.marchPipeline && state.paramsCB;
    state.initialized = true;

    Msg("* [VolumetricPass] Init: compute=%s temporal=%s particles=%s shafts=%s",
        state.computeEnabled ? "OK" : "FAIL",
        state.temporalPipeline ? "OK" : "off",
        state.particlePipeline ? "OK" : "off",
        state.lightShaftPipeline ? "OK" : "off");
}

void ShutdownVolumetricPass(VolumetricPassState& state)
{
    state.clearPipeline = nullptr;
    state.fogPipeline = nullptr;
    state.lightPipeline = nullptr;
    state.particlePipeline = nullptr;
    state.lightShaftPipeline = nullptr;
    state.temporalPipeline = nullptr;
    state.marchPipeline = nullptr;
    state.clearLayout = nullptr;
    state.fogLayout = nullptr;
    state.lightLayout = nullptr;
    state.particleLayout = nullptr;
    state.lightShaftLayout = nullptr;
    state.temporalLayout = nullptr;
    state.marchLayout = nullptr;
    state.linearSampler = nullptr;
    state.paramsCB = nullptr;
    state.temporalCB = nullptr;
    state.particleParamsCB = nullptr;
    state.particlePointsCB = nullptr;
    state.lightShaftCB = nullptr;
    state.hasPrevVP = false;
    state.initialized = false;
    state.computeEnabled = false;
}

VirtualResourceHandle setupVolumetricPass(
    FrameGraph& fg,
    RenderDevice* device,
    VolumetricRenderer* volumetric,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    u32 width,
    u32 height,
    VolumetricPassState& state,
    const xr_vector<ParticleBatch>* particleBatches,
    const VolumetricLightingInputs* lighting)
{
    if (!volumetric || !volumetric->IsReady() || !device)
        return sceneColor;

    if (device->GetNVRHIDevice())
        InitializeVolumetricPass(device->GetNVRHIDevice(), state);

    if (!state.computeEnabled)
        return sceneColor;

    volumetric->BeginFrame();

    VolumetricLightingInputs lightingCopy{};
    if (lighting)
        lightingCopy = *lighting;

    nvrhi::ITexture* froxelTex = volumetric->GetFroxelVolume();
    ResourceDesc froxelDesc;
    froxelDesc.type = ResourceDesc::Type::Texture3D;
    froxelDesc.width = VolumetricRenderer::kFroxelX;
    froxelDesc.height = VolumetricRenderer::kFroxelY;
    froxelDesc.depth = VolumetricRenderer::kFroxelZ;
    froxelDesc.format = nvrhi::Format::RGBA16_FLOAT;
    froxelDesc.allowUAV = true;
    froxelDesc.isUAV = true;
    froxelDesc.isImported = true;
    froxelDesc.debugName = "FroxelVolume";
    auto froxelHandle = fg.ImportTexture("FroxelVolume", froxelTex, froxelDesc);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.debugName = "rt_VolumetricOut";

    auto& passData = fg.addCallbackPass<VolumetricPassData>(
        "VolumetricFog",
        [&, sceneColor, depth, froxelHandle, width, height, volumetric, particleBatches, lightingCopy](
            FrameGraph& builder, PassHandle passHandle, VolumetricPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.sceneInput = pb.read(sceneColor, ResourceState::ShaderResource);
            data.depthInput = pb.read(depth, ResourceState::ShaderResource);
            data.froxelVolume = pb.readWrite(froxelHandle, ResourceState::UnorderedAccess);
            data.sceneOutput = pb.createTexture("rt_VolumetricOut", outDesc);
            data.volumetric = volumetric;
            data.passState = &state;
            data.particleBatches = particleBatches;
            data.lighting = lightingCopy;
            data.width = width;
            data.height = height;
        },
        [](const VolumetricPassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.passState || !data.passState->computeEnabled || !data.volumetric)
                return;

            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            auto* froxel = graph.GetPhysicalTexture(data.froxelVolume);
            auto* scene = graph.GetPhysicalTexture(data.sceneInput);
            auto* depthTex = graph.GetPhysicalTexture(data.depthInput);
            auto* output = graph.GetPhysicalTexture(data.sceneOutput);
            if (!cmd || !nv || !froxel || !scene || !depthTex || !output)
                return;

            VolumetricPassState& st = *data.passState;
            VolumetricRenderer& vol = *data.volumetric;

            FroxelParamsCB params{};
            FillFroxelParams(params, vol);
            cmd->writeBuffer(st.paramsCB, &params, sizeof(params));

            const u32 gx = (VolumetricRenderer::kFroxelX + 7) / 8;
            const u32 gy = (VolumetricRenderer::kFroxelY + 7) / 8;
            const u32 gz = VolumetricRenderer::kFroxelZ;

            auto& cache = GetPassResourceCache();
            auto dispatchUAV = [&](nvrhi::IComputePipeline* pipe, nvrhi::IBindingLayout* layout) {
                if (!pipe || !layout)
                    return;
                nvrhi::BindingSetDesc bsDesc;
                bsDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, st.paramsCB),
                    nvrhi::BindingSetItem::Texture_UAV(0, froxel),
                };
                auto bs = cache.GetOrCreateBindingSet(bsDesc, layout, nv);
                if (!bs)
                    return;
                nvrhi::ComputeState cs;
                cs.pipeline = pipe;
                cs.bindings = {bs};
                cmd->setComputeState(cs);
                cmd->dispatch(gx, gy, gz);
            };

            cmd->setTextureState(froxel, nvrhi::TextureSubresourceSet{}, nvrhi::ResourceStates::UnorderedAccess);
            dispatchUAV(st.clearPipeline, st.clearLayout);

            auto* loader = GEnv.Render->GetShaderLoader();
            const auto& sources = vol.GetSources();
            for (auto* source : sources)
            {
                if (!source)
                    continue;
                const char* shaderName = source->GetShaderName();
                if (!shaderName)
                    continue;

                if (strstr(shaderName, "world_fog"))
                {
                    dispatchUAV(st.fogPipeline, st.fogLayout);
                }
                else if (strstr(shaderName, "light_shafts") && st.lightShaftPipeline && st.lightShaftLayout && st.lightShaftCB)
                {
                    LightShaftParamsCB shaft{};
                    shaft.sunDir = params.sunDir;
                    shaft.intensity = params.sunIntensity;
                    shaft.albedo = params.fogAlbedo;
                    shaft.densityBoost = params.fogDensity * 0.5f;
                    shaft.intensity = ResolveSunShaftsIntensity();
                    if (shaft.intensity < 1e-4f)
                        continue;
                    cmd->writeBuffer(st.lightShaftCB, &shaft, sizeof(shaft));

                    nvrhi::BindingSetDesc bsDesc;
                    bsDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, st.paramsCB),
                        nvrhi::BindingSetItem::ConstantBuffer(6, st.lightShaftCB),
                        nvrhi::BindingSetItem::Texture_UAV(0, froxel),
                    };
                    auto bs = cache.GetOrCreateBindingSet(bsDesc, st.lightShaftLayout, nv);
                    if (bs)
                    {
                        nvrhi::ComputeState cs;
                        cs.pipeline = st.lightShaftPipeline;
                        cs.bindings = {bs};
                        cmd->setComputeState(cs);
                        cmd->dispatch(gx, gy, gz);
                    }
                }
                else if (strstr(shaderName, "particle") && st.particlePipeline && st.particleLayout &&
                    st.particlePointsCB && st.particleParamsCB)
                {
                    ParticleInjectPointGPU points[256];
                    u32 count = 0;
                    if (data.particleBatches)
                    {
                        for (const auto& batch : *data.particleBatches)
                        {
                            if (count >= 256 || batch.particleCount == 0 || batch.isHUDMode)
                                continue;
                            if (batch.lightingMode != PS::ParticleLightingMode::Volumetric)
                                continue;
                            auto& p = points[count++];
                            p.position = batch.worldMatrix.c;
                            p.radius = 1.5f + 0.02f * float(std::min(batch.particleCount, 64u));
                            p.albedo.set(0.7f, 0.7f, 0.75f);
                            p.density = 0.08f * float(std::min(batch.particleCount, 32u));
                        }
                    }
                    if (count == 0)
                        continue;
                    cmd->writeBuffer(st.particlePointsCB, points, sizeof(ParticleInjectPointGPU) * count);
                    ParticleInjectParamsCB pp{};
                    pp.particleCount = count;
                    cmd->writeBuffer(st.particleParamsCB, &pp, sizeof(pp));

                    nvrhi::BindingSetDesc bsDesc;
                    bsDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, st.paramsCB),
                        nvrhi::BindingSetItem::ConstantBuffer(6, st.particleParamsCB),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, st.particlePointsCB),
                        nvrhi::BindingSetItem::Texture_UAV(0, froxel),
                    };
                    auto bs = cache.GetOrCreateBindingSet(bsDesc, st.particleLayout, nv);
                    if (bs)
                    {
                        nvrhi::ComputeState cs;
                        cs.pipeline = st.particlePipeline;
                        cs.bindings = {bs};
                        cmd->setComputeState(cs);
                        cmd->dispatch(gx, gy, gz);
                    }
                }
            }

            if (st.lightPipeline && st.lightLayout && loader)
            {
                auto* csRefl = loader->GetCachedReflection("volumetric\\apply_lighting", ".cs");
                if (csRefl)
                {
                    auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                        "Frame", "StaticGlobals", sizeof(StaticGlobals), ctx->GetDevice());
                    StaticGlobals sg = BuildStaticGlobals();
                    cmd->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));

                    BindingSetBuilder bsb(*csRefl, nv, "VolLight");
                    BindVolumetricLighting(bsb, nv, staticGlobalsCB, st.paramsCB, froxel, data.lighting);
                    auto bs = cache.GetOrCreateBindingSet(bsb.Build(), st.lightLayout, nv);
                    if (bs)
                    {
                        nvrhi::ComputeState cs;
                        cs.pipeline = st.lightPipeline;
                        cs.bindings = {bs};
                        cmd->setComputeState(cs);
                        cmd->dispatch(gx, gy, gz);
                    }
                }
            }

            nvrhi::ITexture* prevFroxel = vol.GetPrevFroxelVolume();
            const bool allowVolTemporal = ps_r_upscale == 0;
            if (allowVolTemporal && st.temporalPipeline && st.temporalLayout && st.temporalCB && prevFroxel &&
                vol.HasFroxelHistory() && st.hasPrevVP)
            {
                TemporalParamsCB tcb{};
                tcb.prevVP = st.prevVP;
                tcb.temporalAlpha = 0.88f;
                tcb.frameIndex = Device.dwFrame;
                cmd->writeBuffer(st.temporalCB, &tcb, sizeof(tcb));

                cmd->setTextureState(prevFroxel, nvrhi::TextureSubresourceSet{},
                    nvrhi::ResourceStates::ShaderResource);

                nvrhi::BindingSetDesc bsDesc;
                bsDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, st.paramsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(6, st.temporalCB),
                    nvrhi::BindingSetItem::Texture_SRV(0, prevFroxel),
                    nvrhi::BindingSetItem::Texture_UAV(0, froxel),
                    nvrhi::BindingSetItem::Sampler(0, st.linearSampler),
                };
                auto bs = cache.GetOrCreateBindingSet(bsDesc, st.temporalLayout, nv);
                if (bs)
                {
                    nvrhi::ComputeState cs;
                    cs.pipeline = st.temporalPipeline;
                    cs.bindings = {bs};
                    cmd->setComputeState(cs);
                    cmd->dispatch(gx, gy, gz);
                }
            }

            if (prevFroxel)
            {
                cmd->setTextureState(froxel, nvrhi::TextureSubresourceSet{},
                    nvrhi::ResourceStates::CopySource);
                cmd->setTextureState(prevFroxel, nvrhi::TextureSubresourceSet{},
                    nvrhi::ResourceStates::CopyDest);
                cmd->copyTexture(prevFroxel, nvrhi::TextureSlice{}, froxel, nvrhi::TextureSlice{});
                vol.SwapFroxelHistory();
            }

            st.prevVP = g_taa_unjittered_full_transform;
            st.hasPrevVP = true;

            cmd->setTextureState(froxel, nvrhi::TextureSubresourceSet{}, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(scene, nvrhi::TextureSubresourceSet{}, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(depthTex, nvrhi::TextureSubresourceSet{}, nvrhi::ResourceStates::ShaderResource);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(output);
            auto fb = GetPassResourceCache().GetOrCreateFramebuffer("VolumetricMarch", fbDesc, nv);
            if (!fb || !st.marchPipeline || !st.marchLayout)
                return;

            nvrhi::BindingSetDesc marchBs;
            marchBs.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, st.paramsCB),
                nvrhi::BindingSetItem::Texture_SRV(0, scene),
                nvrhi::BindingSetItem::Texture_SRV(1, depthTex),
                nvrhi::BindingSetItem::Texture_SRV(2, froxel),
                nvrhi::BindingSetItem::Sampler(0, st.linearSampler),
            };
            auto marchSet = cache.GetOrCreateBindingSet(marchBs, st.marchLayout, nv);
            if (!marchSet)
                return;

            nvrhi::GraphicsState gs;
            gs.pipeline = st.marchPipeline;
            gs.framebuffer = fb;
            gs.bindings = {marchSet};
            gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(
                static_cast<float>(data.width), static_cast<float>(data.height)));
            cmd->setGraphicsState(gs);

            nvrhi::DrawArguments args;
            args.vertexCount = 3;
            cmd->draw(args);
        });

    return passData.sceneOutput;
}

} // namespace xray::render::fg::passes
