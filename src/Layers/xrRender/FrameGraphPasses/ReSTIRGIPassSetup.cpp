#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "ShaderConstants.h"
#include "RTEnvironmentSamplingPassSetup.h"
#include "WorldCachePassSetup.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace fg
{
extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

static nvrhi::BufferHandle s_rtgiPlaceholderBuffer;

static void CreatePlaceholders(nvrhi::IDevice* nvDevice)
{
    if (!s_rtgiPlaceholderBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "RTGI_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_rtgiPlaceholderBuffer = nvDevice->createBuffer(desc);
    }
}

static float ComputeRTGICameraConeSpread(const Fmatrix& invViewProj, const Fvector& cameraPos, u32 width, u32 height)
{
    if (!width || !height)
        return 0.0f;

    auto direction = [&](float ndcX, float ndcY, Fvector& out)
    {
        const float x = ndcX * invViewProj._11 + ndcY * invViewProj._21 + invViewProj._31 + invViewProj._41;
        const float y = ndcX * invViewProj._12 + ndcY * invViewProj._22 + invViewProj._32 + invViewProj._42;
        const float z = ndcX * invViewProj._13 + ndcY * invViewProj._23 + invViewProj._33 + invViewProj._43;
        const float w = ndcX * invViewProj._14 + ndcY * invViewProj._24 + invViewProj._34 + invViewProj._44;
        if (!std::isfinite(w) || fabsf(w) < 1e-6f)
            return false;
        Fvector point;
        point.set(x / w, y / w, z / w);
        out.sub(point, cameraPos);
        const float length = out.magnitude();
        if (!std::isfinite(length) || length < 1e-6f)
            return false;
        out.div(length);
        return true;
    };

    Fvector base;
    Fvector stepX;
    Fvector stepY;
    if (!direction(0.0f, 0.0f, base) ||
        !direction(2.0f / float(width), 0.0f, stepX) ||
        !direction(0.0f, 2.0f / float(height), stepY))
        return 0.0f;

    const float angleX = acosf(std::min(1.0f, std::max(-1.0f, base.dotproduct(stepX))));
    const float angleY = acosf(std::min(1.0f, std::max(-1.0f, base.dotproduct(stepY))));
    const float spread = std::max(angleX, angleY);
    return std::isfinite(spread) ? spread : 0.0f;
}

static void InitializeResources(RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized)
        return;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!shaderLoader)
    {
        state.readiness = LightingFallback::ShaderUnavailable;
        return;
    }
    auto* backend = device->GetBackend();
    if (!backend)
    {
        state.readiness = LightingFallback::Unsupported;
        return;
    }
    if (!backend->GetBindlessLayout() || !backend->GetBindlessDescriptorTable())
    {
        state.readiness = LightingFallback::BindingUnavailable;
        return;
    }

    auto& cache = GetPassResourceCache();
    CreatePlaceholders(nvDevice);
    state.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_CB", u32(std::max(sizeof(RTGIRawCB), sizeof(RTGICompositeParams))), device);
    if (!state.cb || !s_rtgiPlaceholderBuffer)
    {
        state.readiness = LightingFallback::ResourcesUnavailable;
        return;
    }

    state.initialized = true;
    const auto createPipeline =
        [&](const char* shader, const char* name, bool rayTracing, nvrhi::BindingLayoutHandle& layout, nvrhi::ComputePipelineHandle& pipeline)
    {
        auto result = shaderLoader->LoadComputeShader(shader);
        if (!result.handle || !result.reflection)
            return LightingFallback::ShaderUnavailable;
        layout = cache.GetOrCreateBindingLayoutFromReflection(name, *result.reflection, nvDevice);
        if (!layout)
            return LightingFallback::PipelineUnavailable;
        nvrhi::ComputePipelineDesc desc;
        desc.CS = result.handle;
        desc.bindingLayouts = { layout };
        if (rayTracing)
            desc.bindingLayouts.push_back(backend->GetBindlessLayout());
        pipeline = nvDevice->createComputePipeline(desc);
        return pipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    };
    state.readiness = createPipeline("rtgi_trace", "RTGI_Trace", true, state.traceLayout, state.tracePipeline);
    if (state.readiness != LightingFallback::None)
        return;
    state.readiness = createPipeline("rtgi_composite", "RTGI_Composite", false, state.compositeLayout, state.compositePipeline);
}

static void EnsureRawTextures(nvrhi::IDevice* nvDevice, ReSTIRGIPassState& state, u32 width, u32 height)
{
    if (state.rawDiffuse &&
        state.rawSpecular &&
        state.emission &&
        state.normalRoughness &&
        state.albedoMetallic &&
        state.pathData &&
        state.surfaceData &&
        state.motion &&
        state.texWidth == width &&
        state.texHeight == height)
        return;

    const auto create = [nvDevice, width, height](const char* name, nvrhi::Format format)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = name;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return nvDevice->createTexture(desc);
    };

    state.rawDiffuse = create("RTGI_RawDiffuse", nvrhi::Format::RGBA32_FLOAT);
    state.rawSpecular = create("RTGI_RawSpecular", nvrhi::Format::RGBA32_FLOAT);
    state.emission = create("RTGI_Emission", nvrhi::Format::RGBA16_FLOAT);
    state.normalRoughness = create("RTGI_NormalRoughness", nvrhi::Format::RGBA16_FLOAT);
    state.albedoMetallic = create("RTGI_AlbedoMetallic", nvrhi::Format::RGBA16_FLOAT);
    state.pathData = create("RTGI_PathData", nvrhi::Format::RGBA32_FLOAT);
    state.surfaceData = create("RTGI_SurfaceData", nvrhi::Format::RGBA32_FLOAT);
    state.motion = create("RTGI_Motion", nvrhi::Format::RG16_FLOAT);
    state.texWidth = width;
    state.texHeight = height;
}

static void ReleaseReconstructionResources(RTGIReconstructionState& recon)
{
    for (u32 i = 0; i < 2; ++i)
    {
        recon.historyDiffuse[i] = nullptr;
        recon.historySpecular[i] = nullptr;
        recon.moments[i] = nullptr;
        recon.fast[i] = nullptr;
    }
    recon.width = 0;
    recon.height = 0;
    recon.recorded = false;
}

