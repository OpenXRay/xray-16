#include "stdafx.h"
#include "GPUCullingManager.h"
#include "ClusterShadowBVH.h"
#include "xrCore/Profiler/Profiler.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/ParticlePassSetup.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/Bindless/VertexConverter.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics bone access
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/Decals/OverlayManager.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrCore/Threading/ParallelFor.hpp"

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg {

static u32 MaterialObjectFlags(u32 materialID)
{
    const auto* material = bindless::MaterialBuffer::Instance().GetMaterial(materialID);
    if (!material)
        return 0u;
    if (material->flags & bindless::MAT_FLAG_ALPHA_BLEND)
        return GPU_OBJECT_NO_RESOLVE;
    return 0u;
}

bool GPUCullingManager::MaterialCastsShadow(u32 materialID)
{
    const auto* material = bindless::MaterialBuffer::Instance().GetMaterial(materialID);
    if (!material)
        return true;
    const bool blended = (material->flags & bindless::MAT_FLAG_ALPHA_BLEND) != 0u;
    if (blended && (material->flags & bindless::MAT_FLAG_WATER) != 0u)
        return false;
    if (const auto* variant = bindless::VariantBuffer::Instance().Get(material->shaderVariant))
    {
        const u32 excludedFlags = bindless::VARIANT_FLAG_NO_SHADOW
            | (blended ? bindless::VARIANT_FLAG_ADDITIVE_EMISSION : 0u);
        return (variant->flags & excludedFlags) == 0u;
    }
    const auto* registry = ShaderVariantRegistry::Instance().GetVariantByIndex(material->shaderVariant);
    return !registry || registry->castsShadow;
}

u32 TransparentKeyForVariant(const ShaderVariantDesc* variant)
{
    u32 key = u32(VariantBlendFactor::SrcAlpha) | (u32(VariantBlendFactor::InvSrcAlpha) << TRANSPARENT_KEY_DST_SHIFT);
    if (!variant)
        return key;
    if (!variant->passes.empty())
    {
        const ShaderPassDesc& pass = variant->passes[0];
        if (pass.blendEnabled)
            key = u32(pass.srcBlend) | (u32(pass.dstBlend) << TRANSPARENT_KEY_DST_SHIFT);
        if (pass.depthWrite)
            key |= TRANSPARENT_KEY_DEPTH_WRITE;
    }
    if (variant->distort)
        key |= TRANSPARENT_KEY_DISTORT;
    if (variant->wmark)
        key |= TRANSPARENT_KEY_WMARK;
    if (variant->colorMode == VariantColorMode::None)
        key |= TRANSPARENT_KEY_NO_COLOR;
    else if (variant->colorMode != VariantColorMode::Lit)
        key |= TRANSPARENT_KEY_UNLIT;
    return key;
}

static u32 TransparentKeyForMaterial(u32 materialID)
{
    const auto* material = bindless::MaterialBuffer::Instance().GetMaterial(materialID);
    if (!material || material->shaderVariant == 0)
        return TransparentKeyForVariant(nullptr);
    return TransparentKeyForVariant(ShaderVariantRegistry::Instance().GetVariantByIndex(material->shaderVariant));
}

static void PartitionTransparentDraws(xr_vector<IndirectDrawArgs>& args, xr_vector<u32>* materialIDs,
    xr_vector<GPUInstanceData>& instances, xr_vector<u32>& keys, xr_vector<TransparentDrawRange>& ranges,
    GPUCullingManager::TransparentDrawScratch& scratch, const xr_vector<float>* sortValues = nullptr,
    xr_vector<GeometryInstanceKey>* identities = nullptr)
{
    ranges.clear();
    R_ASSERT2(args.size() <= UINT32_MAX, "Forward submission count exceeds the addressable range");
    if (identities)
        R_ASSERT2(identities->size() == args.size(), "Forward identities are out of sync with draw args");
    const u32 n = static_cast<u32>(args.size());
    if (n == 0) {
        args.clear();
        instances.clear();
        keys.clear();
        if (materialIDs)
            materialIDs->clear();
        if (identities)
            identities->clear();
        return;
    }
    auto& order = scratch.order;
    order.resize(n);
    for (u32 i = 0; i < order.size(); ++i)
        order[i] = i;
    auto depthLess = [&](u32 a, u32 b) {
        if ((*sortValues)[a] != (*sortValues)[b])
            return (*sortValues)[a] < (*sortValues)[b];
        return a < b;
    };
    std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
        if (keys[a] != keys[b])
            return keys[a] < keys[b];
        return sortValues ? depthLess(a, b) : a < b;
    });
    scratch.args.clear();
    scratch.instances.clear();
    scratch.keys.clear();
    scratch.materialIDs.clear();
    scratch.identities.clear();
    for (u32 i = 0; i < n; ++i) {
        const u32 src = order[i];
        scratch.args.push_back(args[src]);
        scratch.args.back().startInstanceLocation = i;
        scratch.instances.push_back(instances[src]);
        scratch.keys.push_back(keys[src]);
        if (materialIDs)
            scratch.materialIDs.push_back((*materialIDs)[src]);
        if (identities)
            scratch.identities.push_back((*identities)[src]);
    }
    args.swap(scratch.args);
    instances.swap(scratch.instances);
    keys.swap(scratch.keys);
    if (materialIDs)
        materialIDs->swap(scratch.materialIDs);
    if (identities)
        identities->swap(scratch.identities);
    for (u32 i = 0; i < n;) {
        u32 j = i;
        while (j < n && keys[j] == keys[i])
            ++j;
        ranges.push_back({ keys[i], i, j - i });
        i = j;
    }
}

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_OBJECTS = 65536;  // Maximum objects per frame
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;  // Must match [numthreads] in shader

// ═══════════════════════════════════════════════════════
//  DEBUG CONSTANT BUFFER (must match HLSL)
// ═══════════════════════════════════════════════════════

struct CullDebugParamsCB {
    Fmatrix viewProj;
    Fmatrix prevViewProj;
    Fvector cameraPos;
    float maxDistanceSq;
    Fvector4 frustumPlanes[6];
    u32 objectCount;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    float occluderThreshold;
    u32 debugOffset;
    float padding[2];
};

struct CullDebugVSParamsCB {
    Fmatrix view;               // View matrix (for billboard orientation)
    Fmatrix viewProj;           // View-projection matrix
    u32 objectCount;            // Number of objects
    float wireframeAlpha;       // Wireframe transparency
    float padding[2];
};

// ═══════════════════════════════════════════════════════
//  STATIC STATE
// ═══════════════════════════════════════════════════════


// ═══════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_PARTICLES = 16384;

GPUCullingManager::GPUCullingManager()
{
    m_maxObjects = MAX_CULLING_OBJECTS;
    m_maxParticles = MAX_CULLING_PARTICLES;

    m_staticObjectFlags.reserve(MAX_CULLING_OBJECTS);
    m_staticDrawArgsData.reserve(MAX_CULLING_OBJECTS);
    m_staticMaterialIDData.reserve(MAX_CULLING_OBJECTS);
    m_staticInstanceData.reserve(MAX_CULLING_OBJECTS);
    m_staticInstanceIdentities.reserve(MAX_CULLING_OBJECTS);

    m_dynamicObjectFlags.reserve(MAX_CULLING_OBJECTS);
    m_dynamicMaterialIDData.reserve(MAX_CULLING_OBJECTS);
    m_dynamicInstanceData.reserve(MAX_CULLING_OBJECTS);

    m_particleData.reserve(MAX_CULLING_PARTICLES);
}

GPUCullingManager::~GPUCullingManager()
{
    Shutdown();
}

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::Initialize(fg::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [GPUCulling] NVRHI device not available");
        return;
    }

    m_computeEnabled = true;
    CreateBuffers(device);
    CreateDebugResources(device);
    CreateParticleResources(device);

    resources::FGResourceManager* resourceManager = device->GetFGResourceManager();
    m_residency.Initialize(device, resourceManager ? resourceManager->GetIOService() : nullptr);

    m_initialized = true;
    Msg("* [GPUCulling] Initialized (max objects: %d, max particles: %d)", m_maxObjects, m_maxParticles);
}

void GPUCullingManager::CreateBuffers(fg::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto makeInstanceBuffer = [&](const char* name, u32 count) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = u64(count) * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        nvrhi::BufferHandle buffer = nvDevice->createBuffer(desc);
        R_ASSERT2(buffer, name);
        return buffer;
    };

    m_maxTerrainObjects = UINT32_MAX;

    m_maxTransparentObjects = 4096;
    m_transparentInstanceBuffer = makeInstanceBuffer("GPUCull_Transparent_InstanceData", m_maxTransparentObjects);
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Transparent_DrawArgs";
        desc.byteSize = u64(m_maxTransparentObjects) * sizeof(IndirectDrawArgs);
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::IndirectArgument;
        desc.keepInitialState = true;
        m_transparentDrawArgsBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_transparentDrawArgsBuffer, "Failed to create transparent draw args buffer");
    }

    {
        nvrhi::BufferDesc fadeDesc;
        fadeDesc.debugName = "ClusterCull_NeutralFade";
        fadeDesc.byteSize = sizeof(u32);
        fadeDesc.structStride = sizeof(u32);
        fadeDesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        fadeDesc.keepInitialState = true;
        m_neutralFadeBuffer = nvDevice->createBuffer(fadeDesc);
        m_neutralFadeZeroed = false;
    }

    {
        nvrhi::TextureDesc desc;
        desc.debugName = "GPUCull_DummyHiZ";
        desc.width = 1;
        desc.height = 1;
        desc.format = nvrhi::Format::R32_FLOAT;
        desc.isShaderResource = true;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        m_dummyHiZ = nvDevice->createTexture(desc);
    }

    auto makeArgsBuffer = [&](const char* name, u64 byteSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = byteSize;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        nvrhi::BufferHandle buffer = nvDevice->createBuffer(desc);
        R_ASSERT2(buffer, name);
        return buffer;
    };
    m_clusterArgsBuffer = makeArgsBuffer("ClusterCull_Args", sizeof(u32) * 8);
    m_clusterTerrainArgsBuffer = makeArgsBuffer("ClusterCull_TerrainArgs", sizeof(u32) * 8);
    m_clusterArgsBuffer2 = makeArgsBuffer("ClusterCull_RetestArgs", sizeof(u32) * 8);
    m_clusterTerrainArgsBuffer2 = makeArgsBuffer("ClusterCull_RetestTerrainArgs", sizeof(u32) * 8);
    m_clusterSwArgsBuffer = makeArgsBuffer("ClusterCull_SwArgs", sizeof(u32) * 8);
    m_clusterSwArgsBuffer2 = makeArgsBuffer("ClusterCull_RetestSwArgs", sizeof(u32) * 8);
    m_clusterRetestDispatchArgs = makeArgsBuffer("ClusterCull_RetestDispatchArgs", sizeof(u32) * 16);
    m_clusterQueueArgsBuffer = makeArgsBuffer("ClusterCull_QueueArgs", sizeof(u32) * 4 * kClusterQueueArgsSlots);

    CreateSkinnedBuffers(device);
}

void GPUCullingManager::CreateSkinnedBuffers(fg::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GlobalBoneBuffer";
        desc.byteSize = MAX_TOTAL_BONES * BONE_STRIDE;
        desc.structStride = BONE_STRIDE;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_globalBoneBuffer = nvDevice->createBuffer(desc);
        if (!m_globalBoneBuffer) {
            Msg("! [GPUCulling] Failed to create global bone buffer");
            return;
        }
    }

    m_boneStagingBuffer.resize(MAX_TOTAL_BONES);
    m_boneBufferInitialized = true;

    EnsureSkinnedCapacity(1024);
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedEntries";
        desc.byteSize = u64(SKINNED_ENTRY_CAPACITY) * sizeof(GPUClusterEntry);
        desc.structStride = sizeof(GPUClusterEntry);
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        m_skinnedEntryBuffer = nvDevice->createBuffer(desc);
        m_skinnedEntryCapacity = m_skinnedEntryBuffer ? SKINNED_ENTRY_CAPACITY : 0;
    }
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedHudEntries";
        desc.byteSize = u64(SKINNED_HUD_ENTRY_CAPACITY) * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedHudEntryBuffer = nvDevice->createBuffer(desc);
        m_skinnedHudEntryCapacity = m_skinnedHudEntryBuffer ? SKINNED_HUD_ENTRY_CAPACITY : 0;
    }
    m_skinnedEnabled = m_skinnedRecordsBuffer && m_skinnedEntryBuffer;
    Msg("* [GPUCulling] Skinned upload buffers created (max: %u objects, %u bones)",
        m_maxSkinnedObjects, MAX_TOTAL_BONES);
}

void GPUCullingManager::EnsureSkinnedForwardBuffers(nvrhi::IDevice* nvDevice)
{
    if (m_skinnedForwardCapacity >= m_skinnedForwardCount)
        return;
    const u32 capacity = u32(std::min<u64>(UINT32_MAX,
        std::max<u64>(m_skinnedForwardCount, std::max<u64>(u64(m_skinnedForwardCapacity) * 2, 2048))));
    nvrhi::BufferDesc args;
    args.debugName = "GPUCull_SkinnedForward_DrawArgs";
    args.byteSize = u64(capacity) * sizeof(IndirectDrawArgs);
    args.isDrawIndirectArgs = true;
    args.initialState = nvrhi::ResourceStates::IndirectArgument;
    args.keepInitialState = true;
    m_skinnedForwardArgsBuffer = nvDevice->createBuffer(args);
    nvrhi::BufferDesc instances;
    instances.debugName = "GPUCull_SkinnedForward_InstanceData";
    instances.byteSize = u64(capacity) * sizeof(GPUInstanceData);
    instances.structStride = sizeof(GPUInstanceData);
    instances.initialState = nvrhi::ResourceStates::ShaderResource;
    instances.keepInitialState = true;
    m_skinnedForwardInstanceBuffer = nvDevice->createBuffer(instances);
    if (!m_skinnedForwardArgsBuffer || !m_skinnedForwardInstanceBuffer)
        FATAL("[GPUCulling] skinned forward buffer allocation failed");
    m_skinnedForwardCapacity = capacity;
}

void GPUCullingManager::EnsureForwardBuffers(nvrhi::IDevice* device)
{
    if (m_transparentObjectCount > m_maxTransparentObjects)
    {
        const u32 capacity = u32(std::min<u64>(UINT32_MAX,
            std::max<u64>(m_transparentObjectCount, u64(m_maxTransparentObjects) * 2)));
        auto argsDesc = m_transparentDrawArgsBuffer->getDesc();
        argsDesc.byteSize = u64(capacity) * sizeof(IndirectDrawArgs);
        auto instanceDesc = m_transparentInstanceBuffer->getDesc();
        instanceDesc.byteSize = u64(capacity) * sizeof(GPUInstanceData);
        auto args = device->createBuffer(argsDesc);
        auto instances = device->createBuffer(instanceDesc);
        R_ASSERT2(args && instances, "Cannot allocate complete rigid forward draw buffers");
        m_transparentDrawArgsBuffer = args;
        m_transparentInstanceBuffer = instances;
        m_maxTransparentObjects = capacity;
    }
    const u32 count = std::max({ m_transparentObjectCount, m_skinnedForwardCount, 1u });
    if (count <= m_forwardDrawIndexCapacity)
        return;
    const u32 capacity = u32(std::min<u64>(UINT32_MAX,
        std::max<u64>(count, u64(m_forwardDrawIndexCapacity) * 2)));
    nvrhi::BufferDesc desc;
    desc.byteSize = u64(capacity) * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.isVertexBuffer = true;
    desc.debugName = "ForwardDrawIndices";
    desc.initialState = nvrhi::ResourceStates::VertexBuffer;
    desc.keepInitialState = true;
    m_forwardDrawIndexBuffer = device->createBuffer(desc);
    R_ASSERT2(m_forwardDrawIndexBuffer, "Cannot allocate complete forward draw indices");
    m_forwardDrawIndexCapacity = capacity;
    m_forwardDrawIndices.resize(capacity);
    for (u32 i = 0; i < capacity; ++i)
        m_forwardDrawIndices[i] = i;
}

void GPUCullingManager::EnsureSkinnedCapacity(u32 count)
{
    if (count <= m_maxSkinnedObjects)
        return;

    const u32 capacity = std::max(count, m_maxSkinnedObjects * 2);
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedRecords";
        desc.byteSize = u64(capacity) * sizeof(SkinnedDrawRecord);
        desc.structStride = sizeof(SkinnedDrawRecord);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedRecordsBuffer = nvDevice->createBuffer(desc);
    }

    m_maxSkinnedObjects = capacity;
    m_skinnedRecordsData.reserve(capacity);
    for (auto& history : m_skinnedHistory)
        history.reserve(capacity);
}

void GPUCullingManager::Shutdown()
{
    UnloadLevel();
    m_preskinPipeline = nullptr;
    m_preskinLayout = nullptr;
    m_preskinFailed = false;
    m_maxSkinnedObjects = 0;

    m_transparentInstanceBuffer = nullptr;
    m_transparentDrawArgsBuffer = nullptr;
    m_staticObjectCount = 0;
    m_dynamicObjectCount = 0;
    m_transparentObjectCount = 0;
    m_maxTransparentObjects = 0;
    m_staticUploaded = false;

    m_residency.Shutdown();

    m_clusterSet = {};
    m_clusterArgsBuffer = nullptr;
    m_clusterTerrainArgsBuffer = nullptr;
    m_clusterArgsBuffer2 = nullptr;
    m_clusterTerrainArgsBuffer2 = nullptr;
    m_clusterSwArgsBuffer = nullptr;
    m_clusterSwArgsBuffer2 = nullptr;
    m_clusterRetestDispatchArgs = nullptr;
    m_clusterQueueArgsBuffer = nullptr;
    m_geoInstanceData.clear();
    m_clusterRefData.clear();
    m_dynamicGeoInstanceData.clear();
    m_dynamicRefData.clear();
    m_neutralFadeBuffer = nullptr;
    m_neutralFadeZeroed = false;
    m_dummyHiZ = nullptr;

    m_debugBuffer = nullptr;
    m_debugComputeParamsCB = fg::BufferHandle();
    m_debugGraphicsParamsCB = fg::BufferHandle();
    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_particleBuffer = nullptr;

    m_staticDataCached = false;
    m_terrainDataCached = false;

    m_initialized = false;
    m_computeEnabled = false;

    for (u32 i = 0; i < STATS_READBACK_SLOTS; ++i)
        m_statsReadbackBuffers[i] = nullptr;
    m_statsWriteSlot = 0;
    m_statsScheduled = 0;
    m_cullingStats = CullingStats();
}

// ═══════════════════════════════════════════════════════
//  STATS READBACK (for profiling)
// ═══════════════════════════════════════════════════════

void GPUCullingManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_computeEnabled || !m_device)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    nvrhi::BufferHandle& slot = m_statsReadbackBuffers[m_statsWriteSlot];
    if (!slot)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32) * kClusterCountWords;
        desc.debugName = "CullingStatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = nvDevice->createBuffer(desc);

        if (!slot)
            return;
    }

    if (m_clusterSet.countBuffer)
        cmdList->copyBuffer(slot, 0, m_clusterSet.countBuffer, 0, sizeof(u32) * kClusterCountWords);

    m_statsWriteSlot = (m_statsWriteSlot + 1) % STATS_READBACK_SLOTS;
    if (m_statsScheduled < STATS_READBACK_SLOTS)
        ++m_statsScheduled;
}

