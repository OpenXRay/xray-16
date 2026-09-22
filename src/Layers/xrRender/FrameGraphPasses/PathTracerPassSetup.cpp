#include "stdafx.h"
#include "PathTracerPassSetup.h"
#include "RTEnvironmentSamplingPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
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
#include "xrEngine/xr_efflensflare.h"
#include "xrEngine/IGame_Persistent.h"
#include <limits>
#include <nvrhi/utils.h>

namespace fg
{
extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

class PathTracerSnapshotGlobals
{
public:
    StaticGlobals values = {};
};

static nvrhi::ShaderHandle s_pathtrace_shader;
static nvrhi::IBuffer* s_cb = nullptr;
static nvrhi::ComputePipelineHandle s_pipeline;
static nvrhi::BindingLayoutHandle s_layout;
static nvrhi::SamplerHandle s_sampler;
static nvrhi::BufferHandle s_ptPlaceholderBuffer;
static u32 s_pipelineRevision = 1;
static bool s_initialized = false;
static LightingFallback s_readiness = LightingFallback::ResourcesUnavailable;

static void CreatePlaceholderBuffer(nvrhi::IDevice* nvDevice)
{
    if (s_ptPlaceholderBuffer)
        return;

    nvrhi::BufferDesc desc;
    desc.debugName = "PT_PlaceholderBuffer";
    desc.byteSize = 4;
    desc.canHaveRawViews = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    s_ptPlaceholderBuffer = nvDevice->createBuffer(desc);
}

static LightingFallback InitializeResources(RenderDevice* device)
{
    if (s_initialized)
        return s_readiness;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* backend = device->GetBackend();
    if (!shaderLoader || !backend || !backend->GetBindlessLayout() || !backend->GetBindlessDescriptorTable())
        return s_readiness;

    s_initialized = true;
    auto csResult = shaderLoader->LoadComputeShader("rt_pathtrace");
    if (!csResult.handle || !csResult.reflection)
        return s_readiness = LightingFallback::ShaderUnavailable;
    s_pathtrace_shader = csResult.handle;

    auto& cache = GetPassResourceCache();
    s_cb = cache.GetOrCreateVolatileCB("PathTracer", "PathTracerCB", sizeof(PathTracerCB), device);
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    s_sampler = cache.GetOrCreateSampler("PathTracer", samplerDesc, nvDevice);
    CreatePlaceholderBuffer(nvDevice);
    if (!s_cb || !s_sampler || !s_ptPlaceholderBuffer)
    {
        s_initialized = false;
        return s_readiness;
    }

    s_layout = cache.GetOrCreateBindingLayoutFromReflection("PathTracer", *csResult.reflection, nvDevice);
    if (!s_layout)
        return s_readiness = LightingFallback::PipelineUnavailable;
    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = s_pathtrace_shader;
    pipeDesc.bindingLayouts = { s_layout, backend->GetBindlessLayout() };
    s_pipeline = nvDevice->createComputePipeline(pipeDesc);
    s_readiness = s_pipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    return s_readiness;
}

static void EnsureAccumulationBuffer(nvrhi::IDevice* nvDevice, u32 width, u32 height, PathTracerPassState& state)
{
    if (state.accumulation && state.width == width && state.height == height)
        return;

    nvrhi::TextureDesc desc;
    desc.debugName = "PT_Accumulation";
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA32_FLOAT;
    desc.isUAV = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;

    state.accumulation = nvDevice->createTexture(desc);
    state.history.valid = false;
    state.width = width;
    state.height = height;
}

LightingFallback EnsurePathTracerResources(RenderDevice* device, u32 width, u32 height, PathTracerPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !width || !height)
        return LightingFallback::ResourcesUnavailable;
    const auto readiness = InitializeResources(device);
    if (readiness != LightingFallback::None)
        return readiness;
    EnsureAccumulationBuffer(device->GetNVRHIDevice(), width, height, state);
    return state.accumulation ? LightingFallback::None : LightingFallback::ResourcesUnavailable;
}

static u64 EstimateSnapshotTextureBytes(const nvrhi::TextureDesc& desc)
{
    const auto& info = nvrhi::getFormatInfo(desc.format);
    const u32 block = info.blockSize ? info.blockSize : 1;
    const u64 bytesPerBlock = info.bytesPerBlock ? info.bytesPerBlock : 4;
    u64 total = 0;
    u32 w = desc.width;
    u32 h = desc.height;
    u32 d = desc.depth;
    for (u32 mip = 0; mip < desc.mipLevels; ++mip)
    {
        const u64 bx = (u64(w) + block - 1) / block;
        const u64 by = (u64(h) + block - 1) / block;
        total += bx * by * u64(std::max(1u, d)) * bytesPerBlock * u64(desc.arraySize) * u64(desc.sampleCount);
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        d = std::max(1u, d / 2);
    }
    return total;
}

static nvrhi::TextureHandle CreateSnapshotTexture(nvrhi::IDevice* nvDevice, nvrhi::ITexture* source, const char* name)
{
    if (!nvDevice || !source)
        return nullptr;
    const auto& sourceDesc = source->getDesc();
    if (sourceDesc.isTypeless || sourceDesc.format == nvrhi::Format::UNKNOWN || sourceDesc.sampleCount != 1)
        return nullptr;

    nvrhi::TextureDesc desc = sourceDesc;
    desc.isRenderTarget = false;
    desc.isUAV = false;
    desc.isShaderResource = true;
    desc.isTypeless = false;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = name;
    return nvDevice->createTexture(desc);
}

