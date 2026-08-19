#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/RayTracing/ReSTIRMemoryManager.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "Layers/xrRender/FrameGraphPasses/TAAPassSetup.h"
#include <nvrhi/utils.h>

extern ENGINE_API float SunshaftsIntensity;
extern ECORE_API u32 ps_r_sun_shafts;
extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API int ps_r_denoise;
extern ENGINE_API int ps_r_rt_gi_spatial_samples;
extern ENGINE_API float ps_r_rt_gi_spatial_radius;
extern ENGINE_API int ps_r_rt_gi_m_max;
extern ENGINE_API int ps_r_rt_di_candidates;
extern ENGINE_API int ps_r_rt_di_spatial_samples;
extern ENGINE_API float ps_r_rt_di_spatial_radius;
extern ENGINE_API int ps_r_rt_di_m_max;
extern ENGINE_API int ps_r_rt_gi_atrous_steps;
extern ENGINE_API float ps_r_rt_gi_temporal_alpha;
extern ENGINE_API int ps_r_rt_gi_bounces;
extern ENGINE_API int ps_r_rt_gi_cache_size;
extern ENGINE_API float ps_r_rt_gi_cache_cell;
extern ENGINE_API float ps_r_rt_gi_ambient_scale;
extern ENGINE_API float ps_r_rt_detail_dist;
extern ENGINE_API float ps_r_rt_gi_lod_dist;
extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r_rt_sun_angular;
extern ENGINE_API int ps_r_rt_refl;
extern ENGINE_API int ps_r_rt_pt_bounces;
extern ENGINE_API int ps_r_rt_pt_ccap;
extern ENGINE_API int ps_r_rt_pt_decorrelate;

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

struct ReSTIRGICB {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    Fvector4 skyColor;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 frameIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 numLights;
    u32 wetEnabled;
    float wetStrength;
    Fvector4 clusterParams;
    Fvector4 clusterDepth;
    Fvector4 diSampleParams;
    u32 bounces;
    u32 cacheSize;
    float cacheCellSize;
    u32 cacheMaxAge;
    u32 grassShadowEnabled;
    u32 padA[3];
    Fmatrix grassShadowVP;
    Fmatrix worldToView;
    Fvector4 hemiColor;
    float lodDist;
    float ambientScale;
    float sunAngular;
    u32 hudSkinnedStart;
    u32 particleBatchStart;
    float fullWidth;
    float fullHeight;
    u32 padEnd;
    Fmatrix prevInvViewProj;
    u32 hasPrevSunVis;
    float currJitterX;
    float currJitterY;
    float prevJitterX;
    float prevJitterY;
    float windSpeed;
    u32 padSun[2];
};
static_assert(sizeof(ReSTIRGICB) == 592, "ReSTIRGICB must be 592 bytes");

struct DITemporalCB {
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 mMax;
    float currJitterX;
    float currJitterY;
    float prevJitterX;
    float prevJitterY;
    float fullWidth;
    float fullHeight;
    Fvector4 clusterParams;
    Fvector4 clusterScales;
};
static_assert(sizeof(DITemporalCB) == 224, "DITemporalCB must be 224 bytes");

struct DISpatialCB {
    Fmatrix invViewProj;
    Fmatrix worldToView;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 spatialSamples;
    float spatialRadius;
    u32 mMax;
    Fvector4 clusterParams;
    Fvector4 clusterScales;
    float fullWidth;
    float fullHeight;
};
static_assert(sizeof(DISpatialCB) == 216, "DISpatialCB must be 216 bytes");

struct DIShadeCB {
    Fmatrix invViewProj;
    Fmatrix worldToView;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    Fvector4 clusterParams;
    Fvector4 clusterDepth;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 particleBatchStart;
    u32 hudSkinnedStart;
    float fullWidth;
    float fullHeight;
    u32 pad2;
};
static_assert(sizeof(DIShadeCB) == 224, "DIShadeCB must be 224 bytes");

struct BlurCB {
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    float phiNormal;
    float phiDepth;
    u32 step;
    u32 mode;
};
static_assert(sizeof(BlurCB) == 32, "BlurCB must be 32 bytes");

struct TemporalFilterCB {
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    float alpha;
    float envAdapt;
    float currJitterX;
    float currJitterY;
    float prevJitterX;
    float prevJitterY;
    u32 enabled;
    u32 pad1;
};
static_assert(sizeof(TemporalFilterCB) == 192, "TemporalFilterCB must be 192 bytes");

struct SpecCB {
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fmatrix prevViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 spatialSamples;
    float spatialRadius;
    u32 hasPrev;
};
static_assert(sizeof(SpecCB) == 240, "SpecCB must be 240 bytes");

struct PTInitialCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDirIntensity;
    Fvector4 sunColorSky;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 maxBounces;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 numLights;
    u32 particleBatchStart;
    u32 hudSkinnedStart;
    u32 pad0;
    u32 pad1;
};
static_assert(sizeof(PTInitialCB) == 176, "PTInitialCB must be 176 bytes");

struct PTTemporalCB {
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    float cCap;
    u32 hasPrev;
    u32 pad;
};
static_assert(sizeof(PTTemporalCB) == 176, "PTTemporalCB must be 176 bytes");

struct PTSpatialCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 pairIndex;
    u32 flipX;
    u32 flipY;
    int offX;
    int offY;
    u32 pass;
    u32 pad;
};
static_assert(sizeof(PTSpatialCB) == 128, "PTSpatialCB must be 128 bytes");

struct PTDupCB {
    float screenWidth;
    float screenHeight;
    u32 pad0;
    u32 pad1;
};
static_assert(sizeof(PTDupCB) == 16, "PTDupCB must be 16 bytes");

struct TemporalCB {
    Fmatrix invViewProj;
    Fmatrix prevInvViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    float envAdapt;
    float currJitterX;
    float currJitterY;
    float prevJitterX;
    float prevJitterY;
};
static_assert(sizeof(TemporalCB) == 184, "TemporalCB must be 184 bytes");

struct SpatialCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 spatialSamples;
    float spatialRadius;
    u32 mMax;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 particleBatchStart;
    float lodDist;
    u32 hudSkinnedStart;
};
static_assert(sizeof(SpatialCB) == 144, "SpatialCB must be 144 bytes");

struct CompositeCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 denoiseApply;
    Fvector4 fogParams;
    Fvector4 fogColor;
    Fvector4 sunDir;
    Fvector4 sunColor;
    float ambientScale;
    u32 cacheSize;
    float cacheCellSize;
    u32 useDdgi;
    u32 addDirect;
    float giWidth;
    float giHeight;
    u32 shaftWidth;
    u32 shaftHeight;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};
static_assert(sizeof(CompositeCB) == 208, "CompositeCB must be 208 bytes");

struct WaterCB {
    Fmatrix invViewProj;
    Fmatrix viewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    Fvector4 skyColor;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 hudSkinnedStart;
    float lodDist;
    u32 pad1;
    u32 pad2;
    Fvector4 hemiColor;
};
static_assert(sizeof(WaterCB) == 256, "WaterCB must be 256 bytes");

struct WetCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float deltaTime;
    float rainFactor;
    float dryRate;
    float maxWet;
    u32 pad[2];
};
static_assert(sizeof(WetCB) == 112, "WetCB must be 112 bytes");

struct SunshaftCB {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor;
    float screenWidth;
    float screenHeight;
    float shaftIntensity;
    float shaftLength;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 hudSkinnedStart;
    u32 shaftSteps;
    u32 alphaEveryN;
    float fullWidth;
    float fullHeight;
    u32 particleBatchStart;
    u32 frameIndex;
    u32 hasPrev;
    Fvector prevSunDir;
};
static_assert(sizeof(SunshaftCB) == 256, "SunshaftCB must be 256 bytes");

struct DDGICB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    Fvector4 gridOrigin_spacing;
    Fvector4 gridDims_intensity;
    float screenWidth;
    float screenHeight;
    u32 frameIndex;
    float pad;
};
static_assert(sizeof(DDGICB) == 128, "DDGICB must be 128 bytes");

static const u32 kReSTIRPipeVersion = 141;

struct RTBatchStarts {
    u32 identityStatic;
    u32 terrain;
    u32 skinnedStart;
    u32 grassStart;
    u32 hudStart;
    u32 particleStart;
    u32 detailAtlas;
};

static RTBatchStarts ComputeBatchStarts(const RTAccelStructManager* accelMgr)
{
    const auto& bc = accelMgr->GetBatchCounts();
    const u32 base = bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal;
    RTBatchStarts s;
    s.identityStatic = bc.identityStatic;
    s.terrain = bc.terrain;
    s.skinnedStart = bc.skinned > 0 ? base : 0xFFFFFFFFu;
    s.grassStart = bc.grass > 0 ? base + bc.skinned : 0xFFFFFFFFu;
    s.hudStart = (bc.skinnedHud > 0 && s.skinnedStart != 0xFFFFFFFFu)
        ? s.skinnedStart + bc.skinnedWorld
        : 0xFFFFFFFFu;
    s.particleStart = bc.particles > 0
        ? base + bc.skinned + (bc.grass > 0 ? 1u : 0u)
        : 0xFFFFFFFFu;
    s.detailAtlas = accelMgr->GetDetailAtlasIndex();
    return s;
}

} // namespace xray::render::fg::passes

bool g_restirPipelinesReady = false;
bool g_restirReplaceForward = false;

namespace xray::render::fg::passes {

static void ComputeEnvAdapt(const CEnvironment& env, float& envScale, float& envAdapt)
{
    const auto& desc = env.CurrentEnv;
    const auto lum = [](float x, float y, float z) {
        return 0.2126f * x + 0.7152f * y + 0.0722f * z;
    };
    const float envL = std::max(
        lum(desc.sky_color.x, desc.sky_color.y, desc.sky_color.z) +
        lum(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z) +
        lum(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z),
        1e-4f);
    static float s_refEnvL = -1.f;
    static float s_smoothEnvL = -1.f;
    if (s_refEnvL < 0.f)
        s_refEnvL = envL;
    if (s_smoothEnvL < 0.f)
        s_smoothEnvL = envL;
    const float prevSmooth = s_smoothEnvL;
    s_smoothEnvL = std::clamp(envL, prevSmooth * 0.94f, prevSmooth * 1.06f);
    envAdapt = s_smoothEnvL / prevSmooth;
    envScale = s_smoothEnvL / s_refEnvL;
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state);

static void InitializeResources(fg::RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized && state.pipeVersion == kReSTIRPipeVersion)
        return;

    if (state.initialized) {
        state.initialPipeline = nullptr;
        state.initialLayout = nullptr;
        state.temporalPipeline = nullptr;
        state.temporalLayout = nullptr;
        state.spatialPipeline = nullptr;
        state.spatialLayout = nullptr;
        state.compositePipeline = nullptr;
        state.compositeLayout = nullptr;
        state.wetPipeline = nullptr;
        state.wetLayout = nullptr;
        state.sunshaftsPipeline = nullptr;
        state.sunshaftsLayout = nullptr;
        state.ddgiPipeline = nullptr;
        state.ddgiLayout = nullptr;
        state.waterPipeline = nullptr;
        state.waterLayout = nullptr;
        state.diTemporalPipeline = nullptr;
        state.diTemporalLayout = nullptr;
        state.diSpatialPipeline = nullptr;
        state.diSpatialLayout = nullptr;
        state.diShadePipeline = nullptr;
        state.diShadeLayout = nullptr;
        state.blurPipeline = nullptr;
        state.blurLayout = nullptr;
        state.temporalFilterPipeline = nullptr;
        state.temporalFilterLayout = nullptr;
        state.specTemporalPipeline = nullptr;
        state.specTemporalLayout = nullptr;
        state.ptInitialPipeline = nullptr;
        state.ptInitialLayout = nullptr;
        state.ptTemporalPipeline = nullptr;
        state.ptTemporalLayout = nullptr;
        state.ptSpatialPipeline = nullptr;
        state.ptSpatialLayout = nullptr;
        state.ptDupPipeline = nullptr;
        state.ptDupLayout = nullptr;
        state.initialized = false;
        state.enabled = false;
        g_restirPipelinesReady = false;
        g_restirReplaceForward = false;
    }

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    ReSTIRMemoryManager::Instance().Init(nvDevice);

    const u32 cbSize = (u32)std::max({
        sizeof(ReSTIRGICB), sizeof(TemporalCB), sizeof(SpatialCB), sizeof(CompositeCB),
        sizeof(WetCB), sizeof(SunshaftCB), sizeof(DDGICB), sizeof(WaterCB),
        sizeof(DITemporalCB), sizeof(DISpatialCB), sizeof(DIShadeCB),
        sizeof(BlurCB), sizeof(TemporalFilterCB), sizeof(SpecCB),
        sizeof(PTInitialCB), sizeof(PTTemporalCB), sizeof(PTSpatialCB), sizeof(PTDupCB) });
        state.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_CB_v103", cbSize, device, 256);