void GPUCullingManager::ProcessStatsReadback()
{
    if (m_statsScheduled < STATS_READBACK_SLOTS || !m_device)
        return;

    static u32 frameCounter = 0;
    frameCounter++;
    const u32 throttleInterval = xray::profiler::GetCPUProfiler().GetThrottleInterval();
    if ((frameCounter % throttleInterval) != 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    nvrhi::IBuffer* oldest = m_statsReadbackBuffers[m_statsWriteSlot];
    void* mappedData = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        const u32* counts = static_cast<const u32*>(mappedData);
        m_cullingStats.clusterVisible = counts[0] + counts[5];
        m_cullingStats.clusterTerrainVisible = counts[1] + counts[6];
        m_cullingStats.clusterTrianglesDrawn = counts[2] + counts[7];
        m_cullingStats.clusterTerrainTrianglesDrawn = counts[3] + counts[8];
        m_cullingStats.clusterCandidates = counts[4];
        m_cullingStats.clusterRetestVisible = counts[5] + counts[6];
        m_cullingStats.clusterDeferredNodes = counts[14];
        m_cullingStats.clusterDeferredInstances = counts[15];
        m_cullingStats.clusterLeafVisits = counts[16] + counts[17];
        m_cullingStats.clusterInstanceVisits = counts[18];
        m_cullingStats.clusterNodeVisits = counts[19];
        m_cullingStats.clusterOverflow = counts[9];

        nvDevice->unmapBuffer(oldest);
    }
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SCENE OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::PrepareSceneGeometry(const GeometryCollector* geometry)
{
    ZoneScopedN("GPUCull::PrepareSceneGeometry");

    if (!m_computeEnabled || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    const auto& staticBatches = geometry->GetStaticBatches();
    const u32 totalBatches = static_cast<u32>(batches.size() + staticBatches.size());

    if (totalBatches == 0) {
        m_staticObjectCount = 0;
        m_dynamicObjectCount = 0;
        m_transparentResidualCount = 0;
        m_rtRayOnlyDynamicCount = 0;
        m_dynamicObjectFlags.clear();
        m_dynamicMaterialIDData.clear();
        m_dynamicInstanceData.clear();
        m_dynamicBatchKeys.clear();
        m_dynamicInstanceIdentities.clear();
        m_clusterSet.dynamicRefCount = 0;
        m_clusterSet.dynamicInstanceCount = 0;
        m_clusterSet.dynamicResidualCount = 0;
        return;
    }

    if (!m_staticDataCached) {
        m_staticObjectFlags.clear();
        m_staticObjectFlags.reserve(totalBatches);
        m_staticDrawArgsData.clear();
        m_staticDrawArgsData.reserve(totalBatches);
        m_staticMaterialIDData.clear();
        m_staticMaterialIDData.reserve(totalBatches);
        m_staticInstanceData.clear();
        m_staticInstanceData.reserve(totalBatches);
        m_staticInstanceIdentities.clear();
        m_staticInstanceIdentities.reserve(totalBatches);
        m_staticBatchVertexCounts.clear();
        m_staticBatchVertexCounts.reserve(totalBatches);
        m_staticBatchKeys.clear();
        m_staticBatchKeys.reserve(totalBatches);
        m_staticLightmapData.clear();
        m_staticLightmapData.reserve(totalBatches);
    }

    m_dynamicObjectFlags.clear();
    m_dynamicObjectFlags.reserve(totalBatches);
    m_dynamicMaterialIDData.clear();
    m_dynamicMaterialIDData.reserve(totalBatches);
    m_dynamicInstanceData.clear();
    m_dynamicInstanceData.reserve(totalBatches);
    m_dynamicBatchKeys.clear();
    m_dynamicBatchKeys.reserve(totalBatches);
    m_dynamicInstanceIdentities.clear();
    m_dynamicInstanceIdentities.reserve(totalBatches);

    if (!m_terrainDataCached) {
        m_terrainDrawArgsData.clear();
        m_terrainDrawArgsData.reserve(totalBatches / 4);
        m_terrainMaterialIDData.clear();
        m_terrainMaterialIDData.reserve(totalBatches / 4);
        m_terrainInstanceData.clear();
        m_terrainInstanceData.reserve(totalBatches / 4);
        m_terrainInstanceIdentities.clear();
        m_terrainInstanceIdentities.reserve(totalBatches / 4);
        m_terrainBatchKeys.clear();
        m_terrainBatchKeys.reserve(totalBatches / 4);
        m_terrainLightmapData.clear();
        m_terrainLightmapData.reserve(totalBatches / 4);
    }

    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();
    m_transparentInstanceIdentities.clear();
    m_transparentKeys.clear();
    m_transparentRanges.clear();
    m_transparentResidualCount = 0;
    m_rtRayOnlyDynamicCount = 0;

    auto batchFlags = [](const GeometryBatch& batch) -> u32 {
        return MaterialObjectFlags(batch.bindlessMaterialID) | (batch.isShadowOnly ? GPU_OBJECT_SHADOW_ONLY : 0u)
            | (batch.isStatic && !batch.isSkinned ? GPU_OBJECT_BAKED_HEMI : 0u);
    };

    auto batchKey = [](const GeometryBatch& batch) {
        ClusterMeshKey key = {};
        if (batch.megaBufferAlloc.valid) {
            key.vertexOffset = batch.megaBufferAlloc.vertexOffset;
            key.indexOffset = batch.megaBufferAlloc.indexOffset;
            key.vertexCount = batch.megaBufferAlloc.vertexCount;
            key.indexCount = batch.megaBufferAlloc.indexCount;
        }
        return key;
    };

    auto appendInstance = [](const GeometryBatch& batch, u32 flags, u32 materialID,
                             xr_vector<u32>& materialIDData,
                             xr_vector<GPUInstanceData>& instanceData) {
        materialIDData.push_back(materialID);

        GPUInstanceData inst;
        inst.world = batch.worldMatrix;
        inst.materialID = materialID;
        inst.flags = flags;
        inst.hemiScale = batch.hemiScale;
        inst.hemiBias = batch.hemiBias;
        instanceData.push_back(inst);
    };

    auto appendBatch = [&](const GeometryBatch& batch, u32 flags, u32 materialID,
                           xr_vector<IndirectDrawArgs>& drawArgsData,
                           xr_vector<u32>& materialIDData,
                           xr_vector<GPUInstanceData>& instanceData) {
        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.megaBufferAlloc.valid ? batch.indexCount : 0u;
        args.instanceCount = 1;
        args.startIndexLocation = batch.megaBufferAlloc.valid ? batch.megaBufferAlloc.indexOffset : 0u;
        args.baseVertexLocation = batch.megaBufferAlloc.valid ? static_cast<s32>(batch.megaBufferAlloc.vertexOffset) : 0;
        args.startInstanceLocation = static_cast<u32>(drawArgsData.size());
        drawArgsData.push_back(args);
        appendInstance(batch, flags, materialID, materialIDData, instanceData);
    };

    {
    ZoneScopedN("Upload::Rebuild");
    const bool rebuildStatics = !m_staticDataCached || !m_terrainDataCached;
    if (rebuildStatics) {
        for (const auto& batch : staticBatches) {
            if (batch.isSkinned)
                continue;

            if (batch.isTerrain) {
                if (m_terrainDataCached)
                    continue;
                appendBatch(batch, 0u, batch.terrainMaterialID,
                    m_terrainDrawArgsData, m_terrainMaterialIDData, m_terrainInstanceData);
                m_terrainInstanceIdentities.push_back(GeometryInstanceKey{ batch.renderableLifetimeID,
                    batch.visualLifetimeID, batch.geometrySubset });
                m_terrainBatchKeys.push_back(batchKey(batch));
                m_terrainLightmapData.push_back(batch.lightmapTexture);
                continue;
            }

            if (batch.IsStrictB2F())
                continue;

            if (m_staticDataCached)
                continue;

            const u32 flags = batchFlags(batch);
            m_staticObjectFlags.push_back(flags);
            appendBatch(batch, flags, batch.bindlessMaterialID,
                m_staticDrawArgsData, m_staticMaterialIDData, m_staticInstanceData);
            m_staticInstanceIdentities.push_back(GeometryInstanceKey{ batch.renderableLifetimeID,
                batch.visualLifetimeID, batch.geometrySubset });
            m_staticBatchVertexCounts.push_back(batch.megaBufferAlloc.valid ? batch.megaBufferAlloc.vertexCount : 0);
            m_staticBatchKeys.push_back(batchKey(batch));
            m_staticLightmapData.push_back(batch.lightmapTexture);
        }
        ++m_staticBuildCount;
    }

    for (u32 index : geometry->GetStaticTransparentIndices()) {
        const auto& batch = staticBatches[index];
        if (!batch.megaBufferAlloc.valid)
            ++m_transparentResidualCount;
        appendBatch(batch, batchFlags(batch), batch.bindlessMaterialID,
            m_transparentDrawArgsData, m_transparentMaterialIDData, m_transparentInstanceData);
        m_transparentInstanceIdentities.push_back(GeometryInstanceKey{ batch.renderableLifetimeID,
            batch.visualLifetimeID, batch.geometrySubset });
        m_transparentKeys.push_back(TransparentKeyForMaterial(batch.bindlessMaterialID));
    }

    for (const auto& batch : batches) {
        if (batch.isSkinned)
            continue;

        if (batch.isTerrain) {
            if (m_terrainDataCached)
                continue;
            appendBatch(batch, 0u, batch.terrainMaterialID,
                m_terrainDrawArgsData, m_terrainMaterialIDData, m_terrainInstanceData);
            m_terrainInstanceIdentities.push_back(GeometryInstanceKey{ batch.renderableLifetimeID,
                batch.visualLifetimeID, batch.geometrySubset });
            m_terrainBatchKeys.push_back(batchKey(batch));
            m_terrainLightmapData.push_back(batch.lightmapTexture);
            continue;
        }

        if (batch.IsStrictB2F()) {
            if (!batch.megaBufferAlloc.valid)
                ++m_transparentResidualCount;
            appendBatch(batch, batchFlags(batch), batch.bindlessMaterialID,
                m_transparentDrawArgsData, m_transparentMaterialIDData, m_transparentInstanceData);
            m_transparentInstanceIdentities.push_back(GeometryInstanceKey{ batch.renderableLifetimeID,
                batch.visualLifetimeID, batch.geometrySubset });
            m_transparentKeys.push_back(TransparentKeyForMaterial(batch.bindlessMaterialID));
            continue;
        }

        const u32 flags = batchFlags(batch);
        m_dynamicObjectFlags.push_back(flags);
        if (flags & GPU_OBJECT_SHADOW_ONLY)
            ++m_rtRayOnlyDynamicCount;
        appendInstance(batch, flags, batch.bindlessMaterialID,
            m_dynamicMaterialIDData, m_dynamicInstanceData);
        m_dynamicBatchKeys.push_back(batchKey(batch));
        GeometryInstanceKey identity;
        identity.renderable = batch.renderableLifetimeID;
        identity.visual = batch.visualLifetimeID;
        identity.subset = batch.geometrySubset;
        m_dynamicInstanceIdentities.push_back(identity);
    }
    }

    if (m_staticInstanceData.size() > UINT32_MAX || m_dynamicInstanceData.size() > UINT32_MAX)
        FATAL("[GPUCulling] scene submission count exceeds the addressable range");

    m_staticObjectCount = static_cast<u32>(m_staticInstanceData.size());
    m_dynamicObjectCount = static_cast<u32>(m_dynamicInstanceData.size());

    const u64 forwardVertices = u64(m_forwardVertexBase) + m_forwardVertices.size();
    const u64 forwardIndices = u64(m_forwardIndexBase) + m_forwardIndices.size();
    R_ASSERT(forwardVertices <= INT32_MAX && forwardIndices <= UINT32_MAX);
    if (!EnsureMegaCapacity(u32(forwardVertices), u32(forwardIndices)))
        FATAL("[GPUCulling] forward geometry could not be given native storage");

    PartitionTransparentDraws(m_transparentDrawArgsData, &m_transparentMaterialIDData,
        m_transparentInstanceData, m_transparentKeys,
        m_transparentRanges, m_transparentDrawScratch, nullptr, &m_transparentInstanceIdentities);
    m_transparentObjectCount = u32(m_transparentDrawArgsData.size());
    m_transparentNativeArgs = m_transparentDrawArgsData;
    for (auto& args : m_transparentNativeArgs)
    {
        if (args.indexCountPerInstance == 0)
            continue;
        ClusterMeshKey key = {};
        key.vertexOffset = u32(args.baseVertexLocation);
        key.indexOffset = args.startIndexLocation;
        key.indexCount = args.indexCountPerInstance;
        const auto it = m_forwardAllocations.find(key);
        R_ASSERT2(it != m_forwardAllocations.end(), "Forward draw has no retained source range");
        args.baseVertexLocation = s32(it->second.vertexOffset);
        args.startIndexLocation = it->second.indexOffset;
    }

    if (m_geometryTablesDirty) {
        m_clusterSet.metaBuffer = nullptr;
        m_clusterSet.memberBuffer = nullptr;
        m_clusterSet.assetNodeBuffer = nullptr;
        m_clusterSet.uploaded = false;
        m_geometryTablesDirty = false;
    }

    if (!m_staticUploaded) {
        BuildStaticGeometryInstances(geometry);
        m_geometryHistoryFrame = 0;
    }
    m_staticHistoryValid = m_geometryHistoryFrame != 0 && m_geometryHistoryFrame + 1u == Device.dwFrame;
    m_geometryHistoryFrame = Device.dwFrame;
    BuildDynamicGeometryInstances(geometry);
    m_clusterSet.traversalDepth = m_clusterDAG.MaxAssetNodeDepth();
    if (m_clusterSet.traversalDepth > kMaxClusterTraversalDepth)
        FATAL_F("[GPUCulling] asset hierarchy depth %u exceeds the supported traversal depth %u",
            m_clusterSet.traversalDepth, kMaxClusterTraversalDepth);
    if (!EnsureClusterStreamBuffers(m_device->GetNVRHIDevice())
        || !EnsureGeometryTableBuffers(m_device->GetNVRHIDevice()))
        FATAL("[GPUCulling] geometry resources could not be prepared");
}

void GPUCullingManager::UploadSceneObjects(fg::RenderContext* ctx)
{
    ZoneScopedN("GPUCull::UploadSceneObjects");

    if (!m_computeEnabled || !m_device)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (!m_forwardVertices.empty() || !m_forwardIndices.empty() || !m_forwardDrawIndices.empty()
        || m_megaCopySourceVB || m_megaCopySourceIB)
    {
        R_ASSERT(GEnv.Backend && GEnv.Backend->SupportsSubmissionLeases());
        auto& upload = m_forwardUploads.emplace_back();
        upload.lease = GEnv.Backend->OpenSubmissionLease();
        R_ASSERT(upload.lease);
        upload.vertices = m_megaVertexBuffer;
        upload.indices = m_megaIndexBuffer;
        upload.drawIndices = m_forwardDrawIndexBuffer;
        upload.copyVertices = std::move(m_megaCopySourceVB);
        upload.copyIndices = std::move(m_megaCopySourceIB);
        upload.vertexStaging.swap(m_forwardVertices);
        upload.indexStaging.swap(m_forwardIndices);
        upload.drawIndexStaging.swap(m_forwardDrawIndices);
        if (upload.copyVertices && m_megaCopyVertexCount)
            cmdList->copyBuffer(upload.vertices, 0, upload.copyVertices, 0,
                u64(m_megaCopyVertexCount) * sizeof(ForwardVertex));
        if (upload.copyIndices && m_megaCopyIndexCount)
            cmdList->copyBuffer(upload.indices, 0, upload.copyIndices, 0,
                u64(m_megaCopyIndexCount) * sizeof(u32));
        if (!upload.vertexStaging.empty())
            cmdList->writeBuffer(upload.vertices, upload.vertexStaging.data(),
                upload.vertexStaging.size() * sizeof(ForwardVertex),
                u64(m_forwardVertexBase) * sizeof(ForwardVertex));
        if (!upload.indexStaging.empty())
            cmdList->writeBuffer(upload.indices, upload.indexStaging.data(),
                upload.indexStaging.size() * sizeof(u32),
                u64(m_forwardIndexBase) * sizeof(u32));
        if (!upload.drawIndexStaging.empty())
            cmdList->writeBuffer(upload.drawIndices, upload.drawIndexStaging.data(),
                upload.drawIndexStaging.size() * sizeof(u32));
        m_forwardVertexBase += u32(upload.vertexStaging.size());
        m_forwardIndexBase += u32(upload.indexStaging.size());
    }
    m_megaDataUploaded = m_megaBuffersReady;

    if (m_neutralFadeBuffer && !m_neutralFadeZeroed) {
        u32 neutral = 0;
        cmdList->writeBuffer(m_neutralFadeBuffer, &neutral, sizeof(u32));
        m_neutralFadeZeroed = true;
    }

    UploadGeometryTables(cmdList);

    if (!m_staticUploaded && (m_staticObjectCount > 0 || m_clusterSet.instanceCount > 0))
    {
        m_staticUploaded = true;
        m_staticDataCached = true;
        Msg("* [GPUCulling] Static object data prepared: %u objects, %u cluster instances",
            m_staticObjectCount, m_clusterSet.instanceCount);
    }

    if (m_clusterSet.uploaded) {
        ZoneScopedN("Upload::GeometryInstances");
        if (m_clusterSet.dynamicInstanceCount > 0 && m_clusterSet.instanceBuffer)
            cmdList->writeBuffer(m_clusterSet.instanceBuffer, m_dynamicGeoInstanceData.data(),
                u64(m_clusterSet.dynamicInstanceCount) * sizeof(GPUGeoInstance),
                u64(m_clusterSet.instanceCount) * sizeof(GPUGeoInstance));
        if (m_clusterSet.dynamicRefCount > 0 && m_clusterSet.refBuffer)
            cmdList->writeBuffer(m_clusterSet.refBuffer, m_dynamicRefData.data(),
                u64(m_clusterSet.dynamicRefCount) * 2ull * sizeof(u32),
                u64(m_clusterSet.refCount) * 2ull * sizeof(u32));
    }

    m_terrainObjectCount = std::min(static_cast<u32>(m_terrainInstanceData.size()), m_maxTerrainObjects);

    if (m_terrainObjectCount > 0 && !m_terrainDataCached) {
        m_terrainDataCached = true;
        Msg("* [GPUCulling] Terrain data cached: %u objects", m_terrainObjectCount);
    }

    if (m_transparentObjectCount > 0 && m_transparentInstanceBuffer && m_transparentDrawArgsBuffer) {
        ZoneScopedN("Upload::TransparentWrite");
        cmdList->writeBuffer(m_transparentDrawArgsBuffer,
            m_transparentNativeArgs.data(),
            m_transparentObjectCount * sizeof(IndirectDrawArgs));
        cmdList->setBufferState(m_transparentDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

        cmdList->writeBuffer(m_transparentInstanceBuffer,
            m_transparentInstanceData.data(),
            m_transparentObjectCount * sizeof(GPUInstanceData));
        cmdList->setBufferState(m_transparentInstanceBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    }
}

void GPUCullingManager::InvalidateStaticCullingData()
{
    m_staticDataCached = false;
    m_staticUploaded = false;
    m_staticObjectCount = 0;
    ++m_staticBuildCount;

    m_staticObjectFlags.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_staticInstanceData.clear();
    m_staticInstanceIdentities.clear();
    m_staticBatchVertexCounts.clear();
    m_staticBatchKeys.clear();
    m_staticLightmapData.clear();

    m_terrainDataCached = false;
    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainInstanceIdentities.clear();
    m_terrainBatchKeys.clear();
    m_terrainLightmapData.clear();
    m_terrainObjectCount = 0;

    m_clusterSet = {};
    m_geoInstanceData.clear();
    m_clusterRefData.clear();
    m_dynamicGeoInstanceData.clear();
    m_dynamicRefData.clear();
    m_dynamicHistory[0].clear();
    m_dynamicHistory[1].clear();
    m_runtimeVertices.clear();
    m_runtimeIndices.clear();
    m_geometryTablesDirty = false;
    m_geometryHistoryFrame = 0;
    m_staticHistoryValid = false;

    Msg("* [GPUCulling] Static culling data invalidated");
}

void GPUCullingManager::InvalidateShadersAndPipelines()
{
    m_clusterInstancePipeline = nullptr;
    m_clusterInstanceLayout = nullptr;
    m_clusterNodePipeline = nullptr;
    m_clusterNodeLayout = nullptr;
    m_clusterLeafPipeline = nullptr;
    m_clusterLeafLayout = nullptr;
    m_clusterQueueArgsPipeline = nullptr;
    m_clusterQueueArgsLayout = nullptr;
    m_clusterArgsPipeline = nullptr;
    m_clusterArgsLayout = nullptr;

    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;
    m_preskinPipeline = nullptr;
    m_preskinLayout = nullptr;
    m_preskinFailed = false;
    for (auto& format : m_preskinBindingSets)
        for (auto& binding : format)
            binding = {};

    m_initialized = false;
    m_computeEnabled = false;
    m_skinnedEnabled = false;

    m_staticDataCached = false;
    m_terrainDataCached = false;
    m_staticUploaded = false;
    m_clusterSet = {};
    m_geoInstanceData.clear();
    m_clusterRefData.clear();
    m_dynamicGeoInstanceData.clear();
    m_dynamicRefData.clear();
    m_dynamicHistory[0].clear();
    m_dynamicHistory[1].clear();

    Msg("* [GPUCulling] Shaders and pipelines invalidated for hot-reload");
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SKINNED OBJECTS
// ═══════════════════════════════════════════════════════

static const u32 kSkinnedKindOrder[4] = { 0u, 2u, 1u, 3u };

void GPUCullingManager::EnsureSkinnedChunkBuffer(nvrhi::IDevice* nvDevice, u32 chunkTotal)
{
    if (!nvDevice || m_skinnedChunkCapacity >= chunkTotal)
        return;
    u64 grown = std::max<u64>(u64(m_skinnedChunkCapacity) * 2ull, 1024ull);
    while (grown < chunkTotal)
        grown *= 2ull;
    const u32 capacity = u32(std::min<u64>(grown, UINT32_MAX));
    nvrhi::BufferDesc desc;
    desc.debugName = "GPUCull_SkinnedChunks";
    desc.byteSize = u64(capacity) * sizeof(SkinnedChunk);
    desc.structStride = sizeof(SkinnedChunk);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    m_skinnedChunkBuffer = nvDevice->createBuffer(desc);
    if (!m_skinnedChunkBuffer)
        FATAL_F("[GPUCulling] skinned chunk buffer allocation for %u chunks failed", capacity);
    m_skinnedChunkCapacity = capacity;
}

void GPUCullingManager::EnsureSkinnedEntryBuffers(nvrhi::IDevice* nvDevice, u32 entryTotal, u32 hudEntryTotal)
{
    if (!nvDevice)
        return;

    if (m_skinnedEntryCapacity < entryTotal)
    {
        u64 grown = std::max<u64>(u64(m_skinnedEntryCapacity) * 2ull, SKINNED_ENTRY_CAPACITY);
        while (grown < entryTotal)
            grown *= 2ull;
        if (entryTotal > kSkinnedEntryLimit)
            FATAL_F("[GPUCulling] skinned entry demand %u exceeds the %u representable visibility entries",
                entryTotal, kSkinnedEntryLimit);
        const u32 capacity = u32(std::min<u64>(grown, kSkinnedEntryLimit));
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedEntries";
        desc.byteSize = u64(capacity) * sizeof(GPUClusterEntry);
        desc.structStride = sizeof(GPUClusterEntry);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedEntryBuffer = nvDevice->createBuffer(desc);
        if (!m_skinnedEntryBuffer)
            FATAL_F("[GPUCulling] skinned entry buffer allocation for %u entries failed", capacity);
        m_skinnedEntryCapacity = capacity;
    }

    if (m_skinnedHudEntryCapacity < hudEntryTotal)
    {
        u64 grown = std::max<u64>(u64(m_skinnedHudEntryCapacity) * 2ull, SKINNED_HUD_ENTRY_CAPACITY);
        while (grown < hudEntryTotal)
            grown *= 2ull;
        const u32 capacity = u32(std::min<u64>(grown, kSkinnedEntryLimit));
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedHudEntries";
        desc.byteSize = u64(capacity) * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedHudEntryBuffer = nvDevice->createBuffer(desc);
        if (!m_skinnedHudEntryBuffer)
            FATAL_F("[GPUCulling] skinned HUD entry buffer allocation for %u entries failed", capacity);
        m_skinnedHudEntryCapacity = capacity;
    }
}

CKinematics* GPUCullingManager::GetBatchSkeleton(const GeometryBatch& batch)
{
    const u32 visualType = batch.visual ? batch.visual->getType() : 0;
    if (visualType == MT_SKELETON_GEOMDEF_ST)
        return static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    if (visualType == MT_SKELETON_GEOMDEF_PM)
        return static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
    return nullptr;
}

void GPUCullingManager::PrepareSkinnedGeometry(const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches, decals::OverlayManager* overlayMgr)
{
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
        for (auto& batches : m_skinnedBuckets[f].kinds)
            batches.clear();
    m_skinnedForwardArgsData.clear();
    m_skinnedForwardInstanceData.clear();
    m_skinnedForwardKeys.clear();
    m_skinnedForwardSort.clear();
    m_skinnedForwardRanges.clear();
    m_skinnedForwardCount = 0;
    m_skinnedObjectCount = 0;
    m_skinnedEntryCount = 0;
    m_skinnedVisibleEntryCount = 0;
    m_skinnedHudEntryCount = 0;
    m_skinnedPrepared = false;
    m_skinnedPreparedVertexCount = 0;
    if (!IsSkinnedEnabled() || !geometry || !m_device)
        return;

    u64 pooledTotal = 0;
    u64 chunkDemand = 0;
    u64 entryDemand = 0;
    u64 hudEntryDemand = 0;
    u64 vertexDemand = 0;

    auto resolveSkinnedKind = [](const GeometryBatch& batch, u8& kind) -> bool
    {
        const bool forward = (MaterialObjectFlags(batch.bindlessMaterialID) & GPU_OBJECT_NO_RESOLVE) != 0;
        if (forward && kind == 0u)
            kind = 3u;
        const bool formatValid = batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
            && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
        if (!formatValid)
            return false;
        if (!forward || kind == 3u)
            return true;
        if (kind != 1u)
            return false;
        return MaterialCastsShadow(batch.bindlessMaterialID);
    };

    auto account = [&](const GeometryBatch& batch, u8 kind)
    {
        if (!batch.isSkinned)
            return;
        PrepareSkeletonPalette(GetBatchSkeleton(batch));
        if (kind != 2u)
            ++m_skinnedObjectCount;
        if (!resolveSkinnedKind(batch, kind))
            return;

        ++pooledTotal;
        vertexDemand += batch.vertexCount;
        chunkDemand += (u64(batch.vertexCount) + SKINNED_CHUNK_VERTICES - 1ull) / SKINNED_CHUNK_VERTICES;
        const u64 entries = (u64(batch.indexCount) + SKINNED_ENTRY_INDICES - 1ull) / SKINNED_ENTRY_INDICES;
        if (kind != 3u)
        {
            entryDemand += entries;
            if (kind == 2u)
                hudEntryDemand += entries;
        }
        else if (MaterialCastsShadow(batch.bindlessMaterialID))
            entryDemand += entries;
    };

    for (const auto& batch : geometry->GetBatches())
        account(batch, batch.isShadowOnly ? 1u : 0u);
    if (hudBatches)
    {
        for (const auto& batch : *hudBatches)
            account(batch, 2u);
    }

    if (m_currentBoneOffset > 0)
        m_skinnedPools.PrepareUploads(m_device->GetNVRHIDevice());

    if (pooledTotal == 0)
        return;
    if (vertexDemand > (1ull << 32) / sizeof(bindless::UnifiedVertex)
        || pooledTotal > UINT32_MAX || chunkDemand > UINT32_MAX
        || entryDemand > kSkinnedEntryLimit || hudEntryDemand > kSkinnedEntryLimit)
        FATAL("[GPUCulling] skinned working set exceeds the addressable range");

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!EnsurePreskinPipeline(nvDevice))
        FATAL("[GPUCulling] skinned deformation requires the preskin pipeline; no conventional skinned path exists");

    EnsureSkinnedCapacity(u32(pooledTotal));
    m_skinnedPreVBRecreated = EnsurePreskinBuffers(nvDevice, u32(vertexDemand)) || m_skinnedPreVBRecreated;
    EnsureSkinnedChunkBuffer(nvDevice, u32(chunkDemand));
    EnsureSkinnedEntryBuffers(nvDevice, u32(entryDemand), u32(hudEntryDemand));

    auto addBatch = [&](const GeometryBatch& batch, u8 kind) {
        if (!resolveSkinnedKind(batch, kind))
            return;

        CKinematics* skeleton = GetBatchSkeleton(batch);

        SkinnedBucket::Batch prepared{ &batch, GetPreparedSkeletonOffset(skeleton), 0, 0 };
        if (overlayMgr && skeleton) {
            auto sr = overlayMgr->GetSplatRange(skeleton);
            prepared.splatOffset = sr.offset;
            prepared.splatCount = sr.count;
        }
        m_skinnedBuckets[batch.skinnedPoolFormat].kinds[kind].push_back(prepared);
    };
    for (const auto& batch : geometry->GetBatches()) {
        if (batch.isSkinned)
            addBatch(batch, batch.isShadowOnly ? 1u : 0u);
    }
    if (hudBatches) {
        for (const auto& batch : *hudBatches) {
            if (batch.isSkinned)
                addBatch(batch, 2u);
        }
    }
    m_skinnedPreVBIndex ^= 1u;
    const bool preVBRecreated = m_skinnedPreVBRecreated;
    const bool historyValid = !preVBRecreated && m_skinnedHistoryFrame + 1u == Device.dwFrame;
    const auto& prevHistory = m_skinnedHistory[m_skinnedHistoryIndex];
    auto& nextHistory = m_skinnedHistory[m_skinnedHistoryIndex ^ 1u];
    nextHistory.clear();

    m_skinnedRecordsData.clear();
    m_skinnedChunkData.clear();
    m_skinnedEntryData.clear();
    m_skinnedShadowEntryData.clear();
    m_skinnedHudEntryData.clear();
    bool hudBoundsValid = false;
    u32 vertexTotal = 0;
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        const u32 formatIndexBase = m_skinnedPools.GetFormatIndexBase(f);
        m_skinnedChunkBase[f] = static_cast<u32>(m_skinnedChunkData.size());
        for (u32 kind : kSkinnedKindOrder) {
            for (const auto& prepared : m_skinnedBuckets[f].kinds[kind]) {
                const GeometryBatch& batch = *prepared.source;
                const u32 slot = static_cast<u32>(m_skinnedRecordsData.size());
                const u32 vertexCount = batch.vertexCount;
                SkinnedDrawRecord rec;
                rec.world = batch.worldMatrix;
                rec.boneOffset = prepared.boneOffset;
                rec.splatOffset = prepared.splatOffset;
                rec.splatCount = prepared.splatCount;
                rec.prevFirstVertex = 0xFFFFFFFFu;
                rec.bounds.set(batch.worldBoundsCenter.x, batch.worldBoundsCenter.y, batch.worldBoundsCenter.z, batch.worldBoundsRadius);
                SkinnedHistoryEntry identity;
                identity.visual = batch.visualLifetimeID;
                identity.renderable = batch.renderableLifetimeID;
                identity.firstVertex = vertexTotal;
                identity.slot = slot;
                if (historyValid) {
                    auto it = std::lower_bound(prevHistory.begin(), prevHistory.end(), identity);
                    if (it != prevHistory.end() && it->SameIdentity(identity))
                        rec.prevFirstVertex = it->firstVertex;
                }
                m_skinnedRecordsData.push_back(rec);
                if (kind == 2u) {
                    if (!hudBoundsValid) {
                        m_skinnedHudBounds = rec.bounds;
                        hudBoundsValid = true;
                    } else {
                        fg::passes::MergeBoundingSphere(m_skinnedHudBounds, rec.bounds);
                    }
                }
                nextHistory.push_back(identity);
                for (u32 v0 = 0; v0 < vertexCount; v0 += SKINNED_CHUNK_VERTICES) {
                    SkinnedChunk chunk;
                    chunk.slot = slot;
                    chunk.srcVertex = u32(batch.skinnedPoolBaseVertex) + v0;
                    chunk.dstVertex = vertexTotal + v0;
                    chunk.count = std::min(SKINNED_CHUNK_VERTICES, vertexCount - v0);
                    m_skinnedChunkData.push_back(chunk);
                }
                const u32 ibBase = formatIndexBase + batch.skinnedPoolFirstIndex;
                const bool castsShadow = MaterialCastsShadow(batch.bindlessMaterialID);
                if (kind == 3u)
                {
                    IndirectDrawArgs forwardArgs;
                    forwardArgs.indexCountPerInstance = batch.indexCount;
                    forwardArgs.instanceCount = 1;
                    forwardArgs.startIndexLocation = ibBase;
                    forwardArgs.baseVertexLocation = static_cast<s32>(vertexTotal);
                    forwardArgs.startInstanceLocation = static_cast<u32>(m_skinnedForwardArgsData.size());
                    m_skinnedForwardArgsData.push_back(forwardArgs);
                    GPUInstanceData inst;
                    inst.world.identity();
                    inst.materialID = batch.bindlessMaterialID;
                    inst.flags = GPU_OBJECT_SKINNED_FORWARD;
                    inst.hemiScale = 0.0f;
                    inst.hemiBias = 1.0f;
                    m_skinnedForwardInstanceData.push_back(inst);
                    m_skinnedForwardKeys.push_back(TransparentKeyForMaterial(batch.bindlessMaterialID));
                    m_skinnedForwardSort.push_back(batch.ssa);
                }
                if (kind != 3u || castsShadow)
                {
                    const bool shadowOnlyEntry = kind == 1u || kind == 3u;
                    GPUClusterEntry entry = {};
                    entry.sphere = rec.bounds;
                    entry.extent.set(rec.bounds.w, rec.bounds.w, rec.bounds.w, 0.0f);
                    entry.firstVertex = vertexTotal;
                    entry.batchIndex = slot;
                    entry.materialID = batch.bindlessMaterialID;
                    entry.flags = GPU_CLUSTER_ENTRY_SKINNED
                        | (kind == 2u ? GPU_CLUSTER_ENTRY_HUD : 0u)
                        | (shadowOnlyEntry ? GPU_CLUSTER_ENTRY_SHADOW_ONLY : 0u);
                    if (!castsShadow)
                        entry.flags |= GPU_CLUSTER_ENTRY_NO_SHADOW;
                    for (u64 i0 = 0; i0 < batch.indexCount; i0 += SKINNED_ENTRY_INDICES)
                    {
                        entry.indexCount = u32(std::min<u64>(SKINNED_ENTRY_INDICES, u64(batch.indexCount) - i0));
                        entry.ibFirst = ibBase + u32(i0);
                        if (kind == 2u)
                            m_skinnedHudEntryData.push_back(static_cast<u32>(m_skinnedEntryData.size()));
                        (shadowOnlyEntry ? m_skinnedShadowEntryData : m_skinnedEntryData).push_back(entry);
                    }
                }
                vertexTotal += vertexCount;
            }
        }
        m_skinnedChunkCount[f] = static_cast<u32>(m_skinnedChunkData.size()) - m_skinnedChunkBase[f];
    }
    m_skinnedPreparedVertexCount = vertexTotal;
    m_skinnedForwardCount = u32(m_skinnedForwardArgsData.size());
    PartitionTransparentDraws(m_skinnedForwardArgsData, nullptr, m_skinnedForwardInstanceData,
        m_skinnedForwardKeys, m_skinnedForwardRanges,
        m_transparentDrawScratch, &m_skinnedForwardSort);
    EnsureSkinnedForwardBuffers(nvDevice);
    m_skinnedVisibleEntryCount = static_cast<u32>(m_skinnedEntryData.size());
    m_skinnedEntryData.insert(m_skinnedEntryData.end(), m_skinnedShadowEntryData.begin(), m_skinnedShadowEntryData.end());
    m_skinnedEntryCount = u32(m_skinnedEntryData.size());
    m_skinnedHudEntryCount = u32(m_skinnedHudEntryData.size());
    if (m_skinnedEntryCount > m_skinnedEntryCapacity || m_skinnedHudEntryCount > m_skinnedHudEntryCapacity)
        FATAL("[GPUCulling] prepared skinned entries exceed their allocated ranges");
    m_skinnedPrepared = true;
}

void GPUCullingManager::UploadSkinnedObjects(fg::RenderContext* ctx, decals::OverlayManager* overlayMgr)
{
    ZoneScopedN("GPUCull::UploadSkinnedObjects");
    auto cmdList = ctx->GetCommandList();
    m_skinnedPools.FlushUploads(cmdList);
    FlushBoneBatch(cmdList);
    if (!m_skinnedPrepared)
        return;

    if (overlayMgr)
        overlayMgr->UploadSplats(cmdList);

    if (m_skinnedForwardCount > 0)
    {
        cmdList->writeBuffer(m_skinnedForwardArgsBuffer, m_skinnedForwardArgsData.data(),
            u64(m_skinnedForwardCount) * sizeof(IndirectDrawArgs));
        cmdList->setBufferState(m_skinnedForwardArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
        cmdList->writeBuffer(m_skinnedForwardInstanceBuffer, m_skinnedForwardInstanceData.data(),
            u64(m_skinnedForwardCount) * sizeof(GPUInstanceData));
        cmdList->setBufferState(m_skinnedForwardInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
    }

    cmdList->writeBuffer(m_skinnedRecordsBuffer, m_skinnedRecordsData.data(),
        m_skinnedRecordsData.size() * sizeof(SkinnedDrawRecord));
    if (m_skinnedEntryCount > 0)
        cmdList->writeBuffer(m_skinnedEntryBuffer, m_skinnedEntryData.data(),
            u64(m_skinnedEntryCount) * sizeof(GPUClusterEntry));
    if (m_skinnedHudEntryCount > 0)
        cmdList->writeBuffer(m_skinnedHudEntryBuffer, m_skinnedHudEntryData.data(),
            u64(m_skinnedHudEntryCount) * sizeof(u32));

    auto& nextHistory = m_skinnedHistory[m_skinnedHistoryIndex ^ 1u];
    if (DispatchPreskin(cmdList, overlayMgr, m_skinnedPreparedVertexCount))
    {
        std::sort(nextHistory.begin(), nextHistory.end(), [](const SkinnedHistoryEntry& a, const SkinnedHistoryEntry& b)
        {
            if (a.visual != b.visual)
                return a.visual < b.visual;
            if (a.renderable != b.renderable)
                return a.renderable < b.renderable;
            return a.slot > b.slot;
        });
        nextHistory.erase(std::unique(nextHistory.begin(), nextHistory.end(),
            [](const SkinnedHistoryEntry& a, const SkinnedHistoryEntry& b) { return a.SameIdentity(b); }), nextHistory.end());
        m_skinnedHistoryIndex ^= 1u;
        m_skinnedHistoryFrame = Device.dwFrame;
        m_skinnedPreVBRecreated = false;
    }
    else
    {
        nextHistory.clear();
    }
    m_skinnedPrepared = false;
}

bool GPUCullingManager::EnsurePreskinPipeline(nvrhi::IDevice* nvDevice)
{
    if (m_preskinPipeline)
        return true;
    if (m_preskinFailed)
        return false;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto csResult = shaderLoader->LoadComputeShader("skinned_preskin", "main");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [GPUCulling] skinned_preskin.cs failed to load");
        m_preskinFailed = true;
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    m_preskinLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_SkinnedPreskin", *csResult.reflection, nvDevice);
    if (!m_preskinLayout) {
        m_preskinFailed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    pipeDesc.bindingLayouts = { m_preskinLayout };
    m_preskinPipeline = cache.GetOrCreateComputePipeline("GPUCull_SkinnedPreskin", pipeDesc, nvDevice);
    if (!m_preskinPipeline) {
        m_preskinFailed = true;
        return false;
    }
    return true;
}

bool GPUCullingManager::EnsurePreskinBuffers(nvrhi::IDevice* nvDevice, u32 vertexTotal)
{
    if (m_skinnedPreVBCapacity >= vertexTotal)
        return false;
    u64 grown = std::max<u64>(u64(m_skinnedPreVBCapacity) * 2ull, 65536ull);
    while (grown < vertexTotal)
        grown *= 2ull;
    const u64 vertexLimit = (1ull << 32) / sizeof(bindless::UnifiedVertex);
    if (vertexTotal > vertexLimit)
        FATAL_F("[GPUCulling] skinned deformation demand %u exceeds the shader addressable range", vertexTotal);
    const u32 capacity = u32(std::min(grown, vertexLimit));
    for (u32 i = 0; i < 2; ++i) {
        nvrhi::BufferDesc desc;
        desc.debugName = i == 0 ? "GPUCull_SkinnedPreVB0" : "GPUCull_SkinnedPreVB1";
        desc.byteSize = u64(capacity) * sizeof(bindless::UnifiedVertex);
        desc.isVertexBuffer = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;
        m_skinnedPreVB[i] = nvDevice->createBuffer(desc);
        if (!m_skinnedPreVB[i])
            FATAL("[GPUCulling] skinned deformation buffer allocation failed");
    }
    m_skinnedPreVBCapacity = capacity;
    return true;
}

bool GPUCullingManager::DispatchPreskin(nvrhi::ICommandList* cmdList, decals::OverlayManager* overlayMgr, u32 vertexTotal)
{
    ZoneScopedN("GPUCull::DispatchPreskin");

    if (vertexTotal == 0 || m_skinnedChunkData.empty())
        return false;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!EnsurePreskinPipeline(nvDevice))
        FATAL("[GPUCulling] skinned deformation pipeline readiness was lost during frame execution");

    const u32 chunkCount = static_cast<u32>(m_skinnedChunkData.size());
    if (chunkCount > m_skinnedChunkCapacity)
        FATAL_F("[GPUCulling] %u skinned chunks exceed the prepared capacity %u", chunkCount, m_skinnedChunkCapacity);
    if (!m_skinnedPreVB[0] || !m_skinnedPreVB[1] || !m_skinnedChunkBuffer)
        FATAL("[GPUCulling] skinned deformation buffers were not prepared before graph construction");

    nvrhi::IBuffer* dstVB = m_skinnedPreVB[m_skinnedPreVBIndex];
    cmdList->writeBuffer(m_skinnedChunkBuffer, m_skinnedChunkData.data(), u64(chunkCount) * sizeof(SkinnedChunk));

    auto& cache = framegraph::GetPassResourceCache();
    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("skinned_preskin", ".cs");
    if (!refl)
        FATAL("[GPUCulling] skinned deformation reflection was lost during frame execution");
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(fg::passes::StaticGlobals), m_device);
    auto paramsCB = cache.GetOrCreateVolatileCB("GPUCull", "PreskinParams", 16, m_device, 64);
    nvrhi::IBuffer* splatBuffer = overlayMgr ? overlayMgr->GetSplatBuffer() : nullptr;

    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        if (m_skinnedChunkCount[f] == 0)
            continue;
        nvrhi::IBuffer* srcVB = m_skinnedPools.GetVertexBuffer(f);
        if (!srcVB)
            FATAL_F("[GPUCulling] skinned source pool %u has no vertex storage for %u prepared chunks",
                f, m_skinnedChunkCount[f]);

        struct alignas(16) PreskinParams {
            u32 chunkBase;
            u32 formatID;
            u32 stride;
            u32 pad;
        } params;
        params.chunkBase = m_skinnedChunkBase[f];
        params.formatID = f;
        params.stride = SkinnedFormatStride(f);
        params.pad = 0;
        cmdList->writeBuffer(paramsCB, &params, sizeof(params));

        nvrhi::IBuffer* resources[] = {
            staticGlobalsCB, paramsCB, srcVB, m_skinnedChunkBuffer,
            m_skinnedRecordsBuffer, m_globalBoneBuffer, splatBuffer, dstVB
        };
        auto& binding = m_preskinBindingSets[f][m_skinnedPreVBIndex];
        if (!binding.handle || !std::equal(std::begin(resources), std::end(resources), std::begin(binding.resources))) {
            framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.SkinnedPreskin");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
               .ConstantBuffer("PreskinParams", paramsCB)
               .BufferSRV("g_SrcVB", srcVB)
               .BufferSRV("g_Chunks", m_skinnedChunkBuffer)
               .BufferSRV("g_SkinnedRecords", m_skinnedRecordsBuffer)
               .BufferSRV("g_BoneMatrices", m_globalBoneBuffer)
               .BufferSRV("g_PaintSplats", splatBuffer)
               .BufferUAV("g_DstVB", dstVB);
            binding.handle = cache.GetOrCreateBindingSet(bsb.Build(), m_preskinLayout, nvDevice);
            std::copy(std::begin(resources), std::end(resources), std::begin(binding.resources));
        }
        if (!binding.handle)
            FATAL("[GPUCulling] skinned deformation binding set creation failed during frame execution");

        nvrhi::ComputeState cs;
        cs.pipeline = m_preskinPipeline;
        cs.bindings = { binding.handle };
        cmdList->setComputeState(cs);
        cmdList->dispatch(m_skinnedChunkCount[f], 1, 1);
    }
    cmdList->setBufferState(dstVB, nvrhi::ResourceStates::VertexBuffer);
    return true;
}

// ═══════════════════════════════════════════════════════
//  SKELETON BONE BUFFER
// ═══════════════════════════════════════════════════════

void GPUCullingManager::BeginSkinnedFrame()
{
    ++m_boneUploadFrameId;
    m_currentBoneOffset = 0;
    m_boneBatchStart = 0;
}

u32 GPUCullingManager::GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton)
{
    const u32 offset = PrepareSkeletonPalette(skeleton);
    FlushBoneBatch(cmdList);
    return offset;
}

u32 GPUCullingManager::PrepareSkeletonPalette(CKinematics* skeleton)
{
    if (!m_boneBufferInitialized || !skeleton)
        FATAL("[GPUCulling] skinned source has no initialized skeleton palette");
    if (skeleton->fg_bone_upload_frame == m_boneUploadFrameId)
        return skeleton->fg_bone_upload_offset;
    const u32 boneCount = skeleton->LL_BoneCount();
    if (boneCount == 0 || boneCount > MAX_TOTAL_BONES - m_currentBoneOffset)
        FATAL_F("[GPUCulling] skeleton palette requires %u bones with %u available",
            boneCount, MAX_TOTAL_BONES - m_currentBoneOffset);
    const u32 offset = m_currentBoneOffset;
    for (u32 i = 0; i < boneCount; ++i)
        m_boneStagingBuffer[offset + i] = skeleton->LL_GetTransform_R(u16(i));
    m_currentBoneOffset += boneCount;
    skeleton->fg_bone_upload_frame = m_boneUploadFrameId;
    skeleton->fg_bone_upload_offset = offset;
    skeleton->fg_bone_pose_signature_valid = false;
    return offset;
}

void GPUCullingManager::FlushBoneBatch(nvrhi::ICommandList* cmdList)
{
    if (!cmdList || m_currentBoneOffset <= m_boneBatchStart)
        return;

    ZoneScopedN("Animation::PalettePublication");

    const u64 byteOffset = static_cast<u64>(m_boneBatchStart) * BONE_STRIDE;
    const u64 byteSize = static_cast<u64>(m_currentBoneOffset - m_boneBatchStart) * BONE_STRIDE;
    cmdList->writeBuffer(m_globalBoneBuffer, m_boneStagingBuffer.data() + m_boneBatchStart, byteSize, byteOffset);
    m_boneBatchStart = m_currentBoneOffset;
}

// ═══════════════════════════════════════════════════════
//  FRUSTUM PLANE EXTRACTION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::ExtractFrustumPlanes(Fmatrix& M, Fvector4* outPlanes)
{
    // Use CFrustum::CreateFromMatrix - EXACTLY matches detail_cull.cs setup
    // Extract LRTB + FAR planes (5 planes), skip NEAR to avoid culling close objects
    CFrustum frustum;
    frustum.CreateFromMatrix(M, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    for (u32 i = 0; i < frustum.p_count && i < 6; i++)
    {
        outPlanes[i].set(
            frustum.planes[i].n.x,
            frustum.planes[i].n.y,
            frustum.planes[i].n.z,
            frustum.planes[i].d
        );
    }
}

// ═══════════════════════════════════════════════════════
//  SETUP CULLING PASS
// ═══════════════════════════════════════════════════════

GeometryFrameBuffers ResolveGeometryResources(const framegraph::FrameGraph& fg, const GeometryFrameResources& res)
{
    GeometryFrameBuffers out;
    auto resolve = [&](const framegraph::VirtualResourceHandle& handle) -> nvrhi::IBuffer* {
        return handle.is_valid() ? fg.GetPhysicalBuffer(handle) : nullptr;
    };
    out.clusterMeta = resolve(res.clusterMeta);
    out.clusterRefs = resolve(res.clusterRefs);
    out.instances = resolve(res.instances);
    out.clusterPages = resolve(res.clusterPages);
    out.clusterPayload = resolve(res.clusterPayload);
    out.clusterVertices = resolve(res.clusterVertices);
    out.clusterGroups = resolve(res.clusterGroups);
    out.groupResidency = resolve(res.groupResidency);
    out.shadowBvhNodes = resolve(res.shadowBvhNodes);
    out.shadowBvhIndices = resolve(res.shadowBvhIndices);
    out.skinnedEntries = resolve(res.skinnedEntries);
    out.deformedVertices = resolve(res.deformedVertices);
    out.skinnedIndices = resolve(res.skinnedIndices);
    out.skinnedHudEntries = resolve(res.skinnedHudEntries);
    out.neutralFades = resolve(res.neutralFades);
    out.materials = resolve(res.materials);
    return out;
}

GeometryFrameResources GPUCullingManager::ImportGeometryResources(framegraph::FrameGraph& fg)
{
    using namespace framegraph;
    EnsureForwardBuffers(m_device->GetNVRHIDevice());
    PrepareRTSourceGeneration();

    GeometryFrameResources out;
    ClusterCullBuffers& set = m_clusterSet;

    auto importStructured = [&](const char* name, nvrhi::IBuffer* buffer, bool uav)
    {
        VirtualResourceHandle handle;
        if (!buffer)
            return handle;
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.debugName = name;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.structStride = buffer->getDesc().structStride;
        desc.isUAV = uav;
        desc.isTransient = false;
        return fg.ImportBuffer(name, buffer, desc);
    };

    auto importArgs = [&](const char* name, nvrhi::IBuffer* buffer) {
        VirtualResourceHandle handle;
        if (!buffer)
            return handle;
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.debugName = name;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.isUAV = buffer->getDesc().canHaveUAVs;
        desc.isIndirectArgs = true;
        desc.isTransient = false;
        return fg.ImportBuffer(name, buffer, desc);
    };

    out.drawArgs = importArgs("cluster_args", m_clusterArgsBuffer);
    out.terrainArgs = importArgs("cluster_terrain_args", m_clusterTerrainArgsBuffer);
    out.swArgs = importArgs("cluster_sw_args", m_clusterSwArgsBuffer);
    out.retestDrawArgs = importArgs("cluster_retest_args", m_clusterArgsBuffer2);
    out.retestTerrainArgs = importArgs("cluster_retest_terrain_args", m_clusterTerrainArgsBuffer2);
    out.retestSwArgs = importArgs("cluster_retest_sw_args", m_clusterSwArgsBuffer2);
    out.retestDispatchArgs = importArgs("cluster_retest_dispatch_args", m_clusterRetestDispatchArgs);
    out.queueArgs = importArgs("cluster_queue_args", m_clusterQueueArgsBuffer);

    out.rtVertices = importStructured("rt_source_vertices", m_rtVertexBuffer, false);
    out.rtIndices = importStructured("rt_source_indices", m_rtIndexBuffer, false);
    out.rtCopySourceVertices = importStructured("rt_source_previous_vertices", m_rtCopySourceVertexBuffer.Get(), false);
    out.rtCopySourceIndices = importStructured("rt_source_previous_indices", m_rtCopySourceIndexBuffer.Get(), false);
    out.boneMatrices = importStructured("skinned_bone_matrices", m_globalBoneBuffer, false);
    out.megaVertices = importStructured("mega_vertices", m_megaVertexBuffer, false);
    out.megaIndices = importStructured("mega_indices", m_megaIndexBuffer, false);
    out.megaCopySourceVertices = importStructured("mega_vertices_previous", m_megaCopySourceVB, false);
    out.megaCopySourceIndices = importStructured("mega_indices_previous", m_megaCopySourceIB, false);
    out.forwardArgs = importArgs("forward_args", m_transparentDrawArgsBuffer);
    out.forwardInstances = importStructured("forward_instances", m_transparentInstanceBuffer, false);
    out.forwardDrawIndices = importStructured("forward_draw_indices", m_forwardDrawIndexBuffer, false);
    out.materials = importStructured("bindless_materials", bindless::MaterialBuffer::Instance().GetBuffer(), false);
    out.terrainMaterials = importStructured("bindless_terrain_materials", bindless::TerrainMaterialBuffer::Instance().GetBuffer(), false);
    out.variants = importStructured("bindless_variants", bindless::VariantBuffer::Instance().GetBuffer(), false);
    out.variantTextures = importStructured("bindless_variant_textures", bindless::VariantTextureBuffer::Instance().GetBuffer(), false);
    out.skinnedEntries = importStructured("skinned_entries", m_skinnedEntryBuffer, false);
    out.skinnedRecords = importStructured("skinned_records", m_skinnedRecordsBuffer, false);
    out.deformedVertices = importStructured("skinned_deformed_vertices", GetSkinnedPreVertexBuffer(), true);
    out.previousDeformedVertices = importStructured("skinned_previous_vertices", GetSkinnedPrevVertexBuffer(), true);
    out.skinnedHudEntries = importStructured("skinned_hud_entries", m_skinnedHudEntryBuffer, false);
    out.neutralFades = importStructured("neutral_fades", m_neutralFadeBuffer, false);
    out.skinnedIndices = importStructured("skinned_source_indices", m_skinnedPools.GetCombinedIndexBuffer(), false);
    out.skinnedForwardArgs = importStructured("skinned_forward_args", m_skinnedForwardArgsBuffer, false);
    out.skinnedForwardInstances = importStructured("skinned_forward_instances", m_skinnedForwardInstanceBuffer, false);

    if (!set.metaBuffer || !set.refBuffer || !set.instanceBuffer || !set.countBuffer
        || !m_residency.IsActive())
        return out;

    out.clusterMeta = importStructured("cluster_meta", set.metaBuffer, false);
    out.assetMembers = importStructured("cluster_asset_members", set.memberBuffer, false);
    out.assetNodes = importStructured("cluster_asset_nodes", set.assetNodeBuffer, false);
    out.clusterPages = importStructured("cluster_pages", m_residency.GetPageTableBuffer(), false);
    out.clusterPayload = importStructured("cluster_payload", m_residency.GetPayloadArenaBuffer(), false);
    out.clusterVertices = importStructured("cluster_page_vertices", m_residency.GetVertexArenaBuffer(), false);
    out.clusterGroups = importStructured("cluster_groups", m_residency.GetClusterGroupBuffer(), false);
    out.groupResidency = importStructured("cluster_group_residency", m_residency.GetGroupBitsBuffer(), false);
    out.shadowCuts = importStructured("geometry_shadow_cuts", m_residency.GetShadowCutBuffer(), false);
    out.arenaCopySourceVertices = importStructured("geometry_old_vertex_arena", m_residency.GetArenaCopySource(true), false);
    out.arenaCopySourcePayload = importStructured("geometry_old_payload_arena", m_residency.GetArenaCopySource(false), false);
    out.clusterRefs = importStructured("cluster_refs", set.refBuffer, true);
    out.instances = importStructured("cluster_instances", set.instanceBuffer, true);
    out.counts = importStructured("cluster_counts", set.countBuffer, true);
    out.shadowBvhNodes = importStructured("cluster_shadow_bvh_nodes", set.bvhNodeBuffer, false);
    out.shadowBvhIndices = importStructured("cluster_shadow_bvh_indices", set.bvhIndexBuffer, false);
    out.visibleEntries = importStructured("cluster_visible_entries", set.visibleEntryBuffer, true);
    out.fades = importStructured("cluster_fades", set.fadeBuffer, true);
    out.terrainVisibleEntries = importStructured("cluster_terrain_visible_entries", set.terrainVisibleEntryBuffer, true);
    out.terrainFades = importStructured("cluster_terrain_fades", set.terrainFadeBuffer, true);
    out.swEntries = importStructured("cluster_sw_entries", set.swEntryBuffer, true);
    out.candidates = importStructured("cluster_candidates", set.candidateBuffer, true);
    out.retestVisibleEntries = importStructured("cluster_retest_visible_entries", set.visibleEntryBuffer2, true);
    out.retestFades = importStructured("cluster_retest_fades", set.fadeBuffer2, true);
    out.retestTerrainVisibleEntries = importStructured("cluster_retest_terrain_visible_entries", set.terrainVisibleEntryBuffer2, true);
    out.retestTerrainFades = importStructured("cluster_retest_terrain_fades", set.terrainFadeBuffer2, true);
    out.retestSwEntries = importStructured("cluster_retest_sw_entries", set.swEntryBuffer2, true);
    out.nodeQueueA = importStructured("cluster_node_queue_a", set.nodeQueue[0], true);
    out.nodeQueueB = importStructured("cluster_node_queue_b", set.nodeQueue[1], true);
    out.deferredNodes = importStructured("cluster_deferred_nodes", set.deferredNodeBuffer, true);
    out.deferredInstances = importStructured("cluster_deferred_instances", set.deferredInstanceBuffer, true);
    out.leafQueue = importStructured("cluster_leaf_queue", set.leafQueueBuffer, true);

    out.valid = out.clusterMeta.is_valid() && out.clusterRefs.is_valid() && out.instances.is_valid()
        && out.assetMembers.is_valid() && out.assetNodes.is_valid() && out.counts.is_valid()
        && out.clusterPages.is_valid() && out.clusterPayload.is_valid() && out.clusterVertices.is_valid()
        && out.clusterGroups.is_valid() && out.groupResidency.is_valid();
    return out;
}

framegraph::VirtualResourceHandle GPUCullingManager::SetupGeometryPreparePass(
    framegraph::FrameGraph& fg,
    const GeometryCollector* geometry,
    GeometryFrameResources& resources)
{
    using namespace framegraph;

    if (!m_computeEnabled || !geometry)
        return VirtualResourceHandle();


    auto& passData = fg.addCallbackPass<GeometryPreparePassData>(
        "Geometry Prepare",
        [&](FrameGraph& builder, PassHandle passHandle, GeometryPreparePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.manager = this;
            if (resources.instances.is_valid())
                data.instances = passBuilder.write(resources.instances, ResourceState::CopyDest);
            if (resources.clusterRefs.is_valid())
                data.clusterRefs = passBuilder.write(resources.clusterRefs, ResourceState::CopyDest);
            if (resources.megaVertices.is_valid())
                data.megaVertices = passBuilder.write(resources.megaVertices, ResourceState::CopyDest);
            if (resources.megaIndices.is_valid())
                data.megaIndices = passBuilder.write(resources.megaIndices, ResourceState::CopyDest);
            if (resources.megaCopySourceVertices.is_valid())
                passBuilder.read(resources.megaCopySourceVertices, ResourceState::CopySource);
            if (resources.megaCopySourceIndices.is_valid())
                passBuilder.read(resources.megaCopySourceIndices, ResourceState::CopySource);
            if (resources.rtCopySourceVertices.is_valid())
                passBuilder.read(resources.rtCopySourceVertices, ResourceState::CopySource);
            if (resources.rtCopySourceIndices.is_valid())
                passBuilder.read(resources.rtCopySourceIndices, ResourceState::CopySource);
            if (resources.clusterMeta.is_valid())
                passBuilder.write(resources.clusterMeta, ResourceState::CopyDest);
            if (resources.assetMembers.is_valid())
                passBuilder.write(resources.assetMembers, ResourceState::CopyDest);
            if (resources.assetNodes.is_valid())
                passBuilder.write(resources.assetNodes, ResourceState::CopyDest);
            if (resources.clusterPages.is_valid())
                passBuilder.write(resources.clusterPages, ResourceState::CopyDest);
            if (resources.clusterPayload.is_valid())
                passBuilder.write(resources.clusterPayload, ResourceState::CopyDest);
            if (resources.clusterVertices.is_valid())
                passBuilder.write(resources.clusterVertices, ResourceState::CopyDest);
            if (resources.clusterGroups.is_valid())
                passBuilder.write(resources.clusterGroups, ResourceState::CopyDest);
            if (resources.rtVertices.is_valid())
                passBuilder.write(resources.rtVertices, ResourceState::CopyDest);
            if (resources.rtIndices.is_valid())
                passBuilder.write(resources.rtIndices, ResourceState::CopyDest);
            if (resources.groupResidency.is_valid())
                passBuilder.write(resources.groupResidency, ResourceState::CopyDest);
            if (resources.shadowCuts.is_valid())
                passBuilder.write(resources.shadowCuts, ResourceState::CopyDest);
            for (const auto handle : { resources.arenaCopySourceVertices, resources.arenaCopySourcePayload })
            {
                if (handle.is_valid())
                    passBuilder.read(handle, ResourceState::CopySource);
            }
            if (resources.shadowBvhNodes.is_valid())
                passBuilder.write(resources.shadowBvhNodes, ResourceState::CopyDest);
            if (resources.shadowBvhIndices.is_valid())
                passBuilder.write(resources.shadowBvhIndices, ResourceState::CopyDest);
            if (resources.counts.is_valid())
                passBuilder.write(resources.counts, ResourceState::CopyDest);
            if (resources.drawArgs.is_valid())
                passBuilder.write(resources.drawArgs, ResourceState::CopyDest);
            if (resources.terrainArgs.is_valid())
                passBuilder.write(resources.terrainArgs, ResourceState::CopyDest);
            if (resources.swArgs.is_valid())
                passBuilder.write(resources.swArgs, ResourceState::CopyDest);
            if (resources.retestDrawArgs.is_valid())
                passBuilder.write(resources.retestDrawArgs, ResourceState::CopyDest);
            if (resources.retestTerrainArgs.is_valid())
                passBuilder.write(resources.retestTerrainArgs, ResourceState::CopyDest);
            if (resources.retestSwArgs.is_valid())
                passBuilder.write(resources.retestSwArgs, ResourceState::CopyDest);
            if (resources.retestDispatchArgs.is_valid())
                passBuilder.write(resources.retestDispatchArgs, ResourceState::CopyDest);
            if (resources.queueArgs.is_valid())
                passBuilder.write(resources.queueArgs, ResourceState::CopyDest);
            for (const auto handle : { resources.materials, resources.terrainMaterials,
                resources.variants, resources.variantTextures, resources.forwardArgs, resources.forwardInstances,
                resources.forwardDrawIndices })
            {
                if (handle.is_valid())
                    passBuilder.write(handle, ResourceState::CopyDest);
            }
        },
        [](const GeometryPreparePassData& data, const FrameGraph&, fg::RenderContext* ctx) {
            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
                return;
            mgr->UploadSceneObjects(ctx);
            mgr->UploadRTSource(ctx->GetCommandList());
            mgr->m_residency.PromoteShadowDemand();
            mgr->AccumulateGeometryDemand();
            mgr->m_residency.ResolveDemand();
            mgr->m_residency.RecordUploads(ctx->GetCommandList());
            bindless::MaterialBuffer::Instance().Upload(ctx);
            bindless::TerrainMaterialBuffer::Instance().Upload(ctx);
            bindless::VariantBuffer::Instance().Upload(ctx);
            bindless::VariantTextureBuffer::Instance().Upload(ctx);
        }
    );

    return passData.instances;
}

void GPUCullingManager::SetupCullingPass(
    framegraph::FrameGraph& fg,
    const GeometryCollector* geometry,
    framegraph::VirtualResourceHandle prevHiZ,
    const Fmatrix& prevViewProj,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    GeometryFrameResources& resources)
{
    using namespace framegraph;

    if (!m_computeEnabled || !geometry || !resources.valid)
        return;

    if (!EnsureClusterCullPipeline(m_device->GetNVRHIDevice()))
        FATAL("[GPUCulling] opaque visibility requires the cluster traversal, leaf and argument pipelines");


    fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",
        [&, prevHiZ, prevViewProj, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.prevViewProj = prevViewProj;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            passBuilder.read(resources.clusterMeta, ResourceState::ShaderResource);
            passBuilder.read(resources.assetMembers, ResourceState::ShaderResource);
            passBuilder.read(resources.assetNodes, ResourceState::ShaderResource);
            passBuilder.read(resources.clusterRefs, ResourceState::ShaderResource);
            passBuilder.read(resources.instances, ResourceState::ShaderResource);
            passBuilder.readWrite(resources.counts, ResourceState::UnorderedAccess);
            passBuilder.write(resources.visibleEntries, ResourceState::UnorderedAccess);
            passBuilder.write(resources.fades, ResourceState::UnorderedAccess);
            passBuilder.write(resources.terrainVisibleEntries, ResourceState::UnorderedAccess);
            passBuilder.write(resources.terrainFades, ResourceState::UnorderedAccess);
            passBuilder.write(resources.swEntries, ResourceState::UnorderedAccess);
            passBuilder.write(resources.candidates, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.queueArgs, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.nodeQueueA, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.nodeQueueB, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.deferredNodes, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.deferredInstances, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.leafQueue, ResourceState::UnorderedAccess);
            passBuilder.write(resources.drawArgs, ResourceState::UnorderedAccess);
            passBuilder.write(resources.terrainArgs, ResourceState::UnorderedAccess);
            passBuilder.write(resources.swArgs, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestDispatchArgs, ResourceState::UnorderedAccess);
            if (prevHiZ.is_valid())
                data.prevHiZ = passBuilder.read(prevHiZ, ResourceState::ShaderResource);
        },
        [](const GPUCullPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
                return;
            nvrhi::ITexture* prevHiZ = data.prevHiZ.is_valid() ? fg.GetPhysicalTexture(data.prevHiZ) : nullptr;
            mgr->DispatchClusterCull(ctx->GetCommandList(), mgr->m_device->GetNVRHIDevice(), prevHiZ, data.prevViewProj,
                data.hizWidth, data.hizHeight, data.hizMipLevels);
        }
    );
}

framegraph::VirtualResourceHandle GPUCullingManager::SetupClusterRetestPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    GeometryFrameResources& resources)
{
    using namespace framegraph;

    if (!m_computeEnabled || !hizPyramid.is_valid() || !resources.valid)
        return VirtualResourceHandle();


    auto& passData = fg.addCallbackPass<ClusterRetestPassData>(
        "Cluster Retest",
        [&, hizPyramid, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, ClusterRetestPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.manager = this;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;
            data.hiz = passBuilder.read(hizPyramid, ResourceState::ShaderResource);
            passBuilder.read(resources.clusterMeta, ResourceState::ShaderResource);
            passBuilder.read(resources.assetMembers, ResourceState::ShaderResource);
            passBuilder.read(resources.assetNodes, ResourceState::ShaderResource);
            passBuilder.read(resources.clusterRefs, ResourceState::ShaderResource);
            passBuilder.read(resources.instances, ResourceState::ShaderResource);
            passBuilder.readWrite(resources.counts, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.candidates, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestVisibleEntries, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestFades, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestTerrainVisibleEntries, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestTerrainFades, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestSwEntries, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.queueArgs, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.nodeQueueA, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.nodeQueueB, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.deferredNodes, ResourceState::UnorderedAccess);
            passBuilder.read(resources.deferredInstances, ResourceState::ShaderResource);
            passBuilder.readWrite(resources.leafQueue, ResourceState::UnorderedAccess);
            passBuilder.readWrite(resources.retestDispatchArgs, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestTerrainArgs, ResourceState::UnorderedAccess);
            passBuilder.write(resources.retestSwArgs, ResourceState::UnorderedAccess);
            data.args = passBuilder.write(resources.retestDrawArgs, ResourceState::UnorderedAccess);
        },
        [](const ClusterRetestPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            nvrhi::ITexture* hiz = fg.GetPhysicalTexture(data.hiz);
            if (!hiz)
                return;
            data.manager->DispatchClusterRetest(ctx->GetCommandList(), data.manager->m_device->GetNVRHIDevice(),
                hiz, data.hizWidth, data.hizHeight, data.hizMipLevels);
        }
    );

    return passData.args;
}

// ═══════════════════════════════════════════════════════
//  SETUP SKINNED CULLING PASS
// ═══════════════════════════════════════════════════════

framegraph::VirtualResourceHandle GPUCullingManager::SetupSkinnedUploadPass(
    framegraph::FrameGraph& fg,
    decals::OverlayManager* overlayMgr,
    GeometryFrameResources& resources)
{
    using namespace framegraph;

    if (!IsSkinnedEnabled() || (!m_skinnedPrepared && m_currentBoneOffset == 0))
        return VirtualResourceHandle{};

    auto importStructured = [&](const char* name, nvrhi::IBuffer* buffer, bool uav)
    {
        VirtualResourceHandle handle;
        if (!buffer)
            return handle;
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.debugName = name;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.structStride = buffer->getDesc().structStride;
        desc.isUAV = uav;
        desc.isTransient = false;
        return fg.ImportBuffer(name, buffer, desc);
    };

    if (!resources.skinnedEntries.is_valid())
        resources.skinnedEntries = importStructured("skinned_entries", m_skinnedEntryBuffer, false);
    if (!resources.skinnedRecords.is_valid())
        resources.skinnedRecords = importStructured("skinned_records", m_skinnedRecordsBuffer, false);
    if (!resources.deformedVertices.is_valid())
        resources.deformedVertices = importStructured("skinned_deformed_vertices", GetSkinnedPreVertexBuffer(), true);
    const VirtualResourceHandle previousVertices = importStructured("skinned_previous_vertices", GetSkinnedPrevVertexBuffer(), true);
    const VirtualResourceHandle hudEntries = importStructured("skinned_hud_entries", m_skinnedHudEntryBuffer, false);
    const VirtualResourceHandle chunks = importStructured("skinned_chunks", m_skinnedChunkBuffer, false);
    const VirtualResourceHandle bones = importStructured("skinned_bone_matrices", m_globalBoneBuffer, false);
    const VirtualResourceHandle splats = importStructured("skinned_paint_splats", overlayMgr ? overlayMgr->GetSplatBuffer() : nullptr, false);
    const VirtualResourceHandle forwardArgs = importStructured("skinned_forward_args", m_skinnedForwardArgsBuffer, false);
    const VirtualResourceHandle forwardInstances = importStructured("skinned_forward_instances", m_skinnedForwardInstanceBuffer, false);
    VirtualResourceHandle sourceVertices[SkinnedGeometryPools::FORMAT_COUNT];
    VirtualResourceHandle sourceFormatIndices[SkinnedGeometryPools::FORMAT_COUNT];
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
    {
        string64 vertexName;
        string64 indexName;
        xr_sprintf(vertexName, "skinned_source_vertices_%u", f);
        xr_sprintf(indexName, "skinned_source_indices_%u", f);
        sourceVertices[f] = importStructured(vertexName, m_skinnedPools.GetVertexBuffer(f), false);
        sourceFormatIndices[f] = importStructured(indexName, m_skinnedPools.GetIndexBuffer(f), false);
    }
    const VirtualResourceHandle sourceIndices = importStructured("skinned_source_indices", m_skinnedPools.GetCombinedIndexBuffer(), false);
    resources.previousDeformedVertices = previousVertices;
    resources.skinnedHudEntries = hudEntries;
    resources.boneMatrices = bones;
    resources.paintSplats = splats;
    resources.skinnedIndices = sourceIndices;
    resources.skinnedForwardArgs = forwardArgs;
    resources.skinnedForwardInstances = forwardInstances;

    if (m_skinnedPrepared && !resources.skinnedEntries.is_valid())
        return VirtualResourceHandle{};

    auto& passData = fg.addCallbackPass<SkinnedUploadPassData>(
        "Skinned Upload",
        [&, overlayMgr, previousVertices, hudEntries, chunks, bones, sourceIndices, splats, forwardArgs, forwardInstances](FrameGraph& builder,
            PassHandle passHandle, SkinnedUploadPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.manager = this;
            data.overlayMgr = overlayMgr;
            if (bones.is_valid())
                passBuilder.readWrite(bones, ResourceState::ShaderResource);
            for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
            {
                if (sourceVertices[f].is_valid())
                    passBuilder.readWrite(sourceVertices[f], ResourceState::ShaderResource);
                if (sourceFormatIndices[f].is_valid())
                    passBuilder.readWrite(sourceFormatIndices[f], ResourceState::ShaderResource);
            }
            if (sourceIndices.is_valid())
                passBuilder.readWrite(sourceIndices, ResourceState::ShaderResource);

            if (!m_skinnedPrepared)
                return;

            data.entries = passBuilder.write(resources.skinnedEntries, ResourceState::CopyDest);
            if (resources.skinnedRecords.is_valid())
                passBuilder.readWrite(resources.skinnedRecords, ResourceState::ShaderResource);
            if (resources.deformedVertices.is_valid())
                passBuilder.write(resources.deformedVertices, ResourceState::UnorderedAccess);
            if (previousVertices.is_valid())
                passBuilder.read(previousVertices, ResourceState::ShaderResource);
            if (hudEntries.is_valid())
                passBuilder.write(hudEntries, ResourceState::CopyDest);
            if (chunks.is_valid())
                passBuilder.readWrite(chunks, ResourceState::ShaderResource);
            if (splats.is_valid())
                passBuilder.readWrite(splats, ResourceState::ShaderResource);
            if (forwardArgs.is_valid())
                passBuilder.write(forwardArgs, ResourceState::CopyDest);
            if (forwardInstances.is_valid())
                passBuilder.write(forwardInstances, ResourceState::CopyDest);
        },
        [](const SkinnedUploadPassData& data, const FrameGraph&, fg::RenderContext* ctx)
        {
            data.manager->UploadSkinnedObjects(ctx, data.overlayMgr);
        });

    return passData.entries;
}

// ═══════════════════════════════════════════════════════
//  DEBUG VISUALIZATION
// ═══════════════════════════════════════════════════════

bool GPUCullingManager::IsDebugEnabled() const
{
    return ps_r4_debug_gpu_culling != 0 && m_computeEnabled && m_particleDebugComputePipeline && m_debugGraphicsPipeline;
}

void GPUCullingManager::CreateDebugResources(fg::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto particleDebugCsResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("particle_cull_debug");
    auto debugVsResult = GEnv.Render->GetShaderLoader()->LoadVertexShader("cull_debug");
    auto debugPsResult = GEnv.Render->GetShaderLoader()->LoadPixelShader("cull_debug");

    if (!particleDebugCsResult.handle) {
        Msg("! [GPUCulling] particle_cull_debug.cs not found - debug visualization disabled");
        return;
    }
    if (!debugVsResult.handle) {
        Msg("! [GPUCulling] cull_debug.vs not found - debug visualization disabled");
        return;
    }
    if (!debugPsResult.handle) {
        Msg("! [GPUCulling] cull_debug.ps not found - debug visualization disabled");
        return;
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugData";
        desc.byteSize = m_maxParticles * sizeof(CullDebugData);
        desc.structStride = sizeof(CullDebugData);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_debugBuffer = nvDevice->createBuffer(desc);
        if (!m_debugBuffer) {
            Msg("! [GPUCulling] Failed to create debug buffer");
            return;
        }
    }

    {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_DebugComputeParams";
        desc.byteSize = sizeof(CullDebugParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;

        m_debugComputeParamsCB = m_device->CreateBuffer(desc);
        if (!m_debugComputeParamsCB.IsValid()) {
            Msg("! [GPUCulling] Failed to create debug compute constant buffer");
            return;
        }
    }
    {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_DebugGraphicsParams";
        desc.byteSize = sizeof(CullDebugVSParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;

        m_debugGraphicsParamsCB = m_device->CreateBuffer(desc);
        if (!m_debugGraphicsParamsCB.IsValid()) {
            Msg("! [GPUCulling] Failed to create debug graphics constant buffer");
            return;
        }
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* debugCsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("particle_cull_debug", ".cs");
        m_debugComputeLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_DebugCompute", *debugCsRefl, nvDevice);
        if (!m_debugComputeLayout) {
            Msg("! [GPUCulling] Failed to create debug compute binding layout");
            return;
        }

        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = particleDebugCsResult.handle;
        pipeDesc.bindingLayouts = { m_debugComputeLayout };

        m_particleDebugComputePipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_particleDebugComputePipeline) {
            Msg("! [GPUCulling] Failed to create debug compute pipeline");
            return;
        }
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* debugVsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".vs");
        auto* debugPsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".ps");
        m_debugGraphicsLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_DebugGraphics", *debugVsRefl, *debugPsRefl, nvDevice);
        if (!m_debugGraphicsLayout) {
            Msg("! [GPUCulling] Failed to create debug graphics binding layout");
            return;
        }

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = debugVsResult.handle;
        pipeDesc.PS = debugPsResult.handle;
        pipeDesc.bindingLayouts = { m_debugGraphicsLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
        pipeDesc.inputLayout = nullptr;

        pipeDesc.renderState.blendState.targets[0].setBlendEnable(true);
        pipeDesc.renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
        pipeDesc.renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
        pipeDesc.renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::Zero);
        pipeDesc.renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);

        pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
        pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);

        pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

        nvrhi::FramebufferInfoEx framebufferInfo;
        framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        framebufferInfo.setDepthFormat(nvrhi::Format::D32);

        m_debugGraphicsPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebufferInfo);
        if (!m_debugGraphicsPipeline) {
            Msg("! [GPUCulling] Failed to create debug graphics pipeline");
            return;
        }
    }

    Msg("* [GPUCulling] Debug visualization resources created");
}

