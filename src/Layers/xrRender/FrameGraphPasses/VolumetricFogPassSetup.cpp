#include "stdafx.h"
#include "VolumetricFogPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Volumetrics/VolumetricFogManager.h"
#include "Layers/xrRender/RayTracing/ReSTIRMemoryManager.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

extern ENGINE_API int ps_r_vol_fog;
extern ENGINE_API float ps_r_vol_fog_density;
extern ENGINE_API float ps_r_vol_fog_height;
extern ENGINE_API float ps_r_vol_fog_falloff;
extern ENGINE_API float ps_r_vol_fog_g;
extern ENGINE_API float ps_r_vol_fog_noise;
extern ENGINE_API int ps_r_vol_fog_gi;
extern ENGINE_API int ps_r_vol_fog_sun;
extern ENGINE_API int ps_r_vol_fog_rt;
extern ENGINE_API int ps_r_vol_fog_lights;
extern ENGINE_API int ps_r_vol_fog_temporal;
extern ENGINE_API int ps_r_vol_fog_spot;
extern ENGINE_API int ps_r_atmosphere;
extern ENGINE_API float ps_r_atmosphere_strength;

namespace xray::render::fg::passes {

using namespace framegraph;

struct VolFogCB {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir;
    Fvector4 sunColor;
    Fvector4 fogTune;
    Fvector4 fogTune2;
    Fvector4 fogColor;
    float screenWidth;
    float screenHeight;
    float zNear;
    float zFar;
    u32 frameIndex;
    u32 enableGI;
    u32 enableSun;
    u32 enableRT;
    u32 enableLights;
    u32 enableTemporal;
    u32 spotMode;
    u32 numLights;
    float atmosphereStrength;
    u32 enableAtmosphere;
    u32 playerLight;
    u32 padFog;
    Fvector4 skyColor;
    Fvector4 hemiColor;
};
static_assert(sizeof(VolFogCB) == 320, "VolFogCB must be 320 bytes");

static const u32 kVolFogPipeVersion = 22;

void ShutdownVolumetricFog(VolumetricFogPassState& state)
{
    state.densityPipeline = nullptr;
    state.densityLayout = nullptr;
    state.injectPipeline = nullptr;
    state.injectLayout = nullptr;
    state.accumulatePipeline = nullptr;
    state.accumulateLayout = nullptr;
    state.applyPipeline = nullptr;
    state.applyLayout = nullptr;
    state.cb = nullptr;
    state.sampler = nullptr;
    state.initialized = false;
    state.enabled = false;
    state.pipeVersion = 0;
    VolumetricFogManager::Instance().Shutdown();
}

static void InitializeVolFog(fg::RenderDevice* device, VolumetricFogPassState& state)
{
    if (state.initialized && state.pipeVersion == kVolFogPipeVersion)
        return;
    if (state.initialized)
        ShutdownVolumetricFog(state);

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    VolumetricFogManager::Instance().Init(nv);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    state.sampler = cache.GetOrCreateSampler("VolFog", samplerDesc, nv);
    state.cb = cache.GetOrCreateVolatileCB("VolFog", "VolFog_CB", sizeof(VolFogCB), device, 64);

    auto loadPipe = [&](const char* name, nvrhi::ComputePipelineHandle& pipe, nvrhi::BindingLayoutHandle& layout) {
        auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader(name);
        if (!cs.handle)
            return;
        char layoutName[64];
        xr_sprintf(layoutName, "%s_v%u", name, kVolFogPipeVersion);
        layout = cache.GetOrCreateBindingLayoutFromReflection(layoutName, *cs.reflection, nv);
        nvrhi::ComputePipelineDesc desc;
        desc.CS = cs.handle;
        desc.bindingLayouts = { layout };
        pipe = nv->createComputePipeline(desc);
    };
    loadPipe("vol_fog_density", state.densityPipeline, state.densityLayout);
    {
        auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader("vol_fog_inject");
        if (cs.handle) {
            char layoutName[64];
            xr_sprintf(layoutName, "vol_fog_inject_v%u", kVolFogPipeVersion);
            state.injectLayout = cache.GetOrCreateBindingLayoutFromReflection(layoutName, *cs.reflection, nv);
            nvrhi::ComputePipelineDesc desc;
            desc.CS = cs.handle;
            desc.bindingLayouts = { state.injectLayout };
            state.injectPipeline = nv->createComputePipeline(desc);
        }
    }
    loadPipe("vol_fog_accumulate", state.accumulatePipeline, state.accumulateLayout);
    loadPipe("vol_fog_apply", state.applyPipeline, state.applyLayout);

    state.enabled = state.densityPipeline && state.injectPipeline && state.accumulatePipeline && state.applyPipeline;
    state.initialized = true;
    state.pipeVersion = kVolFogPipeVersion;
}

VirtualResourceHandle setupVolumetricFogPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle worldPos,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height,
    VolumetricFogPassState& state,
    RTAccelStructManager* accelMgr)
{
    if (!ps_r_vol_fog)
        return sceneColor;

    InitializeVolFog(device, state);
    auto& fog = VolumetricFogManager::Instance();
    fog.Ensure();
    if (!state.enabled || !fog.IsReady())
        return sceneColor;

    CEnvironment& env = g_pGamePersistent->Environment();
    Fvector sunDir = env.CurrentEnv.sun_dir;
    Fvector3 sc = { env.CurrentEnv.sun_color.x, env.CurrentEnv.sun_color.y, env.CurrentEnv.sun_color.z };
    const float sunI = std::max({ sc.x, sc.y, sc.z });

    VolFogCB cb{};
    cb.invViewProj = invViewProj;
    cb.prevViewProj = prevViewProj;
    cb.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    cb.sunDir = { sunDir.x, sunDir.y, sunDir.z, 0 };
    cb.sunColor = { sc.x, sc.y, sc.z, sunI > 1e-4f ? 1.f : 0.f };
    const float envDens = std::max(env.CurrentEnv.fog_density, 0.01f);
    const float envDist = std::max(env.CurrentEnv.fog_distance, 1.f);
    cb.fogTune = { ps_r_vol_fog_height, ps_r_vol_fog_falloff, ps_r_vol_fog_density * envDens, ps_r_vol_fog_noise };
    (void)envDist;
    {
        const Fvector3& fog = env.CurrentEnv.fog_color;
        const Fvector3& sky = env.CurrentEnv.sky_color;
        const Fvector4& envc = env.CurrentEnv.env_color;
        cb.fogColor = { fog.x, fog.y, fog.z, 1.f };
        cb.skyColor = { sky.x, sky.y, sky.z, env.CurrentEnv.weight };
        cb.hemiColor = { envc.x, envc.y, envc.z, 1.f };
    }
    auto& mem = ReSTIRMemoryManager::Instance();
    cb.fogTune2 = {
        ps_r_vol_fog_g,
        Device.fTimeGlobal,
        mem.GetIrradianceCache() ? 1.f : 0.f,
        (float)mem.GetIrradianceCacheSize()
    };
    cb.screenWidth = (float)width;
    cb.screenHeight = (float)height;
    cb.zNear = 0.2f;
    cb.zFar = std::max(env.CurrentEnv.far_plane, 80.f);
    cb.frameIndex = Device.dwFrame;
    cb.enableGI = ps_r_vol_fog_gi ? 1u : 0u;
    cb.enableSun = ps_r_vol_fog_sun ? 1u : 0u;
    cb.enableRT = (ps_r_vol_fog_rt && accelMgr && accelMgr->GetTLAS()) ? 1u : 0u;
    cb.enableLights = (u32)std::max(0, ps_r_vol_fog_lights);
    cb.enableTemporal = ps_r_vol_fog_temporal ? 1u : 0u;
    cb.spotMode = (u32)std::clamp(ps_r_vol_fog_spot, 0, 2);
    cb.numLights = ClusteredLightManager::Instance().GetLightCount();
    cb.atmosphereStrength = std::clamp(ps_r_atmosphere_strength, 0.f, 4.f);
    cb.enableAtmosphere = ps_r_atmosphere ? 1u : 0u;
    cb.playerLight = 0;

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    ResolveEnvSkyCubes(device, sky0Tex, sky1Tex);
    if (!sky0Tex)
        sky0Tex = mem.GetPlaceholderCube();
    if (!sky1Tex)
        sky1Tex = mem.GetPlaceholderCube();

    ResourceDesc volDesc;
    volDesc.type = ResourceDesc::Type::Texture3D;
    volDesc.width = VolumetricFogManager::kWidth;
    volDesc.height = VolumetricFogManager::kHeight;
    volDesc.depth = VolumetricFogManager::kDepth;
    volDesc.format = nvrhi::Format::RGBA16_FLOAT;
    volDesc.isUAV = true;
    volDesc.isImported = true;
    volDesc.isTransient = false;
    VirtualResourceHandle fgDensity = fg.ImportTexture("volfog_Density", fog.GetDensity(), volDesc);
    VirtualResourceHandle fgLighting = fg.ImportTexture("volfog_Lighting", fog.GetLighting(), volDesc);
    VirtualResourceHandle fgAccum = fg.ImportTexture("volfog_Accum", fog.GetAccumulated(), volDesc);

    struct DensData {
        fg::RenderDevice* device;
        VolumetricFogPassState* state;
        VolFogCB cb;
    };
    fg.addCallbackPass<DensData>(
        "VolFog Density",
        [&, cb, fgDensity](FrameGraph& builder, PassHandle passHandle, DensData& data) {
            RenderPassBuilder pb(builder, passHandle);
            pb.write(fgDensity, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.state = &state;
            data.cb = cb;
        },
        [](const DensData& data, const FrameGraph&, fg::RenderContext* ctx) {
            auto& fogMgr = VolumetricFogManager::Instance();
            if (!data.state->densityPipeline || !fogMgr.GetDensity())
                return;
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            cmd->writeBuffer(data.state->cb, &data.cb, sizeof(VolFogCB));
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("vol_fog_density", ".cs");
            if (!refl) return;
            BindingSetBuilder bsb(*refl, nv, "VolFog.Density");
            bsb.ConstantBuffer("VolFogParams", data.state->cb);
            bsb.TextureUAV("u_Density", fogMgr.GetDensity());
            auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->densityLayout, nv);
            if (!bs) return;
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->densityPipeline;
            cs.bindings = { bs };
            cmd->setComputeState(cs);
            cmd->dispatch((VolumetricFogManager::kWidth + 7) / 8, (VolumetricFogManager::kHeight + 7) / 8, (VolumetricFogManager::kDepth + 3) / 4);
        });

    struct InjData {
        fg::RenderDevice* device;
        VolumetricFogPassState* state;
        RTAccelStructManager* accelMgr;
        VolFogCB cb;
        nvrhi::ITexture* sky0;
        nvrhi::ITexture* sky1;
    };
    fg.addCallbackPass<InjData>(
        "VolFog Inject",
        [&, cb, fgDensity, fgLighting, accelMgr, sky0Tex, sky1Tex](FrameGraph& builder, PassHandle passHandle, InjData& data) {
            RenderPassBuilder pb(builder, passHandle);
            pb.read(fgDensity, ResourceState::ShaderResource);
            pb.write(fgLighting, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.state = &state;
            data.accelMgr = accelMgr;
            data.cb = cb;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
        },
        [](const InjData& data, const FrameGraph&, fg::RenderContext* ctx) {
            auto& fogMgr = VolumetricFogManager::Instance();
            auto& mem = ReSTIRMemoryManager::Instance();
            auto& clm = ClusteredLightManager::Instance();
            if (!data.state->injectPipeline || !fogMgr.GetLighting())
                return;
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            cmd->writeBuffer(data.state->cb, &data.cb, sizeof(VolFogCB));
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("vol_fog_inject", ".cs");
            if (!refl) return;
            nvrhi::IBuffer* cache = mem.GetIrradianceCache();
            if (!cache) cache = mem.GetPlaceholderBuffer();
            BindingSetBuilder bsb(*refl, nv, "VolFog.Inject");
            bsb.ConstantBuffer("VolFogParams", data.state->cb);
            bsb.Texture("t_Density", fogMgr.GetDensity());
            bsb.BufferSRV("t_IrradianceCache", cache);
            nvrhi::rt::IAccelStruct* tlas = data.accelMgr ? data.accelMgr->GetTLAS() : nullptr;
            if (!tlas && data.accelMgr)
                tlas = data.accelMgr->GetOrCreateEmptyTLAS(cmd);
            if (!tlas)
                return;
            bsb.AccelStruct("g_SceneTLAS", tlas);
            nvrhi::IBuffer* lights = clm.GetLightDataBuffer();
            if (!lights) lights = mem.GetPlaceholderBuffer();
            if (bsb.HasSRV("g_Lights"))
                bsb.BufferSRV("g_Lights", lights);
            if (bsb.HasSRV("t_BlueNoise"))
                bsb.Texture("t_BlueNoise", mem.GetBlueNoise() ? mem.GetBlueNoise() : mem.GetPlaceholderTex3D());
            if (bsb.HasSRV("t_PrevLighting"))
                bsb.Texture("t_PrevLighting", fogMgr.GetLightingHist() ? fogMgr.GetLightingHist() : mem.GetPlaceholderTex3D());
            if (bsb.HasSRV("g_Sky0"))
                bsb.Texture("g_Sky0", data.sky0 ? data.sky0 : mem.GetPlaceholderCube());
            if (bsb.HasSRV("g_Sky1"))
                bsb.Texture("g_Sky1", data.sky1 ? data.sky1 : mem.GetPlaceholderCube());
            bsb.TextureUAV("u_Lighting", fogMgr.GetLighting());
            auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->injectLayout, nv);
            if (!bs) return;
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->injectPipeline;
            cs.bindings = { bs };
            cmd->setComputeState(cs);
            cmd->dispatch((VolumetricFogManager::kWidth + 7) / 8, (VolumetricFogManager::kHeight + 7) / 8, (VolumetricFogManager::kDepth + 3) / 4);
        });

    struct AccData {
        fg::RenderDevice* device;
        VolumetricFogPassState* state;
        VolFogCB cb;
    };
    fg.addCallbackPass<AccData>(
        "VolFog Accumulate",
        [&, cb, fgLighting, fgAccum](FrameGraph& builder, PassHandle passHandle, AccData& data) {
            RenderPassBuilder pb(builder, passHandle);
            pb.read(fgLighting, ResourceState::ShaderResource);
            pb.write(fgAccum, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.state = &state;
            data.cb = cb;
        },
        [](const AccData& data, const FrameGraph&, fg::RenderContext* ctx) {
            auto& fogMgr = VolumetricFogManager::Instance();
            if (!data.state->accumulatePipeline || !fogMgr.GetAccumulated())
                return;
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            cmd->writeBuffer(data.state->cb, &data.cb, sizeof(VolFogCB));
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("vol_fog_accumulate", ".cs");
            if (!refl) return;
            BindingSetBuilder bsb(*refl, nv, "VolFog.Accum");
            bsb.ConstantBuffer("VolFogParams", data.state->cb);
            bsb.Texture("t_Lighting", fogMgr.GetLighting());
            bsb.TextureUAV("u_Accum", fogMgr.GetAccumulated());
            auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->accumulateLayout, nv);
            if (!bs) return;
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->accumulatePipeline;
            cs.bindings = { bs };
            cmd->setComputeState(cs);
            cmd->dispatch((VolumetricFogManager::kWidth + 7) / 8, (VolumetricFogManager::kHeight + 7) / 8, 1);
            if (fogMgr.GetLighting() && fogMgr.GetLightingHist())
                cmd->copyTexture(fogMgr.GetLightingHist(), nvrhi::TextureSlice(), fogMgr.GetLighting(), nvrhi::TextureSlice());
        });

    struct ApplyData {
        fg::RenderDevice* device;
        VolumetricFogPassState* state;
        VirtualResourceHandle depth;
        VirtualResourceHandle worldPos;
        VirtualResourceHandle sceneColor;
        VolFogCB cb;
        nvrhi::ITexture* sky0;
        nvrhi::ITexture* sky1;
        u32 width, height;
    };
    auto& apply = fg.addCallbackPass<ApplyData>(
        "VolFog Apply",
        [&, cb, fgAccum, worldPos, sky0Tex, sky1Tex](FrameGraph& builder, PassHandle passHandle, ApplyData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            if (worldPos.is_valid())
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            pb.read(fgAccum, ResourceState::ShaderResource);
            data.sceneColor = pb.readWrite(sceneColor, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.cb = cb;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
            data.width = width;
            data.height = height;
        },
        [](const ApplyData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto& fogMgr = VolumetricFogManager::Instance();
            auto& mem = ReSTIRMemoryManager::Instance();
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* colorTex = fgGraph.GetPhysicalTexture(data.sceneColor);
            nvrhi::ITexture* worldPosTex = data.worldPos.is_valid()
                ? fgGraph.GetPhysicalTexture(data.worldPos)
                : nullptr;
            if (!worldPosTex)
                worldPosTex = mem.GetPlaceholderTex();
            if (!data.state->applyPipeline || !depthTex || !colorTex)
                return;
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            cmd->writeBuffer(data.state->cb, &data.cb, sizeof(VolFogCB));
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("vol_fog_apply", ".cs");
            if (!refl) return;
            BindingSetBuilder bsb(*refl, nv, "VolFog.Apply");
            bsb.ConstantBuffer("VolFogParams", data.state->cb);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Accum", fogMgr.GetAccumulated());
            bsb.Texture("t_WorldPos", worldPosTex);
            if (bsb.HasSRV("g_Sky0"))
                bsb.Texture("g_Sky0", data.sky0 ? data.sky0 : mem.GetPlaceholderCube());
            if (bsb.HasSRV("g_Sky1"))
                bsb.Texture("g_Sky1", data.sky1 ? data.sky1 : mem.GetPlaceholderCube());
            bsb.TextureUAV("u_SceneColor", colorTex);
            auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->applyLayout, nv);
            if (!bs) return;
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->applyPipeline;
            cs.bindings = { bs };
            cmd->setComputeState(cs);
            cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        });

    return apply.sceneColor;
}

}