    auto loadPipe = [&](const char* name, nvrhi::ComputePipelineHandle& pipe, nvrhi::BindingLayoutHandle& layout, bool bindless) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader(name);
        if (!csResult.handle) {
            Msg("! [ReSTIR] Failed to load shader '%s'", name);
            return;
        }
        layout = cache.GetOrCreateBindingLayoutFromReflection(name, *csResult.reflection, nvDevice);
        nvrhi::IBindingLayout* bindlessLayout = nullptr;
        if (bindless && GEnv.Backend)
            bindlessLayout = GEnv.Backend->GetBindlessLayout();
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = csResult.handle;
        if (bindlessLayout)
            pipeDesc.bindingLayouts = { layout, bindlessLayout };
        else
            pipeDesc.bindingLayouts = { layout };
        pipe = nvDevice->createComputePipeline(pipeDesc);
        if (!pipe)
            Msg("! [ReSTIR] Failed to create pipeline '%s'", name);
    };

    loadPipe("restir_gi_initial", state.initialPipeline, state.initialLayout, true);
    loadPipe("restir_gi_temporal", state.temporalPipeline, state.temporalLayout, false);
    loadPipe("restir_gi_spatial", state.spatialPipeline, state.spatialLayout, true);
    loadPipe("restir_gi_composite", state.compositePipeline, state.compositeLayout, false);
    loadPipe("restir_wet", state.wetPipeline, state.wetLayout, true);
    loadPipe("restir_sunshafts", state.sunshaftsPipeline, state.sunshaftsLayout, true);
    loadPipe("restir_ddgi", state.ddgiPipeline, state.ddgiLayout, false);
    loadPipe("restir_water_rt", state.waterPipeline, state.waterLayout, true);
    loadPipe("restir_di_temporal", state.diTemporalPipeline, state.diTemporalLayout, true);
    loadPipe("restir_di_spatial", state.diSpatialPipeline, state.diSpatialLayout, true);
    loadPipe("restir_di_shade", state.diShadePipeline, state.diShadeLayout, true);
    loadPipe("restir_gi_blur", state.blurPipeline, state.blurLayout, false);
    loadPipe("restir_gi_temporal_filter", state.temporalFilterPipeline, state.temporalFilterLayout, false);
    loadPipe("restir_spec_temporal", state.specTemporalPipeline, state.specTemporalLayout, false);
    loadPipe("restir_pt_initial", state.ptInitialPipeline, state.ptInitialLayout, true);
    loadPipe("restir_pt_temporal", state.ptTemporalPipeline, state.ptTemporalLayout, false);
    loadPipe("restir_pt_spatial", state.ptSpatialPipeline, state.ptSpatialLayout, false);
    loadPipe("restir_pt_dupmap", state.ptDupPipeline, state.ptDupLayout, false);

    state.enabled = state.initialPipeline && state.temporalPipeline && state.spatialPipeline && state.compositePipeline;
    state.initialized = true;
    state.pipeVersion = kReSTIRPipeVersion;
    g_restirPipelinesReady = state.enabled;
    if (!state.enabled)
        g_restirReplaceForward = false;

    if (state.enabled)
        Msg("* [ReSTIR] Pipelines ready v%u (water=%s bindless=%s)",
            kReSTIRPipeVersion,
            state.waterPipeline ? "ok" : "off",
            GEnv.Backend && GEnv.Backend->GetBindlessLayout() ? "yes" : "no");
    else
        Msg("! [ReSTIR] Pipeline creation failed (initial=%s temporal=%s spatial=%s composite=%s)",
            state.initialPipeline ? "ok" : "FAIL",
            state.temporalPipeline ? "ok" : "FAIL",
            state.spatialPipeline ? "ok" : "FAIL",
            state.compositePipeline ? "ok" : "FAIL");
}

struct InitialPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle sceneColorIn;
    ReSTIRGICB cbData;
    u32 width, height;
    nvrhi::ITexture* sky0;
    nvrhi::ITexture* sky1;
    VirtualResourceHandle grassShadow;
    nvrhi::ITexture* grassShadowTex = nullptr;
    bool hasGrassShadow = false;
    VirtualResourceHandle prevDepth;
    VirtualResourceHandle prevNormals;
    VirtualResourceHandle motionVectors;
    bool hasPrevSunVis = false;
};

struct TemporalPassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle prevNormals;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle prevDepth;
    VirtualResourceHandle motionVectors;
    TemporalCB cbData;
    u32 width, height;
};

struct CompositePassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle sceneColorIn;
    VirtualResourceHandle sceneColor;
    CompositeCB cbData;
    nvrhi::ITexture* sky0;
    nvrhi::ITexture* sky1;
    u32 width, height;
};

struct WetPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle worldPos;
    WetCB cbData;
    u32 width, height;
};

struct SunshaftPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle prevDepth;
    VirtualResourceHandle normal;
    SunshaftCB cbData;
    u32 width, height;
};

struct DDGIPassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    DDGICB cbData;
    u32 width, height;
};