void GPUCullingManager::SetupDebugVisualizationPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    framegraph::VirtualResourceHandle colorTarget,
    framegraph::VirtualResourceHandle depthTarget,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const Fmatrix& prevViewProj,
    const xr_vector<passes::ParticleBatch>* particleBatches)
{
    using namespace framegraph;

    const u32 particleCount = particleBatches ? std::min(static_cast<u32>(particleBatches->size()), m_maxParticles) : 0;
    if (!IsDebugEnabled() || particleCount == 0)
        return;

    struct DebugPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle colorTarget;
        VirtualResourceHandle depthTarget;

        GPUCullingManager* manager;
        const xr_vector<passes::ParticleBatch>* particleBatches;
        u32 particleCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
        Fmatrix prevViewProj;
    };

    fg.addCallbackPass<DebugPassData>(
        "GPU Culling Debug",

        [&, hizWidth, hizHeight, hizMipLevels, particleCount, particleBatches, prevViewProj](FrameGraph& builder, PassHandle passHandle, DebugPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.particleBatches = particleBatches;
            data.particleCount = particleCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;
            data.prevViewProj = prevViewProj;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);
            data.colorTarget = passBuilder.write(colorTarget, ResourceState::RenderTarget);
            data.depthTarget = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
        },

        [](const DebugPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.colorTarget);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthTarget);

            if (!hizTexture || !colorTexture || !depthTexture) {
                Msg("! [GPUCulling] Debug pass missing textures");
                return;
            }

            mgr->m_particleData.clear();
            mgr->m_particleData.reserve(data.particleCount);

            for (u32 i = 0; i < data.particleCount; i++) {
                const auto& batch = (*data.particleBatches)[i];
                if (!batch.visual) continue;

                GPUParticleData particle;
                batch.worldMatrix.transform_tiny(particle.position, batch.visual->vis.sphere.P);
                const float scale = std::max({batch.worldMatrix.i.magnitude(),
                    batch.worldMatrix.j.magnitude(), batch.worldMatrix.k.magnitude()});
                particle.radius = batch.visual->vis.sphere.R * scale;
                particle.batchIndex = i;
                particle.flags = 0;
                particle.pad0 = 0.0f;
                particle.pad1 = 0.0f;
                mgr->m_particleData.push_back(particle);
            }

            if (mgr->m_particleData.empty())
                return;

            const u32 debugCount = static_cast<u32>(mgr->m_particleData.size());
            cmdList->writeBuffer(mgr->m_particleBuffer, mgr->m_particleData.data(),
                                 debugCount * sizeof(GPUParticleData));

            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;

            CullDebugParamsCB cb;
            cb.viewProj = Device.mFullTransform;
            cb.prevViewProj = data.prevViewProj;
            cb.cameraPos = Device.vCameraPosition;
            cb.maxDistanceSq = farPlane * farPlane;
            cb.objectCount = debugCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;
            cb.occluderThreshold = 50.0f;
            cb.debugOffset = 0;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
            cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB), &cb, sizeof(cb));

            auto* particleDebugRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("particle_cull_debug", ".cs");
            framegraph::BindingSetBuilder bsb(*particleDebugRefl, nvDevice, "GPUCull.ParticleDebug");
            bsb.ConstantBuffer("CullDebugParams", mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB))
               .BufferSRV("g_Particles", mgr->m_particleBuffer)
               .Texture("g_HiZPyramid", hizTexture)
               .BufferUAV("g_DebugOutput", mgr->m_debugBuffer);

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_debugComputeLayout);
            R_ASSERT2(bindingSet, "Particle debug binding set creation failed");

            nvrhi::ComputeState state;
            state.pipeline = mgr->m_particleDebugComputePipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            const u32 groupCount = (debugCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::ShaderResource);

            CullDebugVSParamsCB vsCB;
            vsCB.view = Device.mView;
            vsCB.viewProj = Device.mFullTransform;
            vsCB.objectCount = debugCount;
            vsCB.wireframeAlpha = 0.7f;

            cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_debugGraphicsParamsCB), &vsCB, sizeof(vsCB));

            auto* debugVsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".vs");
            auto* debugPsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".ps");
            framegraph::BindingSetBuilder drawBsb(*debugVsRefl, *debugPsRefl, nvDevice, "GPUCull.DebugDraw");
            drawBsb.ConstantBuffer("CullDebugVSParams", mgr->m_device->GetNativeBuffer(mgr->m_debugGraphicsParamsCB))
                   .BufferSRV("g_DebugData", mgr->m_debugBuffer);

            nvrhi::BindingSetHandle drawBindingSet = nvDevice->createBindingSet(drawBsb.Build(), mgr->m_debugGraphicsLayout);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
            fbDesc.setDepthAttachment(depthTexture);
            nvrhi::FramebufferHandle framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer(fbDesc, nvDevice);

            nvrhi::GraphicsState gfxState;
            gfxState.pipeline = mgr->m_debugGraphicsPipeline;
            gfxState.bindings = { drawBindingSet };
            gfxState.framebuffer = framebuffer;

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(colorTexture->getDesc().width);
            viewport.maxY = static_cast<float>(colorTexture->getDesc().height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;
            gfxState.viewport.addViewportAndScissorRect(viewport);

            cmdList->setGraphicsState(gfxState);

            nvrhi::DrawArguments drawArgs;
            drawArgs.vertexCount = 4;
            drawArgs.instanceCount = debugCount;
            drawArgs.startVertexLocation = 0;
            drawArgs.startInstanceLocation = 0;
            cmdList->draw(drawArgs);

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::UnorderedAccess);
        }
    );
}

