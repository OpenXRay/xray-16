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
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/SunShaftsPassSetup.h"
#if defined(XR_PLATFORM_WINDOWS)
#include "Layers/xrRender/Backend/D3D12Backend.h"
#endif
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API int ps_r_rt_gi_spatial_samples;
extern ENGINE_API float ps_r_rt_gi_spatial_radius;
extern ENGINE_API int ps_r_rt_gi_m_max;
extern ENGINE_API int ps_r_rt_gi_local_samples;
extern ENGINE_API int ps_r_rt_di_candidates;
extern ENGINE_API int ps_r_rt_di_spatial_samples;
extern ENGINE_API float ps_r_rt_di_spatial_radius;
extern ENGINE_API int ps_r_rt_di_m_max;
extern ENGINE_API int ps_r_rt_gi_atrous_steps;
extern ENGINE_API float ps_r_rt_gi_temporal_alpha;
extern ENGINE_API int ps_r_rt_gi_bounces;
extern ENGINE_API int ps_r_rt_gi_cache_size;
extern ENGINE_API float ps_r_rt_gi_cache_cell;
extern ENGINE_API int ps_r_rt_vol_steps;

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

static nvrhi::BufferHandle s_rtgiPlaceholderBuffer;
static nvrhi::TextureHandle s_rtgiPlaceholderCube;

struct ReSTIRGICB {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fmatrix worldToView;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 frameIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 localLightSamples;
    u32 diCandidates;
    u32 bounces;
    Fvector4 diSampleParams;
    Fvector4 clusterParams;
    Fvector4 clusterScales;
    Fvector4 hemiColor;
    u32 cacheSize;
    float cacheCellSize;
    u32 cacheMaxAge;
    u32 pad1;
};
static_assert(sizeof(ReSTIRGICB) == 368, "ReSTIRGICB must be 368 bytes");

struct TemporalCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 pad[2];
};
static_assert(sizeof(TemporalCB) == 128, "TemporalCB must be 128 bytes");

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
    u32 pad[3];
};
static_assert(sizeof(SpatialCB) == 144, "SpatialCB must be 144 bytes");

struct DITemporalCB {
    Fmatrix worldToView;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 mMax;
    float pad[2];
    Fvector4 clusterParams;
    Fvector4 clusterScales;
};
static_assert(sizeof(DITemporalCB) == 144, "DITemporalCB must be 144 bytes");

struct DISpatialCB {
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
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};
static_assert(sizeof(DISpatialCB) == 176, "DISpatialCB must be 176 bytes");

struct DIShadeCB {
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    Fvector4 clusterParams;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 pad;
};
static_assert(sizeof(DIShadeCB) == 64, "DIShadeCB must be 64 bytes");

struct WaterCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    Fvector4 hemiColor;
};
static_assert(sizeof(WaterCB) == 160, "WaterCB must be 160 bytes");

struct CompositeCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 pad;
    Fvector4 fogParams;
    Fvector4 fogColor;
};
static_assert(sizeof(CompositeCB) == 128, "CompositeCB must be 128 bytes");

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
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    float alpha;
    float pad0;
    u32 enabled;
    u32 pad1;
};
static_assert(sizeof(TemporalFilterCB) == 32, "TemporalFilterCB must be 32 bytes");

struct RTVolCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_fog;
    float screenWidth;
    float screenHeight;
    float fogDensity;
    float fogHeight;
    u32 steps;
    float fogFar;
    float heightFalloff;
    float shaftIntensity;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 pad0;
    u32 pad1;
};
static_assert(sizeof(RTVolCB) == 160, "RTVolCB must be 160 bytes");

bool IsRTGIActive(const RTAccelStructManager* accelMgr)
{
    return ::ps_r_rt_gi && accelMgr && accelMgr->IsSupported() && accelMgr->IsReady();
}

static void CreatePlaceholders(nvrhi::IDevice* nvDevice)
{
    if (!s_rtgiPlaceholderBuffer) {
        nvrhi::BufferDesc desc;
        desc.debugName = "RTGI_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_rtgiPlaceholderBuffer = nvDevice->createBuffer(desc);
    }
    if (!s_rtgiPlaceholderCube) {
        nvrhi::TextureDesc desc;
        desc.debugName = "RTGI_PlaceholderCube";
        desc.width = 1;
        desc.height = 1;
        desc.dimension = nvrhi::TextureDimension::TextureCube;
        desc.arraySize = 6;
        desc.format = nvrhi::Format::RGBA8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_rtgiPlaceholderCube = nvDevice->createTexture(desc);
    }
}

static void EnsureVolumetricPipeline(fg::RenderDevice* device, ReSTIRGIPassState& state)
{
    constexpr u32 kVolPipeVersion = 4;
    static u32 s_volPipeVersion = 0;
    if (state.volPipeline && state.volLayout && s_volPipeVersion == kVolPipeVersion)
        return;

    state.volPipeline = nullptr;
    state.volLayout = nullptr;
    s_volPipeVersion = 0;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    BindingSetBuilder::InvalidateReflectionCache();
    auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_volumetric");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [ReSTIR GI] rt_volumetric shader missing");
        return;
    }

    nvrhi::IBindingLayout* bindlessLayout = nullptr;
    if (auto* backend = device->GetBackend())
        bindlessLayout = backend->GetBindlessLayout();

    state.volLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_VolShafts_v4", *csResult.reflection, nvDevice);
    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { state.volLayout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { state.volLayout };
    state.volPipeline = nvDevice->createComputePipeline(pipeDesc);
    if (state.volPipeline)
        s_volPipeVersion = kVolPipeVersion;
    Msg("* [ReSTIR GI] RT sunshafts pipeline %s", state.volPipeline ? "ready" : "FAILED");
}

static void EnsureWaterPipeline(fg::RenderDevice* device, ReSTIRGIPassState& state)
{
    constexpr u32 kWaterPipeVersion = 10;
    static u32 s_waterPipeVersion = 0;
    if (state.waterPipeline && state.waterLayout && s_waterPipeVersion == kWaterPipeVersion)
        return;

    state.waterPipeline = nullptr;
    state.waterLayout = nullptr;
    s_waterPipeVersion = 0;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    BindingSetBuilder::InvalidateReflectionCache();
    auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_water_rt");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [ReSTIR GI] restir_water_rt shader missing");
        return;
    }

    nvrhi::IBindingLayout* bindlessLayout = nullptr;
    if (auto* backend = device->GetBackend())
        bindlessLayout = backend->GetBindlessLayout();

    state.waterLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Water_RT_v10", *csResult.reflection, nvDevice);
    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { state.waterLayout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { state.waterLayout };
    state.waterPipeline = nvDevice->createComputePipeline(pipeDesc);
    if (state.waterPipeline)
        s_waterPipeVersion = kWaterPipeVersion;
}