ReSTIRGIOutput setupReSTIRGIPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle worldPos,
    VirtualResourceHandle prevNormals,
    VirtualResourceHandle prevDepth,
    VirtualResourceHandle motionVectors,
    VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fmatrix& prevInvViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData,
    const GrassShadowOutputs& grassShadow)
{
    InitializeResources(device, state);

    ReSTIRGIOutput output{};
    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.debugName = "rtgi_SceneColor";
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isUAV = true;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = false;
    VirtualResourceHandle outHandle = fg.CreateTexture("rtgi_SceneColor", outDesc);

    if (!state.enabled || !accelMgr || !accelMgr->IsReady()) {
        static bool s_logged = false;
        if (!s_logged && ps_r_rt_gi) {
            s_logged = true;
            Msg("! [ReSTIR] Pass inactive (enabled=%d accelReady=%d) — passthrough",
                state.enabled ? 1 : 0, (accelMgr && accelMgr->IsReady()) ? 1 : 0);
        }
        g_restirReplaceForward = false;
        output.sceneColor = sceneColorIn;
        return output;
    }

    g_restirReplaceForward = true;

    auto& mem = ReSTIRMemoryManager::Instance();
    mem.Ensure(width, height);
    if (!mem.IsReady()) {
        Msg("! [ReSTIR] MemoryManager not ready after Ensure(%ux%u)", width, height);
        g_restirReplaceForward = false;
        output.sceneColor = sceneColorIn;
        return output;
    }

    const u32 giW = mem.GetGiWidth();
    const u32 giH = mem.GetGiHeight();
    const u32 shaftW = mem.GetShaftWidth();
    const u32 shaftH = mem.GetShaftHeight();

    output.noisyDiffuse = mem.GetNoisyDiffuse();
    output.noisySpecular = mem.GetNoisySpecular();
    output.hitDistance = mem.GetHitDistance();

    const u32 workIdx = ReSTIRMemoryManager::WorkIndex();
    const u32 histIdx = ReSTIRMemoryManager::HistoryIndex();

    CEnvironment& env = g_pGamePersistent->Environment();
    float envScale = 1.f;
    float envAdapt = 1.f;
    ComputeEnvAdapt(env, envScale, envAdapt);
    nvrhi::ITexture* sky0Tex = mem.GetPlaceholderCube();
    nvrhi::ITexture* sky1Tex = mem.GetPlaceholderCube();
    float skyWeight = env.CurrentEnv.weight;

    nvrhi::ITexture* envSky0 = nullptr;
    nvrhi::ITexture* envSky1 = nullptr;
    ResolveEnvSkyCubes(device, envSky0, envSky1);
    if (envSky0)
        sky0Tex = envSky0;
    if (envSky1)
        sky1Tex = envSky1;

    Fvector sunDir = env.CurrentEnv.sun_dir;
    Fvector3 sc = { env.CurrentEnv.sun_color.x, env.CurrentEnv.sun_color.y, env.CurrentEnv.sun_color.z };
    float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    Fvector sunColor;
    if (sunIntensity > 0.001f)
        sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
    else
        sunColor.set(0, 0, 0);

    auto& clm = ClusteredLightManager::Instance();
    const auto& batchCounts = accelMgr->GetBatchCounts();

    const bool wetEnabled = ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF);
    float rainFactor = 0.f;
    if (g_pGamePersistent && wetEnabled)
        rainFactor = g_pGamePersistent->Environment().CurrentEnv.rain_density;
    float shaftIntensityEarly = 0.f;
    if (ps_r_sun_shafts > 0 && g_pGamePersistent && !Device.dwPrecacheFrame)
    {
        shaftIntensityEarly = g_pGamePersistent->Environment().CurrentEnv.m_fSunShaftsIntensity;
        if (SunshaftsIntensity > 0.f)
            shaftIntensityEarly = SunshaftsIntensity;
    }
    const bool wantSunshafts = state.sunshaftsPipeline && ps_r_sun_shafts > 0 && shaftIntensityEarly >= 0.0001f;

    ReSTIRGICB initialCB;
    initialCB.invViewProj = invViewProj;
    initialCB.prevViewProj = prevViewProj;
    initialCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    initialCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    initialCB.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
    {
        const Fvector3& skc = env.CurrentEnv.sky_color;
        initialCB.skyColor = { skc.x, skc.y, skc.z, envScale };
    }
    initialCB.screenWidth = (float)giW;
    initialCB.screenHeight = (float)giH;
    initialCB.giIntensity = giIntensity;
    initialCB.frameIndex = Device.dwFrame;
    initialCB.identityStaticCount = batchCounts.identityStatic;
    initialCB.terrainBatchCount = batchCounts.terrain;
    initialCB.skinnedBatchStart = batchCounts.skinned > 0
        ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal
        : 0xFFFFFFFFu;
    initialCB.grassBatchStart = batchCounts.grass > 0
        ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned
        : 0xFFFFFFFFu;
    initialCB.detailAtlasIndex = accelMgr->GetDetailAtlasIndex();
    initialCB.hudSkinnedStart = (batchCounts.skinnedHud > 0 && initialCB.skinnedBatchStart != 0xFFFFFFFFu)
        ? initialCB.skinnedBatchStart + batchCounts.skinnedWorld
        : 0xFFFFFFFFu;
    initialCB.particleBatchStart = batchCounts.particles > 0
        ? (initialCB.grassBatchStart != 0xFFFFFFFFu
            ? initialCB.grassBatchStart + 1u
            : (initialCB.skinnedBatchStart != 0xFFFFFFFFu
                ? initialCB.skinnedBatchStart + batchCounts.skinned
                : batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal))
        : 0xFFFFFFFFu;
    initialCB.fullWidth = (float)width;
    initialCB.fullHeight = (float)height;
    initialCB.padEnd = 0;
    initialCB.prevInvViewProj = prevInvViewProj;
    initialCB.hasPrevSunVis = (hasPrevFrameData && motionVectors.is_valid()) ? 1u : 0u;
    initialCB.currJitterX = g_taa_jitter_px;
    initialCB.currJitterY = g_taa_jitter_py;
    initialCB.prevJitterX = g_taa_jitter_prev_px;
    initialCB.prevJitterY = g_taa_jitter_prev_py;
    initialCB.windSpeed = 0.f;
    if (g_pGamePersistent)
        initialCB.windSpeed = g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier;
    initialCB.padSun[0] = 0;
    initialCB.padSun[1] = 0;
    initialCB.numLights = clm.GetLightCount();
    if (initialCB.numLights > RESTIR_MAX_LIGHTS)
        initialCB.numLights = RESTIR_MAX_LIGHTS;
    initialCB.wetEnabled = wetEnabled ? 1u : 0u;
    initialCB.wetStrength = 1.0f;

    ClusterCB ccb = clm.BuildClusterCB(width, height, 0.2f, 500.f);
    initialCB.clusterParams = { ccb.gridDims.x, ccb.gridDims.y, ccb.gridDims.z, (float)clm.GetLightCount() };
    initialCB.clusterDepth = ccb.depthParams;
    const u32 diCand = (u32)std::clamp(ps_r_rt_di_candidates, 1, 16);
    initialCB.diSampleParams = {
        (float)clm.GetDILightCount(),
        clm.GetDIPowerSum(),
        (float)diCand,
        0.f
    };
    initialCB.bounces = (u32)std::clamp(ps_r_rt_gi_bounces, 1, 2);
    initialCB.cacheSize = (u32)std::max(0, ps_r_rt_gi_cache_size);
    initialCB.cacheCellSize = std::max(0.05f, ps_r_rt_gi_cache_cell);
    initialCB.cacheMaxAge = 64;
    initialCB.grassShadowEnabled = grassShadow.valid ? 1u : 0u;
    if (grassShadow.valid)
        initialCB.grassShadowVP = grassShadow.sampleVP;
    else
        initialCB.grassShadowVP.identity();
    initialCB.worldToView = Device.mView;
    {
        const Fvector4& hc = env.CurrentEnv.hemi_color;
        initialCB.hemiColor = { hc.x, hc.y, hc.z, hc.w };
    }
    initialCB.lodDist = std::max(10.f, ps_r_rt_gi_lod_dist);
    initialCB.ambientScale = std::clamp(ps_r_rt_gi_ambient_scale, 0.f, 1.f);
    initialCB.sunAngular = std::clamp(ps_r_rt_sun_angular, 0.001f, 0.05f);
    mem.EnsureIrradianceCache(initialCB.cacheSize);
    const bool usePT = (ps_r_rt_gi >= 2) && state.ptInitialPipeline && mem.GetPTReservoirA(0);
    if (usePT)
        initialCB.bounces = (u32)std::clamp(ps_r_rt_pt_bounces, 1, 8);

    ResourceDesc persistDesc;
    persistDesc.type = ResourceDesc::Type::Texture2D;
    persistDesc.width = giW;
    persistDesc.height = giH;
    persistDesc.isImported = true;
    persistDesc.isTransient = false;
    persistDesc.isUAV = true;

    auto dlDesc = persistDesc;
    dlDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgDirectLighting = fg.ImportTexture("rtgi_DirectLighting", mem.GetDirectLighting(), dlDesc);
    VirtualResourceHandle fgNoisyDiff = fg.ImportTexture("rtgi_NoisyDiffuse", mem.GetNoisyDiffuse(), dlDesc);
    VirtualResourceHandle fgNoisySpec = fg.ImportTexture("rtgi_NoisySpecular", mem.GetNoisySpecular(), dlDesc);
    VirtualResourceHandle fgDdgiAmb = fg.ImportTexture("rtgi_DDGIAmbient", mem.GetDdgiAmbient(), dlDesc);

    auto shaftDesc = persistDesc;
    shaftDesc.width = shaftW;
    shaftDesc.height = shaftH;
    shaftDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgSunshafts = fg.ImportTexture("rtgi_Sunshafts", mem.GetSunshafts(), shaftDesc);

    auto hitDesc = persistDesc;
    hitDesc.format = nvrhi::Format::R16_FLOAT;
    VirtualResourceHandle fgHitDist = fg.ImportTexture("rtgi_HitDistance", mem.GetHitDistance(), hitDesc);

    auto wetDesc = persistDesc;
    wetDesc.width = width;
    wetDesc.height = height;
    wetDesc.format = nvrhi::Format::R16_FLOAT;
    VirtualResourceHandle fgWet = fg.ImportTexture("rtgi_WetAccum", mem.GetWetAccum(), wetDesc);

    fg.GetRTRegistry().RegisterRT("rt_DirectLighting", fgDirectLighting);
    fg.GetRTRegistry().RegisterRT("rt_GI_NoisyDiffuse", fgNoisyDiff);
    fg.GetRTRegistry().RegisterRT("rt_GI_NoisySpecular", fgNoisySpec);
    fg.GetRTRegistry().RegisterRT("rt_GI_HitDistance", fgHitDist);

    const bool runWetPass = state.wetPipeline != nullptr;
    if (runWetPass) {
        WetCB wetCB{};
        wetCB.invViewProj = invViewProj;
        wetCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        wetCB.screenWidth = (float)width;
        wetCB.screenHeight = (float)height;
        wetCB.deltaTime = Device.fTimeDelta;
        wetCB.rainFactor = (wetEnabled ? rainFactor : 0.f) * 0.35f;
        wetCB.dryRate = 0.08f;
        wetCB.maxWet = 1.0f;

        fg.addCallbackPass<WetPassData>(
            "ReSTIR Wet",
            [&, wetCB, fgWet, accelMgr](FrameGraph& builder, PassHandle passHandle, WetPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.readWrite(fgWet, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = wetCB;
                data.width = width;
                data.height = height;
            },
            [](const WetPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* wetTex = ReSTIRMemoryManager::Instance().GetWetAccum();
                auto* skyOpenTex = ReSTIRMemoryManager::Instance().GetSkyOpen();
                if (!depthTex || !normalTex || !wetTex || !skyOpenTex || !data.state->wetPipeline || !data.accelMgr)
                    return;
                auto* tlas = data.accelMgr->GetTLAS();
                auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                auto* megaVB = data.accelMgr->GetMegaVB();
                auto* megaIB = data.accelMgr->GetMegaIB();
                auto* matBuf = data.accelMgr->GetMaterialBuffer();
                if (!tlas || !batchInfo || !megaVB || !megaIB || !matBuf)
                    return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(WetCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_wet", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.Wet");
                bsb.ConstantBuffer("WetParams", data.state->cb);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : ReSTIRMemoryManager::Instance().GetPlaceholderColorTex());
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_Materials", matBuf);
                bsb.TextureUAV("u_WetAccum", wetTex);
                bsb.TextureUAV("u_SkyOpen", skyOpenTex);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->wetLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->wetPipeline;
                cs.bindings = { bs };
                auto* backend = data.device->GetBackend();
                if (backend && backend->GetBindlessDescriptorTable())
                    cs.bindings.push_back(backend->GetBindlessDescriptorTable());
                cmd->setTextureState(skyOpenTex, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
                cmd->setTextureState(wetTex, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                cmd->setTextureState(skyOpenTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                cmd->setTextureState(wetTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }
        );

    } else if (mem.GetSkyOpen()) {
        struct SkyOpenClearData {};
        fg.addCallbackPass<SkyOpenClearData>(
            "ReSTIR SkyOpen Clear",
            [&](FrameGraph& builder, PassHandle passHandle, SkyOpenClearData&) {
                RenderPassBuilder pb(builder, passHandle);
                pb.sideEffects();
            },
            [](const SkyOpenClearData&, const FrameGraph&, fg::RenderContext* ctx) {
                auto* sky = ReSTIRMemoryManager::Instance().GetSkyOpen();
                if (!sky || !ctx) return;
                ctx->GetCommandList()->clearTextureFloat(sky, nvrhi::AllSubresources, nvrhi::Color(1.f));
            });
    }

    if (!usePT)
    fg.addCallbackPass<InitialPassData>(
        "ReSTIR Initial",
        [&, sky0Tex, sky1Tex, initialCB, fgDirectLighting, fgNoisyDiff, fgNoisySpec, fgHitDist, fgWet, grassShadow, sceneColorIn, hasPrevFrameData](
            FrameGraph& builder, PassHandle passHandle, InitialPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            if (worldPos.is_valid())
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
            pb.write(fgDirectLighting, ResourceState::UnorderedAccess);
            pb.write(fgNoisyDiff, ResourceState::UnorderedAccess);
            pb.write(fgNoisySpec, ResourceState::UnorderedAccess);
            pb.write(fgHitDist, ResourceState::UnorderedAccess);
            pb.read(fgWet, ResourceState::ShaderResource);
            data.hasGrassShadow = grassShadow.valid && grassShadow.shadowMap.is_valid();
            if (data.hasGrassShadow)
                data.grassShadow = pb.read(grassShadow.shadowMap, ResourceState::ShaderResource);
            data.grassShadowTex = grassShadow.shadowTex;
            data.hasPrevSunVis = hasPrevFrameData && motionVectors.is_valid();
            if (data.hasPrevSunVis) {
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
            }
            pb.sideEffects();
            data.device = device;
            data.accelMgr = accelMgr;
            data.state = &state;
            data.cbData = initialCB;
            data.width = giW;
            data.height = giH;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
        },
        [workIdx](const InitialPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto& memMgr = ReSTIRMemoryManager::Instance();
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* sceneInTex = fgGraph.GetPhysicalTexture(data.sceneColorIn);
            if (!depthTex || !normalTex || !baseColorTex)
                return;

            nvrhi::ITexture* sky0 = data.sky0;
            nvrhi::ITexture* sky1 = data.sky1;
            auto* tlas = data.accelMgr->GetTLAS();
            auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
            auto* megaVB = data.accelMgr->GetMegaVB();
            auto* megaIB = data.accelMgr->GetMegaIB();
            auto* matBuf = data.accelMgr->GetMaterialBuffer();
            auto* terrainBuf = data.accelMgr->GetTerrainMaterialBuffer();
            nvrhi::IBuffer* resBuf = memMgr.GetReservoirBuffer(workIdx);
            nvrhi::ITexture* directLit = memMgr.GetDirectLighting();
            nvrhi::ITexture* noisyDiff = memMgr.GetNoisyDiffuse();
            nvrhi::ITexture* noisySpec = memMgr.GetNoisySpecular();
            nvrhi::ITexture* hitDist = memMgr.GetHitDistance();
            nvrhi::ITexture* wetTex = memMgr.GetWetAccum();
            nvrhi::ITexture* skyOpenTex = memMgr.GetSkyOpen();

            if (!sky0 || !sky1 || !directLit || !resBuf || !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf)
                return;

            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            memMgr.EnsureBlueNoiseUploaded(cmd);
            if (memMgr.ConsumeHistoryReset())
                memMgr.ClearHistoryTargets(cmd);

            ReSTIRGICB cb = data.cbData;
            const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
            cb.identityStaticCount = starts.identityStatic;
            cb.terrainBatchCount = starts.terrain;
            cb.skinnedBatchStart = starts.skinnedStart;
            cb.grassBatchStart = starts.grassStart;
            cb.hudSkinnedStart = starts.hudStart;
            cb.particleBatchStart = starts.particleStart;
            cb.detailAtlasIndex = starts.detailAtlas;
            auto& clmLocal = ClusteredLightManager::Instance();
            cb.numLights = clmLocal.GetLightCount();
            if (cb.numLights > RESTIR_MAX_LIGHTS)
                cb.numLights = RESTIR_MAX_LIGHTS;
            ClusterCB ccbLive = clmLocal.BuildClusterCB((u32)data.cbData.fullWidth, (u32)data.cbData.fullHeight, 0.2f, 500.f);
            cb.clusterParams = { ccbLive.gridDims.x, ccbLive.gridDims.y, ccbLive.gridDims.z, (float)clmLocal.GetLightCount() };
            cb.clusterDepth = ccbLive.depthParams;
            cb.diSampleParams = {
                (float)clmLocal.GetDILightCount(),
                clmLocal.GetDIPowerSum(),
                (float)std::clamp(ps_r_rt_di_candidates, 1, 16),
                0.f
            };
            cmd->writeBuffer(data.state->cb, &cb, sizeof(ReSTIRGICB));

            nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
            nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!skinnedVB) skinnedVB = memMgr.GetPlaceholderBuffer();
            if (!skinnedIB) skinnedIB = memMgr.GetPlaceholderBuffer();
            if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
            if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();

            nvrhi::IBuffer* lights = clmLocal.GetLightDataBuffer();
            if (!lights)
                lights = memMgr.GetLightDataBuffer();
            nvrhi::IBuffer* clusterGrid = clmLocal.GetClusterGridBuffer();
            nvrhi::IBuffer* lightIndexList = clmLocal.GetLightIndexListBuffer();
            nvrhi::IBuffer* diIndices = clmLocal.GetDILightIndicesBuffer();
            nvrhi::IBuffer* diCdf = clmLocal.GetDILightCDFBuffer();
            if (!clusterGrid) clusterGrid = memMgr.GetPlaceholderBuffer();
            if (!lightIndexList) lightIndexList = memMgr.GetPlaceholderBuffer();
            if (!diIndices) diIndices = memMgr.GetPlaceholderBuffer();
            if (!diCdf) diCdf = memMgr.GetPlaceholderBuffer();

            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_initial", ".cs");
            if (!refl) return;

            BindingSetBuilder bsb(*refl, nv, "ReSTIR.Initial");
            bsb.ConstantBuffer("ReSTIRGIParams", data.state->cb);
            bsb.AccelStruct("g_SceneTLAS", tlas);
            bsb.BufferSRV("g_BatchInfo", batchInfo);
            bsb.BufferSRV("g_MegaVB", megaVB);
            bsb.BufferSRV("g_MegaIB", megaIB);
            bsb.Texture("g_Sky0", sky0);
            bsb.Texture("g_Sky1", sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", matBuf);
            bsb.BufferSRV("g_TerrainMaterials", terrainBuf);
            bsb.BufferSRV("g_VariantTextures", bindless::VariantTextureBuffer::Instance().GetBuffer());
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_SceneColorIn", sceneInTex ? sceneInTex : memMgr.GetPlaceholderColorTex());
            bsb.BufferSRV("g_Lights", lights);
            bsb.Texture("t_WetAccum", wetTex ? wetTex : memMgr.GetPlaceholderTex());
            if (bsb.HasSRV("t_SkyOpen"))
                bsb.Texture("t_SkyOpen", skyOpenTex ? skyOpenTex : memMgr.GetPlaceholderTex());
            nvrhi::ITexture* grassShadowTex = data.grassShadowTex;
            if (!grassShadowTex && data.hasGrassShadow && data.grassShadow.is_valid())
                grassShadowTex = fgGraph.GetPhysicalTexture(data.grassShadow);
            bsb.Texture("t_GrassShadow", grassShadowTex ? grassShadowTex : memMgr.GetPlaceholderTex());
            nvrhi::ITexture* blueNoise = memMgr.GetBlueNoise();
            if (bsb.HasSRV("t_BlueNoise"))
                bsb.Texture("t_BlueNoise", blueNoise ? blueNoise : memMgr.GetPlaceholderTex3D());
            const u32 sunWrite = data.state->currTemporalIdx & 1u;
            nvrhi::ITexture* prevSunVis = data.hasPrevSunVis ? memMgr.GetSunVis(1u - sunWrite) : nullptr;
            bsb.Texture("t_PrevSunVis", prevSunVis ? prevSunVis : memMgr.GetPlaceholderTex());
            auto* mvTex = data.motionVectors.is_valid() ? fgGraph.GetPhysicalTexture(data.motionVectors) : nullptr;
            auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
            auto* prevNormalTex = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
            bsb.Texture("t_MotionVectors", mvTex ? mvTex : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_PrevDepth", prevDepthTex ? prevDepthTex : memMgr.GetPlaceholderTex());
            bsb.Texture("t_PrevNormal", prevNormalTex ? prevNormalTex : memMgr.GetPlaceholderColorTex());
            nvrhi::IBuffer* particleVB = data.accelMgr->GetParticleOutputVB();
            nvrhi::IBuffer* particleIB = data.accelMgr->GetParticleIB();
            if (!particleVB) particleVB = memMgr.GetPlaceholderBuffer();
            if (!particleIB) particleIB = memMgr.GetPlaceholderBuffer();
            bsb.BufferSRV("g_ParticleVB", particleVB);
            bsb.BufferSRV("g_ParticleIB", particleIB);
            bsb.BufferSRV("g_ClusterGrid", clusterGrid);
            bsb.BufferSRV("g_LightIndexList", lightIndexList);
            bsb.BufferSRV("g_DILightIndices", diIndices);
            bsb.BufferSRV("g_DILightCDF", diCdf);
            bsb.TextureUAV("u_DirectLighting", directLit);
            bsb.BufferUAV("u_Reservoir", resBuf);
            bsb.TextureUAV("u_NoisyDiffuse", noisyDiff);
            bsb.TextureUAV("u_NoisySpecular", noisySpec);
            bsb.TextureUAV("u_HitDistance", hitDist);
            nvrhi::ITexture* diRes = memMgr.GetDIReservoir(data.state->currTemporalIdx);
            bsb.TextureUAV("u_DIReservoir", diRes ? diRes : memMgr.GetPlaceholderColorTex());
            nvrhi::ITexture* specA = memMgr.GetSpecReservoirA(0);
            nvrhi::ITexture* specB = memMgr.GetSpecReservoirB(0);
            bsb.TextureUAV("u_SpecReservoirA", specA ? specA : memMgr.GetPlaceholderColorTex());
            bsb.TextureUAV("u_SpecReservoirB", specB ? specB : memMgr.GetPlaceholderColorTex());
            nvrhi::IBuffer* cache = memMgr.GetIrradianceCache();
            bsb.BufferUAV("u_IrradianceCache", cache ? cache : memMgr.GetPlaceholderBuffer());
            nvrhi::ITexture* sunVis = memMgr.GetSunVis(sunWrite);
            bsb.TextureUAV("u_SunVis", sunVis ? sunVis : memMgr.GetPlaceholderTex());
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->initialLayout, nv);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->initialPipeline;
            cs.bindings = { bindingSet };
            if (GEnv.Backend) {
                if (auto* bindlessTable = GEnv.Backend->GetBindlessDescriptorTable())
                    cs.addBindingSet(bindlessTable);
            }
            cmd->setComputeState(cs);
            cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    if (!usePT && hasPrevFrameData && motionVectors.is_valid() && state.temporalPipeline) {
        TemporalCB temporalCB;
        temporalCB.invViewProj = invViewProj;
        temporalCB.prevInvViewProj = prevInvViewProj;
        temporalCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        temporalCB.screenWidth = (float)giW;
        temporalCB.screenHeight = (float)giH;
        temporalCB.invScreenWidth = 1.0f / (float)giW;
        temporalCB.invScreenHeight = 1.0f / (float)giH;
        temporalCB.frameIndex = Device.dwFrame;
        temporalCB.envAdapt = envAdapt;
        temporalCB.currJitterX = g_taa_jitter_px;
        temporalCB.currJitterY = g_taa_jitter_py;
        temporalCB.prevJitterX = g_taa_jitter_prev_px;
        temporalCB.prevJitterY = g_taa_jitter_prev_py;

        fg.addCallbackPass<TemporalPassData>(
            "ReSTIR Temporal",
            [&, temporalCB](FrameGraph& builder, PassHandle passHandle, TemporalPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = temporalCB;
                data.width = giW;
                data.height = giH;
            },
            [workIdx, histIdx](const TemporalPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* mvTex = fgGraph.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !normalTex || !baseColorTex || !mvTex) return;

                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalCB));

                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_temporal", ".cs");
                if (!refl) return;

                BindingSetBuilder bsb(*refl, nv, "ReSTIR.Temporal");
                bsb.ConstantBuffer("ReSTIRTemporalParams", data.state->cb);
                bsb.BufferSRV("t_PrevReservoir", memMgr.GetReservoirBuffer(histIdx));
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex);
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                bsb.BufferUAV("u_Reservoir", memMgr.GetReservoirBuffer(workIdx));
                auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->temporalLayout, nv);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalPipeline;
                cs.bindings = { bindingSet };
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    if (!usePT && state.spatialPipeline && ps_r_rt_gi_spatial_samples > 0) {
        SpatialCB spatialCB;
        spatialCB.invViewProj = invViewProj;
        spatialCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        spatialCB.screenWidth = (float)giW;
        spatialCB.screenHeight = (float)giH;
        spatialCB.invScreenWidth = 1.0f / (float)giW;
        spatialCB.invScreenHeight = 1.0f / (float)giH;
        spatialCB.frameIndex = Device.dwFrame;
        spatialCB.spatialSamples = (u32)ps_r_rt_gi_spatial_samples;
        spatialCB.spatialRadius = ps_r_rt_gi_spatial_radius;
        spatialCB.mMax = (u32)std::max(1, ps_r_rt_gi_m_max);
        spatialCB.identityStaticCount = initialCB.identityStaticCount;
        spatialCB.terrainBatchCount = initialCB.terrainBatchCount;
        spatialCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        spatialCB.grassBatchStart = initialCB.grassBatchStart;
        spatialCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        spatialCB.particleBatchStart = initialCB.particleBatchStart;
        spatialCB.lodDist = initialCB.lodDist;
        spatialCB.hudSkinnedStart = initialCB.hudSkinnedStart;

        struct SpatialPassDataRT {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            SpatialCB cbData;
            u32 width, height;
        };

        fg.addCallbackPass<SpatialPassDataRT>(
            "ReSTIR Spatial",
            [&, spatialCB](FrameGraph& builder, PassHandle passHandle, SpatialPassDataRT& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = spatialCB;
                data.width = giW;
                data.height = giH;
            },
            [workIdx, histIdx](const SpatialPassDataRT& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                if (!depthTex || !normalTex || !baseColorTex) return;

                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                SpatialCB cb = data.cbData;
                const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
                cb.identityStaticCount = starts.identityStatic;
                cb.terrainBatchCount = starts.terrain;
                cb.skinnedBatchStart = starts.skinnedStart;
                cb.grassBatchStart = starts.grassStart;
                cb.hudSkinnedStart = starts.hudStart;
                cb.particleBatchStart = starts.particleStart;
                cb.detailAtlasIndex = starts.detailAtlas;
                cmd->writeBuffer(data.state->cb, &cb, sizeof(SpatialCB));

                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_spatial", ".cs");
                if (!refl) return;

                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
                if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();
                nvrhi::IBuffer* particleVB = data.accelMgr->GetParticleOutputVB();
                nvrhi::IBuffer* particleIB = data.accelMgr->GetParticleIB();
                if (!particleVB) particleVB = memMgr.GetPlaceholderBuffer();
                if (!particleIB) particleIB = memMgr.GetPlaceholderBuffer();

                BindingSetBuilder bsb(*refl, nv, "ReSTIR.Spatial");
                bsb.ConstantBuffer("ReSTIRSpatialParams", data.state->cb);
                bsb.BufferSRV("t_InReservoir", memMgr.GetReservoirBuffer(workIdx));
                bsb.AccelStruct("g_SceneTLAS", data.accelMgr->GetTLAS());
                bsb.BufferSRV("g_BatchInfo", data.accelMgr->GetBatchInfoBuffer());
                bsb.BufferSRV("g_MegaVB", data.accelMgr->GetMegaVB());
                bsb.BufferSRV("g_MegaIB", data.accelMgr->GetMegaIB());
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.BufferSRV("g_ParticleVB", particleVB);
                bsb.BufferSRV("g_ParticleIB", particleIB);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                if (bsb.HasSRV("t_BlueNoise"))
                    bsb.Texture("t_BlueNoise", memMgr.GetBlueNoise() ? memMgr.GetBlueNoise() : memMgr.GetPlaceholderTex3D());
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                BindBindlessMaterialTables(bsb);
                bsb.BufferUAV("u_OutReservoir", memMgr.GetReservoirBuffer(histIdx));
                bsb.TextureUAV("u_NoisyDiffuse", memMgr.GetNoisyDiffuse());
                auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->spatialLayout, nv);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->spatialPipeline;
                cs.bindings = { bindingSet };
                if (GEnv.Backend)
                    if (auto* t = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(t);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    if (!usePT && ps_r_rt_refl > 0 && state.specTemporalPipeline) {
        SpecCB specCB{};
        specCB.invViewProj = invViewProj;
        specCB.prevInvViewProj = prevInvViewProj;
        specCB.prevViewProj = prevViewProj;
        specCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        specCB.screenWidth = (float)giW;
        specCB.screenHeight = (float)giH;
        specCB.invScreenWidth = 1.0f / (float)giW;
        specCB.invScreenHeight = 1.0f / (float)giH;
        specCB.frameIndex = Device.dwFrame;
        specCB.spatialSamples = (ps_r_rt_refl >= 2) ? 4u : 0u;
        specCB.spatialRadius = 8.0f;
        specCB.hasPrev = hasPrevFrameData ? 1u : 0u;
        struct SpecPassData {
            fg::RenderDevice* device;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, prevNormals, baseColor, worldPos, prevDepth, motionVectors;
            SpecCB cbData;
            u32 width, height;
        };
        fg.addCallbackPass<SpecPassData>(
            "ReSTIR Spec Temporal",
            [&, specCB](FrameGraph& builder, PassHandle passHandle, SpecPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                if (motionVectors.is_valid())
                    data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = specCB;
                data.width = giW;
                data.height = giH;
            },
            [](const SpecPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* prevN = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* mvTex = data.motionVectors.is_valid() ? fgGraph.GetPhysicalTexture(data.motionVectors) : nullptr;
                if (!depthTex || !normalTex || !baseColorTex) return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(SpecCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_spec_temporal", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.SpecTemporal");
                bsb.ConstantBuffer("ReSTIRSpecParams", data.state->cb);
                bsb.Texture("t_CurrA", memMgr.GetSpecReservoirA(0));
                bsb.Texture("t_CurrB", memMgr.GetSpecReservoirB(0));
                bsb.Texture("t_PrevA", memMgr.GetSpecReservoirA(1));
                bsb.Texture("t_PrevB", memMgr.GetSpecReservoirB(1));
                bsb.Texture("t_MotionVectors", mvTex ? mvTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_PrevNormal", prevN);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex);
                bsb.TextureUAV("u_OutA", memMgr.GetSpecReservoirA(1));
                bsb.TextureUAV("u_OutB", memMgr.GetSpecReservoirB(1));
                bsb.TextureUAV("u_NoisySpecular", memMgr.GetNoisySpecular());
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->specTemporalLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->specTemporalPipeline;
                cs.bindings = { bs };
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            });
    }

    const u32 diWrite = state.currTemporalIdx & 1u;
    const u32 diRead = 1u - diWrite;
    Fmatrix worldToView = Device.mView;
    ClusterCB clusterForDI = clm.BuildClusterCB(width, height, 0.2f, 500.f);

    if (!usePT && hasPrevFrameData && motionVectors.is_valid() && state.diTemporalPipeline && mem.GetDIReservoir(diWrite)) {
        DITemporalCB diTempCB{};
        diTempCB.invViewProj = invViewProj;
        {
            diTempCB.prevInvViewProj = prevInvViewProj;
        }
        diTempCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diTempCB.screenWidth = (float)giW;
        diTempCB.screenHeight = (float)giH;
        diTempCB.fullWidth = (float)width;
        diTempCB.fullHeight = (float)height;
        diTempCB.invScreenWidth = 1.0f / (float)giW;
        diTempCB.invScreenHeight = 1.0f / (float)giH;
        diTempCB.frameIndex = Device.dwFrame;
        diTempCB.mMax = (u32)std::max(1, ps_r_rt_di_m_max);
        diTempCB.currJitterX = g_taa_jitter_px;
        diTempCB.currJitterY = g_taa_jitter_py;
        diTempCB.prevJitterX = g_taa_jitter_prev_px;
        diTempCB.prevJitterY = g_taa_jitter_prev_py;
        diTempCB.clusterParams = { clusterForDI.gridDims.x, clusterForDI.gridDims.y, clusterForDI.gridDims.z, (float)clm.GetLightCount() };
        diTempCB.clusterScales = clusterForDI.depthParams;

        struct DITemporalPassData {
            fg::RenderDevice* device;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, baseColor, worldPos, prevDepth, prevNormals, motionVectors;
            DITemporalCB cbData;
            u32 width, height, writeIdx, readIdx;
        };

        fg.addCallbackPass<DITemporalPassData>(
            "ReSTIR DI Temporal",
            [&, diTempCB, diWrite, diRead](FrameGraph& builder, PassHandle passHandle, DITemporalPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = diTempCB;
                data.width = giW;
                data.height = giH;
                data.writeIdx = diWrite;
                data.readIdx = diRead;
            },
            [](const DITemporalPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* motionTex = fgGraph.GetPhysicalTexture(data.motionVectors);
                auto* lights = ClusteredLightManager::Instance().GetLightDataBuffer();
                if (!depthTex || !normalTex || !baseColorTex || !motionTex || !lights) return;
                nvrhi::ITexture* dst = memMgr.GetDIReservoir(data.writeIdx);
                nvrhi::ITexture* prev = memMgr.GetDIReservoir(data.readIdx);
                if (!dst || !prev) return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(DITemporalCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_temporal", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.DITemporal");
                bsb.ConstantBuffer("ReSTIRDITemporalParams", data.state->cb);
                bsb.BufferSRV("g_LightData", lights);
                bsb.Texture("t_PrevDI", prev);
                bsb.Texture("t_MotionVectors", motionTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex);
                bsb.Texture("t_Normal", normalTex);
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                bsb.TextureUAV("u_DIReservoir", dst);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->diTemporalLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diTemporalPipeline;
                cs.bindings = { bs };
                if (GEnv.Backend)
                    if (auto* t = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(t);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    u32 diSrc = diWrite;
    u32 diDst = diRead;
    if (!usePT && state.diSpatialPipeline && ps_r_rt_di_spatial_samples > 0 && mem.GetDIReservoir(0)) {
        DISpatialCB diSpatCB{};
        diSpatCB.invViewProj = invViewProj;
        diSpatCB.worldToView = worldToView;
        diSpatCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diSpatCB.screenWidth = (float)giW;
        diSpatCB.screenHeight = (float)giH;
        diSpatCB.fullWidth = (float)width;
        diSpatCB.fullHeight = (float)height;
        diSpatCB.invScreenWidth = 1.0f / (float)giW;
        diSpatCB.invScreenHeight = 1.0f / (float)giH;
        diSpatCB.frameIndex = Device.dwFrame;
        diSpatCB.spatialSamples = (u32)ps_r_rt_di_spatial_samples;
        diSpatCB.spatialRadius = ps_r_rt_di_spatial_radius;
        diSpatCB.mMax = (u32)std::max(1, ps_r_rt_di_m_max);
        diSpatCB.clusterParams = { clusterForDI.gridDims.x, clusterForDI.gridDims.y, clusterForDI.gridDims.z, (float)clm.GetLightCount() };
        diSpatCB.clusterScales = clusterForDI.depthParams;

        struct DISpatialPassData {
            fg::RenderDevice* device;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, baseColor, worldPos;
            DISpatialCB cbData;
            u32 width, height, srcIdx, dstIdx;
        };

        fg.addCallbackPass<DISpatialPassData>(
            "ReSTIR DI Spatial",
            [&, diSpatCB, diSrc, diDst](FrameGraph& builder, PassHandle passHandle, DISpatialPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = diSpatCB;
                data.width = giW;
                data.height = giH;
                data.srcIdx = diSrc;
                data.dstIdx = diDst;
            },
            [](const DISpatialPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* lights = ClusteredLightManager::Instance().GetLightDataBuffer();
                nvrhi::ITexture* src = memMgr.GetDIReservoir(data.srcIdx);
                nvrhi::ITexture* dst = memMgr.GetDIReservoir(data.dstIdx);
                if (!depthTex || !normalTex || !baseColorTex || !src || !dst || !lights) return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(DISpatialCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_spatial", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.DISpatial");
                bsb.ConstantBuffer("ReSTIRDISpatialParams", data.state->cb);
                bsb.BufferSRV("g_LightData", lights);
                bsb.Texture("t_SrcDI", src);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_Normal", normalTex);
                bsb.TextureUAV("u_DIReservoir", dst);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->diSpatialLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diSpatialPipeline;
                cs.bindings = { bs };
                if (GEnv.Backend)
                    if (auto* t = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(t);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
        std::swap(diSrc, diDst);
    }

    if (!usePT && state.diShadePipeline && mem.GetDIReservoir(diSrc)) {
        DIShadeCB diShadeCB{};
        diShadeCB.invViewProj = invViewProj;
        diShadeCB.worldToView = worldToView;
        diShadeCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diShadeCB.screenWidth = (float)giW;
        diShadeCB.screenHeight = (float)giH;
        diShadeCB.fullWidth = (float)width;
        diShadeCB.fullHeight = (float)height;
        diShadeCB.grassBatchStart = initialCB.grassBatchStart;
        diShadeCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        diShadeCB.clusterParams = { clusterForDI.gridDims.x, clusterForDI.gridDims.y, clusterForDI.gridDims.z, (float)clm.GetLightCount() };
        diShadeCB.clusterDepth = clusterForDI.depthParams;
        diShadeCB.identityStaticCount = initialCB.identityStaticCount;
        diShadeCB.terrainBatchCount = initialCB.terrainBatchCount;
        diShadeCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        diShadeCB.particleBatchStart = initialCB.particleBatchStart;
        diShadeCB.hudSkinnedStart = initialCB.hudSkinnedStart;
        diShadeCB.pad2 = Device.dwFrame;

        struct DIShadePassData {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, baseColor, worldPos;
            DIShadeCB cbData;
            u32 width, height, srcIdx;
        };

        fg.addCallbackPass<DIShadePassData>(
            "ReSTIR DI Shade",
            [&, diShadeCB, diSrc, fgDirectLighting](FrameGraph& builder, PassHandle passHandle, DIShadePassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.readWrite(fgDirectLighting, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = diShadeCB;
                data.width = giW;
                data.height = giH;
                data.srcIdx = diSrc;
            },
            [](const DIShadePassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* lights = ClusteredLightManager::Instance().GetLightDataBuffer();
                auto* clusterGrid = ClusteredLightManager::Instance().GetClusterGridBuffer();
                auto* lightIndexList = ClusteredLightManager::Instance().GetLightIndexListBuffer();
                auto* tlas = data.accelMgr->GetTLAS();
                auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                auto* megaVB = data.accelMgr->GetMegaVB();
                auto* megaIB = data.accelMgr->GetMegaIB();
                auto* diTex = memMgr.GetDIReservoir(data.srcIdx);
                auto* direct = memMgr.GetDirectLighting();
                if (!depthTex || !diTex || !direct || !tlas || !lights || !clusterGrid || !lightIndexList) return;
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
                if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();
                nvrhi::IBuffer* particleVB = data.accelMgr->GetParticleOutputVB();
                nvrhi::IBuffer* particleIB = data.accelMgr->GetParticleIB();
                if (!particleVB) particleVB = memMgr.GetPlaceholderBuffer();
                if (!particleIB) particleIB = memMgr.GetPlaceholderBuffer();
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                DIShadeCB cb = data.cbData;
                const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
                cb.identityStaticCount = starts.identityStatic;
                cb.terrainBatchCount = starts.terrain;
                cb.skinnedBatchStart = starts.skinnedStart;
                cb.grassBatchStart = starts.grassStart;
                cb.hudSkinnedStart = starts.hudStart;
                cb.particleBatchStart = starts.particleStart;
                cb.detailAtlasIndex = starts.detailAtlas;
                cmd->writeBuffer(data.state->cb, &cb, sizeof(DIShadeCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_shade", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.DIShade");
                bsb.ConstantBuffer("ReSTIRDIShadeParams", data.state->cb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_LightData", lights);
                bsb.BufferSRV("g_ClusterGrid", clusterGrid);
                bsb.BufferSRV("g_LightIndexList", lightIndexList);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.BufferSRV("g_ParticleVB", particleVB);
                bsb.BufferSRV("g_ParticleIB", particleIB);
                bsb.Texture("t_DIReservoir", diTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_Normal", normalTex);
                if (bsb.HasSRV("t_BlueNoise"))
                    bsb.Texture("t_BlueNoise", memMgr.GetBlueNoise() ? memMgr.GetBlueNoise() : memMgr.GetPlaceholderTex3D());
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                BindBindlessMaterialTables(bsb);
                bsb.TextureUAV("u_DirectLighting", direct);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->diShadeLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diShadePipeline;
                cs.bindings = { bs };
                if (GEnv.Backend)
                    if (auto* t = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(t);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    if (usePT) {
        struct PTInitData {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, baseColor, worldPos, sceneColorIn;
            VirtualResourceHandle grassShadow, prevDepth, prevNormals, motionVectors;
            nvrhi::ITexture* grassShadowTex = nullptr;
            bool hasGrassShadow = false;
            bool hasPrevSunVis = false;
            ReSTIRGICB cb;
            nvrhi::ITexture* sky0;
            nvrhi::ITexture* sky1;
            u32 width, height;
        };
        fg.addCallbackPass<PTInitData>(
            "ReSTIR PT Initial",
            [&, initialCB, sky0Tex, sky1Tex, grassShadow, sceneColorIn, hasPrevFrameData](
                FrameGraph& builder, PassHandle passHandle, PTInitData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
                data.hasGrassShadow = grassShadow.valid && grassShadow.shadowMap.is_valid();
                if (data.hasGrassShadow)
                    data.grassShadow = pb.read(grassShadow.shadowMap, ResourceState::ShaderResource);
                data.grassShadowTex = grassShadow.shadowTex;
                data.hasPrevSunVis = hasPrevFrameData && motionVectors.is_valid();
                if (data.hasPrevSunVis) {
                    if (prevDepth.is_valid())
                        data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                    if (prevNormals.is_valid())
                        data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                    data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                }
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cb = initialCB;
                data.sky0 = sky0Tex;
                data.sky1 = sky1Tex;
                data.width = giW;
                data.height = giH;
            },
            [](const PTInitData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto& clm = ClusteredLightManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* sceneInTex = data.sceneColorIn.is_valid() ? fgGraph.GetPhysicalTexture(data.sceneColorIn) : nullptr;
                auto* tlas = data.accelMgr->GetTLAS();
                if (!depthTex || !tlas) return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                ReSTIRGICB cb = data.cb;
                const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
                cb.identityStaticCount = starts.identityStatic;
                cb.terrainBatchCount = starts.terrain;
                cb.skinnedBatchStart = starts.skinnedStart;
                cb.grassBatchStart = starts.grassStart;
                cb.hudSkinnedStart = starts.hudStart;
                cb.detailAtlasIndex = starts.detailAtlas;
                cb.numLights = clm.GetLightCount();
                ClusterCB ccbLive = clm.BuildClusterCB((u32)cb.fullWidth, (u32)cb.fullHeight, 0.2f, 500.f);
                cb.clusterParams = { ccbLive.gridDims.x, ccbLive.gridDims.y, ccbLive.gridDims.z, (float)clm.GetLightCount() };
                cb.clusterDepth = ccbLive.depthParams;
                cb.diSampleParams = {
                    (float)clm.GetDILightCount(),
                    clm.GetDIPowerSum(),
                    (float)std::clamp(ps_r_rt_di_candidates, 1, 16),
                    0.f
                };
                cb.hasPrevSunVis = data.hasPrevSunVis ? 1u : 0u;
                cmd->writeBuffer(data.state->cb, &cb, sizeof(ReSTIRGICB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_pt_initial", ".cs");
                if (!refl) return;
                nvrhi::IBuffer* lights = clm.GetLightDataBuffer();
                if (!lights) lights = memMgr.GetPlaceholderBuffer();
                nvrhi::IBuffer* clusterGrid = clm.GetClusterGridBuffer();
                nvrhi::IBuffer* lightIndexList = clm.GetLightIndexListBuffer();
                nvrhi::IBuffer* diIndices = clm.GetDILightIndicesBuffer();
                nvrhi::IBuffer* diCdf = clm.GetDILightCDFBuffer();
                if (!clusterGrid) clusterGrid = memMgr.GetPlaceholderBuffer();
                if (!lightIndexList) lightIndexList = memMgr.GetPlaceholderBuffer();
                if (!diIndices) diIndices = memMgr.GetPlaceholderBuffer();
                if (!diCdf) diCdf = memMgr.GetPlaceholderBuffer();
                nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
                nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!skinnedVB) skinnedVB = memMgr.GetPlaceholderBuffer();
                if (!skinnedIB) skinnedIB = memMgr.GetPlaceholderBuffer();
                if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
                if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();
                nvrhi::IBuffer* particleVB = data.accelMgr->GetParticleOutputVB();
                nvrhi::IBuffer* particleIB = data.accelMgr->GetParticleIB();
                if (!particleVB) particleVB = memMgr.GetPlaceholderBuffer();
                if (!particleIB) particleIB = memMgr.GetPlaceholderBuffer();
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.PTInitial");
                bsb.ConstantBuffer("ReSTIRGIParams", data.state->cb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", data.accelMgr->GetBatchInfoBuffer());
                bsb.BufferSRV("g_MegaVB", data.accelMgr->GetMegaVB());
                bsb.BufferSRV("g_MegaIB", data.accelMgr->GetMegaIB());
                bsb.Texture("g_Sky0", data.sky0 ? data.sky0 : memMgr.GetPlaceholderCube());
                bsb.Texture("g_Sky1", data.sky1 ? data.sky1 : memMgr.GetPlaceholderCube());
                bsb.BufferSRV("g_SkinnedVB", skinnedVB);
                bsb.BufferSRV("g_SkinnedIB", skinnedIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.BufferSRV("g_Lights", lights);
                bsb.Texture("t_WetAccum", memMgr.GetWetAccum() ? memMgr.GetWetAccum() : memMgr.GetPlaceholderTex());
                bsb.BufferSRV("g_ClusterGrid", clusterGrid);
                bsb.BufferSRV("g_LightIndexList", lightIndexList);
                bsb.BufferSRV("g_DILightIndices", diIndices);
                bsb.BufferSRV("g_DILightCDF", diCdf);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_SceneColorIn", sceneInTex ? sceneInTex : memMgr.GetPlaceholderColorTex());
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                nvrhi::ITexture* grassShadowTex = data.grassShadowTex;
                if (!grassShadowTex && data.hasGrassShadow && data.grassShadow.is_valid())
                    grassShadowTex = fgGraph.GetPhysicalTexture(data.grassShadow);
                bsb.Texture("t_GrassShadow", grassShadowTex ? grassShadowTex : memMgr.GetPlaceholderTex());
                bsb.BufferSRV("g_ParticleVB", particleVB);
                bsb.BufferSRV("g_ParticleIB", particleIB);
                nvrhi::ITexture* blueNoise = memMgr.GetBlueNoise();
                if (bsb.HasSRV("t_BlueNoise"))
                    bsb.Texture("t_BlueNoise", blueNoise ? blueNoise : memMgr.GetPlaceholderTex3D());
                const u32 sunWrite = data.state->currTemporalIdx & 1u;
                nvrhi::ITexture* prevSunVis = data.hasPrevSunVis ? memMgr.GetSunVis(1u - sunWrite) : nullptr;
                bsb.Texture("t_PrevSunVis", prevSunVis ? prevSunVis : memMgr.GetPlaceholderTex());
                auto* mvTex = data.motionVectors.is_valid() ? fgGraph.GetPhysicalTexture(data.motionVectors) : nullptr;
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* prevNormalTex = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                bsb.Texture("t_MotionVectors", mvTex ? mvTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex ? prevDepthTex : memMgr.GetPlaceholderTex());
                bsb.Texture("t_PrevNormal", prevNormalTex ? prevNormalTex : memMgr.GetPlaceholderColorTex());
                BindBindlessMaterialTables(bsb);
                bsb.TextureUAV("u_PTA", memMgr.GetPTReservoirA(0));
                bsb.TextureUAV("u_PTB", memMgr.GetPTReservoirB(0));
                bsb.TextureUAV("u_NoisyDiffuse", memMgr.GetNoisyDiffuse());
                bsb.TextureUAV("u_NoisySpecular", memMgr.GetNoisySpecular());
                bsb.TextureUAV("u_HitDistance", memMgr.GetHitDistance());
                bsb.TextureUAV("u_DirectLighting", memMgr.GetDirectLighting());
                nvrhi::ITexture* sunVis = memMgr.GetSunVis(sunWrite);
                bsb.TextureUAV("u_SunVis", sunVis ? sunVis : memMgr.GetPlaceholderTex());
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->ptInitialLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->ptInitialPipeline;
                cs.bindings = { bs };
                if (GEnv.Backend)
                    if (auto* t = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(t);
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            });

        if (state.ptDupPipeline) {
            PTDupCB dupCB{ (float)giW, (float)giH, 0, 0 };
            struct PTDupData {
                fg::RenderDevice* device;
                ReSTIRGIPassState* state;
                PTDupCB cb;
                u32 width, height;
            };
            fg.addCallbackPass<PTDupData>(
                "ReSTIR PT DupMap",
                [&, dupCB](FrameGraph& builder, PassHandle passHandle, PTDupData& data) {
                    RenderPassBuilder pb(builder, passHandle);
                    pb.sideEffects();
                    data.device = device;
                    data.state = &state;
                    data.cb = dupCB;
                    data.width = giW;
                    data.height = giH;
                },
                [](const PTDupData& data, const FrameGraph&, fg::RenderContext* ctx) {
                    auto& memMgr = ReSTIRMemoryManager::Instance();
                    nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                    nvrhi::ICommandList* cmd = ctx->GetCommandList();
                    cmd->writeBuffer(data.state->cb, &data.cb, sizeof(PTDupCB));
                    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_pt_dupmap", ".cs");
                    if (!refl) return;
                    BindingSetBuilder bsb(*refl, nv, "ReSTIR.PTDup");
                    bsb.ConstantBuffer("ReSTIRPTDup", data.state->cb);
                    bsb.Texture("t_PTB", memMgr.GetPTReservoirB(0));
                    bsb.TextureUAV("u_Dup", memMgr.GetPTDupMap());
                    auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->ptDupLayout, nv);
                    if (!bs) return;
                    nvrhi::ComputeState cs;
                    cs.pipeline = data.state->ptDupPipeline;
                    cs.bindings = { bs };
                    cmd->setComputeState(cs);
                    cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                });
        }

        if (hasPrevFrameData && motionVectors.is_valid() && state.ptTemporalPipeline) {
            PTTemporalCB tcb{};
            tcb.invViewProj = invViewProj;
            tcb.prevInvViewProj = prevInvViewProj;
            tcb.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
            tcb.screenWidth = (float)giW;
            tcb.screenHeight = (float)giH;
            tcb.invScreenWidth = 1.f / (float)giW;
            tcb.invScreenHeight = 1.f / (float)giH;
            tcb.frameIndex = Device.dwFrame;
            tcb.cCap = (float)std::clamp(ps_r_rt_pt_ccap, 1, 32);
            tcb.hasPrev = 1;
            struct PTTempData {
                fg::RenderDevice* device;
                ReSTIRGIPassState* state;
                VirtualResourceHandle depth, normal, prevNormals, worldPos, prevDepth, motionVectors, baseColor;
                PTTemporalCB cb;
                u32 width, height;
            };
            fg.addCallbackPass<PTTempData>(
                "ReSTIR PT Temporal",
                [&, tcb](FrameGraph& builder, PassHandle passHandle, PTTempData& data) {
                    RenderPassBuilder pb(builder, passHandle);
                    data.depth = pb.read(depth, ResourceState::ShaderResource);
                    data.normal = pb.read(normal, ResourceState::ShaderResource);
                    data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                    if (prevNormals.is_valid())
                        data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                    if (worldPos.is_valid())
                        data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                    if (prevDepth.is_valid())
                        data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                    data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                    pb.sideEffects();
                    data.device = device;
                    data.state = &state;
                    data.cb = tcb;
                    data.width = giW;
                    data.height = giH;
                },
                [](const PTTempData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                    auto& memMgr = ReSTIRMemoryManager::Instance();
                    auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                    auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                    auto* prevN = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                    auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                    auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                    auto* mvTex = fgGraph.GetPhysicalTexture(data.motionVectors);
                    if (!depthTex || !mvTex) return;
                    nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                    nvrhi::ICommandList* cmd = ctx->GetCommandList();
                    cmd->writeBuffer(data.state->cb, &data.cb, sizeof(PTTemporalCB));
                    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_pt_temporal", ".cs");
                    if (!refl) return;
                    BindingSetBuilder bsb(*refl, nv, "ReSTIR.PTTemporal");
                    bsb.ConstantBuffer("ReSTIRPTTemporal", data.state->cb);
                    bsb.Texture("t_CurrA", memMgr.GetPTReservoirA(0));
                    bsb.Texture("t_CurrB", memMgr.GetPTReservoirB(0));
                    bsb.Texture("t_PrevA", memMgr.GetPTReservoirA(1));
                    bsb.Texture("t_PrevB", memMgr.GetPTReservoirB(1));
                    bsb.Texture("t_MotionVectors", mvTex);
                    bsb.Texture("t_Depth", depthTex);
                    bsb.Texture("t_Normal", normalTex);
                    bsb.Texture("t_PrevNormal", prevN);
                    bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                    bsb.Texture("t_PrevDepth", prevDepthTex);
                    bsb.Texture("t_DupMap", memMgr.GetPTDupMap() ? memMgr.GetPTDupMap() : memMgr.GetPlaceholderTex());
                    {
                        auto* bcTex = data.baseColor.is_valid() ? fgGraph.GetPhysicalTexture(data.baseColor) : nullptr;
                        bsb.Texture("t_BaseColor", bcTex ? bcTex : memMgr.GetPlaceholderColorTex());
                    }
                    if (bsb.HasSRV("t_SkyOpen"))
                        bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                    bsb.TextureUAV("u_OutA", memMgr.GetPTReservoirA(1));
                    bsb.TextureUAV("u_OutB", memMgr.GetPTReservoirB(1));
                    bsb.TextureUAV("u_NoisyDiffuse", memMgr.GetNoisyDiffuse());
                    bsb.TextureUAV("u_NoisySpecular", memMgr.GetNoisySpecular());
                    bsb.TextureUAV("u_HitDistance", memMgr.GetHitDistance());
                    auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->ptTemporalLayout, nv);
                    if (!bs) return;
                    nvrhi::ComputeState cs;
                    cs.pipeline = data.state->ptTemporalPipeline;
                    cs.bindings = { bs };
                    cmd->setComputeState(cs);
                    cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                });
        }

        if (state.ptSpatialPipeline && mem.GetPairingTex(0)) {
            for (u32 pair = 0; pair < 3; ++pair) {
                PTSpatialCB scb{};
                scb.invViewProj = invViewProj;
                scb.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
                scb.screenWidth = (float)giW;
                scb.screenHeight = (float)giH;
                scb.invScreenWidth = 1.f / (float)giW;
                scb.invScreenHeight = 1.f / (float)giH;
                scb.frameIndex = Device.dwFrame;
                scb.pairIndex = pair;
                scb.flipX = ps_r_rt_pt_decorrelate ? (Device.dwFrame & 1u) : 0;
                scb.flipY = ps_r_rt_pt_decorrelate ? ((Device.dwFrame >> 1) & 1u) : 0;
                scb.offX = ps_r_rt_pt_decorrelate ? int((Device.dwFrame * 13u + pair * 7u) % 17u) - 8 : 0;
                scb.offY = ps_r_rt_pt_decorrelate ? int((Device.dwFrame * 29u + pair * 11u) % 17u) - 8 : 0;
                scb.pass = pair;
                struct PTSpData {
                    fg::RenderDevice* device;
                    ReSTIRGIPassState* state;
                    VirtualResourceHandle depth, normal, worldPos, baseColor;
                    PTSpatialCB cb;
                    u32 width, height, pair;
                };
                fg.addCallbackPass<PTSpData>(
                    "ReSTIR PT Spatial",
                    [&, scb, pair](FrameGraph& builder, PassHandle passHandle, PTSpData& data) {
                        RenderPassBuilder pb(builder, passHandle);
                        data.depth = pb.read(depth, ResourceState::ShaderResource);
                        data.normal = pb.read(normal, ResourceState::ShaderResource);
                        data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                        if (worldPos.is_valid())
                            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                        pb.sideEffects();
                        data.device = device;
                        data.state = &state;
                        data.cb = scb;
                        data.width = giW;
                        data.height = giH;
                        data.pair = pair;
                    },
                    [](const PTSpData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                        auto& memMgr = ReSTIRMemoryManager::Instance();
                        auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                        auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                        auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                        if (!depthTex) return;
                        nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                        nvrhi::ICommandList* cmd = ctx->GetCommandList();
                        cmd->writeBuffer(data.state->cb, &data.cb, sizeof(PTSpatialCB));
                        auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_pt_spatial", ".cs");
                        if (!refl) return;
                        nvrhi::ITexture* srcA = (data.pair == 0) ? memMgr.GetPTReservoirA(1) : memMgr.GetPTReservoirA(0);
                        nvrhi::ITexture* srcB = (data.pair == 0) ? memMgr.GetPTReservoirB(1) : memMgr.GetPTReservoirB(0);
                        nvrhi::ITexture* dstA = (data.pair == 0) ? memMgr.GetPTReservoirA(0) : memMgr.GetPTReservoirA(1);
                        nvrhi::ITexture* dstB = (data.pair == 0) ? memMgr.GetPTReservoirB(0) : memMgr.GetPTReservoirB(1);
                        if (!srcA) srcA = memMgr.GetPTReservoirA(0);
                        if (!srcB) srcB = memMgr.GetPTReservoirB(0);
                        BindingSetBuilder bsb(*refl, nv, "ReSTIR.PTSpatial");
                        bsb.ConstantBuffer("ReSTIRPTSpatial", data.state->cb);
                        bsb.Texture("t_CurrA", srcA);
                        bsb.Texture("t_CurrB", srcB);
                        bsb.Texture("t_Depth", depthTex);
                        bsb.Texture("t_Normal", normalTex);
                        bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                        bsb.Texture("t_Pair", memMgr.GetPairingTex(data.pair) ? memMgr.GetPairingTex(data.pair) : memMgr.GetPlaceholderColorTex());
                        {
                            auto* bcTex = data.baseColor.is_valid() ? fgGraph.GetPhysicalTexture(data.baseColor) : nullptr;
                            bsb.Texture("t_BaseColor", bcTex ? bcTex : memMgr.GetPlaceholderColorTex());
                        }
                        bsb.TextureUAV("u_OutA", dstA);
                        bsb.TextureUAV("u_OutB", dstB);
                        bsb.TextureUAV("u_NoisyDiffuse", memMgr.GetNoisyDiffuse());
                        bsb.TextureUAV("u_NoisySpecular", memMgr.GetNoisySpecular());
                        auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->ptSpatialLayout, nv);
                        if (!bs) return;
                        nvrhi::ComputeState cs;
                        cs.pipeline = data.state->ptSpatialPipeline;
                        cs.bindings = { bs };
                        cmd->setComputeState(cs);
                        cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                    });
            }
        }
    }

    const float shaftIntensity = shaftIntensityEarly;
    const bool renderSunshafts = wantSunshafts;

    if (renderSunshafts) {
        SunshaftCB shaftCB{};
        shaftCB.invViewProj = invViewProj;
        shaftCB.prevViewProj = prevViewProj;
        shaftCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        shaftCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
        shaftCB.sunColor = { sc.x, sc.y, sc.z, 0 };
        shaftCB.screenWidth = (float)shaftW;
        shaftCB.screenHeight = (float)shaftH;
        shaftCB.shaftIntensity = shaftIntensity;
        float shaftFar = 250.f;
        if (g_pGamePersistent)
            shaftFar = std::max(250.f, std::min(g_pGamePersistent->Environment().CurrentEnv.far_plane, 400.f));
        shaftCB.shaftLength = shaftFar;
        shaftCB.identityStaticCount = batchCounts.identityStatic;
        shaftCB.terrainBatchCount = batchCounts.terrain;
        shaftCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        shaftCB.grassBatchStart = initialCB.grassBatchStart;
        shaftCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        shaftCB.hudSkinnedStart = initialCB.hudSkinnedStart;
        const u32 shaftSteps = (ps_r_sun_shafts <= 1) ? 20u : (ps_r_sun_shafts == 2 ? 20u : 40u);
        shaftCB.shaftSteps = shaftSteps;
        shaftCB.alphaEveryN = (ps_r_sun_shafts <= 1) ? 4u : 2u;
        shaftCB.fullWidth = (float)width;
        shaftCB.fullHeight = (float)height;
        shaftCB.particleBatchStart = initialCB.particleBatchStart;
        shaftCB.frameIndex = Device.dwFrame;
        shaftCB.hasPrev = hasPrevFrameData ? 1u : 0u;
        static Fvector s_prevSunDir = { 0, -1, 0 };
        shaftCB.prevSunDir = s_prevSunDir;
        s_prevSunDir = sunDir;

        fg.addCallbackPass<SunshaftPassData>(
            "ReSTIR Sunshafts",
            [&, shaftCB, fgSunshafts](FrameGraph& builder, PassHandle passHandle, SunshaftPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                if (hasPrevFrameData && prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                else
                    data.prevDepth = {};
                pb.write(fgSunshafts, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = shaftCB;
                data.width = shaftW;
                data.height = shaftH;
            },
            [](const SunshaftPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* outTex = memMgr.GetSunshafts();
                auto* tlas = data.accelMgr->GetTLAS();
                if (!depthTex || !outTex || !tlas) return;

                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                SunshaftCB cb = data.cbData;
                const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
                cb.identityStaticCount = starts.identityStatic;
                cb.terrainBatchCount = starts.terrain;
                cb.skinnedBatchStart = starts.skinnedStart;
                cb.grassBatchStart = starts.grassStart;
                cb.hudSkinnedStart = starts.hudStart;
                cb.detailAtlasIndex = starts.detailAtlas;
                cb.particleBatchStart = starts.particleStart;
                cmd->writeBuffer(data.state->cb, &cb, sizeof(SunshaftCB));

                nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
                nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!skinnedVB) skinnedVB = memMgr.GetPlaceholderBuffer();
                if (!skinnedIB) skinnedIB = memMgr.GetPlaceholderBuffer();
                if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
                if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();

                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_sunshafts", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.Sunshafts");
                bsb.ConstantBuffer("SunshaftParams", data.state->cb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", data.accelMgr->GetBatchInfoBuffer());
                bsb.BufferSRV("g_MegaVB", data.accelMgr->GetMegaVB());
                bsb.BufferSRV("g_MegaIB", data.accelMgr->GetMegaIB());
                bsb.BufferSRV("g_SkinnedVB", skinnedVB);
                bsb.BufferSRV("g_SkinnedIB", skinnedIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : nullptr;
                bsb.Texture("t_Depth", depthTex);
                if (bsb.HasSRV("t_SkyOpen"))
                    bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
                if (bsb.HasSRV("t_BlueNoise"))
                    bsb.Texture("t_BlueNoise", memMgr.GetBlueNoise() ? memMgr.GetBlueNoise() : memMgr.GetPlaceholderTex3D());
                bsb.Texture("t_PrevSunshafts", memMgr.GetSunshaftsHist() ? memMgr.GetSunshaftsHist() : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex ? prevDepthTex : memMgr.GetPlaceholderTex());
                BindBindlessMaterialTables(bsb);
                bsb.TextureUAV("u_Sunshafts", outTex);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->sunshaftsLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->sunshaftsPipeline;
                cs.bindings = { bs };
                if (GEnv.Backend) {
                    if (auto* bindlessTable = GEnv.Backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                if (memMgr.GetSunshaftsHist() && outTex)
                    cmd->copyTexture(memMgr.GetSunshaftsHist(), nvrhi::TextureSlice(), outTex, nvrhi::TextureSlice());
            }
        );
    } else if (mem.GetSunshafts()) {
        struct SunshaftClearData {};
        fg.addCallbackPass<SunshaftClearData>(
            "ReSTIR Sunshafts Clear",
            [&, fgSunshafts](FrameGraph& builder, PassHandle passHandle, SunshaftClearData&) {
                RenderPassBuilder pb(builder, passHandle);
                pb.write(fgSunshafts, ResourceState::UnorderedAccess);
                pb.sideEffects();
            },
            [](const SunshaftClearData&, const FrameGraph&, fg::RenderContext* ctx) {
                auto* outTex = ReSTIRMemoryManager::Instance().GetSunshafts();
                if (!outTex || !ctx) return;
                ctx->GetCommandList()->clearTextureFloat(
                    outTex, nvrhi::AllSubresources, nvrhi::Color(0.f, 0.f, 0.f, 0.f));
            }
        );
    }

    if (state.ddgiPipeline && initialCB.cacheSize == 0) {
        DDGICB ddgiCB{};
        ddgiCB.invViewProj = invViewProj;
        ddgiCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        ddgiCB.gridOrigin_spacing = { cameraPos.x - 32.f, cameraPos.y - 8.f, cameraPos.z - 32.f, 4.0f };
        ddgiCB.gridDims_intensity = { 16.f, 8.f, 16.f, 0.35f * giIntensity };
        ddgiCB.screenWidth = (float)giW;
        ddgiCB.screenHeight = (float)giH;
        ddgiCB.frameIndex = Device.dwFrame;
        ddgiCB.pad = envAdapt;

        fg.addCallbackPass<DDGIPassData>(
            "ReSTIR DDGI",
            [&, ddgiCB, fgDirectLighting, fgDdgiAmb](FrameGraph& builder, PassHandle passHandle, DDGIPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                pb.read(fgDirectLighting, ResourceState::ShaderResource);
                pb.write(fgDdgiAmb, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = ddgiCB;
                data.width = giW;
                data.height = giH;
            },
            [](const DDGIPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* direct = memMgr.GetDirectLighting();
                auto* probes = memMgr.GetDdgiProbes();
                auto* ambient = memMgr.GetDdgiAmbient();
                if (!depthTex || !normalTex || !baseColorTex || !direct || !probes || !ambient) return;

                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(DDGICB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_ddgi", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.DDGI");
                bsb.ConstantBuffer("DDGIParams", data.state->cb);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_DirectLighting", direct);
                bsb.TextureUAV("u_ProbeIrradiance", probes);
                bsb.TextureUAV("u_AmbientOut", ambient);
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->ddgiLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->ddgiPipeline;
                cs.bindings = { bs };
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    const bool inTreeDenoise = state.blurPipeline != nullptr;

    if (inTreeDenoise && hasPrevFrameData && motionVectors.is_valid() && state.temporalFilterPipeline) {
        TemporalFilterCB tfCB{};
        tfCB.invViewProj = invViewProj;
        tfCB.prevInvViewProj = prevInvViewProj;
        tfCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        tfCB.screenWidth = (float)giW;
        tfCB.screenHeight = (float)giH;
        tfCB.invScreenWidth = 1.0f / (float)giW;
        tfCB.invScreenHeight = 1.0f / (float)giH;
        tfCB.alpha = std::clamp(ps_r_rt_gi_temporal_alpha, 0.f, 0.98f);
        tfCB.envAdapt = envAdapt;
        tfCB.currJitterX = g_taa_jitter_px;
        tfCB.currJitterY = g_taa_jitter_py;
        tfCB.prevJitterX = g_taa_jitter_prev_px;
        tfCB.prevJitterY = g_taa_jitter_prev_py;
        tfCB.enabled = 1;
        tfCB.pad1 = 0;
        struct TFPassData {
            fg::RenderDevice* device;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth, normal, worldPos, prevDepth, prevNormals, motionVectors;
            TemporalFilterCB cbData;
            u32 width, height;
        };
        fg.addCallbackPass<TFPassData>(
            "ReSTIR Temporal Filter",
            [&, tfCB](FrameGraph& builder, PassHandle passHandle, TFPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (worldPos.is_valid())
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = tfCB;
                data.width = giW;
                data.height = giH;
            },
            [](const TFPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                auto* prevDepthTex = data.prevDepth.is_valid() ? fgGraph.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fgGraph.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* motionTex = fgGraph.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !memMgr.GetNoisyDiffuse() || !memMgr.GetHistDiffuse()) return;
                nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmd = ctx->GetCommandList();
                cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalFilterCB));
                auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_temporal_filter", ".cs");
                if (!refl) return;
                BindingSetBuilder bsb(*refl, nv, "ReSTIR.TemporalFilter");
                bsb.ConstantBuffer("TemporalFilterParams", data.state->cb);
                bsb.Texture("t_CurrDiffuse", memMgr.GetNoisyDiffuse());
                bsb.Texture("t_CurrSpecular", memMgr.GetNoisySpecular());
                bsb.Texture("t_HistDiffuse", memMgr.GetHistDiffuse());
                bsb.Texture("t_HistSpecular", memMgr.GetHistSpecular());
                bsb.Texture("t_MotionVectors", motionTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                bsb.Texture("t_PrevDepth", prevDepthTex ? prevDepthTex : depthTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex ? prevNormalsTex : normalTex);
                bsb.TextureUAV("u_OutDiffuse", memMgr.GetBlurTemp());
                bsb.TextureUAV("u_OutSpecular", memMgr.GetBlurTempSpec());
                auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->temporalFilterLayout, nv);
                if (!bs) return;
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalFilterPipeline;
                cs.bindings = { bs };
                cmd->setComputeState(cs);
                cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                cmd->copyTexture(memMgr.GetNoisyDiffuse(), nvrhi::TextureSlice(), memMgr.GetBlurTemp(), nvrhi::TextureSlice());
                cmd->copyTexture(memMgr.GetNoisySpecular(), nvrhi::TextureSlice(), memMgr.GetBlurTempSpec(), nvrhi::TextureSlice());
            }
        );
    }

    if (inTreeDenoise) {
        const int atrousSteps = std::clamp(ps_r_rt_gi_atrous_steps, 1, 6);
        for (int i = 0; i < atrousSteps; ++i) {
            const bool last = (i == atrousSteps - 1);
            BlurCB blurCB{};
            blurCB.screenWidth = (float)giW;
            blurCB.screenHeight = (float)giH;
            blurCB.invScreenWidth = 1.0f / (float)giW;
            blurCB.invScreenHeight = 1.0f / (float)giH;
            blurCB.phiNormal = 4.f;
            blurCB.phiDepth = 6.f;
            blurCB.step = 1u << (u32)(atrousSteps - 1 - i);
            blurCB.mode = 0u;
            struct BlurPassData {
                fg::RenderDevice* device;
                ReSTIRGIPassState* state;
                VirtualResourceHandle depth, normal, worldPos, sceneColorIn, sceneColor;
                BlurCB cbData;
                u32 width, height;
                bool last;
            };
            fg.addCallbackPass<BlurPassData>(
                "ReSTIR Blur",
                [&, blurCB, last](FrameGraph& builder, PassHandle passHandle, BlurPassData& data) {
                    RenderPassBuilder pb(builder, passHandle);
                    data.depth = pb.read(depth, ResourceState::ShaderResource);
                    data.normal = pb.read(normal, ResourceState::ShaderResource);
                    if (worldPos.is_valid())
                        data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                    data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
                    pb.sideEffects();
                    data.device = device;
                    data.state = &state;
                    data.cbData = blurCB;
                    data.width = giW;
                    data.height = giH;
                    data.last = last;
                },
                [](const BlurPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                    auto& memMgr = ReSTIRMemoryManager::Instance();
                    auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                    auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                    auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
                    auto* sceneIn = fgGraph.GetPhysicalTexture(data.sceneColorIn);
                    if (!depthTex || !memMgr.GetNoisyDiffuse()) return;
                    nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
                    nvrhi::ICommandList* cmd = ctx->GetCommandList();
                    cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(BlurCB));
                    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_blur", ".cs");
                    if (!refl) return;
                    BindingSetBuilder bsb(*refl, nv, "ReSTIR.Blur");
                    bsb.ConstantBuffer("BlurParams", data.state->cb);
                    bsb.Texture("t_DirectLighting", memMgr.GetDirectLighting());
                    bsb.Texture("t_NoisyDiffuse", memMgr.GetNoisyDiffuse());
                    bsb.Texture("t_Depth", depthTex);
                    bsb.Texture("t_Normal", normalTex);
                    bsb.Texture("t_SceneColorIn", sceneIn ? sceneIn : memMgr.GetPlaceholderColorTex());
                    bsb.Texture("t_NoisySpecular", memMgr.GetNoisySpecular());
                    bsb.Texture("t_ClassifyWorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                    bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
                    bsb.TextureUAV("u_SceneColor", memMgr.GetPlaceholderColorTex());
                    bsb.TextureUAV("u_FilteredDiffuse", memMgr.GetBlurTemp());
                    bsb.TextureUAV("u_FilteredSpecular", memMgr.GetBlurTempSpec());
                    auto bs = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->blurLayout, nv);
                    if (!bs) return;
                    nvrhi::ComputeState cs;
                    cs.pipeline = data.state->blurPipeline;
                    cs.bindings = { bs };
                    cmd->setComputeState(cs);
                    cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                    cmd->copyTexture(memMgr.GetNoisyDiffuse(), nvrhi::TextureSlice(), memMgr.GetBlurTemp(), nvrhi::TextureSlice());
                    cmd->copyTexture(memMgr.GetNoisySpecular(), nvrhi::TextureSlice(), memMgr.GetBlurTempSpec(), nvrhi::TextureSlice());
                    if (data.last) {
                        cmd->copyTexture(memMgr.GetHistDiffuse(), nvrhi::TextureSlice(), memMgr.GetNoisyDiffuse(), nvrhi::TextureSlice());
                        cmd->copyTexture(memMgr.GetHistSpecular(), nvrhi::TextureSlice(), memMgr.GetNoisySpecular(), nvrhi::TextureSlice());
                    }
                }
            );
        }
    }

    CompositeCB compositeCB;
    compositeCB.invViewProj = invViewProj;
    compositeCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    compositeCB.screenWidth = (float)width;
    compositeCB.screenHeight = (float)height;
    compositeCB.giWidth = (float)giW;
    compositeCB.giHeight = (float)giH;
    compositeCB.shaftWidth = shaftW;
    compositeCB.shaftHeight = shaftH;
    compositeCB.giIntensity = giIntensity;
    compositeCB.denoiseApply = 0u;
    compositeCB.ambientScale = std::clamp(ps_r_rt_gi_ambient_scale, 0.f, 1.f);
    compositeCB.cacheSize = initialCB.cacheSize;
    compositeCB.cacheCellSize = initialCB.cacheCellSize;
    compositeCB.useDdgi = (initialCB.cacheSize == 0) ? 1u : 0u;
    compositeCB.addDirect = g_restirReplaceForward ? 1u : 0u;
    compositeCB.pad0 = Device.dwFrame;
    {
        StaticGlobals fogFill{};
        FillGlobalConstants(fogFill);
        compositeCB.fogParams = fogFill.fog_params;
        {
            const Fvector3& sky = env.CurrentEnv.sky_color;
            compositeCB.fogColor = { sky.x, sky.y, sky.z, fogFill.fog_color.w };
            compositeCB.cameraPos.w = env.CurrentEnv.weight;
        }
        compositeCB.sunDir = { sunDir.x, sunDir.y, sunDir.z, 0 };
        compositeCB.sunColor = { sc.x, sc.y, sc.z, sunIntensity };
    }

    auto& compositeData = fg.addCallbackPass<CompositePassData>(
        "ReSTIR Composite",
        [&, compositeCB, outHandle, fgDirectLighting, fgSunshafts, fgDdgiAmb, fgNoisyDiff, fgNoisySpec, sky0Tex, sky1Tex](
            FrameGraph& builder, PassHandle passHandle, CompositePassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            if (worldPos.is_valid())
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
            pb.read(fgDirectLighting, ResourceState::ShaderResource);
            pb.read(fgSunshafts, ResourceState::ShaderResource);
            pb.read(fgDdgiAmb, ResourceState::ShaderResource);
            pb.read(fgNoisyDiff, ResourceState::ShaderResource);
            pb.read(fgNoisySpec, ResourceState::ShaderResource);
            data.sceneColor = pb.write(outHandle, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.cbData = compositeCB;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
            data.width = width;
            data.height = height;
        },
        [histIdx](const CompositePassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto& memMgr = ReSTIRMemoryManager::Instance();
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* sceneColorInTex = fgGraph.GetPhysicalTexture(data.sceneColorIn);
            auto* outTex = fgGraph.GetPhysicalTexture(data.sceneColor);
            if (!depthTex || !normalTex || !baseColorTex || !sceneColorInTex || !outTex) return;

            nvrhi::ITexture* directLit = memMgr.GetDirectLighting();
            nvrhi::IBuffer* resBuf = memMgr.GetReservoirBuffer(histIdx);
            nvrhi::ITexture* shafts = memMgr.GetSunshafts();
            nvrhi::ITexture* ddgi = memMgr.GetDdgiAmbient();
            auto blitIn = [&]() {
                nvrhi::ICommandList* c = ctx->GetCommandList();
                c->copyTexture(outTex, nvrhi::TextureSlice(), sceneColorInTex, nvrhi::TextureSlice());
            };
            if (!directLit || !resBuf) { blitIn(); return; }

            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            cmd->writeBuffer(data.state->cb, &data.cbData, sizeof(CompositeCB));

            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_composite", ".cs");
            if (!refl) { blitIn(); return; }

            BindingSetBuilder bsb(*refl, nv, "ReSTIR.Composite");
            bsb.ConstantBuffer("CompositeParams", data.state->cb);
            bsb.Texture("t_DirectLighting", directLit);
            bsb.BufferSRV("t_Reservoir", resBuf);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.Texture("t_SceneColorIn", sceneColorInTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_NoisyDiffuse", memMgr.GetNoisyDiffuse());
            bsb.Texture("t_NoisySpecular", memMgr.GetNoisySpecular());
            bsb.Texture("t_Sunshafts", shafts ? shafts : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_DDGIAmbient", ddgi ? ddgi : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_WorldPos", worldPosTex ? worldPosTex : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_SpecReservoirA", memMgr.GetSpecReservoirA(0) ? memMgr.GetSpecReservoirA(0) : memMgr.GetPlaceholderColorTex());
            bsb.Texture("t_SpecReservoirB", memMgr.GetSpecReservoirB(0) ? memMgr.GetSpecReservoirB(0) : memMgr.GetPlaceholderColorTex());
            if (auto* cache = memMgr.GetIrradianceCache())
                bsb.BufferSRV("g_IrradianceCache", cache);
            else
                bsb.BufferSRV("g_IrradianceCache", memMgr.GetPlaceholderBuffer());
            if (bsb.HasSRV("t_SkyOpen"))
                bsb.Texture("t_SkyOpen", memMgr.GetSkyOpen() ? memMgr.GetSkyOpen() : memMgr.GetPlaceholderTex());
            if (bsb.HasSRV("g_Sky0"))
                bsb.Texture("g_Sky0", data.sky0 ? data.sky0 : memMgr.GetPlaceholderCube());
            if (bsb.HasSRV("g_Sky1"))
                bsb.Texture("g_Sky1", data.sky1 ? data.sky1 : memMgr.GetPlaceholderCube());
            bsb.TextureUAV("u_SceneColor", outTex);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->compositeLayout, nv);
            if (!bindingSet) { blitIn(); return; }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->compositePipeline;
            cs.bindings = { bindingSet };
            cmd->setComputeState(cs);
            cmd->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    VirtualResourceHandle finalColor = compositeData.sceneColor;

    if (state.waterPipeline && state.waterLayout && accelMgr && worldPos.is_valid()) {
        WaterCB waterCB{};
        waterCB.invViewProj = invViewProj;
        waterCB.viewProj = Device.mFullTransform;
        waterCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 80.f };
        waterCB.sunDir_intensity = initialCB.sunDir_intensity;
        waterCB.sunColor_skyWeight = initialCB.sunColor_skyWeight;
        waterCB.skyColor = initialCB.skyColor;
        waterCB.screenWidth = (float)width;
        waterCB.screenHeight = (float)height;
        waterCB.lodDist = std::max(10.f, ps_r_rt_gi_lod_dist);
        waterCB.giIntensity = giIntensity;
        waterCB.identityStaticCount = initialCB.identityStaticCount;
        waterCB.terrainBatchCount = initialCB.terrainBatchCount;
        waterCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        waterCB.grassBatchStart = initialCB.grassBatchStart;
        waterCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        waterCB.hudSkinnedStart = initialCB.hudSkinnedStart;
        waterCB.pad1 = waterCB.pad2 = 0;
        {
            Fvector4 hemi = { 0.3f, 0.4f, 0.5f, 1.f };
            if (g_pGamePersistent)
            {
                const auto& h = g_pGamePersistent->Environment().CurrentEnv.hemi_color;
                hemi.set(h.x, h.y, h.z, h.w);
            }
            waterCB.hemiColor = hemi;
        }

        ResourceDesc waterOutDesc = outDesc;
        waterOutDesc.debugName = "rtgi_WaterSceneColor";
        VirtualResourceHandle waterOut = fg.CreateTexture("rtgi_WaterSceneColor", waterOutDesc);

        struct WaterPassData {
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            VirtualResourceHandle sceneIn;
            VirtualResourceHandle sceneOut;
            fg::RenderDevice* device = nullptr;
            RTAccelStructManager* accelMgr = nullptr;
            ReSTIRGIPassState* state = nullptr;
            WaterCB cbData{};
            nvrhi::ITexture* sky0 = nullptr;
            nvrhi::ITexture* sky1 = nullptr;
            nvrhi::ITexture* underWorldPos = nullptr;
            nvrhi::ITexture* underColor = nullptr;
            u32 width = 0;
            u32 height = 0;
        };

        auto& waterData = fg.addCallbackPass<WaterPassData>(
            "ReSTIR Water RT",
            [&, waterCB, finalColor, waterOut, sky0Tex, sky1Tex](FrameGraph& builder, PassHandle passHandle, WaterPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                data.sceneIn = pb.read(finalColor, ResourceState::ShaderResource);
                data.sceneOut = pb.write(waterOut, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = waterCB;
                data.sky0 = sky0Tex;
                data.sky1 = sky1Tex;
                data.underWorldPos = state.waterUnderWorldPos;
                data.underColor = state.waterUnderColor;
                data.width = width;
                data.height = height;
            },
            [](const WaterPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                auto& memMgr = ReSTIRMemoryManager::Instance();
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
                auto* sceneIn = fgGraph.GetPhysicalTexture(data.sceneIn);
                auto* sceneOut = fgGraph.GetPhysicalTexture(data.sceneOut);
                auto blitIn = [&]() {
                    if (sceneIn && sceneOut)
                        ctx->GetCommandList()->copyTexture(
                            sceneOut, nvrhi::TextureSlice(), sceneIn, nvrhi::TextureSlice());
                };
                if (!data.state || !data.state->waterPipeline || !data.accelMgr) {
                    blitIn();
                    return;
                }
                auto* tlas = data.accelMgr->GetTLAS();
                auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                auto* megaVB = data.accelMgr->GetMegaVB();
                auto* megaIB = data.accelMgr->GetMegaIB();
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !sceneIn || !sceneOut ||
                    !tlas || !batchInfo || !megaVB || !megaIB) {
                    blitIn();
                    return;
                }

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                WaterCB cb = data.cbData;
                const RTBatchStarts starts = ComputeBatchStarts(data.accelMgr);
                cb.identityStaticCount = starts.identityStatic;
                cb.terrainBatchCount = starts.terrain;
                cb.skinnedBatchStart = starts.skinnedStart;
                cb.grassBatchStart = starts.grassStart;
                cb.hudSkinnedStart = starts.hudStart;
                cb.detailAtlasIndex = starts.detailAtlas;
                cmdList->writeBuffer(data.state->cb, &cb, sizeof(WaterCB));

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_water_rt", ".cs");
                if (!csReflection) {
                    blitIn();
                    return;
                }

                nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
                nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!skinnedVB) skinnedVB = memMgr.GetPlaceholderBuffer();
                if (!skinnedIB) skinnedIB = memMgr.GetPlaceholderBuffer();
                if (!grassVB) grassVB = memMgr.GetPlaceholderBuffer();
                if (!grassIB) grassIB = memMgr.GetPlaceholderBuffer();

                auto& cache = GetPassResourceCache();
                nvrhi::ITexture* sky0 = data.sky0 ? data.sky0 : memMgr.GetPlaceholderCube();
                nvrhi::ITexture* sky1 = data.sky1 ? data.sky1 : memMgr.GetPlaceholderCube();
                nvrhi::ITexture* underWP = data.underWorldPos
                    ? data.underWorldPos
                    : cache.GetDummyContactHistory(nvDevice);
                nvrhi::ITexture* underColor = data.underColor
                    ? data.underColor
                    : memMgr.GetPlaceholderColorTex();

                BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIR.Water");
                bsb.ConstantBuffer("ReSTIRWaterParams", data.state->cb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.Texture("g_Sky0", sky0);
                bsb.Texture("g_Sky1", sky1);
                bsb.BufferSRV("g_SkinnedVB", skinnedVB);
                BindBindlessMaterialTables(bsb);
                bsb.BufferSRV("g_SkinnedIB", skinnedIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_ClassifyWorldPos", worldPosTex);
                bsb.Texture("t_SceneColorIn", sceneIn);
                bsb.Texture("t_UnderWorldPos", underWP);
                bsb.Texture("t_UnderColor", underColor);
                bsb.TextureUAV("u_SceneColor", sceneOut);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->waterLayout, nvDevice);
                if (!bindingSet) {
                    blitIn();
                    return;
                }

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->waterPipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
        finalColor = waterData.sceneOut;
    }

    state.currTemporalIdx = 1u - (state.currTemporalIdx & 1u);
    output.sceneColor = finalColor;
    return output;
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state)
{
    state.initialPipeline = nullptr;
    state.initialLayout = nullptr;
    state.temporalPipeline = nullptr;
    state.temporalLayout = nullptr;
    state.spatialPipeline = nullptr;
    state.spatialLayout = nullptr;
    state.compositePipeline = nullptr;
    state.compositeLayout = nullptr;
    state.wetPipeline = nullptr;
    state.wetLayout = nullptr;
    state.sunshaftsPipeline = nullptr;
    state.sunshaftsLayout = nullptr;
    state.ddgiPipeline = nullptr;
    state.ddgiLayout = nullptr;
    state.waterPipeline = nullptr;
    state.waterLayout = nullptr;
    state.diTemporalPipeline = nullptr;
    state.diTemporalLayout = nullptr;
    state.diSpatialPipeline = nullptr;
    state.diSpatialLayout = nullptr;
    state.diShadePipeline = nullptr;
    state.diShadeLayout = nullptr;
    state.blurPipeline = nullptr;
    state.blurLayout = nullptr;
    state.temporalFilterPipeline = nullptr;
    state.temporalFilterLayout = nullptr;
    state.specTemporalPipeline = nullptr;
    state.specTemporalLayout = nullptr;
    state.ptInitialPipeline = nullptr;
    state.ptInitialLayout = nullptr;
    state.ptTemporalPipeline = nullptr;
    state.ptTemporalLayout = nullptr;
    state.ptSpatialPipeline = nullptr;
    state.ptSpatialLayout = nullptr;
    state.ptDupPipeline = nullptr;
    state.ptDupLayout = nullptr;
    state.cb = nullptr;
    state.waterUnderWorldPos = nullptr;
    state.waterUnderColor = nullptr;
    ReSTIRMemoryManager::Instance().Shutdown();
    state.initialized = false;
    state.enabled = false;
    state.pipeVersion = 0;
    g_restirPipelinesReady = false;
    g_restirReplaceForward = false;
}

}