void GPUCullingManager::CreateParticleResources(fg::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    nvrhi::BufferDesc desc;
    desc.debugName = "GPUCull_Particles";
    desc.byteSize = m_maxParticles * sizeof(GPUParticleData);
    desc.structStride = sizeof(GPUParticleData);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    m_particleBuffer = nvDevice->createBuffer(desc);
    if (!m_particleBuffer)
        Msg("! [GPUCulling] Failed to create particle buffer");
}

// ═══════════════════════════════════════════════════════
//  MEGA-BUFFER SYSTEM IMPLEMENTATION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::BeginLevelLoad(u32 estimatedVertices, u32 estimatedIndices)
{
    if (m_levelLoadInProgress) {
        Msg("! [GPUCulling] BeginLevelLoad called while already in progress");
        return;
    }

    RetireRTSourceUpload(true);
    RetireForwardUploads(true);
    m_levelLoadInProgress = true;
    m_megaBuffersReady = false;
    m_megaDataUploaded = false;

    InvalidateStaticCullingData();

    // Clear and pre-allocate mega-buffers
    m_megaVertices.clear();
    m_megaVertices.reserve(estimatedVertices);
    m_megaIndices.clear();
    m_megaIndices.reserve(estimatedIndices);
    m_megaSourceNormals.clear();
    m_megaSourceNormals.shrink_to_fit();
    m_forwardVertices.clear();
    m_forwardIndices.clear();
    m_forwardAllocations.clear();
    m_forwardVertexBase = 0;
    m_forwardIndexBase = 0;
    m_megaVertexCapacity = 0;
    m_megaIndexCapacity = 0;
    m_megaVertexBuffer = nullptr;
    m_megaIndexBuffer = nullptr;
    m_megaCopySourceVB = nullptr;
    m_megaCopySourceIB = nullptr;
    m_runtimeSourceLookup.clear();
    m_rtVertexBuffer = nullptr;
    m_rtIndexBuffer = nullptr;
    m_rtCopySourceVertexBuffer = nullptr;
    m_rtCopySourceIndexBuffer = nullptr;
    m_rtVertexStaging.clear();
    m_rtVertexStaging.shrink_to_fit();
    m_rtIndexStaging.clear();
    m_rtIndexStaging.shrink_to_fit();
    m_rtSubmittedVertexStaging.clear();
    m_rtSubmittedVertexStaging.shrink_to_fit();
    m_rtSubmittedIndexStaging.clear();
    m_rtSubmittedIndexStaging.shrink_to_fit();
    m_rtVertexCount = 0;
    m_rtIndexCount = 0;
    m_rtVertexUploaded = 0;
    m_rtIndexUploaded = 0;
    m_rtVertexCapacity = 0;
    m_rtIndexCapacity = 0;
    m_rtRuntimeVertexCount = 0;
    m_rtRayOnlyDynamicCount = 0;
    m_rtSourceUploaded = false;
    m_rtSourceFailed = false;
    m_megaSourceNormalsActive = false;

    // Clear VB/IB pool tracking
    m_vbPools.clear();
    m_ibPools.clear();
    m_vbPoolsAlt.clear();
    m_ibPoolsAlt.clear();

    m_totalVertexCount = 0;
    m_totalIndexCount = 0;

    Msg("* [GPUCulling] BeginLevelLoad - estimated %u vertices, %u indices", estimatedVertices, estimatedIndices);
}

void GPUCullingManager::CreateMegaBuffers()
{
    if (!EnsureMegaCapacity(u32(m_forwardVertices.size()), u32(m_forwardIndices.size())))
        FATAL("[GPUCulling] retained forward geometry allocation failed");
}

void GPUCullingManager::EndLevelLoad()
{
    R_ASSERT(m_levelLoadInProgress);
    CaptureRTSource();
    CreateMegaBuffers();
    m_megaVertices.clear();
    m_megaVertices.shrink_to_fit();
    m_megaIndices.clear();
    m_megaIndices.shrink_to_fit();
    m_megaSourceNormals.clear();
    m_megaSourceNormals.shrink_to_fit();
    m_megaSourceNormalsActive = false;
    m_megaBuffersReady = true;
    m_levelLoadInProgress = false;
    Msg("* [GPUCulling] retained forward geometry: %zu vertices (%.2f MB), %zu indices (%.2f MB)",
        m_forwardVertices.size(), double(m_forwardVertices.size() * sizeof(ForwardVertex)) / (1024.0 * 1024.0),
        m_forwardIndices.size(), double(m_forwardIndices.size() * sizeof(u32)) / (1024.0 * 1024.0));
}