static LightingFallback EnsureReconstructionResources(RenderDevice* device, ReSTIRGIPassState& state, u32 width, u32 height)
{
    auto& recon = state.reconstruction;
    auto* nvDevice = device->GetNVRHIDevice();
    if (!recon.initialized)
    {
        auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
        if (!shaderLoader)
            return LightingFallback::ShaderUnavailable;
        auto& cache = GetPassResourceCache();
        recon.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_TemporalCB", sizeof(RTGITemporalCB), device);
        if (!recon.cb)
            return LightingFallback::ResourcesUnavailable;
        recon.initialized = true;
        auto result = shaderLoader->LoadComputeShader("rtgi_temporal");
        if (!result.handle || !result.reflection)
        {
            recon.readiness = LightingFallback::ShaderUnavailable;
            return recon.readiness;
        }
        recon.temporalLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Temporal", *result.reflection, nvDevice);
        if (!recon.temporalLayout)
        {
            recon.readiness = LightingFallback::PipelineUnavailable;
            return recon.readiness;
        }
        nvrhi::ComputePipelineDesc desc;
        desc.CS = result.handle;
        desc.bindingLayouts = { recon.temporalLayout };
        recon.temporalPipeline = nvDevice->createComputePipeline(desc);
        recon.readiness = recon.temporalPipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    }
    if (recon.readiness != LightingFallback::None)
        return recon.readiness;
    if (ps_r_rt_gi_reconstruct >= 2 && !recon.filterInitialized)
    {
        auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
        auto& cache = GetPassResourceCache();
        recon.filterInitialized = true;
        recon.filterCB = cache.GetOrCreateVolatileCB("RTGI", "RTGI_FilterCB", sizeof(RTGIFilterCB), device, 64);
        recon.filterReadiness = shaderLoader && recon.filterCB ? LightingFallback::None : LightingFallback::ResourcesUnavailable;
        const auto createFilterPipeline = [&](const char* shader, const char* name, nvrhi::BindingLayoutHandle& layout,
            nvrhi::ComputePipelineHandle& pipeline)
        {
            if (recon.filterReadiness != LightingFallback::None)
                return;
            auto result = shaderLoader->LoadComputeShader(shader);
            if (!result.handle || !result.reflection)
            {
                recon.filterReadiness = LightingFallback::ShaderUnavailable;
                return;
            }
            layout = cache.GetOrCreateBindingLayoutFromReflection(name, *result.reflection, nvDevice);
            if (!layout)
            {
                recon.filterReadiness = LightingFallback::PipelineUnavailable;
                return;
            }
            nvrhi::ComputePipelineDesc desc;
            desc.CS = result.handle;
            desc.bindingLayouts = { layout };
            pipeline = nvDevice->createComputePipeline(desc);
            if (!pipeline)
                recon.filterReadiness = LightingFallback::PipelineUnavailable;
        };
        createFilterPipeline("rtgi_variance", "RTGI_Variance", recon.varianceLayout, recon.variancePipeline);
        createFilterPipeline("rtgi_atrous", "RTGI_Atrous", recon.atrousLayout, recon.atrousPipeline);
    }

    bool texturesMatch = recon.width == width && recon.height == height;
    for (u32 i = 0; i < 2 && texturesMatch; ++i)
        texturesMatch = recon.historyDiffuse[i] && recon.historySpecular[i] && recon.moments[i] && recon.fast[i];
    if (texturesMatch)
        return LightingFallback::None;

    const auto create = [nvDevice, width, height](const char* name, nvrhi::Format format)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = name;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return nvDevice->createTexture(desc);
    };
    ReleaseReconstructionResources(recon);
    state.historyValid = false;
    recon.historyIndex = 0;
    bool created = true;
    for (u32 i = 0; i < 2; ++i)
    {
        recon.historyDiffuse[i] = create(i ? "RTGI_HistoryDiffuse_B" : "RTGI_HistoryDiffuse_A", nvrhi::Format::RGBA16_FLOAT);
        recon.historySpecular[i] = create(i ? "RTGI_HistorySpecular_B" : "RTGI_HistorySpecular_A", nvrhi::Format::RGBA16_FLOAT);
        recon.moments[i] = create(i ? "RTGI_HistoryMoments_B" : "RTGI_HistoryMoments_A", nvrhi::Format::RGBA16_FLOAT);
        recon.fast[i] = create(i ? "RTGI_HistoryFast_B" : "RTGI_HistoryFast_A", nvrhi::Format::RG16_FLOAT);
        created = created && recon.historyDiffuse[i] && recon.historySpecular[i] && recon.moments[i] && recon.fast[i];
    }
    if (!created)
    {
        ReleaseReconstructionResources(recon);
        return LightingFallback::ResourcesUnavailable;
    }
    recon.width = width;
    recon.height = height;
    return LightingFallback::None;
}

LightingFallback EnsureReSTIRGIResources(RenderDevice* device, ReSTIRGIPassState& state, u32 width, u32 height, bool reuseRequested)
{
    if (!device || !device->GetNVRHIDevice() || !width || !height)
        return LightingFallback::ResourcesUnavailable;
    InitializeResources(device, state);
    if (state.readiness != LightingFallback::None)
        return state.readiness;
    EnsureRawTextures(device->GetNVRHIDevice(), state, width, height);
    if (!state.rawDiffuse || !state.rawSpecular || !state.emission || !state.normalRoughness ||
        !state.albedoMetallic || !state.pathData || !state.surfaceData || !state.motion)
        return LightingFallback::ResourcesUnavailable;
    if (ps_r_rt_gi_reconstruct != 0)
        state.reconstruction.status = EnsureReconstructionResources(device, state, width, height);
    else
    {
        ReleaseReconstructionResources(state.reconstruction);
        state.reconstruction.status = LightingFallback::None;
    }
    return LightingFallback::None;
}