static bool UnprojectSnapshotPoint(const Fmatrix& invViewProj, float ndcX, float ndcY, Fvector& out)
{
    const float x = ndcX * invViewProj._11 + ndcY * invViewProj._21 + invViewProj._31 + invViewProj._41;
    const float y = ndcX * invViewProj._12 + ndcY * invViewProj._22 + invViewProj._32 + invViewProj._42;
    const float z = ndcX * invViewProj._13 + ndcY * invViewProj._23 + invViewProj._33 + invViewProj._43;
    const float w = ndcX * invViewProj._14 + ndcY * invViewProj._24 + invViewProj._34 + invViewProj._44;
    if (!std::isfinite(w) || fabsf(w) < 1e-6f)
        return false;
    out.set(x / w, y / w, z / w);
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

static float ComputeCameraConeSpread(const Fmatrix& invViewProj, const Fvector& cameraPos, u32 width, u32 height)
{
    if (!width || !height)
        return 0.0f;

    auto direction = [&](float ndcX, float ndcY, Fvector& out)
    {
        Fvector point;
        if (!UnprojectSnapshotPoint(invViewProj, ndcX, ndcY, point))
            return false;
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

bool PathTracerHistory::MatchesCamera(const Fmatrix& view, const Fmatrix& project, const Fvector& position) const
{
    const auto positionMatches = [](float current, float previous)
    {
        if (!std::isfinite(current) || !std::isfinite(previous))
            return false;
        const float tolerance = std::max(1e-4f, 4.0f * std::numeric_limits<float>::epsilon() * fabsf(previous));
        return fabsf(current - previous) <= tolerance;
    };
    if (!valid ||
        !positionMatches(position.x, parameters.cameraPos_pad.x) ||
        !positionMatches(position.y, parameters.cameraPos_pad.y) ||
        !positionMatches(position.z, parameters.cameraPos_pad.z))
        return false;

    for (u32 row = 0; row < 3; ++row)
    {
        for (u32 column = 0; column < 4; ++column)
        {
            if (!(fabsf(view.m[row][column] - cameraView.m[row][column]) <= 1e-6f))
                return false;
        }
    }
    if (view._44 != cameraView._44)
        return false;

    const bool perspective = project._34 == 1.0f && project._44 == 0.0f &&
        cameraProject._34 == 1.0f && cameraProject._44 == 0.0f;
    for (u32 row = 0; row < 4; ++row)
    {
        for (u32 column = 0; column < 4; ++column)
        {
            const float current = project.m[row][column];
            const float previous = cameraProject.m[row][column];
            if (!std::isfinite(current) || !std::isfinite(previous))
                return false;
            if (perspective && column == 2 && row >= 2)
                continue;
            const float tolerance = 1e-6f * std::max(1.0f, std::max(fabsf(current), fabsf(previous)));
            if (!(fabsf(current - previous) <= tolerance))
                return false;
        }
    }
    return true;
}

static void ApplyCameraSettings(StaticGlobals& target, const PathTracerData& data)
{
    const StaticGlobals live = BuildStaticGlobals();
    target.m_V = data.cameraView;
    target.m_P = data.cameraProject;
    target.m_VP.mul(data.cameraProject, data.cameraView);
    target.m_InvVP = data.cbData.invViewProj;
    target.eye_position.set(data.cbData.cameraPos_pad.x, data.cbData.cameraPos_pad.y, data.cbData.cameraPos_pad.z);
    target.camera_direction.set(data.cameraView._13, data.cameraView._23, data.cameraView._33, 0.0f);
    const float horizontalTan = 1.0f / data.cameraProject._11;
    const float verticalTan = -1.0f / data.cameraProject._22;
    target.pos_decompression_params.set(horizontalTan, verticalTan,
        2.0f * horizontalTan / data.cbData.screenWidth, 2.0f * verticalTan / data.cbData.screenHeight);
    target.pos_decompression_params2 = live.pos_decompression_params2;
    target.screen_res = live.screen_res;
    target.parallax = live.parallax;
    target.hud_fov = live.hud_fov;
    target.pbr_diffuse_mode = live.pbr_diffuse_mode;
    target.dev_param_1 = live.dev_param_1;
    target.dev_param_2 = live.dev_param_2;
    target.dev_param_3 = live.dev_param_3;
    target.dev_param_4 = live.dev_param_4;
}

static ResourceDesc::Type SnapshotResourceType(nvrhi::TextureDimension dimension)
{
    switch (dimension)
    {
    case nvrhi::TextureDimension::TextureCube: return ResourceDesc::Type::TextureCube;
    case nvrhi::TextureDimension::Texture3D: return ResourceDesc::Type::Texture3D;
    case nvrhi::TextureDimension::Texture2DArray: return ResourceDesc::Type::Texture2DArray;
    default: return ResourceDesc::Type::Texture2D;
    }
}

static ResourceDesc SnapshotBufferDesc(nvrhi::IBuffer* buffer, const char* name)
{
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Buffer;
    desc.bufferSize = buffer ? buffer->getDesc().byteSize : 0;
    desc.structStride = buffer ? buffer->getDesc().structStride : 0;
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = name;
    return desc;
}

static VirtualResourceHandle ImportNativeTexture(FrameGraph& fg, const char* name, nvrhi::ITexture* texture)
{
    const auto& native = texture->getDesc();
    ResourceDesc desc;
    desc.type = SnapshotResourceType(native.dimension);
    desc.width = native.width;
    desc.height = native.height;
    desc.depth = native.depth;
    desc.arraySize = native.arraySize;
    desc.mipLevels = native.mipLevels;
    desc.format = native.format;
    desc.sampleCount = native.sampleCount;
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = name;
    return fg.ImportTexture(name, texture, desc);
}

PathTracerSnapshot::~PathTracerSnapshot()
{
    if (scene && scene->retention)
        --scene->retention;
}

void DiscardPathTracerSnapshot(PathTracerPassState& state)
{
    state.history.snapshot.reset();
    state.pending.snapshot.reset();
    state.pending.capturedSnapshot = false;
    state.snapshot.reset();
}

static void InvalidateReferenceSnapshot(PathTracerPassState& state)
{
    if (!state.snapshot)
        return;
    ++state.snapshotStats.failedCaptures;
    DiscardPathTracerSnapshot(state);
}

PathTracerSnapshotReuse EvaluatePathTracerSnapshotReuse(RenderDevice* device, RTAccelStructManager* accelMgr,
    PathTracerPassState& state, bool freezeRequested)
{
    PathTracerSnapshotReuse reuse;
    if (!state.snapshot)
        return reuse;

    PathTracerSnapshot& snapshot = *state.snapshot;
    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    if (!nvDevice ||
        snapshot.device != nvDevice ||
        snapshot.pipelineRevision != s_pipelineRevision ||
        !snapshot.captured ||
        !snapshot.scene ||
        snapshot.scene->failed ||
        !freezeRequested)
    {
        reuse.invalid = true;
        return reuse;
    }

    if (!snapshot.submitted)
        return reuse;

    if (accelMgr && accelMgr->IsSceneValid(*snapshot.scene))
    {
        snapshot.promoted = true;
        reuse.reusable = true;
    }
    else
        reuse.invalid = true;
    return reuse;
}

static u64 EstimateSceneBufferBytes(const RTSceneGeneration& scene)
{
    u64 total = 0;
    auto add = [&](const nvrhi::BufferHandle& buffer)
    {
        if (buffer)
            total += buffer->getDesc().byteSize;
    };
    add(scene.batchInfo);
    add(scene.materials);
    add(scene.grassMaterials);
    add(scene.terrainMaterials);
    add(scene.variants);
    add(scene.sourceMaterials);
    add(scene.sourceTerrainMaterials);
    add(scene.sourceVariants);
    add(scene.bones);
    add(scene.skinnedVertices);
    add(scene.skinnedIndices);
    add(scene.grassVertices);
    add(scene.grassIndices);
    add(scene.emissiveTriangleBuffer);
    add(scene.batchTransformBuffer);
    add(scene.emissiveBatchOffsetBuffer);
    if (scene.geometry)
    {
        add(scene.geometry->vertices);
        add(scene.geometry->indices);
    }
    return total;
}

static void AddSceneParameters(PathTracerCB& cb, const RTSceneGeneration& scene)
{
    const auto& batchCounts = scene.counts;
    cb.identityStaticCount = batchCounts.identityStatic;
    cb.terrainBatchCount = batchCounts.terrain;
    cb.transparentBatchCount = batchCounts.transparent;
    if (batchCounts.skinned > 0)
        cb.skinnedBatchStart = batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal;
    else
        cb.skinnedBatchStart = UINT32_MAX;
    if (batchCounts.grass > 0)
        cb.grassBatchStart = batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned;
    else
        cb.grassBatchStart = UINT32_MAX;
    cb.detailAtlasIndex = scene.detailAtlasIndex;
    cb.detailMeshBatchStart = scene.detailMeshBatchStart;
    cb.staticDetailBatchStart = scene.staticDetailBatchStart;
    cb.detailPbrIndex = scene.detailPbrIndex;
    cb.detailBumpIndex = scene.detailBumpIndex;
}

static bool BuildReferenceSnapshot(FrameGraph& fg, RenderDevice* device, RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle lightDataSourceHandle, nvrhi::IBuffer* lightDataSource,
    nvrhi::ITexture* sky0, nvrhi::ITexture* sky1,
    float skyBlend, PathTracerPassState& state, PathTracerCaptureData*& captureOut)
{
    captureOut = nullptr;
    if (!accelMgr || !lightDataSource || !sky0 || !sky1)
        return false;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* backend = device->GetBackend();
    if (!nvDevice || !backend || !backend->GetBindlessLayout())
        return false;

    std::shared_ptr<RTSceneGeneration> scene = accelMgr->GetScene();
    if (!scene || scene->failed || !scene->geometry || !scene->tlas || !scene->batchInfo ||
        !scene->materials || !scene->grassMaterials || !scene->terrainMaterials ||
        !scene->variants || !scene->textures.GetTable())
        return false;

    auto snapshot = std::make_shared<PathTracerSnapshot>();

    xr_vector<u32> indices = scene->textures.GetIndices();
    if (scene->detailAtlasIndex != 0 && scene->detailAtlasIndex != bindless::INVALID_TEXTURE_INDEX)
        indices.push_back(scene->detailAtlasIndex);
    for (const auto& light : ClusteredLightManager::Instance().GetLightDataCPU())
    {
        u32 cookie = 0;
        std::memcpy(&cookie, &light.spotParamsAndType.z, sizeof(cookie));
        if (cookie && cookie != bindless::INVALID_TEXTURE_INDEX)
            indices.push_back(cookie);
    }
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    xr_vector<std::pair<u32, nvrhi::ITexture*>> sampled;
    for (u32 index : indices)
    {
        auto* texture = backend->GetBindlessTexture(index);
        if (!texture)
            return false;
        sampled.push_back({ index, texture });
    }

    xr_map<nvrhi::ITexture*, nvrhi::TextureHandle> clones;
    u64 textureBytes = 0;
    for (const auto& entry : sampled)
    {
        if (clones.find(entry.second) != clones.end())
            continue;
        nvrhi::TextureHandle clone = CreateSnapshotTexture(nvDevice, entry.second, "PT_SnapshotTexture");
        if (!clone)
            return false;
        clones[entry.second] = clone;
        snapshot->textureClones.push_back(clone);
        textureBytes += EstimateSnapshotTextureBytes(entry.second->getDesc());
    }

    nvrhi::DescriptorTableHandle table = nvDevice->createDescriptorTable(backend->GetBindlessLayout());
    if (!table)
        return false;

    u32 capacity = 1;
    for (const auto& entry : sampled)
        capacity = std::max(capacity, entry.first + 1);
    nvDevice->resizeDescriptorTable(table, capacity, false);
    if (sampled.empty())
    {
        if (!nvDevice->writeDescriptorTable(table, nvrhi::BindingSetItem::None(0)))
            return false;
    }
    for (const auto& entry : sampled)
    {
        auto it = clones.find(entry.second);
        if (it == clones.end())
            return false;
        nvrhi::BindingSetItem item = nvrhi::BindingSetItem::Texture_SRV(0, it->second.Get());
        item.slot = entry.first;
        if (!nvDevice->writeDescriptorTable(table, item))
            return false;
    }
    snapshot->textureTable = table;

    const auto& lightSourceDesc = lightDataSource->getDesc();
    nvrhi::BufferDesc lightDesc;
    lightDesc.debugName = "PT_SnapshotLightData";
    lightDesc.byteSize = lightSourceDesc.byteSize;
    lightDesc.structStride = lightSourceDesc.structStride;
    lightDesc.canHaveTypedViews = lightSourceDesc.canHaveTypedViews;
    lightDesc.canHaveRawViews = lightSourceDesc.canHaveRawViews;
    lightDesc.canHaveUAVs = false;
    lightDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    lightDesc.keepInitialState = true;
    snapshot->lightData = nvDevice->createBuffer(lightDesc);
    if (!snapshot->lightData)
        return false;

    nvrhi::BufferDesc globalsDesc;
    globalsDesc.debugName = "PT_SnapshotStaticGlobals";
    globalsDesc.byteSize = sizeof(StaticGlobals);
    globalsDesc.isConstantBuffer = true;
    globalsDesc.isVolatile = true;
    globalsDesc.maxVersions = 16;
    globalsDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    globalsDesc.keepInitialState = true;
    snapshot->staticGlobals = nvDevice->createBuffer(globalsDesc);
    if (!snapshot->staticGlobals)
        return false;

    snapshot->globals = std::make_shared<PathTracerSnapshotGlobals>();
    snapshot->globals->values = BuildStaticGlobals();

    snapshot->sky0 = CreateSnapshotTexture(nvDevice, sky0, "PT_SnapshotSky0");
    snapshot->sky1 = CreateSnapshotTexture(nvDevice, sky1, "PT_SnapshotSky1");
    if (!snapshot->sky0 || !snapshot->sky1)
        return false;

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

    PathTracerCB world = {};
    world.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    world.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyBlend };
    AddSceneParameters(world, *scene);
    world.lightCount = ClusteredLightManager::Instance().GetLightCount();
    world.emissiveCount = scene->emissiveCount;
    world.environmentRotation = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.sky_rotation : 0.0f;
    snapshot->world = world;
    snapshot->skyBlend = skyBlend;
    snapshot->sceneRevision = accelMgr->GetSceneRevision();
    snapshot->poseRevision = accelMgr->GetPoseRevision();
    snapshot->textureRevision = accelMgr->GetTextureRevision(sky0, sky1);
    snapshot->lightingSignature = ClusteredLightManager::Instance().GetTransportSignature();
    snapshot->device = nvDevice;

    const auto lightTargetHandle = fg.ImportBuffer("pt_FrozenLightData", snapshot->lightData,
        SnapshotBufferDesc(snapshot->lightData, "pt_FrozenLightData"));

    xr_vector<nvrhi::ITexture*> textureSources;
    xr_vector<nvrhi::TextureHandle> textureTargets;
    xr_vector<VirtualResourceHandle> sourceHandles;
    xr_vector<VirtualResourceHandle> targetHandles;
    textureSources.push_back(sky0);
    textureTargets.push_back(snapshot->sky0);
    sourceHandles.push_back(ImportNativeTexture(fg, "pt_CaptureSky0", sky0));
    targetHandles.push_back(ImportNativeTexture(fg, "pt_FrozenSky0", snapshot->sky0));
    textureSources.push_back(sky1);
    textureTargets.push_back(snapshot->sky1);
    sourceHandles.push_back(ImportNativeTexture(fg, "pt_CaptureSky1", sky1));
    targetHandles.push_back(ImportNativeTexture(fg, "pt_FrozenSky1", snapshot->sky1));
    for (const auto& entry : clones)
    {
        textureSources.push_back(entry.first);
        textureTargets.push_back(entry.second);
        sourceHandles.push_back(ImportNativeTexture(fg, "pt_CaptureTexture", entry.first));
        targetHandles.push_back(ImportNativeTexture(fg, "pt_FrozenTexture", entry.second));
    }

    nvrhi::BufferHandle lightDataClone = snapshot->lightData;
    auto& capture = fg.addCallbackPass<PathTracerCaptureData>("Path Tracer Reference Capture",
        [&, lightTargetHandle, sourceHandles, targetHandles, textureSources, textureTargets, lightDataClone, lightDataSource]
        (FrameGraph& builder, PassHandle passHandle, PathTracerCaptureData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.snapshot = snapshot.get();
            data.lightDataSource = lightDataSource;
            data.lightDataTarget = lightDataClone.Get();
            passBuilder.read(lightDataSourceHandle, ResourceState::CopySource);
            passBuilder.write(lightTargetHandle, ResourceState::CopyDest);
            for (u32 i = 0; i < u32(textureSources.size()); ++i)
            {
                data.textureSources.push_back(textureSources[i]);
                data.textureTargets.push_back(textureTargets[i]);
                passBuilder.read(sourceHandles[i], ResourceState::CopySource);
                passBuilder.write(targetHandles[i], ResourceState::CopyDest);
            }
            passBuilder.sideEffects();
        },
        [](const PathTracerCaptureData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!ctx || !data.device)
                return;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;
            if (data.lightDataSource && data.lightDataTarget)
                cmdList->copyBuffer(data.lightDataTarget, 0, data.lightDataSource, 0,
                    data.lightDataTarget->getDesc().byteSize);
            for (size_t i = 0; i < data.textureSources.size(); ++i)
            {
                nvrhi::ITexture* source = data.textureSources[i];
                nvrhi::ITexture* target = data.textureTargets[i].Get();
                if (!source || !target)
                    continue;
                const auto& desc = source->getDesc();
                for (u32 mip = 0; mip < desc.mipLevels; ++mip)
                {
                    for (u32 slice = 0; slice < desc.arraySize; ++slice)
                    {
                        nvrhi::TextureSlice region;
                        region.setMipLevel(mip).setArraySlice(slice);
                        cmdList->copyTexture(target, region, source, region);
                    }
                }
            }
            if (data.snapshot)
                data.snapshot->captured = true;
        });
    captureOut = &capture;

    accelMgr->RetainScene(scene);
    snapshot->scene = scene;
    snapshot->pipelineRevision = s_pipelineRevision;
    snapshot->stats.capturedTextures = u32(clones.size());
    snapshot->stats.capturedIndices = u32(sampled.size());
    snapshot->stats.capturedTextureBytes = textureBytes;
    snapshot->stats.capturedBuffers = 1;
    snapshot->stats.capturedBufferBytes = lightSourceDesc.byteSize;
    snapshot->stats.retainedSceneBuffers = 1;
    snapshot->stats.retainedSceneBytes = EstimateSceneBufferBytes(*scene);
    snapshot->stats.lightCount = world.lightCount;
    snapshot->stats.emissiveCount = world.emissiveCount;
    snapshot->stats.sceneRevision = snapshot->sceneRevision;
    snapshot->stats.textureRevision = snapshot->textureRevision;
    snapshot->stats.lightingSignature = snapshot->lightingSignature;

    state.snapshot = snapshot;
    snapshot->captureFrame = ++state.snapshotStats.captures;
    return true;
}

PathTracerOutput setupPathTracerPass(FrameGraph& fg, fg::RenderDevice* device, RTAccelStructManager* accelMgr, VirtualResourceHandle sceneColorIn,
    const ClusterLightOutput& clusterLights, LightingFrameState& lighting, const PathTracerConfig& config,
    const Fmatrix& view, const Fmatrix& project, const Fmatrix& invViewProj, const Fvector& cameraPos, u32 width, u32 height,
    PathTracerPassState& state)
{
    state.pending.valid = false;
    state.pending.capturedSnapshot = false;
    state.pending.snapshot.reset();

    const auto snapshotReuse = EvaluatePathTracerSnapshotReuse(device, accelMgr, state, config.freezeScene);
    if (snapshotReuse.invalid)
        InvalidateReferenceSnapshot(state);

    auto syncStats = [&]()
    {
        PathTracerSnapshotStats stats = state.snapshot ? state.snapshot->stats : PathTracerSnapshotStats{};
        stats.captures = state.snapshotStats.captures;
        stats.failedCaptures = state.snapshotStats.failedCaptures;
        stats.cdfActive = state.snapshotStats.cdfActive;
        stats.freezeRequested = config.freezeScene;
        stats.frozen = state.snapshot != nullptr;
        stats.valid = state.snapshot && state.snapshot->promoted;
        stats.capturePending = state.snapshot && !state.snapshot->promoted;
        stats.fallback = config.freezeScene && !state.snapshot;
        stats.captureFrame = state.snapshot ? state.snapshot->captureFrame : 0;
        stats.sceneValid = state.snapshot && accelMgr && accelMgr->IsSceneValid(*state.snapshot->scene);
        state.snapshotStats = stats;
    };
    syncStats();

    const auto readiness = EnsurePathTracerResources(device, width, height, state);
    if (readiness != LightingFallback::None)
    {
        lighting.Fail(readiness);
        return { sceneColorIn };
    }
    if (!accelMgr || (!snapshotReuse.reusable && !accelMgr->IsReady()))
    {
        lighting.Fail(LightingFallback::SceneUnavailable);
        return { sceneColorIn };
    }
    if (!sceneColorIn.is_valid() || !g_pGamePersistent)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sceneColorIn };
    }
    const auto& outputDesc = fg.GetResourceDesc(sceneColorIn);
    if ((!outputDesc.isUAV && !outputDesc.allowUAV) ||
        outputDesc.format != nvrhi::Format::RGBA16_FLOAT ||
        outputDesc.width != width ||
        outputDesc.height != height ||
        outputDesc.sampleCount != 1)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sceneColorIn };
    }

    ResourceDesc accumulationDesc;
    accumulationDesc.width = width;
    accumulationDesc.height = height;
    accumulationDesc.format = nvrhi::Format::RGBA32_FLOAT;
    accumulationDesc.isUAV = true;
    accumulationDesc.isImported = true;
    accumulationDesc.isTransient = false;
    const auto accumulation = fg.ImportTexture("pt_Accumulation", state.accumulation, accumulationDesc);
    fg.GetRTRegistry().RegisterRT("rt_PT_Accumulation", accumulation);

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    resources::TextureManager* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1])
    {
        const shared_str& name0 = env.Current[0]->sky_texture_name;
        const shared_str& name1 = env.Current[1]->sky_texture_name;
        if (name0.size())
        {
            auto h = texManager->LoadTexture(name0.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
            if (t)
                sky0Tex = t;
        }
        if (name1.size())
        {
            auto h = texManager->LoadTexture(name1.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
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
        return { sceneColorIn };
    }

    auto& lightManager = ClusteredLightManager::Instance();
    if (!lightManager.GetLightDataBuffer() ||
        (lightManager.GetLightCount() > 0 && (!clusterLights.active || !clusterLights.lightData.is_valid())))
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sceneColorIn };
    }
    auto lightData = clusterLights.lightData;
    if (!lightData.is_valid())
        lightData = fg.ImportBuffer("cluster_light_data", lightManager.GetLightDataBuffer(),
            SnapshotBufferDesc(lightManager.GetLightDataBuffer(), "cluster_light_data"));
    const bool clusterListsActive = clusterLights.active && clusterLights.clusterGrid.is_valid() &&
        clusterLights.lightIndexList.is_valid();
    auto clusterGrid = clusterListsActive ? clusterLights.clusterGrid : VirtualResourceHandle();
    auto lightIndexList = clusterListsActive ? clusterLights.lightIndexList : VirtualResourceHandle();
    if (!clusterGrid.is_valid())
        clusterGrid = lightManager.GetClusterGridBuffer() ? fg.ImportBuffer("cluster_light_grid",
            lightManager.GetClusterGridBuffer(), SnapshotBufferDesc(lightManager.GetClusterGridBuffer(), "cluster_light_grid")) : lightData;
    if (!lightIndexList.is_valid())
        lightIndexList = lightManager.GetLightIndexListBuffer() ? fg.ImportBuffer("cluster_light_index_list",
            lightManager.GetLightIndexListBuffer(), SnapshotBufferDesc(lightManager.GetLightIndexListBuffer(), "cluster_light_index_list")) : lightData;

    PathTracerCaptureData* capture = nullptr;
    if (config.freezeScene && !state.snapshot)
    {
        if (!BuildReferenceSnapshot(fg, device, accelMgr, lightData, lightManager.GetLightDataBuffer(),
                sky0Tex, sky1Tex, skyWeight, state, capture))
        {
            state.history.valid = false;
            state.history.samples = 0;
            state.history.snapshot.reset();
            state.pending.snapshot.reset();
        }
    }

    state.pending.snapshot = state.snapshot;
    state.pending.capturedSnapshot = capture != nullptr;

    const bool frozen = state.snapshot != nullptr;
    PathTracerSnapshot* snapshot = state.snapshot.get();
    nvrhi::ITexture* sky0Used = frozen ? snapshot->sky0.Get() : sky0Tex;
    nvrhi::ITexture* sky1Used = frozen ? snapshot->sky1.Get() : sky1Tex;
    const float skyBlendUsed = frozen ? snapshot->skyBlend : skyWeight;
    VirtualResourceHandle frozenLightHandle;
    VirtualResourceHandle frozenSky0Handle;
    VirtualResourceHandle frozenSky1Handle;
    xr_vector<VirtualResourceHandle> frozenTextureHandles;
    if (frozen)
    {
        frozenLightHandle = fg.ImportBuffer("pt_FrozenLightData", snapshot->lightData,
            SnapshotBufferDesc(snapshot->lightData, "pt_FrozenLightData"));
        frozenSky0Handle = ImportNativeTexture(fg, "pt_FrozenSky0", sky0Used);
        frozenSky1Handle = ImportNativeTexture(fg, "pt_FrozenSky1", sky1Used);
        for (const auto& clone : snapshot->textureClones)
            frozenTextureHandles.push_back(ImportNativeTexture(fg, "pt_FrozenTexture", clone));
    }

    const auto environmentCdf = setupRTEnvironmentSamplingPass(fg, device, sky0Used, sky1Used, skyBlendUsed);
    state.snapshotStats.cdfActive = environmentCdf.active;
    if (!environmentCdf.distribution.is_valid())
    {
        lighting.Fail(LightingFallback::EnvironmentUnavailable);
        return { sceneColorIn };
    }
    const auto cdfHandle = environmentCdf.distribution;

    PathTracerCB cbData = {};
    if (frozen)
    {
        cbData = snapshot->world;
    }
    else
    {
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

        cbData.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
        cbData.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
        const auto liveScene = accelMgr->GetScene();
        AddSceneParameters(cbData, *liveScene);
        cbData.lightCount = lightManager.GetLightCount();
        cbData.emissiveCount = liveScene ? liveScene->emissiveCount : 0;
        cbData.environmentRotation = env.CurrentEnv.sky_rotation;
    }
    cbData.invViewProj = invViewProj;
    cbData.cameraPos_pad = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f };
    cbData.screenWidth = static_cast<float>(width);
    cbData.screenHeight = static_cast<float>(height);
    cbData.sampleIndex = 0;
    cbData.maxBounces = config.maxBounces;
    cbData.diffuseMode = config.diffuseMode;
    cbData.diagnosticMode = config.diagnosticMode;
    cbData.maxNullEvents = config.maxNullEvents;
    cbData.maxSamples = config.maxSamples;
    cbData.sunAngularRadius = config.sunAngularRadius;
    cbData.cameraConeSpread = ComputeCameraConeSpread(invViewProj, cameraPos, width, height);
    cbData.clusterLights = clusterListsActive && !frozen ? 1u : 0u;
    cbData.lightRays = config.lightRays;

    const auto& history = state.history;
    state.pending.cameraView = view;
    state.pending.cameraProject = project;
    if (frozen && history.snapshot == state.snapshot && history.MatchesCamera(view, project, cameraPos))
    {
        state.pending.cameraView = history.cameraView;
        state.pending.cameraProject = history.cameraProject;
        cbData.invViewProj = history.parameters.invViewProj;
        cbData.cameraPos_pad = history.parameters.cameraPos_pad;
        cbData.cameraConeSpread = history.parameters.cameraConeSpread;
    }

    state.pending.parameters = cbData;
    state.pending.sky0 = sky0Used;
    state.pending.sky1 = sky1Used;
    state.pending.sceneRevision = frozen ? snapshot->sceneRevision : accelMgr->GetSceneRevision();
    state.pending.poseRevision = frozen ? snapshot->poseRevision : accelMgr->GetPoseRevision();
    state.pending.textureRevision = frozen ? snapshot->textureRevision : accelMgr->GetTextureRevision(sky0Used, sky1Used);
    state.pending.lightingSignature = frozen ? snapshot->lightingSignature : lightManager.GetTransportSignature();
    const StaticGlobals liveGlobals = BuildStaticGlobals();
    if (frozen)
    {
        state.pending.foliageSSS = snapshot->globals->values.foliage_sss;
        state.pending.foliageParams = snapshot->globals->values.foliage_params;
        state.pending.foliageParams2 = snapshot->globals->values.foliage_params2;
    }
    else
    {
        state.pending.foliageSSS = liveGlobals.foliage_sss;
        state.pending.foliageParams = liveGlobals.foliage_params;
        state.pending.foliageParams2 = liveGlobals.foliage_params2;
    }

    u32 sampleIndex = 0;
    const bool historyMatches = history.valid && history.sceneRevision == state.pending.sceneRevision &&
        history.poseRevision == state.pending.poseRevision &&
        history.textureRevision == state.pending.textureRevision &&
        history.lightingSignature == state.pending.lightingSignature &&
        memcmp(&history.foliageSSS, &state.pending.foliageSSS, sizeof(Fvector4)) == 0 &&
        memcmp(&history.foliageParams, &state.pending.foliageParams, sizeof(Fvector4)) == 0 &&
        memcmp(&history.foliageParams2, &state.pending.foliageParams2, sizeof(Fvector4)) == 0 &&
        history.sky0 == sky0Used && history.sky1 == sky1Used &&
        history.snapshot == state.snapshot &&
        memcmp(&history.parameters, &cbData, sizeof(cbData)) == 0;
    if (historyMatches)
        sampleIndex = history.samples;
    const u32 sampleCap = config.maxSamples;
    u32 sampleCount = 0;
    if (sampleCap && sampleIndex >= sampleCap)
    {
        sampleIndex = sampleCap;
        sampleCount = sampleCap;
    }
    else
    {
        sampleCount = sampleCap ? std::min(sampleIndex + 1, sampleCap) : sampleIndex + 1;
    }
    cbData.sampleIndex = sampleIndex;
    state.pending.samples = sampleCount;
    lighting.historyUsed = cbData.sampleIndex != 0;

    syncStats();

    auto& passData = fg.addCallbackPass<PathTracerData>(
        "Path Tracer",

        [&, width, height, cbData, sampleCount, sky0Used, sky1Used, frozen,
            frozenLightHandle, frozenSky0Handle, frozenSky1Handle, frozenTextureHandles,
            lightData, cdfHandle, accumulation, sceneColorIn]
        (FrameGraph& builder, PassHandle passHandle, PathTracerData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.lighting = &lighting;
            data.state = &state;
            if (frozen)
                data.scene = accelMgr->UseScene(builder, passBuilder, state.snapshot->scene);
            else
                data.scene = accelMgr->UseScene(builder, passBuilder);
            if (frozen)
                data.scene.textures = state.snapshot->textureTable;
            data.lightData = passBuilder.read(frozen ? frozenLightHandle : lightData, ResourceState::ShaderResource);
            data.clusterGrid = passBuilder.read(clusterGrid, ResourceState::ShaderResource);
            data.lightIndexList = passBuilder.read(lightIndexList, ResourceState::ShaderResource);
            data.environmentCdf = passBuilder.read(cdfHandle, ResourceState::ShaderResource);
            if (frozen)
            {
                passBuilder.read(frozenSky0Handle, ResourceState::ShaderResource);
                passBuilder.read(frozenSky1Handle, ResourceState::ShaderResource);
                for (const auto handle : frozenTextureHandles)
                    passBuilder.read(handle, ResourceState::ShaderResource);
            }
            data.width = width;
            data.height = height;
            data.cbData = cbData;
            data.cameraView = state.pending.cameraView;
            data.cameraProject = state.pending.cameraProject;
            data.sampleCount = sampleCount;
            data.staticDetailInstanceCount = frozen ? state.snapshot->scene->staticDetailInstanceCount :
                accelMgr->GetScene()->staticDetailInstanceCount;
            data.sky0 = sky0Used;
            data.sky1 = sky1Used;
            data.textureTable = data.scene.textures;
            data.snapshot = frozen ? state.snapshot : nullptr;
            data.staticGlobals = frozen ? state.snapshot->staticGlobals.Get() : nullptr;
            data.globals = frozen ? state.snapshot->globals : nullptr;

            data.accumulation = passBuilder.readWrite(accumulation, ResourceState::UnorderedAccess);
            data.outputTex = passBuilder.write(sceneColorIn, ResourceState::UnorderedAccess);
        },

        [](const PathTracerData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (s_readiness != LightingFallback::None)
            {
                data.lighting->Fail(s_readiness);
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            nvrhi::ITexture* outTex = fg.GetPhysicalTexture(data.outputTex);
            auto* accumulationTex = fg.GetPhysicalTexture(data.accumulation);
            if (!outTex || !accumulationTex)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            nvrhi::IBuffer* staticGlobals = nullptr;
            if (data.globals)
            {
                if (!data.snapshot || !data.snapshot->captured || !data.staticGlobals)
                {
                    data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                    return;
                }
                StaticGlobals patched = data.globals->values;
                ApplyCameraSettings(patched, data);
                cmdList->writeBuffer(data.staticGlobals, &patched, sizeof(StaticGlobals));
                staticGlobals = data.staticGlobals;
            }
            else
            {
                staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            }

            cmdList->writeBuffer(s_cb, &data.cbData, sizeof(PathTracerCB));

            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            if (!scene.tlas || !scene.batchInfo || !scene.vertices || !scene.indices || !scene.materials ||
                !scene.grassMaterials || !scene.terrainMaterials || !scene.variants || !scene.textures ||
                !scene.emissiveTriangles || !scene.batchTransforms || !scene.emissiveBatchOffsets)
            {
                data.lighting->Fail(LightingFallback::SceneUnavailable);
                return;
            }
            nvrhi::IBuffer* skinnedVB = scene.skinnedVertices;
            nvrhi::IBuffer* skinnedIB = scene.skinnedIndices;
            nvrhi::IBuffer* grassVB = scene.grassVertices;
            nvrhi::IBuffer* grassIB = scene.grassIndices;
            if (!skinnedVB)
                skinnedVB = s_ptPlaceholderBuffer.Get();
            if (!skinnedIB)
                skinnedIB = s_ptPlaceholderBuffer.Get();
            if (!grassVB)
                grassVB = s_ptPlaceholderBuffer.Get();
            if (!grassIB)
                grassIB = s_ptPlaceholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rt_pathtrace", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

            auto* lightData = fg.GetPhysicalBuffer(data.lightData);
            auto* clusterGridBuffer = fg.GetPhysicalBuffer(data.clusterGrid);
            auto* lightIndexListBuffer = fg.GetPhysicalBuffer(data.lightIndexList);
            auto* cdfBuffer = fg.GetPhysicalBuffer(data.environmentCdf);
            if (!lightData || !staticGlobals || !cdfBuffer)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "PathTracer");
            bsb.ConstantBuffer("PathTracerParams", s_cb);
            bsb.ConstantBuffer("static_globals", staticGlobals);
            bsb.BufferSRV("g_LightData", lightData);
            bsb.BufferSRV("g_ClusterGrid", clusterGridBuffer ? clusterGridBuffer : lightData);
            bsb.BufferSRV("g_LightIndexList", lightIndexListBuffer ? lightIndexListBuffer : lightData);
            bsb.AccelStruct("g_SceneTLAS", scene.tlas);
            bsb.BufferSRV("g_BatchInfo", scene.batchInfo);
            bsb.BufferSRV("g_MegaVB", scene.vertices);
            bsb.BufferSRV("g_MegaIB", scene.indices);
            bsb.BufferSRV("g_EmissiveTriangles", scene.emissiveTriangles);
            bsb.BufferSRV("g_RTBatchTransforms", scene.batchTransforms);
            bsb.BufferSRV("g_EmissiveBatchOffsets", scene.emissiveBatchOffsets);
            bsb.BufferSRV("g_EnvironmentCDF", cdfBuffer);
            bsb.Texture("g_Sky0", data.sky0);
            bsb.Texture("g_Sky1", data.sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", scene.materials);
            bsb.BufferSRV("g_GrassMaterials", scene.grassMaterials);
            bsb.BufferSRV("g_TerrainMaterials", scene.terrainMaterials);
            bsb.BufferSRV("g_Variants", scene.variants);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.TextureUAV("g_Accumulation", accumulationTex);
            bsb.TextureUAV("g_Output", outTex);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_layout, nvDevice);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState state;
            state.pipeline = s_pipeline;
            state.bindings = { bindingSet };

            if (data.textureTable)
                state.addBindingSet(data.textureTable);

            cmdList->setComputeState(state);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.state->pending.valid = true;
            data.state->pending.samples = data.sampleCount;
            data.lighting->recorded = true;
            data.lighting->recordedSamples = data.sampleCount;
            data.lighting->rayStaticDetailInstances = data.staticDetailInstanceCount;
        });

    return { passData.outputTex };
}

void ShutdownPathTracer()
{
    ++s_pipelineRevision;
    s_pathtrace_shader = nullptr;
    s_cb = nullptr;
    s_pipeline = nullptr;
    s_layout = nullptr;
    s_sampler = nullptr;
    s_ptPlaceholderBuffer = nullptr;
    s_initialized = false;
    s_readiness = LightingFallback::ResourcesUnavailable;
}
}