void GPUCullingManager::UnloadLevel()
{
    if (GEnv.Backend)
        GEnv.Backend->WaitForIdle();
    RetireRTSourceUpload(true);
    RetireForwardUploads(true);
    m_residency.EndLevel();
    m_clusterDAG.Clear();
    InvalidateStaticCullingData();
    m_vbPools.clear();
    m_ibPools.clear();
    m_vbPoolsAlt.clear();
    m_ibPoolsAlt.clear();
    m_megaSourceNormals.clear();
    m_megaSourceNormals.shrink_to_fit();
    m_runtimeSourceNormals.clear();
    m_runtimeSourceNormals.shrink_to_fit();
    m_megaDataUploaded = false;
    m_dynamicObjectCount = 0;
    m_transparentObjectCount = 0;
    m_megaVertexBuffer = nullptr;
    m_megaIndexBuffer = nullptr;
    m_megaCopySourceVB = nullptr;
    m_megaCopySourceIB = nullptr;
    m_megaVertexCapacity = 0;
    m_megaIndexCapacity = 0;
    m_runtimeVertices.clear();
    m_runtimeIndices.clear();
    m_forwardVertexBase = 0;
    m_forwardIndexBase = 0;
    m_forwardVertices.clear();
    m_forwardVertices.shrink_to_fit();
    m_forwardIndices.clear();
    m_forwardDrawIndexBuffer = nullptr;
    m_forwardDrawIndexCapacity = 0;
    m_forwardDrawIndices.clear();
    m_forwardDrawIndices.shrink_to_fit();
    m_forwardIndices.shrink_to_fit();
    m_forwardAllocations.clear();
    m_transparentNativeArgs.clear();
    m_rtVertexBuffer = nullptr;
    m_rtIndexBuffer = nullptr;
    m_rtCopySourceVertexBuffer = nullptr;
    m_rtCopySourceIndexBuffer = nullptr;
    m_rtVertexStaging.clear();
    m_rtVertexStaging.shrink_to_fit();
    m_rtIndexStaging.clear();
    m_rtIndexStaging.shrink_to_fit();
    m_rtSubmittedVertexStaging.clear();
    m_rtSubmittedVertexStaging.shrink_to_fit();
    m_rtSubmittedIndexStaging.clear();
    m_rtSubmittedIndexStaging.shrink_to_fit();
    m_rtVertexCount = 0;
    m_rtIndexCount = 0;
    m_rtVertexUploaded = 0;
    m_rtIndexUploaded = 0;
    m_rtVertexCapacity = 0;
    m_rtIndexCapacity = 0;
    m_rtRuntimeVertexCount = 0;
    m_rtRayOnlyDynamicCount = 0;
    m_rtSourceUploaded = false;
    m_rtSourceFailed = false;
    m_geometryTablesDirty = false;
    m_runtimeSourceLookup.clear();
    m_megaVertices.clear();
    m_megaIndices.clear();
    m_staticInstanceData.clear();
    m_dynamicInstanceData.clear();
    m_staticObjectFlags.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_staticBatchVertexCounts.clear();
    m_staticBatchKeys.clear();
    m_staticLightmapData.clear();
    m_staticInstanceIdentities.clear();
    m_dynamicObjectFlags.clear();
    m_dynamicMaterialIDData.clear();
    m_dynamicBatchKeys.clear();
    m_dynamicInstanceIdentities.clear();
    m_dynamicHistory[0].clear();
    m_dynamicHistory[1].clear();
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_maxMegaVertices = 0;
    m_maxMegaIndices = 0;
    m_megaBuffersReady = false;
    m_levelLoadInProgress = false;

    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainInstanceIdentities.clear();
    m_terrainBatchKeys.clear();
    m_terrainLightmapData.clear();
    m_terrainObjectCount = 0;

    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();
    m_transparentInstanceIdentities.clear();
    m_skinnedPools.Reset();
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
        m_skinnedBuckets[f] = SkinnedBucket{};
    m_skinnedRecordsBuffer = nullptr;
    m_skinnedChunkBuffer = nullptr;
    m_skinnedEntryBuffer = nullptr;
    m_skinnedForwardArgsBuffer = nullptr;
    m_skinnedForwardInstanceBuffer = nullptr;
    m_skinnedForwardCount = 0;
    m_skinnedEntryCapacity = 0;
    m_skinnedEntryCount = 0;
    m_skinnedHudEntryBuffer = nullptr;
    m_skinnedHudEntryCapacity = 0;
    m_skinnedHudEntryCount = 0;
    m_skinnedPreVBRecreated = false;
    m_skinnedPrepared = false;
    m_skinnedPreVB[0] = nullptr;
    m_skinnedPreVB[1] = nullptr;
    m_skinnedChunkCapacity = 0;
    m_skinnedPreVBCapacity = 0;
    for (auto& format : m_preskinBindingSets)
        for (auto& binding : format)
            binding = {};
    m_megaVertices.shrink_to_fit();
    m_megaIndices.shrink_to_fit();
    m_runtimeVertices.shrink_to_fit();
    m_runtimeIndices.shrink_to_fit();
    m_transparentNativeArgs.shrink_to_fit();
    m_skinnedObjectCount = 0;
    m_maxSkinnedObjects = 0;
    m_skinnedVisibleEntryCount = 0;
    m_skinnedForwardCapacity = 0;
    m_skinnedPreparedVertexCount = 0;
    m_skinnedHistory[0].clear();
    m_skinnedHistory[1].clear();
    m_skinnedHistoryFrame = 0;
    m_skinnedRecordsData.clear();
    m_skinnedChunkData.clear();
    m_skinnedEntryData.clear();
    m_skinnedShadowEntryData.clear();
    m_skinnedHudEntryData.clear();
    m_skinnedForwardArgsData.clear();
    m_skinnedForwardInstanceData.clear();
    m_skinnedForwardKeys.clear();
    m_skinnedForwardSort.clear();
    m_skinnedForwardRanges.clear();
    ++m_boneUploadFrameId;
    m_currentBoneOffset = 0;
    m_boneBatchStart = 0;

}

void GPUCullingManager::RetainForwardGeometry(const MeshAllocation& allocation)
{
    R_ASSERT(m_levelLoadInProgress && allocation.valid);
    R_ASSERT(u64(allocation.vertexOffset) + allocation.vertexCount <= m_megaVertices.size());
    R_ASSERT(u64(allocation.indexOffset) + allocation.indexCount <= m_megaIndices.size());
    ClusterSourceView source;
    source.vertices = m_megaVertices.data() + allocation.vertexOffset;
    source.floatNormals = m_megaSourceNormalsActive
        ? m_megaSourceNormals.data() + allocation.vertexOffset : nullptr;
    source.indices = m_megaIndices.data() + allocation.indexOffset;
    source.vertexBase = allocation.vertexOffset;
    AppendForwardGeometry(allocation, source);
}

void GPUCullingManager::AppendForwardGeometry(const MeshAllocation& allocation, const ClusterSourceView& source)
{
    ClusterMeshKey key = {};
    key.vertexOffset = allocation.vertexOffset;
    key.indexOffset = allocation.indexOffset;
    key.indexCount = allocation.indexCount;
    if (m_forwardAllocations.find(key) != m_forwardAllocations.end())
        return;
    const u64 vertexOffset = u64(m_forwardVertexBase) + m_forwardVertices.size();
    const u64 indexOffset = u64(m_forwardIndexBase) + m_forwardIndices.size();
    R_ASSERT(vertexOffset + allocation.vertexCount <= INT32_MAX);
    R_ASSERT(indexOffset + allocation.indexCount <= UINT32_MAX);
    const size_t firstVertex = m_forwardVertices.size();
    m_forwardVertices.resize(firstVertex + allocation.vertexCount);
    for (u32 i = 0; i < allocation.vertexCount; ++i)
    {
        auto& destination = m_forwardVertices[firstVertex + i];
        destination.vertex = source.vertices[i];
        if ((destination.vertex.flags & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0)
        {
            R_ASSERT(source.floatNormals);
            destination.normal = source.floatNormals[i];
            destination.vertex.normal |= 0xFF000000u;
        }
        else
            destination.normal = bindless::VertexConverter::UnpackNormal(destination.vertex.normal);
    }
    const size_t firstIndex = m_forwardIndices.size();
    m_forwardIndices.resize(firstIndex + allocation.indexCount);
    for (u32 i = 0; i < allocation.indexCount; ++i)
    {
        R_ASSERT(source.indices[i] < allocation.vertexCount);
        m_forwardIndices[firstIndex + i] = source.indices[i];
    }
    MeshAllocation retained = allocation;
    retained.vertexOffset = u32(vertexOffset);
    retained.indexOffset = u32(indexOffset);
    m_forwardAllocations.emplace(key, retained);
}

void GPUCullingManager::BakeClusterDAG(const xr_vector<ClusterBakeRange>& ranges,
    const char* cachePath, u64 geomStamp)
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] BakeClusterDAG called outside of level load");
        return;
    }

    m_clusterDAG.Clear();
    if (ranges.empty() || m_megaVertices.empty() || m_megaIndices.empty())
        return;

    ClusterSourceView source;
    source.vertices = m_megaVertices.data();
    source.floatNormals = m_megaSourceNormalsActive ? m_megaSourceNormals.data() : nullptr;
    source.indices = m_megaIndices.data();
    source.vertexBase = 0;

    const bool cacheEnabled = ps_r_cluster_cache != 0;

    if (!cacheEnabled || !m_clusterDAG.TryLoadCache(cachePath, geomStamp, ranges)) {
        m_clusterDAG.Bake(ranges, source);
        if (cacheEnabled)
            m_clusterDAG.SaveCache(cachePath, geomStamp, ranges);
    }

    if (m_clusterDAG.Empty())
        return;

    m_clusterDAG.BuildAssetTable(source);


    m_pageStorePath[0] = 0;
    if (cachePath && cachePath[0]) {
        xr_sprintf(m_pageStorePath, "%s.pages", cachePath);
        if (!m_clusterDAG.WritePageStore(m_pageStorePath, geomStamp, ranges))
            m_pageStorePath[0] = 0;
    }

    if (!m_residency.BeginLevel(&m_clusterDAG, m_pageStorePath))
        FATAL("[GPUCulling] compact geometry residency could not be established");

    const ClusterResidencyStats& residency = m_clusterDAG.ResidencyStats();
    Msg("* [GPUCulling] compact geometry: %u root pages (%.2f MB), %u fine pages (%.2f MB), %u runtime pages (%.2f MB), %u pinned of %u groups",
        residency.rootPages,
        (residency.rootVertexBytes + residency.rootPayloadBytes) / (1024.0f * 1024.0f),
        residency.finePages,
        (residency.fineVertexBytes + residency.finePayloadBytes) / (1024.0f * 1024.0f),
        residency.runtimePages,
        (residency.runtimeVertexBytes + residency.runtimePayloadBytes) / (1024.0f * 1024.0f),
        residency.pinnedGroups, residency.pinnedGroups + residency.fineGroups);
    Msg("* [GPUCulling] resident page table %.2f MB",
        (m_clusterDAG.Pages().size() * sizeof(GPUClusterPage)) / (1024.0f * 1024.0f));
}

void GPUCullingManager::RetireForwardUploads(bool discard)
{
    if (m_forwardUploads.empty())
        return;
    auto* backend = GEnv.Backend;
    R_ASSERT(backend);
    if (discard)
        backend->WaitForIdle();
    for (auto it = m_forwardUploads.begin(); it != m_forwardUploads.end();)
    {
        const auto state = backend->PollSubmissionLease(it->lease);
        using State = IRenderBackend::SubmissionLeaseState;
        R_ASSERT(state != State::Unknown);
        if (!discard && (state == State::Open || state == State::Pending))
        {
            ++it;
            continue;
        }
        R_ASSERT2(discard || state == State::Complete, "Retained forward geometry upload failed");
        backend->ReleaseSubmissionLease(it->lease);
        it = m_forwardUploads.erase(it);
    }
}

namespace
{
void PackRTSourceVertices(const bindless::UnifiedVertex* vertices, const Fvector3* floatNormals,
    u32 vertexCount, u8* destination)
{
    for (u32 v = 0; v < vertexCount; ++v)
    {
        const bindless::UnifiedVertex& src = vertices[v];
        u8* dst = destination + size_t(v) * GPUCullingManager::RT_VERTEX_STRIDE;
        memcpy(dst, &src.position, sizeof(Fvector3));

        Fvector3 normal;
        if (floatNormals && (src.flags & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0)
            normal = floatNormals[v];
        else
            normal = bindless::VertexConverter::UnpackNormal(src.normal);
        memcpy(dst + 12, &normal, sizeof(Fvector3));
        memcpy(dst + 24, &src.texcoord0, sizeof(float) * 2);
        memcpy(dst + 32, &src.tangent, sizeof(u32));
        memcpy(dst + 36, &src.binormal, sizeof(u32));
    }
}

nvrhi::BufferHandle CreateRTSourceBuffer(nvrhi::IDevice* device, const char* name,
    u64 bytes, u32 stride, bool indexBuffer)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = std::max<u64>(bytes, 4ull);
    desc.structStride = stride;
    desc.canHaveRawViews = true;
    desc.isIndexBuffer = indexBuffer;
    desc.isAccelStructBuildInput = true;
    desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    desc.keepInitialState = true;
    return device->createBuffer(desc);
}
}

void GPUCullingManager::RetireRTSourceUpload(bool discard)
{
    if (!m_rtSourceLease)
        return;
    auto* backend = GEnv.Backend;
    R_ASSERT(backend);
    if (discard)
        backend->WaitForIdle();
    const auto state = backend->PollSubmissionLease(m_rtSourceLease);
    using State = IRenderBackend::SubmissionLeaseState;
    R_ASSERT(state != State::Unknown);
    if (!discard && (state == State::Open || state == State::Pending))
        return;
    if (discard)
    {
        m_rtVertexStaging.clear();
        m_rtVertexStaging.shrink_to_fit();
        m_rtIndexStaging.clear();
        m_rtIndexStaging.shrink_to_fit();
        m_rtSubmittedVertexStaging.clear();
        m_rtSubmittedVertexStaging.shrink_to_fit();
        m_rtSubmittedIndexStaging.clear();
        m_rtSubmittedIndexStaging.shrink_to_fit();
        m_rtCopySourceVertexBuffer = nullptr;
        m_rtCopySourceIndexBuffer = nullptr;
    }
    else if (state == State::Failed)
    {
        const u32 submittedVertices = u32(m_rtSubmittedVertexStaging.size() / RT_VERTEX_STRIDE);
        const u32 submittedIndices = u32(m_rtSubmittedIndexStaging.size());
        R_ASSERT(submittedVertices <= m_rtVertexUploaded && submittedIndices <= m_rtIndexUploaded);
        m_rtVertexStaging.insert(m_rtVertexStaging.begin(),
            m_rtSubmittedVertexStaging.begin(), m_rtSubmittedVertexStaging.end());
        m_rtIndexStaging.insert(m_rtIndexStaging.begin(),
            m_rtSubmittedIndexStaging.begin(), m_rtSubmittedIndexStaging.end());
        m_rtSubmittedVertexStaging.clear();
        m_rtSubmittedVertexStaging.shrink_to_fit();
        m_rtSubmittedIndexStaging.clear();
        m_rtSubmittedIndexStaging.shrink_to_fit();
        m_rtVertexUploaded -= submittedVertices;
        m_rtIndexUploaded -= submittedIndices;
        m_rtSourceUploaded = false;
    }
    else
    {
        m_rtSubmittedVertexStaging.clear();
        m_rtSubmittedVertexStaging.shrink_to_fit();
        m_rtSubmittedIndexStaging.clear();
        m_rtSubmittedIndexStaging.shrink_to_fit();
        m_rtCopySourceVertexBuffer = nullptr;
        m_rtCopySourceIndexBuffer = nullptr;
    }
    backend->ReleaseSubmissionLease(m_rtSourceLease);
    m_rtSourceLease = 0;
}

bool GPUCullingManager::IsRTSourceReady() const
{
    return m_rtVertexBuffer && m_rtIndexBuffer && m_rtSourceUploaded && !m_rtSourceFailed;
}

u32 GPUCullingManager::GetRTRuntimeVertexCount() const
{
    return m_rtRuntimeVertexCount;
}

u32 GPUCullingManager::GetRTRayOnlyDynamicCount() const
{
    return m_rtRayOnlyDynamicCount;
}

const xr_vector<GPUInstanceData>& GPUCullingManager::GetDynamicInstanceData() const
{
    return m_dynamicInstanceData;
}

const xr_vector<GeometryInstanceKey>& GPUCullingManager::GetDynamicInstanceIdentities() const
{
    return m_dynamicInstanceIdentities;
}

const xr_vector<ClusterMeshKey>& GPUCullingManager::GetDynamicMeshKeys() const
{
    return m_dynamicBatchKeys;
}

void GPUCullingManager::PrepareRTSourceGeneration()
{
    if (!m_device || !m_rtVertexBuffer || !m_rtIndexBuffer)
        return;
    if (m_rtVertexCount <= m_rtVertexCapacity && m_rtIndexCount <= m_rtIndexCapacity)
        return;
    if (m_rtSourceLease)
        return;
    if (EnsureRTSourceCapacity(m_rtVertexCount, m_rtIndexCount))
        m_rtSourceFailed = false;
    else
        m_rtSourceFailed = true;
}

bool GPUCullingManager::EnsureRTSourceCapacity(u32 vertexCount, u32 indexCount)
{
    if (!m_device || !m_rtVertexBuffer || !m_rtIndexBuffer)
        return false;
    if (vertexCount <= m_rtVertexCapacity && indexCount <= m_rtIndexCapacity)
        return true;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    const u32 vertexCapacity = std::max(vertexCount, m_rtVertexCapacity + m_rtVertexCapacity / 2u);
    const u32 indexCapacity = std::max(indexCount, m_rtIndexCapacity + m_rtIndexCapacity / 2u);

    nvrhi::BufferHandle vertexBuffer = CreateRTSourceBuffer(nvDevice, "RTSource_Vertices",
        u64(vertexCapacity) * RT_VERTEX_STRIDE, RT_VERTEX_STRIDE, false);
    nvrhi::BufferHandle indexBuffer = CreateRTSourceBuffer(nvDevice, "RTSource_Indices",
        u64(indexCapacity) * sizeof(u32), 0u, true);
    if (!vertexBuffer || !indexBuffer)
    {
        Msg("! [GPUCulling] ray tracing source growth to %u vertices / %u indices failed",
            vertexCapacity, indexCapacity);
        return false;
    }

    if (!m_rtCopySourceVertexBuffer)
        m_rtCopySourceVertexBuffer = m_rtVertexBuffer;
    if (!m_rtCopySourceIndexBuffer)
        m_rtCopySourceIndexBuffer = m_rtIndexBuffer;
    m_rtVertexBuffer = vertexBuffer;
    m_rtIndexBuffer = indexBuffer;
    m_rtVertexCapacity = vertexCapacity;
    m_rtIndexCapacity = indexCapacity;
    m_rtSourceUploaded = false;
    Msg("* [GPUCulling] ray tracing source grown to %u vertices / %u indices", vertexCapacity, indexCapacity);
    return true;
}

void GPUCullingManager::UploadRTSource(nvrhi::ICommandList* cmdList)
{
    if (!cmdList || !m_rtVertexBuffer || !m_rtIndexBuffer)
        return;
    if (m_rtSourceLease)
        return;
    if (m_rtVertexCount > m_rtVertexCapacity || m_rtIndexCount > m_rtIndexCapacity)
        return;
    if (m_rtVertexUploaded == m_rtVertexCount && m_rtIndexUploaded == m_rtIndexCount)
        return;

    R_ASSERT(m_rtVertexUploaded + u32(m_rtVertexStaging.size() / RT_VERTEX_STRIDE) == m_rtVertexCount
        && m_rtIndexUploaded + u32(m_rtIndexStaging.size()) == m_rtIndexCount);

    R_ASSERT(GEnv.Backend && GEnv.Backend->SupportsSubmissionLeases() && !m_rtSourceLease);
    m_rtSourceLease = GEnv.Backend->OpenSubmissionLease();
    R_ASSERT(m_rtSourceLease);
    if (m_rtCopySourceVertexBuffer && m_rtVertexUploaded > 0)
        cmdList->copyBuffer(m_rtVertexBuffer, 0, m_rtCopySourceVertexBuffer, 0,
            u64(m_rtVertexUploaded) * RT_VERTEX_STRIDE);
    if (m_rtCopySourceIndexBuffer && m_rtIndexUploaded > 0)
        cmdList->copyBuffer(m_rtIndexBuffer, 0, m_rtCopySourceIndexBuffer, 0,
            u64(m_rtIndexUploaded) * sizeof(u32));
    if (!m_rtVertexStaging.empty())
        cmdList->writeBuffer(m_rtVertexBuffer, m_rtVertexStaging.data(), m_rtVertexStaging.size(),
            u64(m_rtVertexUploaded) * RT_VERTEX_STRIDE);
    if (!m_rtIndexStaging.empty())
        cmdList->writeBuffer(m_rtIndexBuffer, m_rtIndexStaging.data(), m_rtIndexStaging.size() * sizeof(u32),
            u64(m_rtIndexUploaded) * sizeof(u32));
    m_rtVertexUploaded = m_rtVertexCount;
    m_rtIndexUploaded = m_rtIndexCount;
    m_rtSubmittedVertexStaging.swap(m_rtVertexStaging);
    m_rtSubmittedIndexStaging.swap(m_rtIndexStaging);
    m_rtSourceUploaded = true;
}

void GPUCullingManager::AppendRuntimeRTSource(const bindless::UnifiedVertex* vertices,
    const Fvector3* floatNormals, u32 vertexCount, const u32* indices, u32 indexCount)
{
    if (!m_rtVertexBuffer || !vertices || !indices || !vertexCount || !indexCount)
        return;

    R_ASSERT(m_rtVertexUploaded + u32(m_rtVertexStaging.size() / RT_VERTEX_STRIDE) == m_rtVertexCount
        && m_rtIndexUploaded + u32(m_rtIndexStaging.size()) == m_rtIndexCount);
    R_ASSERT(u64(m_rtVertexCount) + vertexCount <= UINT32_MAX
        && u64(m_rtIndexCount) + indexCount <= UINT32_MAX);

    const size_t vertexMark = m_rtVertexStaging.size();
    m_rtVertexStaging.resize(vertexMark + size_t(vertexCount) * RT_VERTEX_STRIDE);
    PackRTSourceVertices(vertices, floatNormals, vertexCount, m_rtVertexStaging.data() + vertexMark);

    const size_t indexMark = m_rtIndexStaging.size();
    m_rtIndexStaging.resize(indexMark + indexCount);
    for (u32 i = 0; i < indexCount; ++i)
    {
        const u32 index = indices[i];
        R_ASSERT(index < vertexCount);
        m_rtIndexStaging[indexMark + i] = index;
    }

    m_rtVertexCount += vertexCount;
    m_rtIndexCount += indexCount;
    m_rtRuntimeVertexCount += vertexCount;
    m_rtSourceUploaded = false;
}

void GPUCullingManager::CaptureRTSource()
{
    if (!m_device || m_megaVertices.empty() || m_megaIndices.empty())
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice || !nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct)
        || !nvDevice->queryFeatureSupport(nvrhi::Feature::RayQuery))
        return;

    if (u32(m_megaVertices.size()) != m_totalVertexCount || u32(m_megaIndices.size()) != m_totalIndexCount)
    {
        m_rtSourceFailed = true;
        Msg("! [GPUCulling] ray tracing source cannot represent %u registered vertices and %u indices resolved at "
            "level load; ray tracing stays unavailable", m_totalVertexCount, m_totalIndexCount);
        return;
    }

    m_rtVertexCount = u32(m_megaVertices.size());
    m_rtIndexCount = u32(m_megaIndices.size());
    m_rtVertexStaging.assign(size_t(m_rtVertexCount) * RT_VERTEX_STRIDE, 0);
    PackRTSourceVertices(m_megaVertices.data(),
        (m_megaSourceNormalsActive && m_megaSourceNormals.size() >= m_megaVertices.size())
            ? m_megaSourceNormals.data() : nullptr,
        m_rtVertexCount, m_rtVertexStaging.data());

    m_rtVertexBuffer = CreateRTSourceBuffer(nvDevice, "RTSource_Vertices",
        u64(m_rtVertexCount) * RT_VERTEX_STRIDE, RT_VERTEX_STRIDE, false);
    m_rtIndexBuffer = CreateRTSourceBuffer(nvDevice, "RTSource_Indices",
        u64(m_rtIndexCount) * sizeof(u32), 0u, true);
    if (!m_rtVertexBuffer || !m_rtIndexBuffer)
    {
        m_rtVertexBuffer = nullptr;
        m_rtIndexBuffer = nullptr;
        m_rtVertexCount = 0;
        m_rtIndexCount = 0;
        m_rtVertexStaging.clear();
        m_rtVertexStaging.shrink_to_fit();
        m_rtSourceFailed = true;
        Msg("! [GPUCulling] exact ray tracing source buffers could not be created; ray tracing stays unavailable");
        return;
    }

    m_rtVertexCapacity = m_rtVertexCount;
    m_rtIndexCapacity = m_rtIndexCount;
    m_rtVertexUploaded = 0;
    m_rtIndexUploaded = 0;
    m_rtSourceUploaded = false;
    m_rtIndexStaging.swap(m_megaIndices);
    Msg("* [GPUCulling] exact ray tracing source captured: %u vertices (%.2f MB, 40 B position+normal+uv0+packed tangent/binormal), %u indices (%.2f MB)",
        m_rtVertexCount, (u64(m_rtVertexCount) * RT_VERTEX_STRIDE) / (1024.0f * 1024.0f),
        m_rtIndexCount, (u64(m_rtIndexCount) * sizeof(u32)) / (1024.0f * 1024.0f));
}

u32 GPUCullingManager::GetPreparedSkeletonOffset(CKinematics* skeleton) const
{
    if (!skeleton || skeleton->fg_bone_upload_frame != m_boneUploadFrameId)
        FATAL("[GPUCulling] skeleton palette was not prepared for this frame");
    return skeleton->fg_bone_upload_offset;
}

const Fmatrix* GPUCullingManager::GetPreparedSkeletonMatrices(CKinematics* skeleton, u32& count) const
{
    const u32 offset = GetPreparedSkeletonOffset(skeleton);
    count = skeleton->LL_BoneCount();
    R_ASSERT(u64(offset) + count <= m_currentBoneOffset);
    return m_boneStagingBuffer.data() + offset;
}