ReSTIRGIOutput setupReSTIRGIPass(FrameGraph& fg, fg::RenderDevice* device, RTAccelStructManager* accelMgr,
    WorldRadianceCache* worldCache, const DefaultOutputLayout& inputs,
    const ClusterLightOutput& clusterLights, const SkyEnvironmentFrame& sky, VirtualResourceHandle prevNormals, VirtualResourceHandle prevDepth,
    VirtualResourceHandle motionVectors, const Fmatrix& invViewProj, const Fmatrix& prevViewProj,
    const Fmatrix& view, const Fmatrix& prevView, const Fmatrix& project, const Fmatrix& prevProject,
    const Fvector& cameraPos, float giIntensity, u32 width,
    u32 height, ReSTIRGIPassState& state, bool hasPrevFrameData, LightingFrameState& lighting)
{
    const auto depth = inputs.depth;
    const auto normal = inputs.normal;
    const auto baseColor = inputs.baseColor;
    const auto material = inputs.material;
    const auto sourceColorIn = inputs.albedo;

    lighting.reuseRequested = lighting.reuseRequested || lighting.reuseReservoirs;
    lighting.reuseAvailable = false;
    lighting.reuseReservoirs = false;

    const auto readiness = EnsureReSTIRGIResources(device, state, width, height, lighting.reuseRequested);
    if (readiness != LightingFallback::None)
    {
        lighting.Fail(readiness);
        return { sourceColorIn };
    }
    if (!accelMgr || !accelMgr->IsReady())
    {
        lighting.Fail(LightingFallback::SceneUnavailable);
        return { sourceColorIn };
    }

    const auto validInput = [&](VirtualResourceHandle handle)
    {
        if (!handle.is_valid())
            return false;
        const auto& desc = fg.GetResourceDesc(handle);
        return desc.type == ResourceDesc::Type::Texture2D && desc.width == width && desc.height == height && desc.sampleCount == 1;
    };
    if (!validInput(sourceColorIn) || !validInput(depth) || !validInput(normal) || !validInput(baseColor) ||
        !validInput(material) || !validInput(motionVectors) || !g_pGamePersistent)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sourceColorIn };
    }
    const auto& sourceDesc = fg.GetResourceDesc(sourceColorIn);
    if ((!sourceDesc.isUAV && !sourceDesc.allowUAV) || sourceDesc.format != nvrhi::Format::RGBA16_FLOAT)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sourceColorIn };
    }

    state.initialRecorded = false;

    if (!sky.Valid())
    {
        lighting.Fail(LightingFallback::EnvironmentUnavailable);
        return { sourceColorIn };
    }

    auto& lightManager = ClusteredLightManager::Instance();
    if (!lightManager.GetLightDataBuffer() ||
        (lightManager.GetLightCount() > 0 && (!clusterLights.active || !clusterLights.lightData.is_valid())))
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sourceColorIn };
    }
    const auto importLightBuffer = [&](const char* name, nvrhi::IBuffer* buffer)
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.structStride = buffer->getDesc().structStride;
        desc.isImported = true;
        desc.isTransient = false;
        return fg.ImportBuffer(name, buffer, desc);
    };
    auto lightData = clusterLights.lightData;
    if (!lightData.is_valid())
        lightData = importLightBuffer("cluster_light_data", lightManager.GetLightDataBuffer());
    const bool clusterListsActive = clusterLights.active && clusterLights.clusterGrid.is_valid() &&
        clusterLights.lightIndexList.is_valid();
    auto clusterGrid = clusterListsActive ? clusterLights.clusterGrid : VirtualResourceHandle();
    auto lightIndexList = clusterListsActive ? clusterLights.lightIndexList : VirtualResourceHandle();
    if (!clusterGrid.is_valid())
        clusterGrid = lightManager.GetClusterGridBuffer() ?
            importLightBuffer("cluster_light_grid", lightManager.GetClusterGridBuffer()) : lightData;
    if (!lightIndexList.is_valid())
        lightIndexList = lightManager.GetLightIndexListBuffer() ?
            importLightBuffer("cluster_light_index_list", lightManager.GetLightIndexListBuffer()) : lightData;

    const auto environmentSampling = setupRTEnvironmentSamplingPass(fg, device, sky.cube);
    if (!environmentSampling.distribution.is_valid())
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sourceColorIn };
    }

    SunLightData sun = {};
    GetSunLightData(sun);
    const Fvector sunDir = sun.direction;
    const Fvector sc = sun.color;
    const float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    Fvector sunColor;
    if (sunIntensity > 0.001f)
        sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
    else
        sunColor.set(0, 0, 0);

    const auto scene = accelMgr->GetScene();
    const auto& batchCounts = scene->counts;
    const u32 bounces = static_cast<u32>(std::clamp(ps_r_rt_gi_bounces, 1, 16));
    const u32 samples = static_cast<u32>(std::clamp(ps_r_rt_gi_samples, 1, 8));
    const float rayDistance = std::clamp(ps_r_rt_gi_ray_distance, 1.0f, 10000.0f);

    RTGIRawCB rawCB = {};
    rawCB.invViewProj = invViewProj;
    rawCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    rawCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    rawCB.sunColor = { sunColor.x, sunColor.y, sunColor.z, 0.0f };
    rawCB.screenWidth = (float)width;
    rawCB.screenHeight = (float)height;
    rawCB.giIntensity = giIntensity;
    rawCB.frameIndex = Device.dwFrame;
    rawCB.identityStaticCount = batchCounts.identityStatic;
    rawCB.terrainBatchCount = batchCounts.terrain;
    rawCB.skinnedBatchStart =
        batchCounts.skinned > 0 ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal : UINT32_MAX;
    rawCB.grassBatchStart = batchCounts.grass > 0 ?
        batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned :
        UINT32_MAX;
    rawCB.detailAtlasIndex = scene->detailAtlasIndex;
    rawCB.diffuseMode = static_cast<u32>(ps_fg_pbr_diffuse_mode);
    rawCB.lightCount = lightManager.GetLightCount();
    rawCB.emissiveCount = scene->emissiveCount;
    rawCB.maxNullEvents = static_cast<u32>(ps_r_rt_max_null_events);
    rawCB.maxBounces = bounces;
    rawCB.samplesPerPixel = samples;
    rawCB.rayDistance = rayDistance;
    rawCB.sunAngularRadius = deg2rad(ps_r_rt_sun_radius);
    rawCB.cameraConeSpread = ComputeRTGICameraConeSpread(invViewProj, cameraPos, width, height);
    rawCB.clusterLights = clusterListsActive ? 1u : 0u;
    rawCB.detailMeshBatchStart = scene->detailMeshBatchStart;
    rawCB.staticDetailBatchStart = scene->staticDetailBatchStart;
    rawCB.detailPbrIndex = scene->detailPbrIndex;
    rawCB.detailBumpIndex = scene->detailBumpIndex;
    rawCB.lightRays = static_cast<u32>(std::clamp(ps_r_rt_light_rays, 0, 2));

    WorldCachePassInputs cacheInputs;
    cacheInputs.accelMgr = accelMgr;
    cacheInputs.cache = worldCache;
    cacheInputs.lighting = &lighting;
    cacheInputs.lightData = lightData;
    cacheInputs.clusterGrid = clusterGrid;
    cacheInputs.lightIndexList = lightIndexList;
    cacheInputs.environmentDistribution = environmentSampling.distribution;
    cacheInputs.sky = sky.cube;
    cacheInputs.sceneConstants = rawCB;
    cacheInputs.cameraPos = cameraPos;
    cacheInputs.frame = Device.dwFrame;
    const auto worldCacheOutput = setupWorldCachePass(fg, device, cacheInputs);
    if (!worldCache || !worldCache->IsAllocated() || !worldCacheOutput.constantBuffer)
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sourceColorIn };
    }
    const u64 worldCacheSignature = worldCacheOutput.scheduled ?
        (u64(worldCache->GetEpoch()) << 8) | (worldCacheOutput.constants.flags & 0xFFu) : 0;
    const bool worldCacheHistoryCompatible = state.worldCacheSignature == worldCacheSignature;
    state.worldCacheSignature = worldCacheSignature;

    ResourceDesc rawDesc;
    rawDesc.type = ResourceDesc::Type::Texture2D;
    rawDesc.width = width;
    rawDesc.height = height;
    rawDesc.isImported = true;
    rawDesc.isTransient = false;
    rawDesc.isUAV = true;

    const auto importRaw = [&](const char* name, nvrhi::ITexture* texture, nvrhi::Format format)
    {
        ResourceDesc desc = rawDesc;
        desc.format = format;
        return fg.ImportTexture(name, texture, desc);
    };

    const auto fgRawDiffuse = importRaw("rtgi_RawDiffuse", state.rawDiffuse.Get(), nvrhi::Format::RGBA32_FLOAT);
    const auto fgRawSpecular = importRaw("rtgi_RawSpecular", state.rawSpecular.Get(), nvrhi::Format::RGBA32_FLOAT);
    const auto fgEmission = importRaw("rtgi_Emission", state.emission.Get(), nvrhi::Format::RGBA16_FLOAT);
    const auto fgNormalRoughness = importRaw("rtgi_NormalRoughness", state.normalRoughness.Get(), nvrhi::Format::RGBA16_FLOAT);
    const auto fgAlbedoMetallic = importRaw("rtgi_AlbedoMetallic", state.albedoMetallic.Get(), nvrhi::Format::RGBA16_FLOAT);
    const auto fgPathData = importRaw("rtgi_PathData", state.pathData.Get(), nvrhi::Format::RGBA32_FLOAT);
    const auto fgSurfaceData = importRaw("rtgi_SurfaceData", state.surfaceData.Get(), nvrhi::Format::RGBA32_FLOAT);
    const auto fgOutMotion = importRaw("rtgi_Motion", state.motion.Get(), nvrhi::Format::RG16_FLOAT);

    if (!fgRawDiffuse.is_valid() || !fgRawSpecular.is_valid() || !fgEmission.is_valid() ||
        !fgNormalRoughness.is_valid() || !fgAlbedoMetallic.is_valid() || !fgPathData.is_valid() ||
        !fgSurfaceData.is_valid() || !fgOutMotion.is_valid())
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sourceColorIn };
    }

    fg.GetRTRegistry().RegisterRT("rt_GI_RawDiffuse", fgRawDiffuse);
    fg.GetRTRegistry().RegisterRT("rt_GI_RawSpecular", fgRawSpecular);
    fg.GetRTRegistry().RegisterRT("rt_GI_Emission", fgEmission);
    fg.GetRTRegistry().RegisterRT("rt_GI_NormalRoughness", fgNormalRoughness);
    fg.GetRTRegistry().RegisterRT("rt_GI_AlbedoMetallic", fgAlbedoMetallic);
    fg.GetRTRegistry().RegisterRT("rt_GI_PathData", fgPathData);
    fg.GetRTRegistry().RegisterRT("rt_GI_SurfaceData", fgSurfaceData);
    fg.GetRTRegistry().RegisterRT("rt_GI_Motion", fgOutMotion);

    fg.addCallbackPass<RTGITracePassData>(
        "RTGI Raw Transport",
        [&](FrameGraph& builder, PassHandle passHandle, RTGITracePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.material = pb.read(material, ResourceState::ShaderResource);
            data.sourceColor = pb.read(sourceColorIn, ResourceState::ShaderResource);
            data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
            data.lightData = pb.read(lightData, ResourceState::ShaderResource);
            data.clusterGrid = pb.read(clusterGrid, ResourceState::ShaderResource);
            data.lightIndexList = pb.read(lightIndexList, ResourceState::ShaderResource);
            data.environmentDistribution = pb.read(environmentSampling.distribution, ResourceState::ShaderResource);
            data.sky = pb.read(sky.cube, ResourceState::ShaderResource);
            data.rawDiffuse = pb.write(fgRawDiffuse, ResourceState::UnorderedAccess);
            data.rawSpecular = pb.write(fgRawSpecular, ResourceState::UnorderedAccess);
            data.emission = pb.write(fgEmission, ResourceState::UnorderedAccess);
            data.normalRoughness = pb.write(fgNormalRoughness, ResourceState::UnorderedAccess);
            data.albedoMetallic = pb.write(fgAlbedoMetallic, ResourceState::UnorderedAccess);
            data.pathData = pb.write(fgPathData, ResourceState::UnorderedAccess);
            data.surfaceData = pb.write(fgSurfaceData, ResourceState::UnorderedAccess);
            data.outMotion = pb.write(fgOutMotion, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.scene = accelMgr->UseScene(builder, pb);
            data.worldCache = worldCache->Use(builder, pb, WorldRadianceCacheAccess::Query);
            data.worldCacheOwner = worldCache;
            data.worldCacheConstants = worldCacheOutput.constants;
            data.worldCacheConstantBuffer = worldCacheOutput.constantBuffer;
            data.state = &state;
            data.lighting = &lighting;
            data.cbData = rawCB;
            data.width = width;
            data.height = height;
        },
        [](const RTGITracePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (data.lighting->effective != LightingMode::RTGI)
                return;

            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* materialTex = fg.GetPhysicalTexture(data.material);
            auto* sourceColorTex = fg.GetPhysicalTexture(data.sourceColor);
            auto* motionTex = fg.GetPhysicalTexture(data.motionVectors);
            auto* rawDiffuse = fg.GetPhysicalTexture(data.rawDiffuse);
            auto* rawSpecular = fg.GetPhysicalTexture(data.rawSpecular);
            auto* emission = fg.GetPhysicalTexture(data.emission);
            auto* normalRoughness = fg.GetPhysicalTexture(data.normalRoughness);
            auto* albedoMetallic = fg.GetPhysicalTexture(data.albedoMetallic);
            auto* pathData = fg.GetPhysicalTexture(data.pathData);
            auto* surfaceData = fg.GetPhysicalTexture(data.surfaceData);
            auto* outMotion = fg.GetPhysicalTexture(data.outMotion);
            if (!depthTex || !normalTex || !baseColorTex || !materialTex || !sourceColorTex || !motionTex ||
                !rawDiffuse || !rawSpecular || !emission || !normalRoughness || !albedoMetallic ||
                !pathData || !surfaceData || !outMotion)
            {
                Msg("! [RTGI Trace] Null FG texture: depth=%d normal=%d base=%d material=%d source=%d motion=%d raw=%d",
                    !!depthTex, !!normalTex, !!baseColorTex, !!materialTex, !!sourceColorTex, !!motionTex, !!rawDiffuse);
                data.lighting->Fail(LightingFallback::InputsUnavailable);
                return;
            }

            nvrhi::ITexture* skyTexture = fg.GetPhysicalTexture(data.sky);
            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            auto* tlas = scene.tlas;
            auto* batchInfo = scene.batchInfo;
            auto* megaVB = scene.vertices;
            auto* megaIB = scene.indices;
            auto* matBuf = scene.materials;
            auto* terrainBuf = scene.terrainMaterials;

            auto* environmentDistribution = fg.GetPhysicalBuffer(data.environmentDistribution);
            if (!skyTexture || !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf ||
                !scene.grassMaterials || !scene.variants || !scene.textures || !scene.emissiveTriangles || !scene.batchTransforms ||
                !scene.emissiveBatchOffsets || !environmentDistribution)
            {
                Msg("! [RTGI Trace] Null binding: sky=%d tlas=%d batch=%d megaVB=%d megaIB=%d mat=%d terrain=%d",
                    !!skyTexture, !!tlas, !!batchInfo, !!megaVB, !!megaIB, !!matBuf, !!terrainBuf);
                data.lighting->Fail(LightingFallback::SceneUnavailable);
                return;
            }

            auto* lightData = fg.GetPhysicalBuffer(data.lightData);
            auto* clusterGridBuffer = fg.GetPhysicalBuffer(data.clusterGrid);
            auto* lightIndexListBuffer = fg.GetPhysicalBuffer(data.lightIndexList);
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            if (!lightData || !staticGlobals)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();

            nvrhi::IBuffer* skinnedVB = scene.skinnedVertices;
            nvrhi::IBuffer* skinnedIB = scene.skinnedIndices;
            nvrhi::IBuffer* grassVB = scene.grassVertices;
            nvrhi::IBuffer* grassIB = scene.grassIndices;
            if (!skinnedVB)
                skinnedVB = s_rtgiPlaceholderBuffer.Get();
            if (!skinnedIB)
                skinnedIB = s_rtgiPlaceholderBuffer.Get();
            if (!grassVB)
                grassVB = s_rtgiPlaceholderBuffer.Get();
            if (!grassIB)
                grassIB = s_rtgiPlaceholderBuffer.Get();

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(RTGIRawCB));
            const auto worldCacheBuffers = WorldRadianceCache::Resolve(fg, data.worldCache);
            if (!worldCacheBuffers.Valid() || !data.worldCacheConstantBuffer || !data.worldCacheOwner)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }
            WorldRadianceCacheCB worldCacheConstants = data.worldCacheConstants;
            if (!data.worldCacheOwner->IsContentReady())
                worldCacheConstants.flags &= ~WorldRadianceCache::kFlagEnabled;
            else if ((worldCacheConstants.flags & WorldRadianceCache::kFlagEnabled) != 0)
                data.worldCacheOwner->TrackRecordedWork();
            const bool worldCacheActive = (worldCacheConstants.flags & WorldRadianceCache::kFlagEnabled) != 0;
            cmdList->writeBuffer(data.worldCacheConstantBuffer, &worldCacheConstants, sizeof(WorldRadianceCacheCB));

            auto* reflection = shaderLoader->GetCachedReflection("rtgi_trace", ".cs");
            if (!reflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }
            framegraph::BindingSetBuilder bsb(*reflection, nvDevice, "rtgi_trace");
            bsb.ConstantBuffer("RTGIRawParams", data.state->cb);
            bsb.ConstantBuffer("static_globals", staticGlobals);
            bsb.BufferSRV("g_LightData", lightData);
            bsb.BufferSRV("g_ClusterGrid", clusterGridBuffer ? clusterGridBuffer : lightData);
            bsb.BufferSRV("g_LightIndexList", lightIndexListBuffer ? lightIndexListBuffer : lightData);
            bsb.AccelStruct("g_SceneTLAS", tlas);
            bsb.BufferSRV("g_BatchInfo", batchInfo);
            bsb.BufferSRV("g_MegaVB", megaVB);
            bsb.BufferSRV("g_MegaIB", megaIB);
            bsb.Texture("g_Sky", skyTexture);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", matBuf);
            bsb.BufferSRV("g_GrassMaterials", scene.grassMaterials);
            bsb.BufferSRV("g_TerrainMaterials", terrainBuf);
            bsb.BufferSRV("g_Variants", scene.variants);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.BufferSRV("g_EmissiveTriangles", scene.emissiveTriangles);
            bsb.BufferSRV("g_RTBatchTransforms", scene.batchTransforms);
            bsb.BufferSRV("g_EmissiveBatchOffsets", scene.emissiveBatchOffsets);
            bsb.BufferSRV("g_EnvironmentCDF", environmentDistribution);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.Texture("t_Material", materialTex);
            bsb.Texture("t_SourceColor", sourceColorTex);
            bsb.Texture("t_MotionVectors", motionTex);
            bsb.TextureUAV("u_RawDiffuse", rawDiffuse);
            bsb.TextureUAV("u_RawSpecular", rawSpecular);
            bsb.TextureUAV("u_Emission", emission);
            bsb.TextureUAV("u_NormalRoughness", normalRoughness);
            bsb.TextureUAV("u_AlbedoMetallic", albedoMetallic);
            bsb.TextureUAV("u_PathData", pathData);
            bsb.TextureUAV("u_SurfaceData", surfaceData);
            bsb.TextureUAV("u_Motion", outMotion);
            BindWorldCacheResources(bsb, *reflection, worldCacheBuffers, data.worldCacheConstantBuffer);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->traceLayout, nvDevice);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }
            nvrhi::ComputeState cs;
            cs.pipeline = data.state->tracePipeline;
            cs.bindings = { bindingSet };
            cs.addBindingSet(scene.textures);
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);

            data.state->initialRecorded = true;
            data.lighting->worldCacheRecorded = worldCacheActive;
            if (worldCacheActive)
                data.worldCacheOwner->ScheduleStatsReadback(cmdList, worldCacheBuffers.stats, true);
        });

    auto& recon = state.reconstruction;
    recon.recorded = false;
    const bool reconstructionRequested = ps_r_rt_gi_reconstruct != 0;
    const u32 maxHistory = static_cast<u32>(std::clamp(ps_r_rt_gi_history, 1, 64));
    bool reconstructionReady = reconstructionRequested && recon.status == LightingFallback::None &&
        recon.readiness == LightingFallback::None && recon.temporalPipeline && recon.temporalLayout && recon.cb &&
        recon.width == width && recon.height == height;
    lighting.reconstructionRequested = reconstructionRequested;
    lighting.reconstructionHistory = maxHistory;
    lighting.reconstructionFallback = LightingFallback::None;
    if (reconstructionRequested && !reconstructionReady)
        lighting.reconstructionFallback = recon.status != LightingFallback::None ? recon.status : LightingFallback::ResourcesUnavailable;

    VirtualResourceHandle fgHistoryDiffuse;
    VirtualResourceHandle fgHistorySpecular;
    VirtualResourceHandle fgMoments;
    VirtualResourceHandle fgFast;
    VirtualResourceHandle fgReconstruction;
    VirtualResourceHandle fgPrevHistoryDiffuse;
    VirtualResourceHandle fgPrevHistorySpecular;
    VirtualResourceHandle fgPrevMoments;
    VirtualResourceHandle fgPrevFast;
    if (reconstructionReady)
    {
        const u32 writeIndex = recon.historyIndex & 1u;
        const u32 readIndex = writeIndex ^ 1u;
        fgHistoryDiffuse = importRaw("rtgi_HistoryDiffuse", recon.historyDiffuse[writeIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgHistorySpecular = importRaw("rtgi_HistorySpecular", recon.historySpecular[writeIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgMoments = importRaw("rtgi_HistoryMoments", recon.moments[writeIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgFast = importRaw("rtgi_HistoryFast", recon.fast[writeIndex].Get(), nvrhi::Format::RG16_FLOAT);
        fgPrevHistoryDiffuse = importRaw("rtgi_PrevHistoryDiffuse", recon.historyDiffuse[readIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgPrevHistorySpecular = importRaw("rtgi_PrevHistorySpecular", recon.historySpecular[readIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgPrevMoments = importRaw("rtgi_PrevHistoryMoments", recon.moments[readIndex].Get(), nvrhi::Format::RGBA16_FLOAT);
        fgPrevFast = importRaw("rtgi_PrevHistoryFast", recon.fast[readIndex].Get(), nvrhi::Format::RG16_FLOAT);
        ResourceDesc reconDesc;
        reconDesc.type = ResourceDesc::Type::Texture2D;
        reconDesc.debugName = "rt_GI_Reconstruction";
        reconDesc.width = width;
        reconDesc.height = height;
        reconDesc.format = nvrhi::Format::RGBA16_FLOAT;
        reconDesc.isUAV = true;
        reconDesc.isTransient = true;
        fgReconstruction = fg.CreateTexture("rt_GI_Reconstruction", reconDesc);
        if (!fgHistoryDiffuse.is_valid() || !fgHistorySpecular.is_valid() || !fgMoments.is_valid() || !fgFast.is_valid() ||
            !fgPrevHistoryDiffuse.is_valid() || !fgPrevHistorySpecular.is_valid() || !fgPrevMoments.is_valid() ||
            !fgPrevFast.is_valid() || !fgReconstruction.is_valid())
        {
            reconstructionReady = false;
            lighting.reconstructionFallback = LightingFallback::ResourcesUnavailable;
        }
        else
        {
            fg.GetRTRegistry().RegisterRT("rt_GI_HistoryDiffuse", fgHistoryDiffuse);
            fg.GetRTRegistry().RegisterRT("rt_GI_HistorySpecular", fgHistorySpecular);
            fg.GetRTRegistry().RegisterRT("rt_GI_Moments", fgMoments);
            fg.GetRTRegistry().RegisterRT("rt_GI_Reconstruction", fgReconstruction);
        }
    }

    if (reconstructionReady)
    {
        const bool historyValid = state.historyValid && worldCacheHistoryCompatible && hasPrevFrameData &&
            validInput(prevDepth) && validInput(prevNormals);
        RTGITemporalCB temporalCB = {};
        temporalCB.invViewProj = invViewProj;
        temporalCB.prevInvViewProj = Fidentity;
        temporalCB.view = view;
        temporalCB.prevView = Fidentity;
        temporalCB.invProj = Fidentity;
        temporalCB.invProj.invert_44(project);
        temporalCB.prevInvProj = Fidentity;
        if (historyValid)
        {
            temporalCB.prevInvViewProj.invert_44(prevViewProj);
            temporalCB.prevView = prevView;
            temporalCB.prevInvProj.invert_44(prevProject);
        }
        temporalCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        temporalCB.screenWidth = static_cast<float>(width);
        temporalCB.screenHeight = static_cast<float>(height);
        temporalCB.invScreenWidth = 1.0f / static_cast<float>(width);
        temporalCB.invScreenHeight = 1.0f / static_cast<float>(height);
        temporalCB.historyValid = historyValid ? 1u : 0u;
        temporalCB.maxHistory = maxHistory;
        temporalCB.planeTolerance = 0.02f;
        temporalCB.normalTolerance = 0.9f;
        temporalCB.frameIndex = Device.dwFrame;
        temporalCB.hudFov = psHUD_FOV > 0.0f ? psHUD_FOV : 1.0f;
        const auto prevDepthInput = historyValid ? prevDepth : depth;
        const auto prevNormalInput = historyValid ? prevNormals : normal;

        fg.addCallbackPass<RTGITemporalPassData>(
            "RTGI Temporal Reconstruction",
            [&](FrameGraph& builder, PassHandle passHandle, RTGITemporalPassData& data)
            {
                RenderPassBuilder pb(builder, passHandle);
                data.rawDiffuse = pb.read(fgRawDiffuse, ResourceState::ShaderResource);
                data.rawSpecular = pb.read(fgRawSpecular, ResourceState::ShaderResource);
                data.normalRoughness = pb.read(fgNormalRoughness, ResourceState::ShaderResource);
                data.albedoMetallic = pb.read(fgAlbedoMetallic, ResourceState::ShaderResource);
                data.surfaceData = pb.read(fgSurfaceData, ResourceState::ShaderResource);
                data.motion = pb.read(fgOutMotion, ResourceState::ShaderResource);
                data.prevDepth = pb.read(prevDepthInput, ResourceState::ShaderResource);
                data.prevNormal = pb.read(prevNormalInput, ResourceState::ShaderResource);
                data.prevHistoryDiffuse = pb.read(fgPrevHistoryDiffuse, ResourceState::ShaderResource);
                data.prevHistorySpecular = pb.read(fgPrevHistorySpecular, ResourceState::ShaderResource);
                data.prevMoments = pb.read(fgPrevMoments, ResourceState::ShaderResource);
                data.prevFast = pb.read(fgPrevFast, ResourceState::ShaderResource);
                data.historyDiffuse = pb.write(fgHistoryDiffuse, ResourceState::UnorderedAccess);
                data.historySpecular = pb.write(fgHistorySpecular, ResourceState::UnorderedAccess);
                data.moments = pb.write(fgMoments, ResourceState::UnorderedAccess);
                data.fast = pb.write(fgFast, ResourceState::UnorderedAccess);
                data.reconstruction = pb.write(fgReconstruction, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.lighting = &lighting;
                data.cbData = temporalCB;
                data.width = width;
                data.height = height;
            },
            [](const RTGITemporalPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
            {
                if (!data.state->initialRecorded || data.lighting->effective != LightingMode::RTGI)
                    return;

                auto* rawDiffuse = fg.GetPhysicalTexture(data.rawDiffuse);
                auto* rawSpecular = fg.GetPhysicalTexture(data.rawSpecular);
                auto* normalRoughness = fg.GetPhysicalTexture(data.normalRoughness);
                auto* albedoMetallic = fg.GetPhysicalTexture(data.albedoMetallic);
                auto* surfaceData = fg.GetPhysicalTexture(data.surfaceData);
                auto* motion = fg.GetPhysicalTexture(data.motion);
                auto* prevDepth = fg.GetPhysicalTexture(data.prevDepth);
                auto* prevNormal = fg.GetPhysicalTexture(data.prevNormal);
                auto* prevHistoryDiffuse = fg.GetPhysicalTexture(data.prevHistoryDiffuse);
                auto* prevHistorySpecular = fg.GetPhysicalTexture(data.prevHistorySpecular);
                auto* prevMoments = fg.GetPhysicalTexture(data.prevMoments);
                auto* prevFast = fg.GetPhysicalTexture(data.prevFast);
                auto* historyDiffuse = fg.GetPhysicalTexture(data.historyDiffuse);
                auto* historySpecular = fg.GetPhysicalTexture(data.historySpecular);
                auto* moments = fg.GetPhysicalTexture(data.moments);
                auto* fast = fg.GetPhysicalTexture(data.fast);
                auto* reconstruction = fg.GetPhysicalTexture(data.reconstruction);
                if (!rawDiffuse || !rawSpecular || !normalRoughness || !albedoMetallic || !surfaceData || !motion ||
                    !prevDepth || !prevNormal || !prevHistoryDiffuse || !prevHistorySpecular || !prevMoments || !prevFast ||
                    !historyDiffuse || !historySpecular || !moments || !fast || !reconstruction)
                {
                    Msg("! [RTGI Temporal] Null FG texture: raw=%d guides=%d prev=%d history=%d",
                        !!rawDiffuse && !!rawSpecular, !!normalRoughness && !!albedoMetallic && !!surfaceData && !!motion,
                        !!prevDepth && !!prevNormal && !!prevHistoryDiffuse && !!prevHistorySpecular && !!prevMoments && !!prevFast,
                        !!historyDiffuse && !!historySpecular && !!moments && !!fast && !!reconstruction);
                    data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                    return;
                }

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* reflection = shaderLoader->GetCachedReflection("rtgi_temporal", ".cs");
                if (!reflection)
                {
                    data.lighting->Fail(LightingFallback::ShaderUnavailable);
                    return;
                }

                auto& recon = data.state->reconstruction;
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                cmdList->writeBuffer(recon.cb, &data.cbData, sizeof(RTGITemporalCB));

                framegraph::BindingSetBuilder bsb(*reflection, nvDevice, "RTGI.Temporal");
                bsb.ConstantBuffer("RTGITemporalParams", recon.cb);
                bsb.Texture("t_RawDiffuse", rawDiffuse);
                bsb.Texture("t_RawSpecular", rawSpecular);
                bsb.Texture("t_NormalRoughness", normalRoughness);
                bsb.Texture("t_AlbedoMetallic", albedoMetallic);
                bsb.Texture("t_SurfaceData", surfaceData);
                bsb.Texture("t_Motion", motion);
                bsb.Texture("t_PrevDepth", prevDepth);
                bsb.Texture("t_PrevNormal", prevNormal);
                bsb.Texture("t_PrevHistoryDiffuse", prevHistoryDiffuse);
                bsb.Texture("t_PrevHistorySpecular", prevHistorySpecular);
                bsb.Texture("t_PrevMoments", prevMoments);
                bsb.Texture("t_PrevFast", prevFast);
                bsb.TextureUAV("u_HistoryDiffuse", historyDiffuse);
                bsb.TextureUAV("u_HistorySpecular", historySpecular);
                bsb.TextureUAV("u_Moments", moments);
                bsb.TextureUAV("u_Fast", fast);
                bsb.TextureUAV("u_Reconstruction", reconstruction);
                auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), recon.temporalLayout, nvDevice);
                if (!bindingSet)
                {
                    data.lighting->Fail(LightingFallback::BindingUnavailable);
                    return;
                }

                nvrhi::ComputeState cs;
                cs.pipeline = recon.temporalPipeline;
                cs.bindings = { bindingSet };
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                recon.recorded = true;
                recon.historyIndex ^= 1u;
                data.lighting->reconstructionActive = true;
                data.lighting->historyUsed = data.cbData.historyValid != 0;
            });
    }

    recon.varianceRecorded = false;
    recon.filterRecorded = false;
    recon.filterIterationsRecorded = 0;
    const bool spatialRequested = ps_r_rt_gi_reconstruct >= 2;
    const u32 filterPasses = static_cast<u32>(std::clamp(ps_r_rt_gi_filter_passes, 1, 6));
    bool spatialReady = spatialRequested && reconstructionReady && recon.filterReadiness == LightingFallback::None &&
        recon.variancePipeline && recon.varianceLayout && recon.atrousPipeline && recon.atrousLayout && recon.filterCB;
    lighting.reconstructionSpatialRequested = spatialRequested;
    lighting.reconstructionFilterPasses = filterPasses;
    lighting.reconstructionSpatialFallback = LightingFallback::None;
    if (spatialRequested && !spatialReady)
        lighting.reconstructionSpatialFallback = !reconstructionReady ? lighting.reconstructionFallback
            : (recon.filterReadiness != LightingFallback::None ? recon.filterReadiness : LightingFallback::ResourcesUnavailable);

    VirtualResourceHandle fgDepthGuide;
    VirtualResourceHandle fgVarianceDiffuse;
    VirtualResourceHandle fgVarianceSpecular;
    VirtualResourceHandle fgFilterDiffuse[2];
    VirtualResourceHandle fgFilterSpecular[2];
    VirtualResourceHandle fgFilteredDiffuse;
    VirtualResourceHandle fgFilteredSpecular;
    if (spatialReady)
    {
        const auto createTransient = [&](const char* name, nvrhi::Format format)
        {
            ResourceDesc desc;
            desc.type = ResourceDesc::Type::Texture2D;
            desc.debugName = name;
            desc.width = width;
            desc.height = height;
            desc.format = format;
            desc.isUAV = true;
            desc.isTransient = true;
            return fg.CreateTexture(name, desc);
        };
        fgDepthGuide = createTransient("rtgi_DepthGuide", nvrhi::Format::R32_FLOAT);
        fgVarianceDiffuse = createTransient("rt_GI_Variance", nvrhi::Format::RGBA16_FLOAT);
        fgVarianceSpecular = createTransient("rtgi_VarianceSpecular", nvrhi::Format::RGBA16_FLOAT);
        fgFilterDiffuse[0] = createTransient("rtgi_FilterDiffuse_A", nvrhi::Format::RGBA16_FLOAT);
        fgFilterDiffuse[1] = createTransient("rtgi_FilterDiffuse_B", nvrhi::Format::RGBA16_FLOAT);
        fgFilterSpecular[0] = createTransient("rtgi_FilterSpecular_A", nvrhi::Format::RGBA16_FLOAT);
        fgFilterSpecular[1] = createTransient("rtgi_FilterSpecular_B", nvrhi::Format::RGBA16_FLOAT);
        if (!fgDepthGuide.is_valid() || !fgVarianceDiffuse.is_valid() || !fgVarianceSpecular.is_valid() ||
            !fgFilterDiffuse[0].is_valid() || !fgFilterDiffuse[1].is_valid() ||
            !fgFilterSpecular[0].is_valid() || !fgFilterSpecular[1].is_valid())
        {
            spatialReady = false;
            lighting.reconstructionSpatialFallback = LightingFallback::ResourcesUnavailable;
        }
    }

    if (spatialReady)
    {
        RTGIFilterCB filterCB = {};
        filterCB.width = width;
        filterCB.height = height;
        filterCB.stepSize = 1;
        filterCB.iteration = 0;
        filterCB.phiColor = 10.0f;
        filterCB.phiNormal = 128.0f;

        fg.addCallbackPass<RTGIVariancePassData>(
            "RTGI Variance Estimate",
            [&](FrameGraph& builder, PassHandle passHandle, RTGIVariancePassData& data)
            {
                RenderPassBuilder pb(builder, passHandle);
                data.historyDiffuse = pb.read(fgHistoryDiffuse, ResourceState::ShaderResource);
                data.historySpecular = pb.read(fgHistorySpecular, ResourceState::ShaderResource);
                data.moments = pb.read(fgMoments, ResourceState::ShaderResource);
                data.normalRoughness = pb.read(fgNormalRoughness, ResourceState::ShaderResource);
                data.surfaceData = pb.read(fgSurfaceData, ResourceState::ShaderResource);
                data.diffuse = pb.write(fgVarianceDiffuse, ResourceState::UnorderedAccess);
                data.specular = pb.write(fgVarianceSpecular, ResourceState::UnorderedAccess);
                data.depthGuide = pb.write(fgDepthGuide, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.lighting = &lighting;
                data.cbData = filterCB;
                data.width = width;
                data.height = height;
            },
            [](const RTGIVariancePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
            {
                auto& recon = data.state->reconstruction;
                if (!data.state->initialRecorded || !recon.recorded || data.lighting->effective != LightingMode::RTGI)
                    return;

                auto* historyDiffuse = fg.GetPhysicalTexture(data.historyDiffuse);
                auto* historySpecular = fg.GetPhysicalTexture(data.historySpecular);
                auto* moments = fg.GetPhysicalTexture(data.moments);
                auto* normalRoughness = fg.GetPhysicalTexture(data.normalRoughness);
                auto* surfaceData = fg.GetPhysicalTexture(data.surfaceData);
                auto* diffuse = fg.GetPhysicalTexture(data.diffuse);
                auto* specular = fg.GetPhysicalTexture(data.specular);
                auto* depthGuide = fg.GetPhysicalTexture(data.depthGuide);
                if (!historyDiffuse || !historySpecular || !moments || !normalRoughness || !surfaceData ||
                    !diffuse || !specular || !depthGuide)
                {
                    Msg("! [RTGI Variance] Null FG texture");
                    data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                    return;
                }

                auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rtgi_variance", ".cs");
                if (!reflection)
                {
                    data.lighting->Fail(LightingFallback::ShaderUnavailable);
                    return;
                }

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                cmdList->writeBuffer(recon.filterCB, &data.cbData, sizeof(RTGIFilterCB));

                framegraph::BindingSetBuilder bsb(*reflection, nvDevice, "RTGI.Variance");
                bsb.ConstantBuffer("RTGIFilterParams", recon.filterCB);
                bsb.Texture("t_HistoryDiffuse", historyDiffuse);
                bsb.Texture("t_HistorySpecular", historySpecular);
                bsb.Texture("t_Moments", moments);
                bsb.Texture("t_NormalRoughness", normalRoughness);
                bsb.Texture("t_SurfaceData", surfaceData);
                bsb.TextureUAV("u_Diffuse", diffuse);
                bsb.TextureUAV("u_Specular", specular);
                bsb.TextureUAV("u_DepthGuide", depthGuide);
                auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), recon.varianceLayout, nvDevice);
                if (!bindingSet)
                {
                    data.lighting->Fail(LightingFallback::BindingUnavailable);
                    return;
                }

                nvrhi::ComputeState cs;
                cs.pipeline = recon.variancePipeline;
                cs.bindings = { bindingSet };
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                recon.varianceRecorded = true;
            });

        static constexpr const char* s_rtgiAtrousNames[6] =
        {
            "RTGI A-Trous 1",
            "RTGI A-Trous 2",
            "RTGI A-Trous 3",
            "RTGI A-Trous 4",
            "RTGI A-Trous 5",
            "RTGI A-Trous 6"
        };
        VirtualResourceHandle currentDiffuse = fgVarianceDiffuse;
        VirtualResourceHandle currentSpecular = fgVarianceSpecular;
        for (u32 iteration = 0; iteration < filterPasses; ++iteration)
        {
            const auto inDiffuse = currentDiffuse;
            const auto inSpecular = currentSpecular;
            const auto outDiffuse = fgFilterDiffuse[iteration & 1u];
            const auto outSpecular = fgFilterSpecular[iteration & 1u];
            RTGIFilterCB iterationCB = filterCB;
            iterationCB.stepSize = 1u << iteration;
            iterationCB.iteration = iteration;
            fg.addCallbackPass<RTGIAtrousPassData>(
                s_rtgiAtrousNames[iteration],
                [&](FrameGraph& builder, PassHandle passHandle, RTGIAtrousPassData& data)
                {
                    RenderPassBuilder pb(builder, passHandle);
                    data.inDiffuse = pb.read(inDiffuse, ResourceState::ShaderResource);
                    data.inSpecular = pb.read(inSpecular, ResourceState::ShaderResource);
                    data.normalRoughness = pb.read(fgNormalRoughness, ResourceState::ShaderResource);
                    data.depthGuide = pb.read(fgDepthGuide, ResourceState::ShaderResource);
                    data.outDiffuse = pb.write(outDiffuse, ResourceState::UnorderedAccess);
                    data.outSpecular = pb.write(outSpecular, ResourceState::UnorderedAccess);
                    pb.sideEffects();
                    data.device = device;
                    data.state = &state;
                    data.lighting = &lighting;
                    data.cbData = iterationCB;
                    data.width = width;
                    data.height = height;
                    data.iteration = iteration;
                    data.iterations = filterPasses;
                },
                [](const RTGIAtrousPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
                {
                    auto& recon = data.state->reconstruction;
                    if (!recon.varianceRecorded || recon.filterIterationsRecorded != data.iteration ||
                        data.lighting->effective != LightingMode::RTGI)
                        return;

                    auto* inDiffuse = fg.GetPhysicalTexture(data.inDiffuse);
                    auto* inSpecular = fg.GetPhysicalTexture(data.inSpecular);
                    auto* normalRoughness = fg.GetPhysicalTexture(data.normalRoughness);
                    auto* depthGuide = fg.GetPhysicalTexture(data.depthGuide);
                    auto* outDiffuse = fg.GetPhysicalTexture(data.outDiffuse);
                    auto* outSpecular = fg.GetPhysicalTexture(data.outSpecular);
                    if (!inDiffuse || !inSpecular || !normalRoughness || !depthGuide || !outDiffuse || !outSpecular)
                    {
                        Msg("! [RTGI A-Trous %u] Null FG texture", data.iteration + 1);
                        data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                        return;
                    }

                    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rtgi_atrous", ".cs");
                    if (!reflection)
                    {
                        data.lighting->Fail(LightingFallback::ShaderUnavailable);
                        return;
                    }

                    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                    cmdList->writeBuffer(recon.filterCB, &data.cbData, sizeof(RTGIFilterCB));

                    framegraph::BindingSetBuilder bsb(*reflection, nvDevice, "RTGI.Atrous");
                    bsb.ConstantBuffer("RTGIFilterParams", recon.filterCB);
                    bsb.Texture("t_Diffuse", inDiffuse);
                    bsb.Texture("t_Specular", inSpecular);
                    bsb.Texture("t_NormalRoughness", normalRoughness);
                    bsb.Texture("t_DepthGuide", depthGuide);
                    bsb.TextureUAV("u_Diffuse", outDiffuse);
                    bsb.TextureUAV("u_Specular", outSpecular);
                    auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), recon.atrousLayout, nvDevice);
                    if (!bindingSet)
                    {
                        data.lighting->Fail(LightingFallback::BindingUnavailable);
                        return;
                    }

                    nvrhi::ComputeState cs;
                    cs.pipeline = recon.atrousPipeline;
                    cs.bindings = { bindingSet };
                    cmdList->setComputeState(cs);
                    cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
                    recon.filterIterationsRecorded = data.iteration + 1;
                    if (recon.filterIterationsRecorded == data.iterations)
                    {
                        recon.filterRecorded = true;
                        data.lighting->reconstructionSpatialActive = true;
                    }
                });
            currentDiffuse = outDiffuse;
            currentSpecular = outSpecular;
        }
        fgFilteredDiffuse = currentDiffuse;
        fgFilteredSpecular = currentSpecular;
        fg.GetRTRegistry().RegisterRT("rt_GI_Variance", fgVarianceDiffuse);
        fg.GetRTRegistry().RegisterRT("rt_GI_FilteredDiffuse", fgFilteredDiffuse);
        fg.GetRTRegistry().RegisterRT("rt_GI_FilteredSpecular", fgFilteredSpecular);
    }

    RTGICompositeParams compositeCB = {};
    compositeCB.width = width;
    compositeCB.height = height;
    compositeCB.remodulate = reconstructionReady ? 1u : 0u;
    compositeCB.pad1 = 0;
    const auto compositeDiffuse = spatialReady ? fgFilteredDiffuse : (reconstructionReady ? fgHistoryDiffuse : fgRawDiffuse);
    const auto compositeSpecular = spatialReady ? fgFilteredSpecular : (reconstructionReady ? fgHistorySpecular : fgRawSpecular);

    auto& compositeData = fg.addCallbackPass<RTGICompositePassData>(
        "RTGI Opaque Lighting",
        [&](FrameGraph& builder, PassHandle passHandle, RTGICompositePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.diffuse = pb.read(compositeDiffuse, ResourceState::ShaderResource);
            data.specular = pb.read(compositeSpecular, ResourceState::ShaderResource);
            data.emission = pb.read(fgEmission, ResourceState::ShaderResource);
            data.albedoMetallic = pb.read(fgAlbedoMetallic, ResourceState::ShaderResource);
            data.sceneColor = pb.readWrite(sourceColorIn, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.state = &state;
            data.lighting = &lighting;
            data.cbData = compositeCB;
            data.bounces = bounces;
            data.samples = samples;
            data.rayDistance = rayDistance;
            data.staticDetailInstanceCount = scene->staticDetailInstanceCount;
            data.width = width;
            data.height = height;
            data.reconstructed = reconstructionReady;
            data.spatial = spatialReady;
        },
        [](const RTGICompositePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.state->initialRecorded || data.lighting->effective != LightingMode::RTGI)
                return;
            if (data.reconstructed && !data.state->reconstruction.recorded)
                return;
            if (data.spatial && !data.state->reconstruction.filterRecorded)
                return;

            auto* diffuse = fg.GetPhysicalTexture(data.diffuse);
            auto* specular = fg.GetPhysicalTexture(data.specular);
            auto* emission = fg.GetPhysicalTexture(data.emission);
            auto* albedoMetallic = fg.GetPhysicalTexture(data.albedoMetallic);
            auto* outTex = fg.GetPhysicalTexture(data.sceneColor);
            if (!diffuse || !specular || !emission || !albedoMetallic || !outTex)
            {
                Msg("! [RTGI Composite] Null FG texture: diffuse=%d specular=%d emission=%d albedo=%d out=%d",
                    !!diffuse, !!specular, !!emission, !!albedoMetallic, !!outTex);
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rtgi_composite", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(RTGICompositeParams));

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "RTGI.Composite");
            bsb.ConstantBuffer("RTGICompositeParams", data.state->cb);
            bsb.Texture("t_Diffuse", diffuse);
            bsb.Texture("t_Specular", specular);
            bsb.Texture("t_Emission", emission);
            bsb.Texture("t_AlbedoMetallic", albedoMetallic);
            bsb.TextureUAV("u_SceneColor", outTex);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.state->compositeLayout, nvDevice);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->compositePipeline;
            cs.bindings = { bindingSet };

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.lighting->recorded = true;
            data.lighting->rawSignalsRecorded = true;
            data.lighting->rtgiBounces = data.bounces;
            data.lighting->rtgiSamples = data.samples;
            data.lighting->rtgiRayDistance = data.rayDistance;
            data.lighting->rayStaticDetailInstances = data.staticDetailInstanceCount;
        });

    ReSTIRGIOutput output;
    output.sceneColor = compositeData.sceneColor;
    output.rawDiffuse = fgRawDiffuse;
    output.rawSpecular = fgRawSpecular;
    output.emission = fgEmission;
    output.normalRoughness = fgNormalRoughness;
    output.albedoMetallic = fgAlbedoMetallic;
    output.pathData = fgPathData;
    output.surfaceData = fgSurfaceData;
    output.motionVectors = fgOutMotion;
    output.historyDiffuse = fgHistoryDiffuse;
    output.historySpecular = fgHistorySpecular;
    output.moments = fgMoments;
    output.reconstruction = fgReconstruction;
    output.variance = fgVarianceDiffuse;
    output.filteredDiffuse = fgFilteredDiffuse;
    output.filteredSpecular = fgFilteredSpecular;
    return output;
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state)
{
    state.tracePipeline = nullptr;
    state.traceLayout = nullptr;
    state.compositePipeline = nullptr;
    state.compositeLayout = nullptr;
    state.cb = nullptr;
    state.reconstruction = {};
    state.rawDiffuse = nullptr;
    state.rawSpecular = nullptr;
    state.emission = nullptr;
    state.normalRoughness = nullptr;
    state.albedoMetallic = nullptr;
    state.pathData = nullptr;
    state.surfaceData = nullptr;
    state.motion = nullptr;
    s_rtgiPlaceholderBuffer = nullptr;
    state.initialized = false;
    state.readiness = LightingFallback::ResourcesUnavailable;
    state.historyValid = false;
    state.worldCacheSignature = 0;
    state.initialRecorded = false;
    state.texWidth = 0;
    state.texHeight = 0;
}
}
