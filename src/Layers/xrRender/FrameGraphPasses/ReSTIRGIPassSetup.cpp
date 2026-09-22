#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "ShaderConstants.h"
#include "RTEnvironmentSamplingPassSetup.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
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
    return LightingFallback::None;
}

ReSTIRGIOutput setupReSTIRGIPass(FrameGraph& fg, fg::RenderDevice* device, RTAccelStructManager* accelMgr, const DefaultOutputLayout& inputs,
    const ClusterLightOutput& clusterLights, VirtualResourceHandle prevNormals, VirtualResourceHandle prevDepth,
    VirtualResourceHandle motionVectors, const Fmatrix& invViewProj, const Fmatrix& prevViewProj, const Fvector& cameraPos, float giIntensity, u32 width,
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

    state.historyValid = false;
    state.initialRecorded = false;

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    auto* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1])
    {
        if (env.Current[0]->sky_texture_name.size())
        {
            auto h0 = texManager->LoadTexture(env.Current[0]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h0);
            if (t)
                sky0Tex = t;
        }
        if (env.Current[1]->sky_texture_name.size())
        {
            auto h1 = texManager->LoadTexture(env.Current[1]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h1);
            if (t)
                sky1Tex = t;
        }
    }
    if (!sky0Tex ||
        !sky1Tex ||
        sky0Tex->getDesc().dimension != nvrhi::TextureDimension::TextureCube ||
        sky1Tex->getDesc().dimension != nvrhi::TextureDimension::TextureCube)
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
    auto lightData = clusterLights.lightData;
    if (!lightData.is_valid())
    {
        auto* buffer = lightManager.GetLightDataBuffer();
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.structStride = buffer->getDesc().structStride;
        desc.isImported = true;
        desc.isTransient = false;
        lightData = fg.ImportBuffer("cluster_light_data", buffer, desc);
    }

    const auto environmentSampling = setupRTEnvironmentSamplingPass(fg, device, sky0Tex, sky1Tex, skyWeight);
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
    rawCB.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
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
    rawCB.environmentRotation = env.CurrentEnv.sky_rotation;
    rawCB.sunAngularRadius = deg2rad(ps_r_rt_sun_radius);
    rawCB.cameraConeSpread = ComputeRTGICameraConeSpread(invViewProj, cameraPos, width, height);
    rawCB.pad = 0;
    rawCB.detailMeshBatchStart = scene->detailMeshBatchStart;
    rawCB.staticDetailBatchStart = scene->staticDetailBatchStart;
    rawCB.detailPbrIndex = scene->detailPbrIndex;
    rawCB.detailBumpIndex = scene->detailBumpIndex;

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
            data.environmentDistribution = pb.read(environmentSampling.distribution, ResourceState::ShaderResource);
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
            data.state = &state;
            data.lighting = &lighting;
            data.cbData = rawCB;
            data.width = width;
            data.height = height;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
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

            nvrhi::ITexture* sky0 = data.sky0;
            nvrhi::ITexture* sky1 = data.sky1;
            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            auto* tlas = scene.tlas;
            auto* batchInfo = scene.batchInfo;
            auto* megaVB = scene.vertices;
            auto* megaIB = scene.indices;
            auto* matBuf = scene.materials;
            auto* terrainBuf = scene.terrainMaterials;

            auto* environmentDistribution = fg.GetPhysicalBuffer(data.environmentDistribution);
            if (!sky0 || !sky1 || !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf ||
                !scene.variants || !scene.textures || !scene.emissiveTriangles || !scene.batchTransforms ||
                !scene.emissiveBatchOffsets || !environmentDistribution)
            {
                Msg("! [RTGI Trace] Null binding: sky0=%d sky1=%d tlas=%d batch=%d megaVB=%d megaIB=%d mat=%d terrain=%d",
                    !!sky0, !!sky1, !!tlas, !!batchInfo, !!megaVB, !!megaIB, !!matBuf, !!terrainBuf);
                data.lighting->Fail(LightingFallback::SceneUnavailable);
                return;
            }

            auto* lightData = fg.GetPhysicalBuffer(data.lightData);
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            if (!lightData || !staticGlobals)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rtgi_trace", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

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

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "RTGI.Trace");
            bsb.ConstantBuffer("RTGIRawParams", data.state->cb);
            bsb.ConstantBuffer("static_globals", staticGlobals);
            bsb.BufferSRV("g_LightData", lightData);
            bsb.AccelStruct("g_SceneTLAS", tlas);
            bsb.BufferSRV("g_BatchInfo", batchInfo);
            bsb.BufferSRV("g_MegaVB", megaVB);
            bsb.BufferSRV("g_MegaIB", megaIB);
            bsb.Texture("g_Sky0", sky0);
            bsb.Texture("g_Sky1", sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", matBuf);
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
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), data.state->traceLayout);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->tracePipeline;
            cs.bindings = { bindingSet };
            if (scene.textures)
                cs.addBindingSet(scene.textures);

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.state->initialRecorded = true;
        });

    RTGICompositeParams compositeCB = {};
    compositeCB.width = width;
    compositeCB.height = height;
    compositeCB.pad0 = 0;
    compositeCB.pad1 = 0;

    auto& compositeData = fg.addCallbackPass<RTGICompositePassData>(
        "RTGI Opaque Lighting",
        [&](FrameGraph& builder, PassHandle passHandle, RTGICompositePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.rawDiffuse = pb.read(fgRawDiffuse, ResourceState::ShaderResource);
            data.rawSpecular = pb.read(fgRawSpecular, ResourceState::ShaderResource);
            data.emission = pb.read(fgEmission, ResourceState::ShaderResource);
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
        },
        [](const RTGICompositePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.state->initialRecorded || data.lighting->effective != LightingMode::RTGI)
                return;

            auto* rawDiffuse = fg.GetPhysicalTexture(data.rawDiffuse);
            auto* rawSpecular = fg.GetPhysicalTexture(data.rawSpecular);
            auto* emission = fg.GetPhysicalTexture(data.emission);
            auto* outTex = fg.GetPhysicalTexture(data.sceneColor);
            if (!rawDiffuse || !rawSpecular || !emission || !outTex)
            {
                Msg("! [RTGI Composite] Null FG texture: raw=%d spec=%d emission=%d out=%d",
                    !!rawDiffuse, !!rawSpecular, !!emission, !!outTex);
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
            bsb.Texture("t_RawDiffuse", rawDiffuse);
            bsb.Texture("t_RawSpecular", rawSpecular);
            bsb.Texture("t_Emission", emission);
            bsb.TextureUAV("u_SceneColor", outTex);
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), data.state->compositeLayout);
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
            data.lighting->historyUsed = false;
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
    return output;
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state)
{
    state.tracePipeline = nullptr;
    state.traceLayout = nullptr;
    state.compositePipeline = nullptr;
    state.compositeLayout = nullptr;
    state.cb = nullptr;
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
    state.initialRecorded = false;
    state.texWidth = 0;
    state.texHeight = 0;
}
}