u64 GPUCullingManager::GetPreparedSkeletonPoseSignature(CKinematics* skeleton) const
{
    u32 boneCount = 0;
    const Fmatrix* bones = GetPreparedSkeletonMatrices(skeleton, boneCount);
    if (!skeleton->fg_bone_pose_signature_valid)
    {
        ZoneScopedN("Animation::PalettePoseSignature");
        u64 signature = 14695981039346656037ull;
        const auto append = [&](const void* data, size_t size)
        {
            const auto* bytes = static_cast<const u8*>(data);
            for (size_t i = 0; i < size; ++i)
            {
                signature ^= bytes[i];
                signature *= 1099511628211ull;
            }
        };
        append(&boneCount, sizeof(boneCount));
        append(bones, size_t(boneCount) * sizeof(Fmatrix));
        skeleton->fg_bone_pose_signature = signature;
        skeleton->fg_bone_pose_signature_valid = true;
    }
    return skeleton->fg_bone_pose_signature;
}

void GPUCullingManager::BeginGeometryResidencyFrame()
{
    RetireRTSourceUpload(false);
    RetireForwardUploads(false);
    if (!m_computeEnabled)
        return;
    if (!m_residency.IsActive() && !m_residency.BeginLevel(&m_clusterDAG, m_pageStorePath))
        FATAL("[GPUCulling] geometry residency could not be initialized");

    m_residency.EndFrame(GEnv.Backend);
    m_residency.BeginFrame(GEnv.Backend);
    m_residency.ClearDemand();

    GeometryDemandView camera;
    camera.origin = Device.vCameraPosition;
    camera.direction = Device.vCameraDirection;
    camera.direction.normalize_safe();
    camera.minDistance = 0.01f;
    camera.planeCount = passes::ExtractFrustumPlanes(camera.planes);
    const float lodPx = std::max(0.05f, ps_r_cluster_lod);
    camera.errorScale = (Device.mProject._22 * float(Device.dwHeight) * 0.5f) / lodPx;
    camera.valid = true;
    m_residency.AddDemandView(camera);
}

void GPUCullingManager::AddShadowGeometryDemand(const GeometryDemandView& view)
{
    m_residency.AddShadowDemand(view);
}

void GPUCullingManager::AccumulateGeometryDemand()
{
    if (!m_residency.IsStreaming())
        return;

    const u32 staticCount = std::min(m_clusterSet.instanceCount, u32(m_geoInstanceData.size()));
    for (u32 i = 0; i < staticCount; ++i) {
        const GPUGeoInstance& inst = m_geoInstanceData[i];
        m_residency.AddInstanceDemand(inst.assetMember, inst.world, inst.scaleBound,
            (inst.flags & GPU_CLUSTER_ENTRY_PLAIN) != 0u);
    }

    const u32 dynamicCount = std::min(m_clusterSet.dynamicInstanceCount, u32(m_dynamicGeoInstanceData.size()));
    for (u32 i = 0; i < dynamicCount; ++i) {
        const GPUGeoInstance& inst = m_dynamicGeoInstanceData[i];
        m_residency.AddInstanceDemand(inst.assetMember, inst.world, inst.scaleBound,
            (inst.flags & GPU_CLUSTER_ENTRY_PLAIN) != 0u);
    }
}

void GPUCullingManager::EndGeometryResidencyFrame()
{
    if (!m_residency.IsActive())
        return;
    m_residency.EndFrame(GEnv.Backend);
}

u32 GPUCullingManager::GetStaticResidualCount() const
{
    if (m_clusterSet.refCount > 0)
        return m_clusterSet.residualStaticCount;
    return m_staticObjectCount;
}

u32 GPUCullingManager::GetTerrainResidualCount() const
{
    if (m_clusterSet.refCount > 0)
        return m_clusterSet.residualTerrainCount;
    return m_terrainObjectCount;
}

bool GPUCullingManager::SkinnedHistoryEntry::SameIdentity(const SkinnedHistoryEntry& o) const
{
    return visual == o.visual && renderable == o.renderable;
}

bool GPUCullingManager::SkinnedHistoryEntry::operator<(const SkinnedHistoryEntry& o) const
{
    if (visual != o.visual)
        return visual < o.visual;
    return renderable < o.renderable;
}

bool GeometryInstanceKey::operator<(const GeometryInstanceKey& o) const
{
    if (renderable != o.renderable)
        return renderable < o.renderable;
    if (visual != o.visual)
        return visual < o.visual;
    return subset < o.subset;
}

static float ConservativeScaleBound(const Fmatrix& m)
{
    const Fvector basis[3] = { m.i, m.j, m.k };
    float g[3][3];
    for (u32 r = 0; r < 3; ++r) {
        for (u32 c = 0; c < 3; ++c)
            g[r][c] = basis[r].dotproduct(basis[c]);
    }
    float best = 0.0f;
    for (u32 r = 0; r < 3; ++r) {
        float sum = g[r][r];
        for (u32 c = 0; c < 3; ++c) {
            if (c != r)
                sum += _abs(g[r][c]);
        }
        best = std::max(best, sum);
    }
    return _sqrt(std::max(best, 0.0f));
}

void GPUCullingManager::BuildStaticGeometryInstances(const GeometryCollector* geometry)
{
    m_geoInstanceData.clear();
    m_clusterRefData.clear();

    ClusterCullBuffers& set = m_clusterSet;
    set.refCount = 0;
    set.staticRefCount = 0;
    set.terrainRefCount = 0;
    set.instanceCount = 0;
    set.staticNodeRefs = 0;
    set.residualStaticCount = 0;
    set.residualTerrainCount = 0;
    set.traversalDepth = 0;

    if (m_clusterDAG.Empty() || m_clusterDAG.AssetMembers().empty())
        return;

    const u32 staticCount = std::min(m_staticObjectCount, u32(m_staticBatchKeys.size()));

    const xr_vector<GPUClusterAssetMember>& members = m_clusterDAG.AssetMembers();
    const xr_vector<GPUClusterMeta>& meta = m_clusterDAG.ClusterMeta();

    u64 refTotal = 0;
    u64 nodeTotal = 0;

    auto memberIsTerrain = [&](const GPUClusterAssetMember& am) {
        return am.clusterCount > 0 && (meta[am.firstCluster].flags & CLUSTER_META_FLAG_TERRAIN) != 0;
    };

    auto emitInstance = [&](u32 assetMember, const Fmatrix& world, u32 materialID, u32 extraFlags,
        float hemiScale, float hemiBias, u32 lightmapTexture) {
        const GPUClusterAssetMember& am = members[assetMember];
        if (am.clusterCount == 0)
            return false;

        const bool terrain = memberIsTerrain(am);
        const bool castsShadow = terrain || MaterialCastsShadow(materialID);
        const float scaleBound = ConservativeScaleBound(world);

        GPUGeoInstance inst;
        inst.world = world;
        inst.prevWorld = world;
        inst.assetMember = assetMember;
        inst.materialID = materialID;
        inst.flags = extraFlags | (castsShadow ? 0u : u32(GPU_CLUSTER_ENTRY_NO_SHADOW));
        inst.scaleBound = scaleBound;
        inst.firstRef = u32(refTotal);
        inst.refCount = am.clusterCount;
        inst.firstPage = am.firstPage;
        inst.historyValid = 0;
        inst.prevScaleBound = scaleBound;
        inst.hemiScale = hemiScale;
        inst.hemiBias = hemiBias;
        inst.lightmapTexture = lightmapTexture;

        refTotal += am.clusterCount;
        nodeTotal += am.nodeCount;
        if (refTotal > UINT32_MAX || nodeTotal > UINT32_MAX)
            FATAL("[GPUCulling] static geometry reference count exceeds the addressable range");

        const u32 instanceIndex = u32(m_geoInstanceData.size());
        m_geoInstanceData.push_back(inst);
        for (u32 c = 0; c < am.clusterCount; ++c) {
            m_clusterRefData.push_back(instanceIndex);
            m_clusterRefData.push_back(am.firstCluster + c);
        }
        return true;
    };

    auto bindBatch = [&](const ClusterMeshKey& key) {
        u32 assetMember = 0;
        if (!m_clusterDAG.FindAssetMember(key, assetMember))
            FATAL_F("[GPUCulling] geometry v=%u+%u i=%u+%u has no cluster representation",
                key.vertexOffset, key.vertexCount, key.indexOffset, key.indexCount);
        return assetMember;
    };

    u32 clusteredBatches = 0;
    u32 shadowOnlyBatches = 0;
    u32 transparentShadowBatches = 0;
    u32 unallocatedBatches = 0;

    for (u32 i = 0; i < staticCount; ++i) {
        const ClusterMeshKey& key = m_staticBatchKeys[i];
        if (key.indexCount == 0) {
            ++unallocatedBatches;
            continue;
        }

        u32 extraFlags = 0;
        if (m_staticObjectFlags[i] & GPU_OBJECT_NO_RESOLVE) {
            if (!MaterialCastsShadow(m_staticMaterialIDData[i]))
                continue;
            extraFlags = GPU_CLUSTER_ENTRY_SHADOW_ONLY | GPU_CLUSTER_ENTRY_AT;
            ++shadowOnlyBatches;
        }

        const GPUInstanceData& source = m_staticInstanceData[i];
        const u32 lightmapTexture = i < m_staticLightmapData.size() ? m_staticLightmapData[i] : UINT32_MAX;
        if (emitInstance(bindBatch(key), source.world, m_staticMaterialIDData[i], extraFlags,
                source.hemiScale, source.hemiBias, lightmapTexture))
            ++clusteredBatches;
    }

    if (geometry)
    {
        const xr_vector<GeometryBatch>& staticBatches = geometry->GetStaticBatches();
        for (u32 index : geometry->GetStaticTransparentIndices())
        {
            if (index >= staticBatches.size())
                continue;
            const GeometryBatch& batch = staticBatches[index];
            if (batch.isSkinned || batch.isTerrain || !batch.megaBufferAlloc.valid)
                continue;
            if (!MaterialCastsShadow(batch.bindlessMaterialID))
                continue;
            ClusterMeshKey key = {};
            key.vertexOffset = batch.megaBufferAlloc.vertexOffset;
            key.indexOffset = batch.megaBufferAlloc.indexOffset;
            key.vertexCount = batch.megaBufferAlloc.vertexCount;
            key.indexCount = batch.megaBufferAlloc.indexCount;
            if (emitInstance(bindBatch(key), batch.worldMatrix, batch.bindlessMaterialID,
                    GPU_CLUSTER_ENTRY_SHADOW_ONLY | GPU_CLUSTER_ENTRY_AT, 0.0f, 1.0f, UINT32_MAX))
                ++transparentShadowBatches;
        }
    }

    set.staticRefCount = u32(refTotal);

    u32 clusteredTerrain = 0;
    u32 unallocatedTerrain = 0;
    const u32 terrainCount = std::min(u32(m_terrainInstanceData.size()), u32(m_terrainBatchKeys.size()));
    for (u32 i = 0; i < terrainCount; ++i) {
        const ClusterMeshKey& key = m_terrainBatchKeys[i];
        if (key.indexCount == 0) {
            ++unallocatedTerrain;
            continue;
        }

        const GPUInstanceData& source = m_terrainInstanceData[i];
        const u32 lightmapTexture = i < m_terrainLightmapData.size() ? m_terrainLightmapData[i] : UINT32_MAX;
        if (emitInstance(bindBatch(key), source.world, m_terrainMaterialIDData[i], 0u,
                source.hemiScale, source.hemiBias, lightmapTexture))
            ++clusteredTerrain;
    }

    set.refCount = u32(refTotal);
    set.terrainRefCount = set.refCount - set.staticRefCount;
    set.instanceCount = u32(m_geoInstanceData.size());
    set.staticNodeRefs = u32(nodeTotal);
    set.residualStaticCount = unallocatedBatches;
    set.residualTerrainCount = unallocatedTerrain;

    Msg("* [GPUCulling] geometry instances: %u static (%u shadow-only) + %u transparent casters + %u terrain, %u references (%u static + %u terrain), %u hierarchy node references",
        clusteredBatches, shadowOnlyBatches, transparentShadowBatches, clusteredTerrain,
        set.refCount, set.staticRefCount, set.terrainRefCount, set.staticNodeRefs);
    if (unallocatedBatches || unallocatedTerrain)
        Msg("! [GPUCulling] %u static and %u terrain batches have no mega-buffer allocation and are not part of the geometry path",
            unallocatedBatches, unallocatedTerrain);
}

void GPUCullingManager::BuildDynamicGeometryInstances(const GeometryCollector* geometry)
{
    ClusterCullBuffers& set = m_clusterSet;
    m_dynamicGeoInstanceData.clear();
    m_dynamicRefData.clear();
    set.dynamicRefCount = 0;
    set.dynamicInstanceCount = 0;
    set.dynamicNodeRefs = 0;
    set.dynamicResidualCount = 0;

    const u32 dynamicCount = m_dynamicObjectCount;
    const bool historyFrameValid = m_dynamicHistoryFrame + 1u == Device.dwFrame;
    const auto& prevHistory = m_dynamicHistory[m_dynamicHistoryIndex];
    auto& nextHistory = m_dynamicHistory[m_dynamicHistoryIndex ^ 1u];
    nextHistory.clear();

    const bool tablesReady = !m_clusterDAG.Empty() && !m_clusterDAG.AssetMembers().empty();
    if (tablesReady)
    {
        const xr_vector<GPUClusterAssetMember>& members = m_clusterDAG.AssetMembers();
        const xr_vector<GPUClusterMeta>& meta = m_clusterDAG.ClusterMeta();

        u64 refTotal = set.refCount;
        u64 nodeTotal = 0;
        u32 residual = 0;

        auto emitDynamic = [&](const ClusterMeshKey& key, const Fmatrix& world, u32 materialID,
            const GeometryInstanceKey& identity, u32 extraFlags)
        {
            u32 assetMember = 0;
            if (!m_clusterDAG.FindAssetMember(key, assetMember))
                FATAL_F("[GPUCulling] dynamic geometry v=%u+%u i=%u+%u has no cluster representation",
                    key.vertexOffset, key.vertexCount, key.indexOffset, key.indexCount);

            const GPUClusterAssetMember& am = members[assetMember];
            if (am.clusterCount == 0)
                return;

            Fmatrix prevWorld = world;
            u32 historyValid = 0;
            if (historyFrameValid) {
                auto it = prevHistory.find(identity);
                if (it != prevHistory.end() && it->second.assetMember == assetMember) {
                    prevWorld = it->second.world;
                    historyValid = 1;
                }
            }
            DynamicHistoryEntry record;
            record.world = world;
            record.assetMember = assetMember;
            nextHistory[identity] = record;

            const bool terrain = (meta[am.firstCluster].flags & CLUSTER_META_FLAG_TERRAIN) != 0;
            const bool castsShadow = terrain || MaterialCastsShadow(materialID);

            GPUGeoInstance inst;
            inst.world = world;
            inst.prevWorld = prevWorld;
            inst.assetMember = assetMember;
            inst.materialID = materialID;
            inst.flags = extraFlags | (castsShadow ? 0u : u32(GPU_CLUSTER_ENTRY_NO_SHADOW));
            inst.scaleBound = ConservativeScaleBound(world);
            inst.firstRef = u32(refTotal);
            inst.refCount = am.clusterCount;
            inst.firstPage = am.firstPage;
            inst.historyValid = historyValid;
            inst.prevScaleBound = historyValid ? ConservativeScaleBound(prevWorld) : inst.scaleBound;
            inst.hemiScale = 0.0f;
            inst.hemiBias = 1.0f;
            inst.lightmapTexture = UINT32_MAX;

            refTotal += am.clusterCount;
            nodeTotal += am.nodeCount;
            if (refTotal > UINT32_MAX || nodeTotal > UINT32_MAX)
                FATAL("[GPUCulling] dynamic geometry reference count exceeds the addressable range");

            const u32 instanceIndex = set.instanceCount + u32(m_dynamicGeoInstanceData.size());
            m_dynamicGeoInstanceData.push_back(inst);
            for (u32 c = 0; c < am.clusterCount; ++c) {
                m_dynamicRefData.push_back(instanceIndex);
                m_dynamicRefData.push_back(am.firstCluster + c);
            }
        };

        for (u32 i = 0; i < dynamicCount; ++i) {
            const ClusterMeshKey& key = m_dynamicBatchKeys[i];
            if (key.indexCount == 0) {
                ++residual;
                continue;
            }

            u32 extraFlags = GPU_CLUSTER_ENTRY_DYNAMIC;
            if (m_dynamicObjectFlags[i] & GPU_OBJECT_NO_RESOLVE) {
                if (!MaterialCastsShadow(m_dynamicMaterialIDData[i]))
                    continue;
                extraFlags |= GPU_CLUSTER_ENTRY_SHADOW_ONLY | GPU_CLUSTER_ENTRY_AT;
            }
            if (m_dynamicObjectFlags[i] & GPU_OBJECT_SHADOW_ONLY)
                extraFlags |= GPU_CLUSTER_ENTRY_SHADOW_ONLY;

            emitDynamic(key, m_dynamicInstanceData[i].world, m_dynamicMaterialIDData[i],
                m_dynamicInstanceIdentities[i], extraFlags);
        }

        xr_set<GeometryInstanceKey> emittedTransparent;
        if (geometry)
        {
            for (const GeometryBatch& batch : geometry->GetBatches())
            {
                if (batch.isSkinned || batch.isTerrain || !batch.IsStrictB2F())
                    continue;
                if (!batch.megaBufferAlloc.valid)
                    continue;
                if (!MaterialCastsShadow(batch.bindlessMaterialID))
                    continue;

                GeometryInstanceKey identity;
                identity.renderable = batch.renderableLifetimeID;
                identity.visual = batch.visualLifetimeID;
                identity.subset = batch.geometrySubset;
                if (!emittedTransparent.insert(identity).second)
                    continue;

                ClusterMeshKey key = {};
                key.vertexOffset = batch.megaBufferAlloc.vertexOffset;
                key.indexOffset = batch.megaBufferAlloc.indexOffset;
                key.vertexCount = batch.megaBufferAlloc.vertexCount;
                key.indexCount = batch.megaBufferAlloc.indexCount;
                emitDynamic(key, batch.worldMatrix, batch.bindlessMaterialID, identity,
                    GPU_CLUSTER_ENTRY_DYNAMIC | GPU_CLUSTER_ENTRY_SHADOW_ONLY | GPU_CLUSTER_ENTRY_AT);
            }
        }

        set.dynamicRefCount = u32(refTotal) - set.refCount;
        set.dynamicInstanceCount = u32(m_dynamicGeoInstanceData.size());
        set.dynamicNodeRefs = u32(nodeTotal);
        set.dynamicResidualCount = residual;
    }

    m_dynamicHistoryIndex ^= 1u;
    m_dynamicHistoryFrame = Device.dwFrame;
}

bool GPUCullingManager::EnsureClusterStreamBuffers(nvrhi::IDevice* nvDevice)
{
    ClusterCullBuffers& set = m_clusterSet;
    if (!nvDevice)
        return false;

    const u64 requiredRefs = u64(set.refCount) + set.dynamicRefCount;
    const u64 requiredInstances = u64(set.instanceCount) + set.dynamicInstanceCount;
    const u64 requiredNodeRefs = u64(set.staticNodeRefs) + set.dynamicNodeRefs;
    if (requiredRefs > UINT32_MAX || requiredInstances > UINT32_MAX || requiredNodeRefs > UINT32_MAX)
        FATAL("[GPUCulling] geometry working set exceeds the addressable range");

    if (set.refCapacity >= requiredRefs && set.instanceCapacity >= requiredInstances
        && set.nodeRefCapacity >= requiredNodeRefs && set.refBuffer && set.instanceBuffer)
        return true;

    auto grow = [](u64 required, u32 current, u32 minimum) {
        u64 target = std::max<u64>(required + required / 2ull, minimum);
        target = std::max<u64>(target, current);
        if (target > UINT32_MAX)
            FATAL("[GPUCulling] geometry working set exceeds the addressable range");
        return u32(target);
    };

    const u32 refCapacity = grow(requiredRefs, set.refCapacity, requiredRefs ? 1024u : 1u);
    const u32 instanceCapacity = grow(requiredInstances, set.instanceCapacity, requiredInstances ? 256u : 1u);
    const u32 nodeRefCapacity = grow(requiredNodeRefs, set.nodeRefCapacity, requiredNodeRefs ? 1024u : 1u);
    const u64 deferredNodeCapacity = u64(nodeRefCapacity) + instanceCapacity;
    if (deferredNodeCapacity > UINT32_MAX)
        FATAL("[GPUCulling] geometry hierarchy working set exceeds the addressable range");

    auto makeStructured = [&](const char* name, u64 elements, u32 stride) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = std::max<u64>(elements, 1ull) * stride;
        desc.structStride = stride;
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };

    set.refBuffer = makeStructured("ClusterCull_References", refCapacity, sizeof(u32) * 2);
    set.instanceBuffer = makeStructured("ClusterCull_Instances", instanceCapacity, sizeof(GPUGeoInstance));
    set.visibleEntryBuffer = makeStructured("ClusterCull_VisibleEntries", refCapacity, sizeof(u32));
    set.fadeBuffer = makeStructured("ClusterCull_Fades", refCapacity, sizeof(u32));
    set.terrainVisibleEntryBuffer = makeStructured("ClusterCull_TerrainVisibleEntries", refCapacity, sizeof(u32));
    set.terrainFadeBuffer = makeStructured("ClusterCull_TerrainFades", refCapacity, sizeof(u32));
    set.candidateBuffer = makeStructured("ClusterCull_Candidates", refCapacity, sizeof(u32));
    set.visibleEntryBuffer2 = makeStructured("ClusterCull_RetestVisibleEntries", refCapacity, sizeof(u32));
    set.fadeBuffer2 = makeStructured("ClusterCull_RetestFades", refCapacity, sizeof(u32));
    set.terrainVisibleEntryBuffer2 = makeStructured("ClusterCull_RetestTerrainVisibleEntries", refCapacity, sizeof(u32));
    set.terrainFadeBuffer2 = makeStructured("ClusterCull_RetestTerrainFades", refCapacity, sizeof(u32));
    set.swEntryBuffer = makeStructured("ClusterCull_SwEntries", refCapacity, sizeof(u32));
    set.swEntryBuffer2 = makeStructured("ClusterCull_RetestSwEntries", refCapacity, sizeof(u32));
    set.leafQueueBuffer = makeStructured("ClusterCull_LeafQueue", refCapacity, sizeof(u32));
    set.nodeQueue[0] = makeStructured("ClusterCull_NodeQueueA", nodeRefCapacity, sizeof(u32) * 2);
    set.nodeQueue[1] = makeStructured("ClusterCull_NodeQueueB", nodeRefCapacity, sizeof(u32) * 2);
    set.deferredNodeBuffer = makeStructured("ClusterCull_DeferredNodes", deferredNodeCapacity, sizeof(u32) * 2);
    set.deferredInstanceBuffer = makeStructured("ClusterCull_DeferredInstances", instanceCapacity, sizeof(u32));
    set.nodeSinkBuffer = makeStructured("ClusterCull_NodeSink", 1u, sizeof(u32) * 2);
    set.u32SinkBuffer = makeStructured("ClusterCull_IndexSink", 1u, sizeof(u32));

    if (!set.refBuffer || !set.instanceBuffer || !set.visibleEntryBuffer || !set.fadeBuffer
        || !set.terrainVisibleEntryBuffer || !set.terrainFadeBuffer || !set.candidateBuffer
        || !set.visibleEntryBuffer2 || !set.fadeBuffer2 || !set.terrainVisibleEntryBuffer2
        || !set.terrainFadeBuffer2 || !set.swEntryBuffer || !set.swEntryBuffer2
        || !set.leafQueueBuffer || !set.nodeQueue[0] || !set.nodeQueue[1]
        || !set.deferredNodeBuffer || !set.deferredInstanceBuffer || !set.nodeSinkBuffer || !set.u32SinkBuffer) {
        Msg("! [GPUCulling] geometry stream buffer creation failed, disabling the cluster path");
        set = {};
        return false;
    }

    set.refCapacity = refCapacity;
    set.instanceCapacity = instanceCapacity;
    set.nodeRefCapacity = nodeRefCapacity;
    set.uploaded = false;

    Msg("* [GPUCulling] geometry stream capacity: %u references, %u instances, %u hierarchy node references (%.1f MB)",
        refCapacity, instanceCapacity, nodeRefCapacity,
        (u64(refCapacity) * (8ull + 13ull * sizeof(u32)) + u64(instanceCapacity) * (sizeof(GPUGeoInstance) + sizeof(u32))
            + deferredNodeCapacity * 8ull + u64(nodeRefCapacity) * 16ull) / (1024.0f * 1024.0f));
    return true;
}