static void InitializeResources(fg::RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized && state.enabled) {
        EnsureVolumetricPipeline(device, state);
        EnsureWaterPipeline(device, state);
        return;
    }

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    CreatePlaceholders(nvDevice);

    if (!state.sampler) {
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllFilters(true);
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        state.sampler = cache.GetOrCreateSampler("RTGI", samplerDesc, nvDevice);
    }

    if (!state.cb) {
        state.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_CB",
            (u32)std::max({ sizeof(ReSTIRGICB), sizeof(TemporalCB), sizeof(SpatialCB), sizeof(DITemporalCB),
                sizeof(DISpatialCB), sizeof(DIShadeCB), sizeof(WaterCB), sizeof(CompositeCB), sizeof(BlurCB),
                sizeof(TemporalFilterCB), sizeof(RTVolCB) }), device);
    }

    nvrhi::IBindingLayout* bindlessLayout = nullptr;
    if (auto* backend = device->GetBackend())
        bindlessLayout = backend->GetBindlessLayout();

    if (!state.initialPipeline && !state.initialLoadFailed) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_initial");
        if (csResult.handle && csResult.reflection) {
            state.initialLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Initial_v4_Emissive", *csResult.reflection, nvDevice);
            if (!state.initialLayout) {
                Msg("! [ReSTIR GI] initial binding layout create failed");
                state.initialLoadFailed = true;
            } else {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = csResult.handle;
                if (bindlessLayout)
                    pipeDesc.bindingLayouts = { state.initialLayout, bindlessLayout };
                else
                    pipeDesc.bindingLayouts = { state.initialLayout };
                state.initialPipeline = nvDevice->createComputePipeline(pipeDesc);
                if (!state.initialPipeline) {
                    Msg("! [ReSTIR GI] initial createComputePipeline failed (bindless=%s)",
                        bindlessLayout ? "yes" : "no");
                    state.initialLoadFailed = true;
                }
            }
        } else {
            state.initialLoadFailed = true;
            Msg("! [ReSTIR GI] restir_gi_initial shader missing (handle=%s refl=%s)",
                csResult.handle ? "ok" : "null",
                csResult.reflection ? "ok" : "null");
        }
    }
    if (state.initialPipeline)
        state.initialLoadFailed = false;

    if (!state.temporalPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_temporal");
        if (csResult.handle && csResult.reflection) {
            state.temporalLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Temporal_v2", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.temporalLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.temporalLayout };
            state.temporalPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.spatialPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_spatial");
        if (csResult.handle && csResult.reflection) {
            state.spatialLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Spatial_v2", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.spatialLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.spatialLayout };
            state.spatialPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.diTemporalPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_di_temporal");
        if (csResult.handle && csResult.reflection) {
            state.diTemporalLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_DI_Temporal_v3", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.diTemporalLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.diTemporalLayout };
            state.diTemporalPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.diSpatialPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_di_spatial");
        if (csResult.handle && csResult.reflection) {
            state.diSpatialLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_DI_Spatial_v5", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.diSpatialLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.diSpatialLayout };
            state.diSpatialPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.diShadePipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_di_shade");
        if (csResult.handle && csResult.reflection) {
            state.diShadeLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_DI_Shade_v6_EmitterSkip", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.diShadeLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.diShadeLayout };
            state.diShadePipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    EnsureWaterPipeline(device, state);

    if (!state.compositePipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_composite");
        if (csResult.handle && csResult.reflection) {
            state.compositeLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Composite_v6", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            pipeDesc.bindingLayouts = { state.compositeLayout };
            state.compositePipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.blurPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_blur");
        if (csResult.handle && csResult.reflection) {
            state.blurLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Blur4_GuideMark", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            pipeDesc.bindingLayouts = { state.blurLayout };
            state.blurPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    if (!state.temporalFilterPipeline) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_temporal_filter");
        if (csResult.handle && csResult.reflection) {
            state.temporalFilterLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_TempFilter", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            pipeDesc.bindingLayouts = { state.temporalFilterLayout };
            state.temporalFilterPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    EnsureVolumetricPipeline(device, state);

    const bool wasEnabled = state.enabled;
    state.enabled = state.initialPipeline && state.temporalPipeline && state.compositePipeline && state.diShadePipeline;
    state.initialized = true;

    if (state.enabled && !wasEnabled)
        Msg("* [ReSTIR GI] Pipelines ready (spatial=%s diTemp=%s diSpat=%s diShade=%s water=%s blur=%s vol=%s)",
            state.spatialPipeline ? "ok" : "off",
            state.diTemporalPipeline ? "ok" : "off",
            state.diSpatialPipeline ? "ok" : "off",
            state.diShadePipeline ? "ok" : "off",
            state.waterPipeline ? "ok" : "off",
            state.blurPipeline ? "ok" : "off",
            state.volPipeline ? "ok" : "FAIL");
    else if (!state.enabled && !wasEnabled)
    {
        static bool s_loggedFail = false;
        if (!s_loggedFail)
        {
            s_loggedFail = true;
            Msg("! [ReSTIR GI] Pipeline creation failed (initial=%s temporal=%s composite=%s diShade=%s) — passthrough scene",
                state.initialPipeline ? "ok" : "FAIL",
                state.temporalPipeline ? "ok" : "FAIL",
                state.compositePipeline ? "ok" : "FAIL",
                state.diShadePipeline ? "ok" : "FAIL");
        }
    }
}

static bool EnsureIrradianceCache(nvrhi::IDevice* nvDevice, ReSTIRGIPassState& state)
{
    const u32 wanted = (u32)std::max(0, ::ps_r_rt_gi_cache_size);
    if (state.irradianceCache && state.irradianceCacheSize == wanted)
        return true;

    state.irradianceCache = nullptr;
    state.irradianceCacheSize = wanted;

    nvrhi::BufferDesc bufDesc;
    bufDesc.debugName = "RTGI_IrradianceCache";
    const u32 entries = wanted > 0 ? wanted : 1u;
    bufDesc.byteSize = (size_t)entries * 16u;
    bufDesc.structStride = 16;
    bufDesc.canHaveUAVs = true;
    bufDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    bufDesc.keepInitialState = true;
    state.irradianceCache = nvDevice->createBuffer(bufDesc);
    if (!state.irradianceCache) {
        Msg("! [ReSTIR GI] Irradiance cache buffer create failed");
        return false;
    }
    return true;
}

static bool EnsurePersistentTextures(nvrhi::IDevice* nvDevice, ReSTIRGIPassState& state, u32 width, u32 height)
{
    const bool sizeOk = state.reservoirA[0] && state.diReservoir[0] && state.reservoirC[0] &&
        state.specReservoirA[0] && state.texWidth == width && state.texHeight == height;
    if (sizeOk) {
        if (!EnsureIrradianceCache(nvDevice, state))
            return false;
        return state.directLighting && state.noisyDiffuse && state.noisySpecular &&
            state.blurTemp && state.blurTempSpec && state.histDiffuse && state.histSpecular && state.hitDistance &&
            state.irradianceCache;
    }

    auto makeTex = [&](const char* name, nvrhi::Format fmt) -> nvrhi::TextureHandle {
        nvrhi::TextureDesc desc;
        desc.debugName = name;
        desc.width = width;
        desc.height = height;
        desc.format = fmt;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return nvDevice->createTexture(desc);
    };

    for (int i = 0; i < 2; i++) {
        state.reservoirA[i] = makeTex(i == 0 ? "RTGI_ReservoirA_0" : "RTGI_ReservoirA_1", nvrhi::Format::RGBA32_FLOAT);
        state.reservoirB[i] = makeTex(i == 0 ? "RTGI_ReservoirB_0" : "RTGI_ReservoirB_1", nvrhi::Format::RGBA32_FLOAT);
        state.reservoirC[i] = makeTex(i == 0 ? "RTGI_ReservoirC_0" : "RTGI_ReservoirC_1", nvrhi::Format::RG32_FLOAT);
        state.specReservoirA[i] = makeTex(i == 0 ? "RTGI_SpecA_0" : "RTGI_SpecA_1", nvrhi::Format::RGBA32_FLOAT);
        state.specReservoirB[i] = makeTex(i == 0 ? "RTGI_SpecB_0" : "RTGI_SpecB_1", nvrhi::Format::RGBA32_FLOAT);
        state.diReservoir[i] = makeTex(i == 0 ? "RTGI_DIReservoir_0" : "RTGI_DIReservoir_1", nvrhi::Format::RGBA32_FLOAT);
        if (!state.reservoirA[i] || !state.reservoirB[i] || !state.reservoirC[i] ||
            !state.specReservoirA[i] || !state.specReservoirB[i] || !state.diReservoir[i]) {
            Msg("! [ReSTIR GI] Reservoir texture create failed");
            state.enabled = false;
            return false;
        }
    }

    state.directLighting = makeTex("RTGI_DirectLighting", nvrhi::Format::RGBA16_FLOAT);
    state.noisyDiffuse = makeTex("RTGI_NoisyDiffuse", nvrhi::Format::RGBA16_FLOAT);
    state.noisySpecular = makeTex("RTGI_NoisySpecular", nvrhi::Format::RGBA16_FLOAT);
    state.blurTemp = makeTex("RTGI_BlurTemp", nvrhi::Format::RGBA16_FLOAT);
    state.blurTempSpec = makeTex("RTGI_BlurTempSpec", nvrhi::Format::RGBA16_FLOAT);
    state.histDiffuse = makeTex("RTGI_HistDiffuse", nvrhi::Format::RGBA16_FLOAT);
    state.histSpecular = makeTex("RTGI_HistSpecular", nvrhi::Format::RGBA16_FLOAT);
    state.hitDistance = makeTex("RTGI_HitDistance", nvrhi::Format::R16_FLOAT);

    if (!EnsureIrradianceCache(nvDevice, state)) {
        state.enabled = false;
        return false;
    }

    if (!state.directLighting || !state.noisyDiffuse || !state.noisySpecular || !state.blurTemp ||
        !state.blurTempSpec || !state.histDiffuse || !state.histSpecular || !state.hitDistance) {
        Msg("! [ReSTIR GI] Persistent texture create failed — GI disabled");
        state.enabled = false;
        return false;
    }

    state.texWidth = width;
    state.texHeight = height;
    return true;
}

struct InitialPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    ReSTIRGICB cbData;
    u32 width, height;
    nvrhi::ITexture* sky0;
    nvrhi::ITexture* sky1;
    u32 writeIdx;
};

struct TemporalPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle prevNormals;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle prevWorldPos;
    VirtualResourceHandle motionVectors;
    TemporalCB cbData;
    u32 width, height;
    u32 readIdx;
    u32 writeIdx;
};

struct CompositePassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle classifyWorldPos;
    VirtualResourceHandle sceneColorIn;
    VirtualResourceHandle sceneColor;
    CompositeCB cbData;
    u32 width, height;
    u32 reservoirIdx;
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
    VirtualResourceHandle prevWorldPos,
    VirtualResourceHandle motionVectors,
    VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData,
    bool skipInTreeDenoise,
    VirtualResourceHandle classifyWorldPos,
    bool skipLightingTemporal)
{
    InitializeResources(device, state);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.debugName = "rtgi_SceneColor";
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isUAV = true;
    outDesc.allowUAV = true;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;
    VirtualResourceHandle outHandle = fg.CreateTexture("rtgi_SceneColor", outDesc);

    if (!state.enabled || !accelMgr || !accelMgr->IsReady()) {
        return { sceneColorIn, {}, {}, {} };
    }

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!EnsurePersistentTextures(nvDevice, state, width, height)) {
        return { sceneColorIn, {}, {}, {} };
    }

    u32 writeIdx = state.currTemporalIdx;
    u32 readIdx = 1 - writeIdx;

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    auto* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = s_rtgiPlaceholderCube.Get();
    nvrhi::ITexture* sky1Tex = s_rtgiPlaceholderCube.Get();
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1]) {
        if (env.Current[0]->sky_texture_name.size()) {
            auto h0 = texManager->LoadTexture(env.Current[0]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h0);
            if (t) sky0Tex = t;
        }
        if (env.Current[1]->sky_texture_name.size()) {
            auto h1 = texManager->LoadTexture(env.Current[1]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h1);
            if (t) sky1Tex = t;
        }
    }

    Fvector sunDir = env.CurrentEnv.sun_dir;
    Fvector3 sc = { env.CurrentEnv.sun_color.x, env.CurrentEnv.sun_color.y, env.CurrentEnv.sun_color.z };
    float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    Fvector sunColor;
    if (sunIntensity > 0.001f)
        sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
    else
        sunColor.set(0, 0, 0);

    const auto& batchCounts = accelMgr->GetBatchCounts();

    ReSTIRGICB initialCB;
    initialCB.invViewProj = invViewProj;
    initialCB.prevViewProj = prevViewProj;
    initialCB.worldToView = Device.mView;
    initialCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    initialCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    initialCB.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
    initialCB.screenWidth = (float)width;
    initialCB.screenHeight = (float)height;
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
    initialCB.localLightSamples = (u32)std::max(0, ::ps_r_rt_gi_local_samples);
    initialCB.bounces = (u32)std::clamp(::ps_r_rt_gi_bounces, 1, 2);
    {
        auto& clm = ClusteredLightManager::Instance();
        const u32 diCount = clm.GetDILightCount();
        const u32 baseCandidates = (u32)std::max(1, ::ps_r_rt_di_candidates);
        const u32 adaptive = diCount > 0
            ? std::min(diCount, std::max(baseCandidates, (diCount + 3u) / 4u))
            : baseCandidates;
        initialCB.diCandidates = std::min(adaptive, 32u);
        initialCB.diSampleParams.set(
            static_cast<float>(diCount),
            clm.GetDIPowerSum(),
            0.f, 0.f);
    }
    {
        StaticGlobals clusterFill{};
        FillClusterParams(clusterFill);
        initialCB.clusterParams = clusterFill.cluster_params;
        initialCB.clusterScales = clusterFill.cluster_scales;
    }
    {
        const auto& hemi = env.CurrentEnv.hemi_color;
        initialCB.hemiColor.set(hemi.x, hemi.y, hemi.z, hemi.w);
    }
    initialCB.cacheSize = state.irradianceCacheSize;
    initialCB.cacheCellSize = std::max(0.05f, ::ps_r_rt_gi_cache_cell);
    initialCB.cacheMaxAge = 64u;
    initialCB.pad1 = 0;

    ResourceDesc persistDesc;
    persistDesc.type = ResourceDesc::Type::Texture2D;
    persistDesc.width = width;
    persistDesc.height = height;
    persistDesc.isImported = true;
    persistDesc.isTransient = false;
    persistDesc.isUAV = true;

    auto dlDesc = persistDesc;
    dlDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgDirectLighting = fg.ImportTexture("rtgi_DirectLighting", state.directLighting.Get(), dlDesc);

    auto resDesc = persistDesc;
    resDesc.format = nvrhi::Format::RGBA32_FLOAT;
    VirtualResourceHandle fgResA = fg.ImportTexture("rtgi_ResA_W", state.reservoirA[writeIdx].Get(), resDesc);
    VirtualResourceHandle fgResB = fg.ImportTexture("rtgi_ResB_W", state.reservoirB[writeIdx].Get(), resDesc);
    auto resCDesc = persistDesc;
    resCDesc.format = nvrhi::Format::RG32_FLOAT;
    VirtualResourceHandle fgResC = fg.ImportTexture("rtgi_ResC_W", state.reservoirC[writeIdx].Get(), resCDesc);
    VirtualResourceHandle fgSpecA = fg.ImportTexture("rtgi_SpecA_W", state.specReservoirA[writeIdx].Get(), resDesc);
    VirtualResourceHandle fgSpecB = fg.ImportTexture("rtgi_SpecB_W", state.specReservoirB[writeIdx].Get(), resDesc);
    VirtualResourceHandle fgDI = fg.ImportTexture("rtgi_DI_W", state.diReservoir[writeIdx].Get(), resDesc);
    auto ndEarlyDesc = persistDesc;
    ndEarlyDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgNoisySpecEarly = fg.ImportTexture("rtgi_NoisySpecular_Init", state.noisySpecular.Get(), ndEarlyDesc);

    fg.GetRTRegistry().RegisterRT("rt_DirectLighting", fgDirectLighting);
    fg.GetRTRegistry().RegisterRT("rt_GI_ReservoirA", fgResA);
    fg.GetRTRegistry().RegisterRT("rt_GI_ReservoirB", fgResB);
    fg.GetRTRegistry().RegisterRT("rt_GI_ReservoirC", fgResC);
    fg.GetRTRegistry().RegisterRT("rt_GI_SpecA", fgSpecA);
    fg.GetRTRegistry().RegisterRT("rt_GI_SpecB", fgSpecB);
    fg.GetRTRegistry().RegisterRT("rt_DI_Reservoir", fgDI);

    u32 diIdx = writeIdx;

    fg.addCallbackPass<InitialPassData>(
        "ReSTIR GI Initial",
        [&, sky0Tex, sky1Tex, initialCB, writeIdx, fgDirectLighting, fgResA, fgResB, fgResC, fgSpecA, fgSpecB, fgDI, fgNoisySpecEarly](FrameGraph& builder, PassHandle passHandle, InitialPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            pb.write(fgDirectLighting, ResourceState::UnorderedAccess);
            pb.write(fgResA, ResourceState::UnorderedAccess);
            pb.write(fgResB, ResourceState::UnorderedAccess);
            pb.write(fgResC, ResourceState::UnorderedAccess);
            pb.write(fgSpecA, ResourceState::UnorderedAccess);
            pb.write(fgSpecB, ResourceState::UnorderedAccess);
            pb.write(fgNoisySpecEarly, ResourceState::UnorderedAccess);
            pb.write(fgDI, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.accelMgr = accelMgr;
            data.state = &state;
            data.cbData = initialCB;
            data.width = width;
            data.height = height;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
            data.writeIdx = writeIdx;
        },
        [](const InitialPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            if (!depthTex || !normalTex || !baseColorTex || !worldPosTex) {
                Msg("! [RTGI Initial] Null FG texture: depth=%d normal=%d baseColor=%d", !!depthTex, !!normalTex, !!baseColorTex);
                return;
            }

            nvrhi::ITexture* sky0 = data.sky0;
            nvrhi::ITexture* sky1 = data.sky1;
            nvrhi::ITexture* directLit = data.state->directLighting.Get();
            nvrhi::ITexture* resA = data.state->reservoirA[data.writeIdx].Get();
            nvrhi::ITexture* resB = data.state->reservoirB[data.writeIdx].Get();
            nvrhi::ITexture* resC = data.state->reservoirC[data.writeIdx].Get();
            nvrhi::ITexture* specA = data.state->specReservoirA[data.writeIdx].Get();
            nvrhi::ITexture* specB = data.state->specReservoirB[data.writeIdx].Get();
            nvrhi::ITexture* diRes = data.state->diReservoir[data.writeIdx].Get();
            nvrhi::ITexture* noisySpec = data.state->noisySpecular.Get();
            nvrhi::IBuffer* irrCache = data.state->irradianceCache.Get();
            auto* tlas = data.accelMgr->GetTLAS();
            auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
            auto* megaVB = data.accelMgr->GetMegaVB();
            auto* megaIB = data.accelMgr->GetMegaIB();
            auto* matBuf = data.accelMgr->GetMaterialBuffer();
            auto* terrainBuf = data.accelMgr->GetTerrainMaterialBuffer();

            if (!sky0 || !sky1 || !directLit || !resA || !resB || !resC || !specA || !specB || !diRes || !noisySpec ||
                !irrCache || !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf) {
                Msg("! [RTGI Initial] Null binding: sky0=%d sky1=%d directLit=%d resA=%d resB=%d di=%d tlas=%d batch=%d megaVB=%d megaIB=%d mat=%d terrain=%d",
                    !!sky0, !!sky1, !!directLit, !!resA, !!resB, !!diRes, !!tlas, !!batchInfo, !!megaVB, !!megaIB, !!matBuf, !!terrainBuf);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            ReSTIRGICB cb = data.cbData;
            const auto& bc = data.accelMgr->GetBatchCounts();
            cb.identityStaticCount = bc.identityStatic;
            cb.terrainBatchCount = bc.terrain;
            cb.skinnedBatchStart = bc.skinned > 0
                ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                : 0xFFFFFFFFu;
            cb.grassBatchStart = bc.grass > 0
                ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                : 0xFFFFFFFFu;
            cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
            cb.cacheSize = data.state->irradianceCacheSize;
            {
                auto& clm = ClusteredLightManager::Instance();
                cb.diSampleParams.set(
                    static_cast<float>(clm.GetDILightCount()),
                    clm.GetDIPowerSum(),
                    0.f, 0.f);
            }
            cmdList->writeBuffer(data.state->cb, &cb, sizeof(ReSTIRGICB));

            nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
            nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!skinnedVB) skinnedVB = s_rtgiPlaceholderBuffer.Get();
            if (!skinnedIB) skinnedIB = s_rtgiPlaceholderBuffer.Get();
            if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
            if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_initial", ".cs");
            if (!csReflection) return;

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Initial");
            bsb.ConstantBuffer("ReSTIRGIParams", data.state->cb);
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
            {
                auto& clm = ClusteredLightManager::Instance();
                nvrhi::IBuffer* lightData = clm.GetLightDataBuffer();
                nvrhi::IBuffer* diIndices = clm.GetDILightIndicesBuffer();
                nvrhi::IBuffer* diCdf = clm.GetDILightCDFBuffer();
                if (!lightData) lightData = s_rtgiPlaceholderBuffer.Get();
                if (!diIndices) diIndices = s_rtgiPlaceholderBuffer.Get();
                if (!diCdf) diCdf = s_rtgiPlaceholderBuffer.Get();
                bsb.BufferSRV("g_LightData", lightData);
                bsb.BufferSRV("g_DILightIndices", diIndices);
                bsb.BufferSRV("g_DILightCDF", diCdf);
            }
            bsb.TextureUAV("u_DirectLighting", directLit);
            bsb.TextureUAV("u_ReservoirA", resA);
            bsb.TextureUAV("u_ReservoirB", resB);
            bsb.TextureUAV("u_NoisySpecular", noisySpec);
            bsb.TextureUAV("u_DIReservoir", diRes);
            bsb.TextureUAV("u_SpecReservoirA", specA);
            bsb.TextureUAV("u_SpecReservoirB", specB);
            bsb.TextureUAV("u_ReservoirC", resC);
            bsb.BufferUAV("u_IrradianceCache", irrCache);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->initialLayout, nvDevice);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->initialPipeline;
            cs.bindings = { bindingSet };

            if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                    cs.addBindingSet(bindlessTable);
            }

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    if (!skipLightingTemporal && hasPrevFrameData && motionVectors.is_valid()) {
        TemporalCB temporalCB;
        temporalCB.invViewProj = invViewProj;
        temporalCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        temporalCB.screenWidth = (float)width;
        temporalCB.screenHeight = (float)height;
        temporalCB.invScreenWidth = 1.0f / width;
        temporalCB.invScreenHeight = 1.0f / height;
        temporalCB.frameIndex = Device.dwFrame;
        temporalCB.identityStaticCount = initialCB.identityStaticCount;
        temporalCB.terrainBatchCount = initialCB.terrainBatchCount;
        temporalCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        temporalCB.grassBatchStart = initialCB.grassBatchStart;
        temporalCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        temporalCB.pad[0] = temporalCB.pad[1] = 0;

        fg.addCallbackPass<TemporalPassData>(
            "ReSTIR GI Temporal",
            [&, temporalCB, readIdx, writeIdx, fgResA, fgResB, fgResC, fgSpecA, fgSpecB](FrameGraph& builder, PassHandle passHandle, TemporalPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevWorldPos.is_valid())
                    data.prevWorldPos = pb.read(prevWorldPos, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.readWrite(fgResA, ResourceState::UnorderedAccess);
                pb.readWrite(fgResB, ResourceState::UnorderedAccess);
                pb.readWrite(fgResC, ResourceState::UnorderedAccess);
                pb.readWrite(fgSpecA, ResourceState::UnorderedAccess);
                pb.readWrite(fgSpecB, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = temporalCB;
                data.width = width;
                data.height = height;
                data.readIdx = readIdx;
                data.writeIdx = writeIdx;
            },
            [](const TemporalPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fg.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                auto* prevWorldPosTex = data.prevWorldPos.is_valid() ? fg.GetPhysicalTexture(data.prevWorldPos) : worldPosTex;
                auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !mvTex) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                TemporalCB cb = data.cbData;
                if (data.accelMgr) {
                    const auto& bc = data.accelMgr->GetBatchCounts();
                    cb.identityStaticCount = bc.identityStatic;
                    cb.terrainBatchCount = bc.terrain;
                    cb.skinnedBatchStart = bc.skinned > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                        : 0xFFFFFFFFu;
                    cb.grassBatchStart = bc.grass > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                        : 0xFFFFFFFFu;
                    cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
                }
                cmdList->writeBuffer(data.state->cb, &cb, sizeof(TemporalCB));

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_temporal", ".cs");
                if (!csReflection) return;

                auto* tlas = data.accelMgr ? data.accelMgr->GetTLAS() : nullptr;
                if (!tlas) return;
                nvrhi::IBuffer* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                nvrhi::IBuffer* megaVB = data.accelMgr->GetMegaVB();
                nvrhi::IBuffer* megaIB = data.accelMgr->GetMegaIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!batchInfo) batchInfo = s_rtgiPlaceholderBuffer.Get();
                if (!megaVB) megaVB = s_rtgiPlaceholderBuffer.Get();
                if (!megaIB) megaIB = s_rtgiPlaceholderBuffer.Get();
                if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
                if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Temporal");
                bsb.ConstantBuffer("ReSTIRTemporalParams", data.state->cb);
                BindBindlessMaterialTables(bsb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.Texture("t_PrevReservoirA", data.state->reservoirA[data.readIdx]);
                bsb.Texture("t_PrevReservoirB", data.state->reservoirB[data.readIdx]);
                bsb.Texture("t_PrevReservoirC", data.state->reservoirC[data.readIdx]);
                bsb.Texture("t_PrevSpecA", data.state->specReservoirA[data.readIdx]);
                bsb.Texture("t_PrevSpecB", data.state->specReservoirB[data.readIdx]);
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_PrevWorldPos", prevWorldPosTex);
                bsb.TextureUAV("u_ReservoirA", data.state->reservoirA[data.writeIdx]);
                bsb.TextureUAV("u_ReservoirB", data.state->reservoirB[data.writeIdx]);
                bsb.TextureUAV("u_ReservoirC", data.state->reservoirC[data.writeIdx]);
                bsb.TextureUAV("u_SpecReservoirA", data.state->specReservoirA[data.writeIdx]);
                bsb.TextureUAV("u_SpecReservoirB", data.state->specReservoirB[data.writeIdx]);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->temporalLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalPipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }

                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    if (!skipLightingTemporal && hasPrevFrameData && motionVectors.is_valid() &&
        state.diTemporalPipeline && state.diTemporalLayout) {
        DITemporalCB diTempCB{};
        diTempCB.worldToView = Device.mView;
        diTempCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diTempCB.screenWidth = (float)width;
        diTempCB.screenHeight = (float)height;
        diTempCB.invScreenWidth = 1.0f / width;
        diTempCB.invScreenHeight = 1.0f / height;
        diTempCB.frameIndex = Device.dwFrame;
        diTempCB.mMax = (u32)std::max(1, ::ps_r_rt_di_m_max);
        diTempCB.pad[0] = diTempCB.pad[1] = 0.f;
        diTempCB.clusterParams = initialCB.clusterParams;
        diTempCB.clusterScales = initialCB.clusterScales;

        VirtualResourceHandle fgDIPrev = fg.ImportTexture("rtgi_DI_Prev", state.diReservoir[readIdx].Get(), resDesc);

        struct DITemporalPassData {
            fg::RenderDevice* device;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle prevNormals;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            VirtualResourceHandle prevWorldPos;
            VirtualResourceHandle motionVectors;
            DITemporalCB cbData;
            u32 width, height;
            u32 readIdx;
            u32 writeIdx;
        };

        fg.addCallbackPass<DITemporalPassData>(
            "ReSTIR DI Temporal",
            [&, diTempCB, readIdx, writeIdx, fgDI, fgDIPrev](FrameGraph& builder, PassHandle passHandle, DITemporalPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevWorldPos.is_valid())
                    data.prevWorldPos = pb.read(prevWorldPos, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.read(fgDIPrev, ResourceState::ShaderResource);
                pb.readWrite(fgDI, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = diTempCB;
                data.width = width;
                data.height = height;
                data.readIdx = readIdx;
                data.writeIdx = writeIdx;
            },
            [](const DITemporalPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fg.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                auto* prevWorldPosTex = data.prevWorldPos.is_valid() ? fg.GetPhysicalTexture(data.prevWorldPos) : worldPosTex;
                auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !mvTex) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(DITemporalCB));

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_temporal", ".cs");
                if (!csReflection) return;

                nvrhi::IBuffer* lightData = ClusteredLightManager::Instance().GetLightDataBuffer();
                if (!lightData) lightData = s_rtgiPlaceholderBuffer.Get();

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRDI.Temporal");
                bsb.ConstantBuffer("ReSTIRDITemporalParams", data.state->cb);
                BindBindlessMaterialTables(bsb);
                bsb.BufferSRV("g_LightData", lightData);
                bsb.Texture("t_PrevDI", data.state->diReservoir[data.readIdx]);
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_PrevWorldPos", prevWorldPosTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.TextureUAV("u_DIReservoir", data.state->diReservoir[data.writeIdx]);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->diTemporalLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diTemporalPipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    u32 compositeReservoirIdx = writeIdx;
    if (state.spatialPipeline && ::ps_r_rt_gi_spatial_samples > 0) {
        SpatialCB spatialCB;
        spatialCB.invViewProj = invViewProj;
        spatialCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        spatialCB.screenWidth = (float)width;
        spatialCB.screenHeight = (float)height;
        spatialCB.invScreenWidth = 1.0f / width;
        spatialCB.invScreenHeight = 1.0f / height;
        spatialCB.frameIndex = Device.dwFrame;
        spatialCB.spatialSamples = (u32)::ps_r_rt_gi_spatial_samples;
        spatialCB.spatialRadius = ::ps_r_rt_gi_spatial_radius;
        spatialCB.mMax = (u32)std::max(1, ::ps_r_rt_gi_m_max);
        spatialCB.identityStaticCount = initialCB.identityStaticCount;
        spatialCB.terrainBatchCount = initialCB.terrainBatchCount;
        spatialCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        spatialCB.grassBatchStart = initialCB.grassBatchStart;
        spatialCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        spatialCB.pad[0] = spatialCB.pad[1] = spatialCB.pad[2] = 0;

        VirtualResourceHandle fgResAOut = fg.ImportTexture("rtgi_ResA_S", state.reservoirA[readIdx].Get(), resDesc);
        VirtualResourceHandle fgResBOut = fg.ImportTexture("rtgi_ResB_S", state.reservoirB[readIdx].Get(), resDesc);
        VirtualResourceHandle fgResCOut = fg.ImportTexture("rtgi_ResC_S", state.reservoirC[readIdx].Get(), resCDesc);
        VirtualResourceHandle fgSpecAOut = fg.ImportTexture("rtgi_SpecA_S", state.specReservoirA[readIdx].Get(), resDesc);
        VirtualResourceHandle fgSpecBOut = fg.ImportTexture("rtgi_SpecB_S", state.specReservoirB[readIdx].Get(), resDesc);

        struct SpatialPassData {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            SpatialCB cbData;
            u32 width, height;
            u32 srcIdx;
            u32 dstIdx;
        };

        fg.addCallbackPass<SpatialPassData>(
            "ReSTIR GI Spatial",
            [&, spatialCB, writeIdx, readIdx, fgResA, fgResB, fgResC, fgSpecA, fgSpecB, fgResAOut, fgResBOut, fgResCOut, fgSpecAOut, fgSpecBOut](FrameGraph& builder, PassHandle passHandle, SpatialPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.read(fgResA, ResourceState::ShaderResource);
                pb.read(fgResB, ResourceState::ShaderResource);
                pb.read(fgResC, ResourceState::ShaderResource);
                pb.read(fgSpecA, ResourceState::ShaderResource);
                pb.read(fgSpecB, ResourceState::ShaderResource);
                pb.write(fgResAOut, ResourceState::UnorderedAccess);
                pb.write(fgResBOut, ResourceState::UnorderedAccess);
                pb.write(fgResCOut, ResourceState::UnorderedAccess);
                pb.write(fgSpecAOut, ResourceState::UnorderedAccess);
                pb.write(fgSpecBOut, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = spatialCB;
                data.width = width;
                data.height = height;
                data.srcIdx = writeIdx;
                data.dstIdx = readIdx;
            },
            [](const SpatialPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex) return;

                nvrhi::ITexture* srcA = data.state->reservoirA[data.srcIdx].Get();
                nvrhi::ITexture* srcB = data.state->reservoirB[data.srcIdx].Get();
                nvrhi::ITexture* srcC = data.state->reservoirC[data.srcIdx].Get();
                nvrhi::ITexture* srcSpecA = data.state->specReservoirA[data.srcIdx].Get();
                nvrhi::ITexture* srcSpecB = data.state->specReservoirB[data.srcIdx].Get();
                nvrhi::ITexture* dstA = data.state->reservoirA[data.dstIdx].Get();
                nvrhi::ITexture* dstB = data.state->reservoirB[data.dstIdx].Get();
                nvrhi::ITexture* dstC = data.state->reservoirC[data.dstIdx].Get();
                nvrhi::ITexture* dstSpecA = data.state->specReservoirA[data.dstIdx].Get();
                nvrhi::ITexture* dstSpecB = data.state->specReservoirB[data.dstIdx].Get();
                auto* tlas = data.accelMgr ? data.accelMgr->GetTLAS() : nullptr;
                if (!srcA || !srcB || !srcC || !srcSpecA || !srcSpecB || !dstA || !dstB || !dstC || !dstSpecA || !dstSpecB || !tlas) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                SpatialCB cb = data.cbData;
                if (data.accelMgr) {
                    const auto& bc = data.accelMgr->GetBatchCounts();
                    cb.identityStaticCount = bc.identityStatic;
                    cb.terrainBatchCount = bc.terrain;
                    cb.skinnedBatchStart = bc.skinned > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                        : 0xFFFFFFFFu;
                    cb.grassBatchStart = bc.grass > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                        : 0xFFFFFFFFu;
                    cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
                }
                cmdList->writeBuffer(data.state->cb, &cb, sizeof(SpatialCB));

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_spatial", ".cs");
                if (!csReflection) return;

                nvrhi::IBuffer* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                nvrhi::IBuffer* megaVB = data.accelMgr->GetMegaVB();
                nvrhi::IBuffer* megaIB = data.accelMgr->GetMegaIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!batchInfo) batchInfo = s_rtgiPlaceholderBuffer.Get();
                if (!megaVB) megaVB = s_rtgiPlaceholderBuffer.Get();
                if (!megaIB) megaIB = s_rtgiPlaceholderBuffer.Get();
                if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
                if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Spatial");
                bsb.ConstantBuffer("ReSTIRSpatialParams", data.state->cb);
                BindBindlessMaterialTables(bsb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_ReservoirA", srcA);
                bsb.Texture("t_ReservoirB", srcB);
                bsb.Texture("t_ReservoirC", srcC);
                bsb.Texture("t_SpecA", srcSpecA);
                bsb.Texture("t_SpecB", srcSpecB);
                bsb.TextureUAV("u_ReservoirA", dstA);
                bsb.TextureUAV("u_ReservoirB", dstB);
                bsb.TextureUAV("u_ReservoirC", dstC);
                bsb.TextureUAV("u_SpecReservoirA", dstSpecA);
                bsb.TextureUAV("u_SpecReservoirB", dstSpecB);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->spatialLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->spatialPipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );

        compositeReservoirIdx = readIdx;
        fgResA = fgResAOut;
        fgResB = fgResBOut;
        fgResC = fgResCOut;
        fgSpecA = fgSpecAOut;
        fgSpecB = fgSpecBOut;
    }

    if (state.diSpatialPipeline && state.diSpatialLayout && ::ps_r_rt_di_spatial_samples > 0) {
        DISpatialCB diSpatCB{};
        diSpatCB.worldToView = Device.mView;
        diSpatCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diSpatCB.screenWidth = (float)width;
        diSpatCB.screenHeight = (float)height;
        diSpatCB.invScreenWidth = 1.0f / width;
        diSpatCB.invScreenHeight = 1.0f / height;
        diSpatCB.frameIndex = Device.dwFrame;
        diSpatCB.spatialSamples = (u32)::ps_r_rt_di_spatial_samples;
        diSpatCB.spatialRadius = ::ps_r_rt_di_spatial_radius;
        diSpatCB.mMax = (u32)std::max(1, ::ps_r_rt_di_m_max);
        diSpatCB.clusterParams = initialCB.clusterParams;
        diSpatCB.clusterScales = initialCB.clusterScales;
        diSpatCB.grassBatchStart = initialCB.grassBatchStart;
        diSpatCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        diSpatCB.identityStaticCount = initialCB.identityStaticCount;
        diSpatCB.terrainBatchCount = initialCB.terrainBatchCount;
        diSpatCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        diSpatCB.pad0 = 0;

        VirtualResourceHandle fgDIOut = fg.ImportTexture("rtgi_DI_S", state.diReservoir[readIdx].Get(), resDesc);

        struct DISpatialPassData {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            DISpatialCB cbData;
            u32 width, height;
            u32 srcIdx;
            u32 dstIdx;
        };

        fg.addCallbackPass<DISpatialPassData>(
            "ReSTIR DI Spatial",
            [&, diSpatCB, writeIdx, readIdx, fgDI, fgDIOut](FrameGraph& builder, PassHandle passHandle, DISpatialPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.read(fgDI, ResourceState::ShaderResource);
                pb.write(fgDIOut, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = diSpatCB;
                data.width = width;
                data.height = height;
                data.srcIdx = writeIdx;
                data.dstIdx = readIdx;
            },
            [](const DISpatialPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex) return;

                nvrhi::ITexture* srcDI = data.state->diReservoir[data.srcIdx].Get();
                nvrhi::ITexture* dstDI = data.state->diReservoir[data.dstIdx].Get();
                auto* tlas = data.accelMgr ? data.accelMgr->GetTLAS() : nullptr;
                if (!srcDI || !dstDI || !tlas) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_spatial", ".cs");
                if (!csReflection) return;

                nvrhi::IBuffer* lightData = ClusteredLightManager::Instance().GetLightDataBuffer();
                if (!lightData) lightData = s_rtgiPlaceholderBuffer.Get();
                nvrhi::IBuffer* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                nvrhi::IBuffer* megaVB = data.accelMgr->GetMegaVB();
                nvrhi::IBuffer* megaIB = data.accelMgr->GetMegaIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!batchInfo) batchInfo = s_rtgiPlaceholderBuffer.Get();
                if (!megaVB) megaVB = s_rtgiPlaceholderBuffer.Get();
                if (!megaIB) megaIB = s_rtgiPlaceholderBuffer.Get();
                if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
                if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

                {
                    const auto& bc = data.accelMgr->GetBatchCounts();
                    DISpatialCB cb = data.cbData;
                    cb.identityStaticCount = bc.identityStatic;
                    cb.terrainBatchCount = bc.terrain;
                    cb.skinnedBatchStart = bc.skinned > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                        : 0xFFFFFFFFu;
                    cb.grassBatchStart = bc.grass > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                        : 0xFFFFFFFFu;
                    cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
                    cmdList->writeBuffer(data.state->cb, &cb, sizeof(DISpatialCB));
                }

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRDI.Spatial");
                bsb.ConstantBuffer("ReSTIRDISpatialParams", data.state->cb);
                BindBindlessMaterialTables(bsb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.BufferSRV("g_LightData", lightData);
                bsb.Texture("t_SrcDI", srcDI);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.TextureUAV("u_DIReservoir", dstDI);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->diSpatialLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diSpatialPipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );

        diIdx = readIdx;
        fgDI = fgDIOut;
    }

    if (state.diShadePipeline && state.diShadeLayout) {
        DIShadeCB diShadeCB{};
        diShadeCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        diShadeCB.screenWidth = (float)width;
        diShadeCB.screenHeight = (float)height;
        diShadeCB.grassBatchStart = initialCB.grassBatchStart;
        diShadeCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        diShadeCB.clusterParams = initialCB.clusterParams;
        diShadeCB.identityStaticCount = initialCB.identityStaticCount;
        diShadeCB.terrainBatchCount = initialCB.terrainBatchCount;
        diShadeCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        diShadeCB.pad = 0;

        struct DIShadePassData {
            fg::RenderDevice* device;
            RTAccelStructManager* accelMgr;
            ReSTIRGIPassState* state;
            VirtualResourceHandle depth;
            VirtualResourceHandle normal;
            VirtualResourceHandle baseColor;
            VirtualResourceHandle worldPos;
            DIShadeCB cbData;
            u32 width, height;
            u32 diIdx;
        };

        fg.addCallbackPass<DIShadePassData>(
            "ReSTIR DI Shade",
            [&, diShadeCB, diIdx, fgDI, fgDirectLighting](FrameGraph& builder, PassHandle passHandle, DIShadePassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                pb.read(fgDI, ResourceState::ShaderResource);
                pb.readWrite(fgDirectLighting, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.accelMgr = accelMgr;
                data.state = &state;
                data.cbData = diShadeCB;
                data.width = width;
                data.height = height;
                data.diIdx = diIdx;
            },
            [](const DIShadePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex) return;

                nvrhi::ITexture* diRes = data.state->diReservoir[data.diIdx].Get();
                nvrhi::ITexture* directLit = data.state->directLighting.Get();
                auto* tlas = data.accelMgr ? data.accelMgr->GetTLAS() : nullptr;
                if (!diRes || !directLit || !tlas) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_di_shade", ".cs");
                if (!csReflection) return;

                nvrhi::IBuffer* lightData = ClusteredLightManager::Instance().GetLightDataBuffer();
                if (!lightData) lightData = s_rtgiPlaceholderBuffer.Get();
                nvrhi::IBuffer* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                nvrhi::IBuffer* megaVB = data.accelMgr->GetMegaVB();
                nvrhi::IBuffer* megaIB = data.accelMgr->GetMegaIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!batchInfo) batchInfo = s_rtgiPlaceholderBuffer.Get();
                if (!megaVB) megaVB = s_rtgiPlaceholderBuffer.Get();
                if (!megaIB) megaIB = s_rtgiPlaceholderBuffer.Get();
                if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
                if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

                {
                    const auto& bc = data.accelMgr->GetBatchCounts();
                    DIShadeCB cb = data.cbData;
                    cb.identityStaticCount = bc.identityStatic;
                    cb.terrainBatchCount = bc.terrain;
                    cb.skinnedBatchStart = bc.skinned > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                        : 0xFFFFFFFFu;
                    cb.grassBatchStart = bc.grass > 0
                        ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                        : 0xFFFFFFFFu;
                    cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
                    cmdList->writeBuffer(data.state->cb, &cb, sizeof(DIShadeCB));
                }

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRDI.Shade");
                bsb.ConstantBuffer("ReSTIRDIShadeParams", data.state->cb);
                BindBindlessMaterialTables(bsb);
                bsb.AccelStruct("g_SceneTLAS", tlas);
                bsb.BufferSRV("g_BatchInfo", batchInfo);
                bsb.BufferSRV("g_MegaVB", megaVB);
                bsb.BufferSRV("g_MegaIB", megaIB);
                bsb.BufferSRV("g_GrassVB", grassVB);
                bsb.BufferSRV("g_GrassIB", grassIB);
                bsb.BufferSRV("g_LightData", lightData);
                bsb.Texture("t_DIReservoir", diRes);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.TextureUAV("u_DirectLighting", directLit);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->diShadeLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->diShadePipeline;
                cs.bindings = { bindingSet };
                if (auto* backend = data.device ? data.device->GetBackend() : nullptr) {
                    if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                        cs.addBindingSet(bindlessTable);
                }
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
    }

    auto ndDesc = persistDesc;
    ndDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgNoisyDiff = fg.ImportTexture("rtgi_NoisyDiffuse", state.noisyDiffuse.Get(), ndDesc);
    VirtualResourceHandle fgNoisySpec = fgNoisySpecEarly;
    VirtualResourceHandle fgBlurTemp = fg.ImportTexture("rtgi_BlurTemp", state.blurTemp.Get(), ndDesc);
    VirtualResourceHandle fgBlurTempSpec = fg.ImportTexture("rtgi_BlurTempSpec", state.blurTempSpec.Get(), ndDesc);
    VirtualResourceHandle fgHistDiff = fg.ImportTexture("rtgi_HistDiffuse", state.histDiffuse.Get(), ndDesc);
    VirtualResourceHandle fgHistSpec = fg.ImportTexture("rtgi_HistSpecular", state.histSpecular.Get(), ndDesc);
    auto hdDesc = persistDesc;
    hdDesc.format = nvrhi::Format::R16_FLOAT;
    VirtualResourceHandle fgHitDist = fg.ImportTexture("rtgi_HitDistance", state.hitDistance.Get(), hdDesc);
    fg.GetRTRegistry().RegisterRT("rt_RT_Diffuse", fgNoisyDiff);
    fg.GetRTRegistry().RegisterRT("rt_RT_Specular", fgNoisySpec);
    fg.GetRTRegistry().RegisterRT("rt_RT_HitDist", fgHitDist);

    CompositeCB compositeCB;
    compositeCB.invViewProj = invViewProj;
    compositeCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    compositeCB.screenWidth = (float)width;
    compositeCB.screenHeight = (float)height;
    compositeCB.giIntensity = giIntensity;
    compositeCB.pad = 0;
    {
        StaticGlobals fogFill{};
        FillGlobalConstants(fogFill);
        compositeCB.fogParams = fogFill.fog_params;
        compositeCB.fogColor = fogFill.fog_color;
    }

    VirtualResourceHandle classifyWP = classifyWorldPos.is_valid() ? classifyWorldPos : worldPos;

    auto& compositeData = fg.addCallbackPass<CompositePassData>(
        "ReSTIR GI Composite",
        [&, compositeCB, compositeReservoirIdx, outHandle, fgDirectLighting, fgResA, fgResB, fgSpecA, fgSpecB, fgNoisyDiff, fgNoisySpec, fgHitDist, classifyWP](FrameGraph& builder, PassHandle passHandle, CompositePassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.classifyWorldPos = pb.read(classifyWP, ResourceState::ShaderResource);
            data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
            pb.read(fgDirectLighting, ResourceState::ShaderResource);
            pb.read(fgResA, ResourceState::ShaderResource);
            pb.read(fgResB, ResourceState::ShaderResource);
            pb.read(fgSpecA, ResourceState::ShaderResource);
            pb.read(fgSpecB, ResourceState::ShaderResource);
            pb.read(fgNoisySpec, ResourceState::ShaderResource);
            data.sceneColor = pb.write(outHandle, ResourceState::UnorderedAccess);
            pb.write(fgNoisyDiff, ResourceState::UnorderedAccess);
            pb.write(fgHitDist, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.cbData = compositeCB;
            data.width = width;
            data.height = height;
            data.reservoirIdx = compositeReservoirIdx;
        },
        [](const CompositePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            auto* classifyTex = fg.GetPhysicalTexture(data.classifyWorldPos);
            auto* sceneColorInTex = fg.GetPhysicalTexture(data.sceneColorIn);
            auto* outTex = fg.GetPhysicalTexture(data.sceneColor);
            if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !classifyTex || !sceneColorInTex || !outTex) return;

            nvrhi::ITexture* directLit = data.state->directLighting.Get();
            nvrhi::ITexture* resA = data.state->reservoirA[data.reservoirIdx].Get();
            nvrhi::ITexture* resB = data.state->reservoirB[data.reservoirIdx].Get();
            nvrhi::ITexture* specA = data.state->specReservoirA[data.reservoirIdx].Get();
            nvrhi::ITexture* specB = data.state->specReservoirB[data.reservoirIdx].Get();
            nvrhi::ITexture* noisyDiff = data.state->noisyDiffuse.Get();
            nvrhi::ITexture* noisySpec = data.state->noisySpecular.Get();
            nvrhi::ITexture* hitDist = data.state->hitDistance.Get();
            if (!directLit || !resA || !resB || !specA || !specB || !noisyDiff || !noisySpec || !hitDist) {
                Msg("! [RTGI Composite] Null persistent texture");
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(CompositeCB));

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_composite", ".cs");
            if (!csReflection) return;

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Composite");
            bsb.ConstantBuffer("CompositeParams", data.state->cb);
            bsb.Texture("t_DirectLighting", directLit);
            bsb.Texture("t_ReservoirA", resA);
            bsb.Texture("t_ReservoirB", resB);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.Texture("t_SceneColorIn", sceneColorInTex);
            bsb.Texture("t_WorldPos", worldPosTex);
            bsb.Texture("t_NoisySpecular", noisySpec);
            bsb.Texture("t_ClassifyWorldPos", classifyTex);
            bsb.Texture("t_SpecReservoirA", specA);
            bsb.Texture("t_SpecReservoirB", specB);
            bsb.TextureUAV("u_SceneColor", outTex);
            bsb.TextureUAV("u_NoisyDiffuse", noisyDiff);
            bsb.TextureUAV("u_HitDist", hitDist);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->compositeLayout, nvDevice);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->compositePipeline;
            cs.bindings = { bindingSet };

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    state.currTemporalIdx = 1 - compositeReservoirIdx;

    VirtualResourceHandle finalColor = compositeData.sceneColor;

    if (!skipInTreeDenoise && state.temporalFilterPipeline && state.temporalFilterLayout && hasPrevFrameData &&
        motionVectors.is_valid() && prevWorldPos.is_valid() && prevNormals.is_valid()) {
        TemporalFilterCB tfCB{};
        tfCB.screenWidth = (float)width;
        tfCB.screenHeight = (float)height;
        tfCB.invScreenWidth = 1.0f / (float)width;
        tfCB.invScreenHeight = 1.0f / (float)height;
        tfCB.alpha = std::clamp(::ps_r_rt_gi_temporal_alpha, 0.f, 0.98f);
        tfCB.pad0 = 0.f;
        tfCB.enabled = 1;
        tfCB.pad1 = 0;

        struct TemporalFilterPassData {
            VirtualResourceHandle depth, normal, worldPos, prevWorldPos, prevNormal, motion;
            fg::RenderDevice* device = nullptr;
            ReSTIRGIPassState* state = nullptr;
            TemporalFilterCB cbData{};
            u32 width = 0, height = 0;
        };

        fg.addCallbackPass<TemporalFilterPassData>(
            "ReSTIR GI Temporal Filter",
            [&, tfCB, fgNoisyDiff, fgNoisySpec, fgHistDiff, fgHistSpec, fgBlurTemp, fgBlurTempSpec](FrameGraph& builder, PassHandle passHandle, TemporalFilterPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                data.prevWorldPos = pb.read(prevWorldPos, ResourceState::ShaderResource);
                data.prevNormal = pb.read(prevNormals, ResourceState::ShaderResource);
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.read(fgNoisyDiff, ResourceState::ShaderResource);
                pb.read(fgNoisySpec, ResourceState::ShaderResource);
                pb.read(fgHistDiff, ResourceState::ShaderResource);
                pb.read(fgHistSpec, ResourceState::ShaderResource);
                pb.write(fgBlurTemp, ResourceState::UnorderedAccess);
                pb.write(fgBlurTempSpec, ResourceState::UnorderedAccess);
                data.device = device;
                data.state = &state;
                data.cbData = tfCB;
                data.width = width;
                data.height = height;
            },
            [](const TemporalFilterPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                if (!data.state || !data.state->temporalFilterPipeline)
                    return;
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
                auto* prevWorldPosTex = fgGraph.GetPhysicalTexture(data.prevWorldPos);
                auto* prevNormalTex = fgGraph.GetPhysicalTexture(data.prevNormal);
                auto* mvTex = fgGraph.GetPhysicalTexture(data.motion);
                nvrhi::ITexture* currD = data.state->noisyDiffuse.Get();
                nvrhi::ITexture* currS = data.state->noisySpecular.Get();
                nvrhi::ITexture* histD = data.state->histDiffuse.Get();
                nvrhi::ITexture* histS = data.state->histSpecular.Get();
                nvrhi::ITexture* outD = data.state->blurTemp.Get();
                nvrhi::ITexture* outS = data.state->blurTempSpec.Get();
                if (!depthTex || !normalTex || !worldPosTex || !prevWorldPosTex || !prevNormalTex || !mvTex ||
                    !currD || !currS || !histD || !histS || !outD || !outS)
                    return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalFilterCB));

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_temporal_filter", ".cs");
                if (!csReflection)
                    return;

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.TempFilter");
                bsb.ConstantBuffer("TemporalFilterParams", data.state->cb);
                bsb.Texture("t_CurrDiffuse", currD);
                bsb.Texture("t_CurrSpecular", currS);
                bsb.Texture("t_HistDiffuse", histD);
                bsb.Texture("t_HistSpecular", histS);
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_PrevWorldPos", prevWorldPosTex);
                bsb.Texture("t_PrevNormal", prevNormalTex);
                bsb.TextureUAV("u_OutDiffuse", outD);
                bsb.TextureUAV("u_OutSpecular", outS);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->temporalFilterLayout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalFilterPipeline;
                cs.bindings = { bindingSet };
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);

                cmdList->copyTexture(currD, nvrhi::TextureSlice(), outD, nvrhi::TextureSlice());
                cmdList->copyTexture(currS, nvrhi::TextureSlice(), outS, nvrhi::TextureSlice());
            }
        );
    }

    if (!skipInTreeDenoise && state.blurPipeline && state.blurLayout) {
        BlurCB blurCB{};
        blurCB.screenWidth = (float)width;
        blurCB.screenHeight = (float)height;
        blurCB.invScreenWidth = 1.0f / (float)width;
        blurCB.invScreenHeight = 1.0f / (float)height;
        blurCB.phiNormal = 4.0f;
        blurCB.phiDepth = 6.0f;
        blurCB.step = 1;
        blurCB.mode = 0;

        struct BlurPassData {
            VirtualResourceHandle depth, normal, sceneColorIn, sceneColor, classifyWorldPos, worldPos;
            fg::RenderDevice* device = nullptr;
            ReSTIRGIPassState* state = nullptr;
            BlurCB cbData{};
            u32 width = 0, height = 0;
            bool srcIsTemp = false;
            bool dstIsTemp = false;
            bool finalPass = false;
        };

        const int atrousSteps = std::clamp(::ps_r_rt_gi_atrous_steps, 1, 6);
        bool srcIsTemp = false;

        for (int i = 0; i < atrousSteps; ++i) {
            const u32 step = 1u << (atrousSteps - 1 - i);
            const bool finalPass = (i == atrousSteps - 1);
            blurCB.step = step;
            blurCB.mode = finalPass ? 1u : 0u;
            const VirtualResourceHandle srcDiff = srcIsTemp ? fgBlurTemp : fgNoisyDiff;
            const VirtualResourceHandle dstDiff = finalPass ? fgNoisyDiff : (srcIsTemp ? fgNoisyDiff : fgBlurTemp);
            const VirtualResourceHandle srcSpec = srcIsTemp ? fgBlurTempSpec : fgNoisySpec;
            const VirtualResourceHandle dstSpec = finalPass ? fgNoisySpec : (srcIsTemp ? fgNoisySpec : fgBlurTempSpec);
            const bool dstIsTemp = !finalPass && !srcIsTemp;

            auto& blurData = fg.addCallbackPass<BlurPassData>(
                finalPass ? "ReSTIR GI Blur Final" : "ReSTIR GI Blur",
                [&, blurCB, outHandle, fgDirectLighting, srcDiff, dstDiff, srcSpec, dstSpec, srcIsTemp, dstIsTemp, finalPass, sceneColorIn, fgHistDiff, fgHistSpec, classifyWP](FrameGraph& builder, PassHandle passHandle, BlurPassData& data) {
                    RenderPassBuilder pb(builder, passHandle);
                    data.depth = pb.read(depth, ResourceState::ShaderResource);
                    data.normal = pb.read(normal, ResourceState::ShaderResource);
                    data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
                    data.classifyWorldPos = pb.read(classifyWP, ResourceState::ShaderResource);
                    data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                    pb.read(fgDirectLighting, ResourceState::ShaderResource);
                    pb.read(srcDiff, ResourceState::ShaderResource);
                    pb.read(srcSpec, ResourceState::ShaderResource);
                    data.sceneColor = pb.write(outHandle, ResourceState::UnorderedAccess);
                    pb.write(dstDiff, ResourceState::UnorderedAccess);
                    pb.write(dstSpec, ResourceState::UnorderedAccess);
                    if (finalPass) {
                        pb.write(fgHistDiff, ResourceState::UnorderedAccess);
                        pb.write(fgHistSpec, ResourceState::UnorderedAccess);
                    }
                    data.device = device;
                    data.state = &state;
                    data.cbData = blurCB;
                    data.width = width;
                    data.height = height;
                    data.srcIsTemp = srcIsTemp;
                    data.dstIsTemp = dstIsTemp;
                    data.finalPass = finalPass;
                },
                [](const BlurPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                    if (!data.state || !data.state->blurPipeline)
                        return;
                    auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                    auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                    auto* sceneInTex = fgGraph.GetPhysicalTexture(data.sceneColorIn);
                    auto* classifyTex = fgGraph.GetPhysicalTexture(data.classifyWorldPos);
                    auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
                    auto* outTex = fgGraph.GetPhysicalTexture(data.sceneColor);
                    nvrhi::ITexture* directLit = data.state->directLighting.Get();
                    nvrhi::ITexture* noisyDiff = data.state->noisyDiffuse.Get();
                    nvrhi::ITexture* blurTemp = data.state->blurTemp.Get();
                    nvrhi::ITexture* noisySpec = data.state->noisySpecular.Get();
                    nvrhi::ITexture* blurTempSpec = data.state->blurTempSpec.Get();
                    if (!depthTex || !normalTex || !sceneInTex || !classifyTex || !worldPosTex || !outTex || !directLit ||
                        !noisyDiff || !blurTemp || !noisySpec || !blurTempSpec)
                        return;

                    nvrhi::ITexture* srcD = data.srcIsTemp ? blurTemp : noisyDiff;
                    nvrhi::ITexture* dstD = data.finalPass ? noisyDiff : (data.dstIsTemp ? blurTemp : noisyDiff);
                    nvrhi::ITexture* srcS = data.srcIsTemp ? blurTempSpec : noisySpec;
                    nvrhi::ITexture* dstS = data.finalPass ? noisySpec : (data.dstIsTemp ? blurTempSpec : noisySpec);

                    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                    cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(BlurCB));

                    auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_gi_blur", ".cs");
                    if (!csReflection)
                        return;

                    framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Blur");
                    bsb.ConstantBuffer("BlurParams", data.state->cb);
                    bsb.Texture("t_DirectLighting", directLit);
                    bsb.Texture("t_NoisyDiffuse", srcD);
                    bsb.Texture("t_Depth", depthTex);
                    bsb.Texture("t_Normal", normalTex);
                    bsb.Texture("t_SceneColorIn", sceneInTex);
                    bsb.Texture("t_NoisySpecular", srcS);
                    bsb.Texture("t_ClassifyWorldPos", classifyTex);
                    bsb.Texture("t_WorldPos", worldPosTex);
                    bsb.TextureUAV("u_SceneColor", outTex);
                    bsb.TextureUAV("u_FilteredDiffuse", dstD);
                    bsb.TextureUAV("u_FilteredSpecular", dstS);
                    auto& cache = GetPassResourceCache();
                    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->blurLayout, nvDevice);
                    if (!bindingSet)
                        return;

                    nvrhi::ComputeState cs;
                    cs.pipeline = data.state->blurPipeline;
                    cs.bindings = { bindingSet };
                    cmdList->setComputeState(cs);
                    cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);

                    if (data.finalPass && data.state->histDiffuse && data.state->histSpecular) {
                        cmdList->copyTexture(data.state->histDiffuse, nvrhi::TextureSlice(), dstD, nvrhi::TextureSlice());
                        cmdList->copyTexture(data.state->histSpecular, nvrhi::TextureSlice(), dstS, nvrhi::TextureSlice());
                    }
                }
            );
            finalColor = blurData.sceneColor;
            if (!finalPass)
                srcIsTemp = !srcIsTemp;
        }
    }

    if (state.waterPipeline && state.waterLayout && accelMgr) {
        WaterCB waterCB{};
        waterCB.invViewProj = invViewProj;
        waterCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        waterCB.sunDir_intensity = initialCB.sunDir_intensity;
        waterCB.sunColor_skyWeight = initialCB.sunColor_skyWeight;
        waterCB.screenWidth = (float)width;
        waterCB.screenHeight = (float)height;
        waterCB.giIntensity = giIntensity;
        waterCB.identityStaticCount = initialCB.identityStaticCount;
        waterCB.terrainBatchCount = initialCB.terrainBatchCount;
        waterCB.skinnedBatchStart = initialCB.skinnedBatchStart;
        waterCB.grassBatchStart = initialCB.grassBatchStart;
        waterCB.detailAtlasIndex = initialCB.detailAtlasIndex;
        waterCB.hemiColor = initialCB.hemiColor;

        ResourceDesc waterOutDesc = outDesc;
        waterOutDesc.debugName = "rtgi_WaterSceneColor";
        VirtualResourceHandle waterOut = fg.CreateTexture("rtgi_WaterSceneColor", waterOutDesc);

        struct WaterPassData {
            VirtualResourceHandle depth, normal, baseColor, worldPos, classifyWorldPos, sceneIn, sceneOut;
            fg::RenderDevice* device = nullptr;
            RTAccelStructManager* accelMgr = nullptr;
            ReSTIRGIPassState* state = nullptr;
            WaterCB cbData{};
            nvrhi::ITexture* sky0 = nullptr;
            nvrhi::ITexture* sky1 = nullptr;
            nvrhi::ITexture* underWorldPos = nullptr;
            u32 width = 0, height = 0;
        };

        auto& waterData = fg.addCallbackPass<WaterPassData>(
            "ReSTIR Water RT",
            [&, waterCB, finalColor, waterOut, sky0Tex, sky1Tex, classifyWP](FrameGraph& builder, PassHandle passHandle, WaterPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                data.classifyWorldPos = pb.read(classifyWP, ResourceState::ShaderResource);
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
                data.width = width;
                data.height = height;
            },
            [](const WaterPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                if (!data.state || !data.state->waterPipeline || !data.accelMgr)
                    return;
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
                auto* classifyTex = fgGraph.GetPhysicalTexture(data.classifyWorldPos);
                auto* sceneIn = fgGraph.GetPhysicalTexture(data.sceneIn);
                auto* sceneOut = fgGraph.GetPhysicalTexture(data.sceneOut);
                auto* tlas = data.accelMgr->GetTLAS();
                auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
                auto* megaVB = data.accelMgr->GetMegaVB();
                auto* megaIB = data.accelMgr->GetMegaIB();
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !classifyTex || !sceneIn || !sceneOut ||
                    !tlas || !batchInfo || !megaVB || !megaIB)
                    return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(WaterCB));

                auto* csReflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_water_rt", ".cs");
                if (!csReflection)
                    return;

                nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
                nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
                nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
                nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
                if (!skinnedVB) skinnedVB = s_rtgiPlaceholderBuffer.Get();
                if (!skinnedIB) skinnedIB = s_rtgiPlaceholderBuffer.Get();
                if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
                if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

                auto& cache = GetPassResourceCache();
                nvrhi::ITexture* sky0 = data.sky0 ? data.sky0 : s_rtgiPlaceholderCube.Get();
                nvrhi::ITexture* sky1 = data.sky1 ? data.sky1 : s_rtgiPlaceholderCube.Get();
                nvrhi::ITexture* underWP = data.underWorldPos ? data.underWorldPos : cache.GetDummyContactHistory(nvDevice);

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Water");
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
                bsb.Texture("t_ClassifyWorldPos", classifyTex);
                bsb.Texture("t_SceneColorIn", sceneIn);
                bsb.Texture("t_UnderWorldPos", underWP);
                bsb.TextureUAV("u_SceneColor", sceneOut);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->waterLayout, nvDevice);
                if (!bindingSet)
                    return;

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

    return { finalColor, fgNoisyDiff, fgNoisySpec, fgHitDist };
}

VirtualResourceHandle setupRTVolumetricPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle worldPos,
    const Fmatrix& invViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height,
    ReSTIRGIPassState& state)
{
    if (!device || !accelMgr || !sceneColor.is_valid())
        return sceneColor;
    InitializeResources(device, state);
    EnsureVolumetricPipeline(device, state);
    if (!state.volPipeline || !state.volLayout || !state.cb || !accelMgr->GetTLAS())
        return sceneColor;
    if (!g_pGamePersistent || !g_pGameLevel)
        return sceneColor;
    if (Device.dwPrecacheFrame || g_pGamePersistent->IsLoadingScreenShown())
        return sceneColor;

    float shaftI = ResolveSunShaftsIntensity();
    if (::ps_r_rt_gi)
        shaftI *= 1.15f;
    if (shaftI < 1e-4f)
        return sceneColor;

    const auto& env = g_pGamePersistent->Environment().CurrentEnv;
    const float fogDensity01 = std::clamp(env.fog_density, 0.f, 1.f);
    const float weatherShaftI = env.m_fSunShaftsIntensity;

    Fvector3 sc = { env.sun_color.x, env.sun_color.y, env.sun_color.z };
    float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    if (sunIntensity < 1e-4f)
        return sceneColor;
    Fvector sunDir = env.sun_dir;

    u32 steps = (::ps_r_rt_vol_steps > 0) ? (u32)::ps_r_rt_vol_steps : 0u;
    if (steps == 0)
    {
        if (ps_r_sun_shafts >= 3)
            steps = 40;
        else if (ps_r_sun_shafts == 2)
            steps = 28;
        else
            steps = 16;
    }

    const float fogFar = std::max(env.fog_distance, 1400.f);

    Fvector camDir = Device.vCameraDirection;
    float sunSat = -camDir.dotproduct(sunDir);
    sunSat = 0.5f * sunSat + 0.5f;
    sunSat = 0.80f * sunSat + 0.20f;

    {
        static float s_lastLogged = -1.f;
        if (fabsf(shaftI - s_lastLogged) > 0.01f)
        {
            s_lastLogged = shaftI;
            Msg("* [RT SunShafts] weather=%.3f final=%.3f steps=%u sunI=%.3f sat=%.2f",
                weatherShaftI, shaftI, steps, sunIntensity, sunSat);
        }
    }

    RTVolCB volCB{};
    volCB.invViewProj = invViewProj;
    volCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    volCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    volCB.sunColor_fog = { sc.x, sc.y, sc.z, sunSat };
    volCB.screenWidth = (float)width;
    volCB.screenHeight = (float)height;
    volCB.fogDensity = 0.f;
    volCB.fogHeight = Device.vCameraPosition.y - 35.f;
    volCB.steps = steps;
    volCB.fogFar = fogFar;
    volCB.heightFalloff = 0.0007f + fogDensity01 * 0.0025f;
    volCB.shaftIntensity = shaftI;
    {
        const auto& bc = accelMgr->GetBatchCounts();
        volCB.grassBatchStart = bc.grass > 0
            ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
            : 0xFFFFFFFFu;
        volCB.detailAtlasIndex = accelMgr->GetDetailAtlasIndex();
        volCB.pad0 = volCB.pad1 = 0;
    }

    if (state.volAllocFailed)
        return sceneColor;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!state.volSceneColor || state.volTexWidth != width || state.volTexHeight != height) {
        nvrhi::TextureDesc desc;
        desc.debugName = "rtgi_VolSceneColor";
        desc.width = width;
        desc.height = height;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.volSceneColor = nvDevice->createTexture(desc);
        state.volTexWidth = width;
        state.volTexHeight = height;
        if (!state.volSceneColor) {
            state.volAllocFailed = true;
            Msg("! [RT SunShafts] VolSceneColor alloc failed (%ux%u) — RT shafts disabled", width, height);
            return sceneColor;
        }
    }

    ResourceDesc volOutDesc;
    volOutDesc.type = ResourceDesc::Type::Texture2D;
    volOutDesc.width = width;
    volOutDesc.height = height;
    volOutDesc.format = nvrhi::Format::RGBA16_FLOAT;
    volOutDesc.isUAV = true;
    volOutDesc.allowUAV = true;
    volOutDesc.isRenderTarget = false;
    volOutDesc.isImported = true;
    volOutDesc.debugName = "rtgi_VolSceneColor";
    VirtualResourceHandle volOut = fg.ImportTexture("rtgi_VolSceneColor", state.volSceneColor.Get(), volOutDesc);

    struct VolPassData {
        VirtualResourceHandle depth, worldPos, sceneIn, sceneOut;
        fg::RenderDevice* device = nullptr;
        RTAccelStructManager* accelMgr = nullptr;
        ReSTIRGIPassState* state = nullptr;
        RTVolCB cbData{};
        u32 width = 0, height = 0;
    };

    auto& volData = fg.addCallbackPass<VolPassData>(
        "RT SunShafts",
        [&, volCB, sceneColor, volOut, depth, worldPos](FrameGraph& builder, PassHandle passHandle, VolPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.sceneIn = pb.read(sceneColor, ResourceState::ShaderResource);
            data.sceneOut = pb.write(volOut, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.accelMgr = accelMgr;
            data.state = &state;
            data.cbData = volCB;
            data.width = width;
            data.height = height;
        },
        [](const VolPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* sceneIn = fgGraph.GetPhysicalTexture(data.sceneIn);
            auto* sceneOut = fgGraph.GetPhysicalTexture(data.sceneOut);
            if (!sceneIn || !sceneOut || !data.device)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto blitScene = [&]() {
                nvrhi::TextureSlice slice;
                cmdList->copyTexture(sceneOut, slice, sceneIn, slice);
            };

            if (!data.state || !data.state->volPipeline || !data.accelMgr)
            {
                blitScene();
                return;
            }
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
            auto* tlas = data.accelMgr->GetTLAS();
            if (!depthTex || !worldPosTex || !tlas)
            {
                blitScene();
                return;
            }

            RTVolCB cb = data.cbData;
            {
                const auto& bc = data.accelMgr->GetBatchCounts();
                cb.grassBatchStart = bc.grass > 0
                    ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                    : 0xFFFFFFFFu;
                cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
            }
            cmdList->writeBuffer(data.state->cb, &cb, sizeof(RTVolCB));

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rt_volumetric", ".cs");
            if (!csReflection)
            {
                blitScene();
                return;
            }

            nvrhi::IBuffer* batchInfo = data.accelMgr->GetBatchInfoBuffer();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!batchInfo) batchInfo = s_rtgiPlaceholderBuffer.Get();
            if (!grassVB) grassVB = s_rtgiPlaceholderBuffer.Get();
            if (!grassIB) grassIB = s_rtgiPlaceholderBuffer.Get();

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Vol");
            bsb.ConstantBuffer("RTVolParams", data.state->cb);
            bsb.AccelStruct("g_SceneTLAS", tlas);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_SceneColorIn", sceneIn);
            bsb.Texture("t_WorldPos", worldPosTex);
            bsb.BufferSRV("g_BatchInfo", batchInfo);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            BindBindlessMaterialTables(bsb);
            bsb.TextureUAV("u_SceneColor", sceneOut);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->volLayout, nvDevice);
            if (!bindingSet)
            {
                blitScene();
                return;
            }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->volPipeline;
            cs.bindings = { bindingSet };
            if (auto* backend = data.device->GetBackend()) {
                if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                    cs.addBindingSet(bindlessTable);
            }
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );
    return volData.sceneOut;
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state)
{
    state.initialPipeline = nullptr;
    state.initialLayout = nullptr;
    state.temporalPipeline = nullptr;
    state.temporalLayout = nullptr;
    state.spatialPipeline = nullptr;
    state.spatialLayout = nullptr;
    state.diTemporalPipeline = nullptr;
    state.diTemporalLayout = nullptr;
    state.diSpatialPipeline = nullptr;
    state.diSpatialLayout = nullptr;
    state.diShadePipeline = nullptr;
    state.diShadeLayout = nullptr;
    state.waterPipeline = nullptr;
    state.waterLayout = nullptr;
    state.compositePipeline = nullptr;
    state.compositeLayout = nullptr;
    state.blurPipeline = nullptr;
    state.blurLayout = nullptr;
    state.temporalFilterPipeline = nullptr;
    state.temporalFilterLayout = nullptr;
    state.volPipeline = nullptr;
    state.volLayout = nullptr;
    state.cb = nullptr;
    state.sampler = nullptr;
    for (int i = 0; i < 2; i++) {
        state.reservoirA[i] = nullptr;
        state.reservoirB[i] = nullptr;
        state.reservoirC[i] = nullptr;
        state.specReservoirA[i] = nullptr;
        state.specReservoirB[i] = nullptr;
        state.diReservoir[i] = nullptr;
    }
    state.directLighting = nullptr;
    state.noisyDiffuse = nullptr;
    state.noisySpecular = nullptr;
    state.blurTemp = nullptr;
    state.blurTempSpec = nullptr;
    state.histDiffuse = nullptr;
    state.histSpecular = nullptr;
    state.hitDistance = nullptr;
    state.volSceneColor = nullptr;
    state.irradianceCache = nullptr;
    state.irradianceCacheSize = 0;
    state.volTexWidth = 0;
    state.volTexHeight = 0;
    state.volAllocFailed = false;
    s_rtgiPlaceholderBuffer = nullptr;
    s_rtgiPlaceholderCube = nullptr;
    state.initialized = false;
    state.enabled = false;
}

}