bool GPUCullingManager::EnsureGeometryTableBuffers(nvrhi::IDevice* nvDevice)
{
    ClusterCullBuffers& set = m_clusterSet;
    if (!nvDevice || !m_residency.IsActive())
        return false;

    const xr_vector<GPUClusterMeta>& meta = m_clusterDAG.ClusterMeta();
    const xr_vector<GPUClusterAssetMember>& members = m_clusterDAG.AssetMembers();
    const xr_vector<GPUClusterAssetNode>& nodes = m_clusterDAG.AssetNodes();
    R_ASSERT(set.refCount + set.dynamicRefCount == 0
        || (!meta.empty() && !members.empty() && !nodes.empty()));

    auto makeImmutable = [&](const char* name, u64 elements, u32 stride) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = std::max<u64>(elements, 1ull) * stride;
        desc.structStride = stride;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };

    if (!set.metaBuffer)
        set.metaBuffer = makeImmutable("ClusterCull_Meta", meta.size(), sizeof(GPUClusterMeta));
    if (!set.memberBuffer)
        set.memberBuffer = makeImmutable("ClusterCull_AssetMembers", members.size(), sizeof(GPUClusterAssetMember));
    if (!set.assetNodeBuffer)
        set.assetNodeBuffer = makeImmutable("ClusterCull_AssetNodes", nodes.size(), sizeof(GPUClusterAssetNode));

    if (!set.countBuffer) {
        nvrhi::BufferDesc desc;
        desc.debugName = "ClusterCull_Count";
        desc.byteSize = sizeof(u32) * kClusterCountWords;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        set.countBuffer = nvDevice->createBuffer(desc);
    }

    if (!set.bvhNodeBuffer && set.refCount > 0) {
        xr_vector<ClusterBvhPrim> prims;
        prims.resize(set.refCount);
        set.shadowCasters.clear();
        set.shadowCasters.reserve(set.refCount);
        set.shadowCapacityWidth = 0.0f;

        for (u32 i = 0; i < set.refCount; ++i) {
            const u32 instanceIndex = m_clusterRefData[size_t(i) * 2 + 0];
            const u32 clusterIndex = m_clusterRefData[size_t(i) * 2 + 1];
            const GPUGeoInstance& inst = m_geoInstanceData[instanceIndex];
            const GPUClusterMeta& cm = meta[clusterIndex];

            Fvector center;
            inst.world.transform_tiny(center, Fvector().set(cm.sphere[0], cm.sphere[1], cm.sphere[2]));
            const Fvector half = Fvector().set(
                _abs(inst.world.i.x) * cm.extent[0] + _abs(inst.world.j.x) * cm.extent[1] + _abs(inst.world.k.x) * cm.extent[2],
                _abs(inst.world.i.y) * cm.extent[0] + _abs(inst.world.j.y) * cm.extent[1] + _abs(inst.world.k.y) * cm.extent[2],
                _abs(inst.world.i.z) * cm.extent[0] + _abs(inst.world.j.z) * cm.extent[1] + _abs(inst.world.k.z) * cm.extent[2]);

            ClusterBvhPrim& prim = prims[i];
            prim.lo.sub(center, half);
            prim.hi.add(center, half);
            prim.centroid = center;
            prim.lodCenter = center;
            prim.lodRadius = cm.sphere[3] * inst.scaleBound;
            const bool plain = (inst.flags & GPU_CLUSTER_ENTRY_PLAIN) != 0;
            prim.selfError = plain ? 0.0f : cm.selfError * inst.scaleBound;
            prim.parentError = plain ? 1e30f : std::min(cm.parentError * inst.scaleBound, 1e30f);
            prim.key = 0;
            prim.payload = i;

            ClusterShadowCaster caster;
            caster.radius = half.magnitude();
            caster.selfError = prim.selfError;
            caster.parentError = prim.parentError;
            const u32 clusterFlags = cm.flags | inst.flags;
            caster.stream = (clusterFlags & GPU_CLUSTER_ENTRY_AT) ? 2u
                : ((clusterFlags & GPU_CLUSTER_ENTRY_TERRAIN) ? 1u : 0u);
            set.shadowCasters.push_back(caster);
        }

        BuildClusterShadowBVH(prims.data(), set.refCount, m_shadowBvh);
        set.bvhNodeCount = u32(m_shadowBvh.nodes.size());
        set.bvhNodeBuffer = makeImmutable("ClusterCull_ShadowBvhNodes", m_shadowBvh.nodes.size(), sizeof(ClusterBvhNode));
        set.bvhIndexBuffer = makeImmutable("ClusterCull_ShadowBvhIndices", m_shadowBvh.indices.size(), sizeof(u32));
        Msg("* [GPUCulling] world shadow BVH: %u nodes, %u leaves, depth %u",
            set.bvhNodeCount, m_shadowBvh.leafCount, m_shadowBvh.maxDepth);
    }

    if (!set.metaBuffer || !set.memberBuffer || !set.assetNodeBuffer || !set.countBuffer
        || (set.refCount > 0 && (!set.bvhNodeBuffer || !set.bvhIndexBuffer))) {
        Msg("! [GPUCulling] geometry metadata buffer creation failed, disabling the cluster path");
        set = {};
        return false;
    }
    return true;
}

void GPUCullingManager::UploadGeometryTables(nvrhi::ICommandList* cmdList)
{
    ClusterCullBuffers& set = m_clusterSet;
    if (set.uploaded || !set.metaBuffer || !set.refBuffer || !set.instanceBuffer)
        return;

    const xr_vector<GPUClusterMeta>& meta = m_clusterDAG.ClusterMeta();
    const xr_vector<GPUClusterAssetMember>& members = m_clusterDAG.AssetMembers();
    const xr_vector<GPUClusterAssetNode>& nodes = m_clusterDAG.AssetNodes();

    if (!meta.empty())
        cmdList->writeBuffer(set.metaBuffer, meta.data(), meta.size() * sizeof(GPUClusterMeta));
    if (!members.empty())
        cmdList->writeBuffer(set.memberBuffer, members.data(), members.size() * sizeof(GPUClusterAssetMember));
    if (!nodes.empty())
        cmdList->writeBuffer(set.assetNodeBuffer, nodes.data(), nodes.size() * sizeof(GPUClusterAssetNode));

    if (set.refCount > 0)
        cmdList->writeBuffer(set.refBuffer, m_clusterRefData.data(), u64(set.refCount) * 2ull * sizeof(u32));
    if (set.instanceCount > 0)
        cmdList->writeBuffer(set.instanceBuffer, m_geoInstanceData.data(), u64(set.instanceCount) * sizeof(GPUGeoInstance));

    if (set.bvhNodeBuffer && !m_shadowBvh.nodes.empty()) {
        cmdList->writeBuffer(set.bvhNodeBuffer, m_shadowBvh.nodes.data(), m_shadowBvh.nodes.size() * sizeof(ClusterBvhNode));
        cmdList->writeBuffer(set.bvhIndexBuffer, m_shadowBvh.indices.data(), m_shadowBvh.indices.size() * sizeof(u32));
    }

    u32 zeroCount[kClusterCountWords] = {};
    cmdList->writeBuffer(set.countBuffer, zeroCount, sizeof(zeroCount));
    u32 zeroArgs[8] = { 384, 0, 0, 0, 0, 0, 1, 0 };
    cmdList->writeBuffer(m_clusterArgsBuffer, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterTerrainArgsBuffer, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterArgsBuffer2, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterTerrainArgsBuffer2, zeroArgs, sizeof(zeroArgs));
    u32 zeroSwArgs[8] = {};
    cmdList->writeBuffer(m_clusterSwArgsBuffer, zeroSwArgs, sizeof(zeroSwArgs));
    cmdList->writeBuffer(m_clusterSwArgsBuffer2, zeroSwArgs, sizeof(zeroSwArgs));
    u32 zeroDispatchArgs[16] = {};
    for (u32 i = 0; i < 4; ++i) {
        zeroDispatchArgs[i * 4 + 1] = 1;
        zeroDispatchArgs[i * 4 + 2] = 1;
    }
    cmdList->writeBuffer(m_clusterRetestDispatchArgs, zeroDispatchArgs, sizeof(zeroDispatchArgs));
    u32 zeroQueueArgs[4 * kClusterQueueArgsSlots] = {};
    for (u32 i = 0; i < kClusterQueueArgsSlots; ++i) {
        zeroQueueArgs[i * 4 + 1] = 1;
        zeroQueueArgs[i * 4 + 2] = 1;
    }
    cmdList->writeBuffer(m_clusterQueueArgsBuffer, zeroQueueArgs, sizeof(zeroQueueArgs));

    set.uploaded = true;

    const ClusterPayloadStats& payloadStats = m_clusterDAG.PayloadStats();
    const GeometryResidencyStats& residency = m_residency.Stats();
    Msg("* [GPUCulling] geometry tables uploaded: %u clusters, %u members, %u nodes, %u pages, %u references (%.2f MB shared metadata)",
        u32(meta.size()), u32(members.size()), u32(nodes.size()), residency.pages, set.refCount,
        (meta.size() * sizeof(GPUClusterMeta) + members.size() * sizeof(GPUClusterAssetMember)
            + nodes.size() * sizeof(GPUClusterAssetNode)
            + u64(residency.pages) * sizeof(GPUClusterPage)) / (1024.0f * 1024.0f));
    Msg("* [GPUCulling] residency arenas: %.2f MB vertices + %.2f MB payload, pinned minimum %.2f MB, %s",
        residency.vertexArenaBytes / (1024.0f * 1024.0f),
        residency.payloadArenaBytes / (1024.0f * 1024.0f),
        (residency.pinnedVertexBytes + residency.pinnedPayloadBytes) / (1024.0f * 1024.0f),
        residency.streaming ? "demand paged" : "fully resident");
    Msg("* [GPUCulling] per-frame instance and reference tables %.2f MB; retained source %.2f MB vertices + %.2f MB indices for ray tracing and forward draws",
        (u64(set.instanceCapacity) * sizeof(GPUGeoInstance) + u64(set.refCapacity) * 2ull * sizeof(u32))
            / (1024.0f * 1024.0f),
        (u64(m_totalVertexCount) * sizeof(bindless::UnifiedVertex)) / (1024.0f * 1024.0f),
        (u64(m_totalIndexCount) * sizeof(u32)) / (1024.0f * 1024.0f));
    Msg("* [GPUCulling] page duplication: %llu page vertex slots for %llu distinct source vertices, %llu cluster references, %u recluster splits",
        (unsigned long long)payloadStats.pageVertexSlots,
        (unsigned long long)payloadStats.distinctSourceVertices,
        (unsigned long long)payloadStats.clusterVertexReferences,
        payloadStats.reclusterSplits);
}

GeometryMemoryStats GPUCullingManager::GetGeometryMemoryStats() const
{
    const auto bytes = [](nvrhi::IBuffer* buffer) -> u64
    {
        return buffer ? buffer->getDesc().byteSize : 0;
    };
    GeometryMemoryStats stats;
    const GeometryResidencyStats& residency = m_residency.Stats();
    stats.sharedMetadataBytes = bytes(m_clusterSet.metaBuffer)
        + bytes(m_clusterSet.memberBuffer) + bytes(m_clusterSet.assetNodeBuffer)
        + bytes(m_residency.GetPageTableBuffer()) + bytes(m_residency.GetClusterGroupBuffer())
        + bytes(m_residency.GetGroupBitsBuffer()) + bytes(m_residency.GetShadowCutBuffer());
    stats.instanceTableBytes = bytes(m_clusterSet.instanceBuffer) + bytes(m_clusterSet.refBuffer)
        + bytes(m_clusterSet.bvhNodeBuffer) + bytes(m_clusterSet.bvhIndexBuffer);
    stats.payloadBytes = bytes(m_residency.GetPayloadArenaBuffer());
    stats.vertexBytes = bytes(m_residency.GetVertexArenaBuffer());
    stats.retainedSourceBytes = bytes(m_megaVertexBuffer) + bytes(m_megaIndexBuffer)
        + bytes(m_rtVertexBuffer) + bytes(m_rtIndexBuffer);
    stats.forwardDrawBytes = bytes(m_transparentDrawArgsBuffer) + bytes(m_transparentInstanceBuffer)
        + bytes(m_skinnedForwardArgsBuffer) + bytes(m_skinnedForwardInstanceBuffer) + bytes(m_forwardDrawIndexBuffer);
    stats.retiringSourceBytes = bytes(m_megaCopySourceVB) + bytes(m_megaCopySourceIB)
        + bytes(m_rtCopySourceVertexBuffer) + bytes(m_rtCopySourceIndexBuffer);
    stats.sourceStagingBytes = m_megaVertices.capacity() * sizeof(bindless::UnifiedVertex)
        + m_megaIndices.capacity() * sizeof(u32) + m_megaSourceNormals.capacity() * sizeof(Fvector3)
        + m_runtimeVertices.capacity() * sizeof(bindless::UnifiedVertex)
        + m_runtimeIndices.capacity() * sizeof(u32) + m_runtimeSourceNormals.capacity() * sizeof(Fvector3)
        + m_forwardVertices.capacity() * sizeof(ForwardVertex) + m_forwardIndices.capacity() * sizeof(u32)
        + m_forwardDrawIndices.capacity() * sizeof(u32) + m_rtVertexStaging.capacity()
        + m_rtIndexStaging.capacity() * sizeof(u32) + m_rtSubmittedVertexStaging.capacity()
        + m_rtSubmittedIndexStaging.capacity() * sizeof(u32) + m_clusterDAG.PageVertexData().capacity()
        + m_clusterDAG.PagePayloadData().capacity() + m_clusterDAG.RuntimePageVertexData().capacity()
        + m_clusterDAG.RuntimePagePayloadData().capacity();
    stats.hostSourceBytes = GetStagingHostMemoryUsage();
    stats.forwardUploadLeases = u32(m_forwardUploads.size());
    for (size_t i = 0; i < m_forwardUploads.size(); ++i)
    {
        const auto& upload = m_forwardUploads[i];
        stats.sourceStagingBytes += upload.vertexStaging.capacity() * sizeof(ForwardVertex)
            + upload.indexStaging.capacity() * sizeof(u32) + upload.drawIndexStaging.capacity() * sizeof(u32);
        for (auto* buffer : { upload.vertices.Get(), upload.indices.Get(), upload.drawIndices.Get(),
            upload.copyVertices.Get(), upload.copyIndices.Get() })
        {
            if (!buffer || buffer == m_megaVertexBuffer || buffer == m_megaIndexBuffer
                || buffer == m_forwardDrawIndexBuffer || buffer == m_megaCopySourceVB || buffer == m_megaCopySourceIB)
                continue;
            bool first = true;
            for (size_t j = 0; j < i; ++j)
            {
                const auto& previous = m_forwardUploads[j];
                first &= buffer != previous.vertices && buffer != previous.indices && buffer != previous.drawIndices
                    && buffer != previous.copyVertices && buffer != previous.copyIndices;
            }
            if (first)
                stats.retiringSourceBytes += bytes(buffer);
        }
    }
    stats.residencyArenaBytes = residency.vertexArenaBytes + residency.payloadArenaBytes;
    stats.residencyUsedBytes = residency.vertexUsedBytes + residency.payloadUsedBytes;
    stats.residencyPinnedBytes = residency.pinnedVertexBytes + residency.pinnedPayloadBytes;
    stats.residencyStagingBytes = residency.stagingBytes;
    return stats;
}

bool GPUCullingManager::GetShadowPairCapacity(float pageWidth, float errorThreshold, u32 pagesAxis, u32* capacity)
{
    auto& state = m_clusterSet;
    if (state.shadowCapacityWidth != pageWidth || state.shadowCapacityError != errorThreshold || state.shadowCapacityAxis != pagesAxis) {
        state.shadowCapacityWidth = pageWidth;
        state.shadowCapacityError = errorThreshold;
        state.shadowCapacityAxis = pagesAxis;
        state.shadowCapacityValid = ClusterShadowPairCapacity(state.shadowCasters,
            pageWidth, errorThreshold, pagesAxis, state.shadowCapacity);
        if (!state.shadowCapacityValid)
            Msg("! [GPUCulling] shadow pair capacity exceeds the supported draw range");
    }
    if (!state.shadowCapacityValid)
        return false;
    for (u32 i = 0; i < 3; ++i)
        capacity[i] = state.shadowCapacity[i];
    return true;
}

struct ClusterCullParamsCB {
    Fmatrix hizViewProj;
    Fvector4 frustumPlanes[6];
    Fvector4 cameraPos;
    Fvector4 viewDir;
    Fvector4 lodParams;
    u32 instanceCount;
    u32 useHiZ;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    float ssaCull;
    float swCull;
    float swNearZ;
    u32 phase;
    u32 srcCountOffset;
    u32 dstCountOffset;
    u32 coarseHiZ;
    u32 refCount;
    u32 nodeQueueCapacity;
    u32 leafQueueCapacity;
    u32 deferredNodeCapacity;
    u32 staticHistoryValid;
    u32 residencyStreaming;
    u32 paramsPad1;
    u32 paramsPad2;
};
static_assert(sizeof(ClusterCullParamsCB) == 288, "ClusterCullParams must match the shader constant buffer");

struct ClusterQueueParamsCB {
    u32 srcCountOffset;
    u32 resetCountOffset;
    u32 argsSlot;
    u32 groupSize;
};

bool GPUCullingManager::EnsureClusterCullPipeline(nvrhi::IDevice* nvDevice)
{
    if (m_clusterInstancePipeline && m_clusterNodePipeline && m_clusterLeafPipeline
        && m_clusterQueueArgsPipeline && m_clusterArgsPipeline
        && m_clusterInstanceLayout && m_clusterNodeLayout && m_clusterLeafLayout
        && m_clusterQueueArgsLayout && m_clusterArgsLayout
        && m_clusterCullParamsCB.IsValid() && m_clusterArgsParamsCB.IsValid() && m_clusterQueueParamsCB.IsValid())
        return true;

    auto* loader = GEnv.Render->GetShaderLoader();
    auto instanceResult = loader->LoadComputeShader("cluster_instance_cull");
    auto nodeResult = loader->LoadComputeShader("cluster_node_cull");
    auto leafResult = loader->LoadComputeShader("cluster_leaf_cull");
    auto queueResult = loader->LoadComputeShader("cluster_queue_args");
    auto argsResult = loader->LoadComputeShader("cluster_draw_args");
    if (!instanceResult.handle || !nodeResult.handle || !leafResult.handle || !queueResult.handle || !argsResult.handle) {
        Msg("! [GPUCulling] cluster traversal shaders failed to load");
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    auto* instanceRefl = loader->GetCachedReflection("cluster_instance_cull", ".cs");
    auto* nodeRefl = loader->GetCachedReflection("cluster_node_cull", ".cs");
    auto* leafRefl = loader->GetCachedReflection("cluster_leaf_cull", ".cs");
    auto* queueRefl = loader->GetCachedReflection("cluster_queue_args", ".cs");
    auto* argsRefl = loader->GetCachedReflection("cluster_draw_args", ".cs");
    if (!instanceRefl || !nodeRefl || !leafRefl || !queueRefl || !argsRefl)
        return false;

    m_clusterInstanceLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterInstance", *instanceRefl, nvDevice);
    m_clusterNodeLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterNode", *nodeRefl, nvDevice);
    m_clusterLeafLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterLeaf", *leafRefl, nvDevice);
    m_clusterQueueArgsLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterQueueArgs", *queueRefl, nvDevice);
    m_clusterArgsLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterArgs", *argsRefl, nvDevice);
    if (!m_clusterInstanceLayout || !m_clusterNodeLayout || !m_clusterLeafLayout
        || !m_clusterQueueArgsLayout || !m_clusterArgsLayout)
        return false;

    auto makePipeline = [&](nvrhi::IShader* shader, nvrhi::IBindingLayout* layout) {
        nvrhi::ComputePipelineDesc desc;
        desc.CS = shader;
        desc.bindingLayouts = { layout };
        return nvDevice->createComputePipeline(desc);
    };
    m_clusterInstancePipeline = makePipeline(instanceResult.handle, m_clusterInstanceLayout);
    m_clusterNodePipeline = makePipeline(nodeResult.handle, m_clusterNodeLayout);
    m_clusterLeafPipeline = makePipeline(leafResult.handle, m_clusterLeafLayout);
    m_clusterQueueArgsPipeline = makePipeline(queueResult.handle, m_clusterQueueArgsLayout);
    m_clusterArgsPipeline = makePipeline(argsResult.handle, m_clusterArgsLayout);
    if (!m_clusterInstancePipeline || !m_clusterNodePipeline || !m_clusterLeafPipeline
        || !m_clusterQueueArgsPipeline || !m_clusterArgsPipeline)
        return false;

    auto makeCB = [&](fg::BufferHandle& handle, const char* name, u32 size, u32 versions) {
        if (handle.IsValid())
            return true;
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = size;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = versions;
        handle = m_device->CreateBuffer(desc);
        return handle.IsValid();
    };

    if (!makeCB(m_clusterArgsParamsCB, "ClusterCull_ArgsParams", 16, 64))
        return false;
    if (!makeCB(m_clusterQueueParamsCB, "ClusterCull_QueueParams", sizeof(ClusterQueueParamsCB), 256))
        return false;
    return makeCB(m_clusterCullParamsCB, "ClusterCull_Params", sizeof(ClusterCullParamsCB), 512);
}

void GPUCullingManager::DispatchClusterArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice, u32 countBase, u32 swCountOffset,
    nvrhi::IBuffer* args, nvrhi::IBuffer* terrainArgs, nvrhi::IBuffer* swArgs, nvrhi::IBuffer* retestDispatchArgs)
{
    struct ClusterArgsParams {
        u32 countBase;
        u32 swCountOffset;
        u32 pad[2];
    };
    ClusterArgsParams cb = {};
    cb.countBase = countBase;
    cb.swCountOffset = swCountOffset;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterArgsParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(args, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(terrainArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(swArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(retestDispatchArgs, nvrhi::ResourceStates::UnorderedAccess);

    auto* argsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_draw_args", ".cs");
    if (!argsRefl)
        FATAL("[GPUCulling] cluster draw-argument reflection was lost during frame execution");
    framegraph::BindingSetBuilder argsBsb(*argsRefl, nvDevice, "GPUCull.ClusterArgs");
    argsBsb.ConstantBuffer("ClusterArgsParams", m_device->GetNativeBuffer(m_clusterArgsParamsCB))
           .BufferSRV("g_Count", m_clusterSet.countBuffer)
           .BufferUAV("g_Args", args)
           .BufferUAV("g_TerrainArgs", terrainArgs)
           .BufferUAV("g_SwArgs", swArgs)
           .BufferUAV("g_RetestArgs", retestDispatchArgs);

    nvrhi::BindingSetHandle argsBindingSet = nvDevice->createBindingSet(argsBsb.Build(), m_clusterArgsLayout);
    if (!argsBindingSet)
        FATAL("[GPUCulling] cluster draw-argument binding set creation failed during frame execution");

    nvrhi::ComputeState argsState;
    argsState.pipeline = m_clusterArgsPipeline;
    argsState.bindings = { argsBindingSet };
    cmdList->setComputeState(argsState);
    cmdList->dispatch(1, 1, 1);

    cmdList->setBufferState(args, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(terrainArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(swArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(retestDispatchArgs, nvrhi::ResourceStates::IndirectArgument);
}

void GPUCullingManager::DispatchQueueArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    u32 srcCountOffset, u32 resetCountOffset, u32 slot, u32 groupSize)
{
    ClusterQueueParamsCB cb = {};
    cb.srcCountOffset = srcCountOffset;
    cb.resetCountOffset = resetCountOffset;
    cb.argsSlot = slot;
    cb.groupSize = groupSize;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterQueueParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterQueueArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);

    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_queue_args", ".cs");
    if (!refl)
        FATAL("[GPUCulling] cluster queue-argument reflection was lost during frame execution");
    framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.ClusterQueueArgs");
    bsb.ConstantBuffer("ClusterQueueParams", m_device->GetNativeBuffer(m_clusterQueueParamsCB))
       .BufferUAV("g_Count", m_clusterSet.countBuffer)
       .BufferUAV("g_QueueArgs", m_clusterQueueArgsBuffer);

    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterQueueArgsLayout);
    if (!bindingSet)
        FATAL("[GPUCulling] cluster queue-argument binding set creation failed during frame execution");

    nvrhi::ComputeState state;
    state.pipeline = m_clusterQueueArgsPipeline;
    state.bindings = { bindingSet };
    cmdList->setComputeState(state);
    cmdList->dispatch(1, 1, 1);

    cmdList->setBufferState(m_clusterQueueArgsBuffer,
        nvrhi::ResourceStates::IndirectArgument | nvrhi::ResourceStates::NonPixelShaderResource);
}

void GPUCullingManager::FillClusterCullParams(ClusterCullParamsCB& cb, const Fmatrix& hizViewProj,
    bool useHiZ, u32 hizWidth, u32 hizHeight, u32 hizMipLevels)
{
    cb = {};
    cb.hizViewProj = hizViewProj;
    ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
    cb.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.0f);
    Fvector dir = Device.vCameraDirection;
    dir.normalize_safe();
    cb.viewDir.set(dir.x, dir.y, dir.z, 0.0f);

    const float lodPx = std::max(0.05f, ps_r_cluster_lod);
    const float pxScale = Device.mProject._22 * float(Device.dwHeight) * 0.5f;
    cb.lodParams.set(pxScale / lodPx, 0.01f, ps_r_cluster_fade, (ps_r_cluster_lod <= 0.0501f) ? 1.0f : 0.0f);
    cb.instanceCount = m_clusterSet.instanceCount + m_clusterSet.dynamicInstanceCount;
    cb.useHiZ = useHiZ ? 1u : 0u;
    cb.hizWidth = useHiZ ? hizWidth : 1u;
    cb.hizHeight = useHiZ ? hizHeight : 1u;
    cb.hizMipLevels = useHiZ ? hizMipLevels : 1u;
    cb.ssaCull = 0.0f;
    cb.swCull = m_clusterSwCull;
    cb.swNearZ = m_clusterSwNearZ;
    cb.phase = 0;
    cb.srcCountOffset = 48;
    cb.dstCountOffset = 52;
    cb.coarseHiZ = useHiZ ? 1u : 0u;
    cb.refCount = m_clusterSet.refCount + m_clusterSet.dynamicRefCount;
    cb.nodeQueueCapacity = m_clusterSet.nodeRefCapacity;
    cb.leafQueueCapacity = m_clusterSet.refCapacity;
    cb.deferredNodeCapacity = m_clusterSet.nodeRefCapacity + m_clusterSet.instanceCapacity;
    cb.staticHistoryValid = m_staticHistoryValid ? 1u : 0u;
    cb.residencyStreaming = m_residency.IsStreaming() ? 1u : 0u;
    cb.paramsPad1 = 0;
    cb.paramsPad2 = 0;
}

void GPUCullingManager::DispatchClusterLeaves(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* hiz, nvrhi::IBuffer* queue, u32 argsSlot, bool late, bool allowDefer)
{
    ClusterCullBuffers& set = m_clusterSet;
    nvrhi::IBuffer* visible = late ? set.visibleEntryBuffer2.Get() : set.visibleEntryBuffer.Get();
    nvrhi::IBuffer* fades = late ? set.fadeBuffer2.Get() : set.fadeBuffer.Get();
    nvrhi::IBuffer* terrainVisible = late ? set.terrainVisibleEntryBuffer2.Get() : set.terrainVisibleEntryBuffer.Get();
    nvrhi::IBuffer* terrainFades = late ? set.terrainFadeBuffer2.Get() : set.terrainFadeBuffer.Get();
    nvrhi::IBuffer* swEntries = late ? set.swEntryBuffer2.Get() : set.swEntryBuffer.Get();

    cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(queue, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(visible, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(fades, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(terrainVisible, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(terrainFades, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(swEntries, nvrhi::ResourceStates::UnorderedAccess);
    if (allowDefer)
        cmdList->setBufferState(set.candidateBuffer, nvrhi::ResourceStates::UnorderedAccess);

    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_leaf_cull", ".cs");
    if (!refl)
        FATAL("[GPUCulling] cluster leaf reflection was lost during frame execution");
    framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.ClusterLeaf");
    bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
       .BufferSRV("g_ClusterRefs", set.refBuffer)
       .BufferSRV("g_ClusterMeta", set.metaBuffer)
       .BufferSRV("g_GeoInstances", set.instanceBuffer)
       .Texture("g_HiZPyramid", hiz ? hiz : m_dummyHiZ.Get())
       .BufferSRV("g_LeafQueue", queue)
       .BufferUAV("g_OutCount", set.countBuffer)
       .BufferUAV("g_OutEntryIndices", visible)
       .BufferUAV("g_OutFades", fades)
       .BufferUAV("g_OutTerrainEntryIndices", terrainVisible)
       .BufferUAV("g_OutTerrainFades", terrainFades)
       .BufferUAV("g_OutCandidates", set.candidateBuffer)
       .BufferUAV("g_OutSwEntries", swEntries)
       .BufferSRV("g_ClusterGroups", m_residency.GetClusterGroupBuffer())
       .BufferSRV("g_ClusterGroupState", m_residency.GetGroupBitsBuffer());

    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterLeafLayout);
    if (!bindingSet)
        FATAL("[GPUCulling] cluster leaf binding set creation failed during frame execution");

    nvrhi::ComputeState state;
    state.pipeline = m_clusterLeafPipeline;
    state.bindings = { bindingSet };
    state.indirectParams = m_clusterQueueArgsBuffer;
    cmdList->setComputeState(state);
    cmdList->dispatchIndirect(argsSlot * 16u);
}

void GPUCullingManager::DispatchClusterTraversal(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* hiz, const Fmatrix& hizViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels, bool late)
{
    ClusterCullBuffers& set = m_clusterSet;

    ClusterCullParamsCB cb;
    FillClusterCullParams(cb, hiz ? hizViewProj : Device.mFullTransform, hiz != nullptr,
        hizWidth, hizHeight, hizMipLevels);
    cb.phase = late ? 1u : 0u;
    if (cb.lodParams.z > 0.0f && !late)
        cb.coarseHiZ = 0u;

    auto* instanceRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_instance_cull", ".cs");
    auto* nodeRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_node_cull", ".cs");
    if (!instanceRefl || !nodeRefl)
        FATAL("[GPUCulling] cluster traversal reflection was lost during frame execution");

    nvrhi::ITexture* hizTexture = hiz ? hiz : m_dummyHiZ.Get();
    nvrhi::IBuffer* rootQueue = late ? set.deferredNodeBuffer.Get() : set.nodeQueue[0].Get();
    const u32 rootCountOffset = late ? 56u : 48u;

    {
        cb.srcCountOffset = late ? 60u : 0u;
        cb.dstCountOffset = rootCountOffset;
        cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));

        cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(rootQueue, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(set.deferredInstanceBuffer,
            late ? nvrhi::ResourceStates::ShaderResource : nvrhi::ResourceStates::UnorderedAccess);

        framegraph::BindingSetBuilder bsb(*instanceRefl, nvDevice, "GPUCull.ClusterInstance");
        bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
           .BufferSRV("g_GeoInstances", set.instanceBuffer)
           .BufferSRV("g_AssetMembers", set.memberBuffer)
           .BufferSRV("g_AssetNodes", set.assetNodeBuffer)
           .Texture("g_HiZPyramid", hizTexture)
           .BufferSRV("g_DeferredInstanceList", late ? set.deferredInstanceBuffer.Get() : set.u32SinkBuffer.Get())
           .BufferUAV("g_OutCount", set.countBuffer)
           .BufferUAV("g_OutNodes", rootQueue)
           .BufferUAV("g_OutDeferredInstances", late ? set.u32SinkBuffer.Get() : set.deferredInstanceBuffer.Get());

        nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterInstanceLayout);
        if (!bindingSet)
            FATAL("[GPUCulling] cluster instance binding set creation failed during frame execution");

        nvrhi::ComputeState state;
        state.pipeline = m_clusterInstancePipeline;
        state.bindings = { bindingSet };
        if (late) {
            state.indirectParams = m_clusterRetestDispatchArgs;
            cmdList->setComputeState(state);
            cmdList->dispatchIndirect(0);
        } else {
            cmdList->setComputeState(state);
            const u32 groups = (cb.instanceCount + kClusterTraversalGroup - 1u) / kClusterTraversalGroup;
            const u32 rows = (groups + 1023u) / 1024u;
            cmdList->dispatch(rows > 1u ? 1024u : groups, std::max(rows, 1u), 1u);
        }
    }

    nvrhi::IBuffer* srcQueue = rootQueue;
    nvrhi::IBuffer* dstQueue = set.nodeQueue[late ? 0u : 1u].Get();
    u32 srcCount = rootCountOffset;
    u32 dstCount = late ? 48u : 52u;
    u32 pingPong = late ? 0u : 1u;

    for (u32 level = 0; level < set.traversalDepth; ++level) {
        DispatchQueueArgs(cmdList, nvDevice, srcCount, dstCount, 0u, kClusterTraversalGroup);

        cb.srcCountOffset = srcCount;
        cb.dstCountOffset = dstCount;
        cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));

        nvrhi::IBuffer* deferSink = late ? set.nodeSinkBuffer.Get() : set.deferredNodeBuffer.Get();
        cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(srcQueue, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(dstQueue, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(deferSink, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(set.leafQueueBuffer, nvrhi::ResourceStates::UnorderedAccess);

        framegraph::BindingSetBuilder bsb(*nodeRefl, nvDevice, "GPUCull.ClusterNode");
        bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
           .BufferSRV("g_GeoInstances", set.instanceBuffer)
           .BufferSRV("g_AssetMembers", set.memberBuffer)
           .BufferSRV("g_AssetNodes", set.assetNodeBuffer)
           .Texture("g_HiZPyramid", hizTexture)
           .BufferSRV("g_SrcNodes", srcQueue)
           .BufferUAV("g_OutCount", set.countBuffer)
           .BufferUAV("g_OutNodes", dstQueue)
           .BufferUAV("g_OutDeferredNodes", deferSink)
           .BufferUAV("g_OutLeaves", set.leafQueueBuffer);

        nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterNodeLayout);
        if (!bindingSet)
            FATAL("[GPUCulling] cluster node binding set creation failed during frame execution");

        nvrhi::ComputeState state;
        state.pipeline = m_clusterNodePipeline;
        state.bindings = { bindingSet };
        state.indirectParams = m_clusterQueueArgsBuffer;
        cmdList->setComputeState(state);
        cmdList->dispatchIndirect(0);

        srcQueue = dstQueue;
        srcCount = dstCount;
        pingPong ^= 1u;
        dstQueue = set.nodeQueue[pingPong].Get();
        dstCount = (pingPong == 0u) ? 48u : 52u;
    }

    const u32 leafCountOffset = late ? 68u : 64u;
    DispatchQueueArgs(cmdList, nvDevice, leafCountOffset, 0xFFFFFFFFu, 1u, kClusterTraversalGroup);
    cb.srcCountOffset = leafCountOffset;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));
    DispatchClusterLeaves(cmdList, nvDevice, hiz, set.leafQueueBuffer, 1u, late, !late);
}

void GPUCullingManager::DispatchClusterCull(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* prevHiZ, const Fmatrix& prevViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels)
{
    ClusterCullBuffers& set = m_clusterSet;
    if (!set.uploaded)
        return;
    if (!EnsureClusterCullPipeline(nvDevice))
        FATAL("[GPUCulling] cluster visibility pipeline readiness was lost during frame execution");

    u32 zero[kClusterCountWords] = {};
    cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(set.countBuffer, zero, sizeof(zero));
    cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::UnorderedAccess);

    if (set.refCount + set.dynamicRefCount != 0)
        DispatchClusterTraversal(cmdList, nvDevice, prevHiZ, prevViewProj, hizWidth, hizHeight, hizMipLevels, false);

    cmdList->setBufferState(set.visibleEntryBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(set.fadeBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(set.terrainVisibleEntryBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(set.terrainFadeBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(set.candidateBuffer, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(set.swEntryBuffer, nvrhi::ResourceStates::NonPixelShaderResource);

    DispatchClusterArgs(cmdList, nvDevice, 0, 40, m_clusterArgsBuffer, m_clusterTerrainArgsBuffer, m_clusterSwArgsBuffer,
        m_clusterRetestDispatchArgs);
}

void GPUCullingManager::DispatchClusterRetest(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* hiz, u32 hizWidth, u32 hizHeight, u32 hizMipLevels)
{
    ClusterCullBuffers& set = m_clusterSet;
    if (set.refCount + set.dynamicRefCount == 0 || !set.uploaded || !hiz)
        return;
    if (!m_clusterInstancePipeline || !m_clusterNodePipeline || !m_clusterLeafPipeline
        || !m_clusterQueueArgsPipeline || !m_clusterArgsPipeline || !m_clusterRetestDispatchArgs)
        FATAL("[GPUCulling] cluster retest pipeline readiness was lost during frame execution");

    DispatchClusterTraversal(cmdList, nvDevice, hiz, Device.mFullTransform, hizWidth, hizHeight, hizMipLevels, true);

    ClusterCullParamsCB cb;
    FillClusterCullParams(cb, Device.mFullTransform, true, hizWidth, hizHeight, hizMipLevels);
    cb.phase = 1u;
    cb.srcCountOffset = 16u;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterRetestDispatchArgs, nvrhi::ResourceStates::IndirectArgument);
    {
        nvrhi::IBuffer* visible = set.visibleEntryBuffer2.Get();
        nvrhi::IBuffer* fades = set.fadeBuffer2.Get();
        nvrhi::IBuffer* terrainVisible = set.terrainVisibleEntryBuffer2.Get();
        nvrhi::IBuffer* terrainFades = set.terrainFadeBuffer2.Get();
        nvrhi::IBuffer* swEntries = set.swEntryBuffer2.Get();

        cmdList->setBufferState(set.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(set.candidateBuffer, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(visible, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(fades, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(terrainVisible, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(terrainFades, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(swEntries, nvrhi::ResourceStates::UnorderedAccess);

        auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_leaf_cull", ".cs");
        if (!refl)
            FATAL("[GPUCulling] cluster candidate reflection was lost during frame execution");
        framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.ClusterLeafRetest");
        bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
           .BufferSRV("g_ClusterRefs", set.refBuffer)
           .BufferSRV("g_ClusterMeta", set.metaBuffer)
           .BufferSRV("g_GeoInstances", set.instanceBuffer)
           .Texture("g_HiZPyramid", hiz)
           .BufferSRV("g_LeafQueue", set.candidateBuffer)
           .BufferUAV("g_OutCount", set.countBuffer)
           .BufferUAV("g_OutEntryIndices", visible)
           .BufferUAV("g_OutFades", fades)
           .BufferUAV("g_OutTerrainEntryIndices", terrainVisible)
           .BufferUAV("g_OutTerrainFades", terrainFades)
           .BufferUAV("g_OutCandidates", set.leafQueueBuffer)
           .BufferUAV("g_OutSwEntries", swEntries)
           .BufferSRV("g_ClusterGroups", m_residency.GetClusterGroupBuffer())
           .BufferSRV("g_ClusterGroupState", m_residency.GetGroupBitsBuffer());

        nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterLeafLayout);
        if (!bindingSet)
            FATAL("[GPUCulling] cluster candidate binding set creation failed during frame execution");

        nvrhi::ComputeState state;
        state.pipeline = m_clusterLeafPipeline;
        state.bindings = { bindingSet };
        state.indirectParams = m_clusterRetestDispatchArgs;
        cmdList->setComputeState(state);
        cmdList->dispatchIndirect(16u);
    }

    cmdList->setBufferState(set.visibleEntryBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.fadeBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.terrainVisibleEntryBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.terrainFadeBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.swEntryBuffer2, nvrhi::ResourceStates::ShaderResource);

    DispatchClusterArgs(cmdList, nvDevice, 20, 44, m_clusterArgsBuffer2, m_clusterTerrainArgsBuffer2, m_clusterSwArgsBuffer2,
        m_clusterRetestDispatchArgs);
}

// ═══════════════════════════════════════════════════════
//  VB/IB POOL REGISTRATION (for level geometry)
// ═══════════════════════════════════════════════════════

void GPUCullingManager::EnsureSourceNormalStorage(u32 vertexTotal)
{
    if (!m_megaSourceNormalsActive)
        return;
    if (m_megaSourceNormals.size() < vertexTotal)
        m_megaSourceNormals.resize(vertexTotal, Fvector3{ 0.0f, 0.0f, 1.0f });
}

Fvector3* GPUCullingManager::ActivateSourceNormals(u32 firstVertex, u32 vertexCount)
{
    if (!m_megaSourceNormalsActive) {
        m_megaSourceNormalsActive = true;
        m_megaSourceNormals.assign(firstVertex, Fvector3{ 0.0f, 0.0f, 1.0f });
    }
    EnsureSourceNormalStorage(firstVertex);
    m_megaSourceNormals.resize(size_t(firstVertex) + vertexCount, Fvector3{ 0.0f, 0.0f, 1.0f });
    return m_megaSourceNormals.data() + firstVertex;
}

u32 GPUCullingManager::RegisterVBPool(
    const void* vertices,
    u32 vertexCount,
    u32 vertexStride,
    const VertexElement* decl,
    bool alternative)
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterVBPool called outside of level load");
        return UINT32_MAX;
    }

    if (!vertices || vertexCount == 0) {
        Msg("! [GPUCulling] RegisterVBPool: Invalid parameters");
        return UINT32_MAX;
    }

    const bindless::SourceVertexLayout layout = bindless::BuildSourceVertexLayout(decl, vertexStride);
    if (!layout.IsValid())
        FATAL_F("[GPUCulling] vertex pool %u uses an unsupported declaration (stride %u, %u elements)",
            u32((alternative ? m_vbPoolsAlt : m_vbPools).size()), vertexStride, GetDeclLength(decl));

    VBPoolInfo poolInfo;
    poolInfo.megaBufferVertexOffset = m_totalVertexCount;
    poolInfo.vertexCount = vertexCount;
    poolInfo.layout = layout;

    const u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    Fvector3* normals = nullptr;
    if (layout.HasFloatBasis())
        normals = ActivateSourceNormals(prevSize, vertexCount);
    else if (m_megaSourceNormalsActive)
        EnsureSourceNormalStorage(prevSize + vertexCount);

    const u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexCount, layout, &m_megaVertices[prevSize], normals);

    if (converted != vertexCount) {
        m_megaVertices.resize(prevSize);
        FATAL_F("[GPUCulling] vertex pool conversion produced %u of %u vertices", converted, vertexCount);
    }

    m_totalVertexCount += vertexCount;

    xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
    const u32 poolID = static_cast<u32>(pools.size());
    pools.push_back(poolInfo);

    Msg("* [GPUCulling] RegisterVBPool[%u]: %u verts (stride=%u, signature=%08X, %s basis, offset=%u)%s",
        poolID, vertexCount, vertexStride, layout.Signature(),
        layout.HasFloatBasis() ? "float" : "packed", poolInfo.megaBufferVertexOffset,
        alternative ? " [ALT]" : "");

    return poolID;
}

u32 GPUCullingManager::RegisterIBPool(
    const u16* indices,
    u32 indexCount,
    bool alternative)
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterIBPool called outside of level load");
        return UINT32_MAX;
    }

    if (!indices || indexCount == 0) {
        // Register placeholder to keep indices in sync
        Msg("! [GPUCulling] RegisterIBPool: Invalid parameters - registering placeholder");

        IBPoolInfo placeholder;
        placeholder.megaBufferIndexOffset = 0;
        placeholder.indexCount = 0;

        xr_vector<IBPoolInfo>& pools = alternative ? m_ibPoolsAlt : m_ibPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;
    }

    // Create pool info
    IBPoolInfo poolInfo;
    poolInfo.megaBufferIndexOffset = m_totalIndexCount;
    poolInfo.indexCount = indexCount;

    // Convert from 16-bit to 32-bit indices
    u32 prevSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
        m_megaIndices[prevSize + i] = static_cast<u32>(indices[i]);
    }

    m_totalIndexCount += indexCount;

    // Store pool info
    xr_vector<IBPoolInfo>& pools = alternative ? m_ibPoolsAlt : m_ibPools;
    u32 poolID = static_cast<u32>(pools.size());
    pools.push_back(poolInfo);

    Msg("* [GPUCulling] RegisterIBPool[%u]: %u indices (offset=%u)%s",
        poolID, indexCount, poolInfo.megaBufferIndexOffset,
        alternative ? " [ALT]" : "");

    return poolID;
}

bool GPUCullingManager::EnsureMegaCapacity(u32 vertexTotal, u32 indexTotal)
{
    if (!m_device)
        return false;
    if (vertexTotal <= m_megaVertexCapacity && indexTotal <= m_megaIndexCapacity)
        return true;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    const u64 wantVertices = std::max<u64>(vertexTotal, u64(m_megaVertexCapacity) * 3ull / 2ull);
    const u64 wantIndices = std::max<u64>(indexTotal, u64(m_megaIndexCapacity) * 3ull / 2ull);
    if (wantVertices > UINT32_MAX || wantIndices > UINT32_MAX)
        FATAL("[GPUCulling] mega-buffer working set exceeds the addressable range");

    nvrhi::BufferDesc vdesc;
    vdesc.debugName = "ForwardVertices";
    vdesc.byteSize = wantVertices * sizeof(ForwardVertex);
    vdesc.isVertexBuffer = true;
    vdesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    vdesc.keepInitialState = true;
    vdesc.canHaveRawViews = true;
    nvrhi::BufferHandle newVB = nvDevice->createBuffer(vdesc);

    nvrhi::BufferDesc idesc;
    idesc.debugName = "ForwardIndices";
    idesc.byteSize = wantIndices * sizeof(u32);
    idesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    idesc.keepInitialState = true;
    idesc.isIndexBuffer = true;
    idesc.canHaveRawViews = true;
    nvrhi::BufferHandle newIB = nvDevice->createBuffer(idesc);

    if (!newVB || !newIB)
    {
        Msg("! [GPUCulling] mega-buffer growth to %u vertices / %u indices failed", u32(wantVertices), u32(wantIndices));
        return false;
    }

    if (m_megaVertexBuffer && m_megaDataUploaded)
    {
        m_megaCopySourceVB = m_megaVertexBuffer;
        m_megaCopySourceIB = m_megaIndexBuffer;
        m_megaCopyVertexCount = m_forwardVertexBase;
        m_megaCopyIndexCount = m_forwardIndexBase;
    }

    m_megaVertexBuffer = newVB;
    m_megaIndexBuffer = newIB;
    m_megaVertexCapacity = u32(wantVertices);
    m_megaIndexCapacity = u32(wantIndices);
    m_maxMegaVertices = m_megaVertexCapacity;
    m_maxMegaIndices = m_megaIndexCapacity;
    m_megaBuffersReady = true;
    return true;
}

bool GeometrySourceKey::operator<(const GeometrySourceKey& o) const
{
    if (vertexSource != o.vertexSource)
        return vertexSource < o.vertexSource;
    if (indexSource != o.indexSource)
        return indexSource < o.indexSource;
    if (vertexBase != o.vertexBase)
        return vertexBase < o.vertexBase;
    if (vertexCount != o.vertexCount)
        return vertexCount < o.vertexCount;
    if (indexBase != o.indexBase)
        return indexBase < o.indexBase;
    if (indexCount != o.indexCount)
        return indexCount < o.indexCount;
    if (flags != o.flags)
        return flags < o.flags;
    if (vertexStride != o.vertexStride)
        return vertexStride < o.vertexStride;
    return vertexFormat < o.vertexFormat;
}

const MeshAllocation* GPUCullingManager::FindRuntimeGeometry(const GeometrySourceKey& source) const
{
    auto it = m_runtimeSourceLookup.find(source);
    return it == m_runtimeSourceLookup.end() ? nullptr : &it->second;
}

MeshAllocation GPUCullingManager::RegisterRuntimeGeometry(
    const GeometrySourceKey& source,
    const void* vertices,
    u32 vertexStride,
    const VertexElement* decl,
    const u16* indices)
{
    MeshAllocation alloc;

    if (const MeshAllocation* existing = FindRuntimeGeometry(source))
        return *existing;

    if (source.vertexStride != vertexStride)
        FATAL_F("[GPUCulling] runtime geometry source key stride %u does not match the supplied stride %u",
            source.vertexStride, vertexStride);

    if (!vertices || !indices || vertexStride == 0 || source.vertexCount == 0
        || source.indexCount < 3 || source.indexCount % 3 != 0)
        FATAL_F("[GPUCulling] unsupported runtime geometry source v=%u+%u i=%u+%u stride=%u",
            source.vertexBase, source.vertexCount, source.indexBase, source.indexCount, vertexStride);

    const bindless::SourceVertexLayout layout = bindless::BuildSourceVertexLayout(decl, vertexStride);
    if (!layout.IsValid())
        FATAL_F("[GPUCulling] runtime geometry source uses an unsupported vertex declaration (stride %u, %u elements)",
            vertexStride, GetDeclLength(decl));
    if (source.vertexFormat != layout.Signature())
        FATAL_F("[GPUCulling] runtime geometry source key signature %08X does not match the decoded layout %08X",
            source.vertexFormat, layout.Signature());

    const u64 vertexOffset64 = m_totalVertexCount;
    const u64 sourceIndexOffset64 = m_totalIndexCount;
    if (vertexOffset64 + source.vertexCount > UINT32_MAX || sourceIndexOffset64 + source.indexCount > UINT32_MAX)
        FATAL("[GPUCulling] runtime geometry exceeds the addressable mega-buffer range");

    const u32 vertexOffset = u32(vertexOffset64);
    const u32 sourceIndexOffset = u32(sourceIndexOffset64);

    const size_t vertexMark = m_runtimeVertices.size();
    m_runtimeVertices.resize(vertexMark + source.vertexCount);
    m_runtimeSourceNormals.clear();
    if (layout.HasFloatBasis())
        m_runtimeSourceNormals.resize(source.vertexCount, Fvector3{ 0.0f, 0.0f, 1.0f });
    const u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, source.vertexCount, layout, m_runtimeVertices.data() + vertexMark,
        m_runtimeSourceNormals.empty() ? nullptr : m_runtimeSourceNormals.data());
    if (converted != source.vertexCount)
    {
        m_runtimeVertices.resize(vertexMark);
        FATAL_F("[GPUCulling] runtime geometry vertex conversion produced %u of %u vertices",
            converted, source.vertexCount);
    }

    const size_t indexMark = m_runtimeIndices.size();
    m_runtimeIndices.resize(indexMark + source.indexCount);
    for (u32 i = 0; i < source.indexCount; ++i)
    {
        const u32 index = indices[source.indexBase + i];
        if (index >= source.vertexCount)
        {
            m_runtimeVertices.resize(vertexMark);
            m_runtimeIndices.resize(indexMark);
            FATAL_F("[GPUCulling] runtime geometry index %u exceeds the %u source vertices", index, source.vertexCount);
        }
        m_runtimeIndices[indexMark + i] = index;
    }

    if (m_rtVertexBuffer)
    {
        R_ASSERT(m_rtVertexCount == m_totalVertexCount && m_rtIndexCount == m_totalIndexCount);
        AppendRuntimeRTSource(m_runtimeVertices.data() + vertexMark,
            m_runtimeSourceNormals.empty() ? nullptr : m_runtimeSourceNormals.data(),
            source.vertexCount, m_runtimeIndices.data() + indexMark, source.indexCount);
    }

    ClusterMeshKey key;
    key.vertexOffset = vertexOffset;
    key.indexOffset = sourceIndexOffset;
    key.vertexCount = source.vertexCount;
    key.indexCount = source.indexCount;

    ClusterSourceView leafSource;
    leafSource.vertices = m_runtimeVertices.data() + vertexMark;
    leafSource.floatNormals = m_runtimeSourceNormals.empty() ? nullptr : m_runtimeSourceNormals.data();
    leafSource.indices = m_runtimeIndices.data() + indexMark;
    leafSource.vertexBase = vertexOffset;

    alloc.vertexOffset = vertexOffset;
    alloc.indexOffset = sourceIndexOffset;
    alloc.vertexCount = source.vertexCount;
    alloc.indexCount = source.indexCount;
    alloc.valid = true;
    if ((source.flags & GEOMETRY_SOURCE_FORWARD) != 0)
        AppendForwardGeometry(alloc, leafSource);
    if ((source.flags & GEOMETRY_SOURCE_SHADOW) != 0
        || (source.flags & GEOMETRY_SOURCE_POLICY_MASK) == 0)
    {
        const u32 leafFlags = (source.flags & ~GEOMETRY_SOURCE_POLICY_MASK)
            | ((source.flags & GEOMETRY_SOURCE_SHADOW) != 0 ? CLUSTER_RANGE_FLAG_AT : 0u);
        u32 assetMember = 0;
        if (!m_clusterDAG.AppendRuntimeLeafAsset(key, leafFlags, leafSource, assetMember))
            FATAL_F("[GPUCulling] runtime geometry with %u vertices and %u indices cannot be represented as clusters",
                source.vertexCount, source.indexCount);
        m_residency.RegisterRuntimePages();
        m_geometryTablesDirty = true;
    }
    m_runtimeSourceNormals.clear();

    const u64 vertexTotal = vertexOffset64 + source.vertexCount;
    const u64 indexTotal = sourceIndexOffset64 + source.indexCount;
    if (vertexTotal > UINT32_MAX || indexTotal > UINT32_MAX)
        FATAL("[GPUCulling] runtime geometry exceeds the addressable mega-buffer range");

    m_totalVertexCount = u32(vertexTotal);
    m_totalIndexCount = u32(indexTotal);
    m_runtimeVertices.clear();
    m_runtimeIndices.clear();
    m_runtimeSourceLookup.emplace(source, alloc);
    return alloc;
}

MeshAllocation GPUCullingManager::GetMeshAllocation(
    u32 vbID, u32 vBase, u32 vCount,
    u32 ibID, u32 iBase, u32 iCount,
    bool alternative) const
{
    MeshAllocation alloc;

    const xr_vector<VBPoolInfo>& vbPools = alternative ? m_vbPoolsAlt : m_vbPools;
    const xr_vector<IBPoolInfo>& ibPools = alternative ? m_ibPoolsAlt : m_ibPools;

    if (vbID >= vbPools.size() || ibID >= ibPools.size()) {
        // Pool not registered - mesh not in mega-buffer
        return alloc;
    }

    const VBPoolInfo& vbPool = vbPools[vbID];
    const IBPoolInfo& ibPool = ibPools[ibID];

    // Check for placeholder pools (unknown format or invalid data)
    if (vbPool.vertexCount == 0 || ibPool.indexCount == 0) {
        // Placeholder pool - mesh uses unsupported vertex format
        return alloc;
    }

    // Validate bounds
    if (vBase + vCount > vbPool.vertexCount) {
        Msg("! [GPUCulling] GetMeshAllocation: VB bounds exceeded (vBase=%u, vCount=%u, poolCount=%u)",
            vBase, vCount, vbPool.vertexCount);
        return alloc;
    }
    if (iBase + iCount > ibPool.indexCount) {
        Msg("! [GPUCulling] GetMeshAllocation: IB bounds exceeded (iBase=%u, iCount=%u, poolCount=%u)",
            iBase, iCount, ibPool.indexCount);
        return alloc;
    }

    // Calculate mega-buffer offsets
    // vBase/iBase are the mesh's starting offsets within its VB/IB pool
    // Indices in X-Ray are relative to the start of the IB pool, and reference
    // vertices relative to vBase=0 of the VB pool
    alloc.vertexOffset = vbPool.megaBufferVertexOffset + vBase;
    alloc.indexOffset = ibPool.megaBufferIndexOffset + iBase;
    alloc.vertexCount = vCount;
    alloc.indexCount = iCount;
    alloc.valid = true;

    return alloc;
}

} // namespace xray::render::fg
