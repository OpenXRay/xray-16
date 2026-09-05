// xrRender/GPUCullingManager.cpp
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
#include "Layers/xrRender/Bindless/VertexConverter.h"
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics bone access
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/Decals/OverlayManager.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg {

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

    m_dynamicObjectFlags.reserve(MAX_CULLING_OBJECTS);
    m_dynamicDrawArgsData.reserve(MAX_CULLING_OBJECTS);
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
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        nvrhi::BufferHandle buffer = nvDevice->createBuffer(desc);
        R_ASSERT2(buffer, name);
        return buffer;
    };

    m_staticInstanceBuffer = makeInstanceBuffer("GPUCull_Static_InstanceData", m_maxObjects);
    m_dynamicInstanceBuffer = makeInstanceBuffer("GPUCull_Dynamic_InstanceData", m_maxObjects);

    m_maxTerrainObjects = 4096;
    m_terrainInstanceBuffer = makeInstanceBuffer("GPUCull_TerrainInstanceData", m_maxTerrainObjects);

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
        fadeDesc.initialState = nvrhi::ResourceStates::ShaderResource;
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
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_dummyHiZ = nvDevice->createTexture(desc);
    }

    auto makeArgsBuffer = [&](const char* name) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        nvrhi::BufferHandle buffer = nvDevice->createBuffer(desc);
        R_ASSERT2(buffer, name);
        return buffer;
    };
    m_clusterArgsBuffer = makeArgsBuffer("ClusterCull_Args");
    m_clusterTerrainArgsBuffer = makeArgsBuffer("ClusterCull_TerrainArgs");
    m_clusterArgsBuffer2 = makeArgsBuffer("ClusterCull_RetestArgs");
    m_clusterTerrainArgsBuffer2 = makeArgsBuffer("ClusterCull_RetestTerrainArgs");

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
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
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
    }
    m_skinnedEnabled = m_skinnedRecordsBuffer && m_skinnedEntryBuffer;
    Msg("* [GPUCulling] Skinned upload buffers created (max: %u objects, %u bones)",
        m_maxSkinnedObjects, MAX_TOTAL_BONES);
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
    m_skinnedMaterialIDData.reserve(capacity);
}

void GPUCullingManager::Shutdown()
{
    m_skinnedPools.Reset();
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
        m_skinnedBuckets[f] = SkinnedBucket{};
    m_skinnedRecordsBuffer = nullptr;
    m_skinnedChunkBuffer = nullptr;
    m_skinnedEntryBuffer = nullptr;
    m_skinnedEntryCapacity = 0;
    m_skinnedEntryCount = 0;
    m_skinnedHudEntryBuffer = nullptr;
    m_skinnedHudEntryCount = 0;
    m_skinnedPreVB[0] = nullptr;
    m_skinnedPreVB[1] = nullptr;
    m_skinnedChunkCapacity = 0;
    m_skinnedPreVBCapacity = 0;
    m_preskinPipeline = nullptr;
    m_preskinLayout = nullptr;
    m_maxSkinnedObjects = 0;

    m_staticInstanceBuffer = nullptr;
    m_dynamicInstanceBuffer = nullptr;
    m_transparentInstanceBuffer = nullptr;
    m_transparentDrawArgsBuffer = nullptr;
    m_dynamicPrevWorldBuffer = nullptr;
    m_staticObjectCount = 0;
    m_dynamicObjectCount = 0;
    m_transparentObjectCount = 0;
    m_maxTransparentObjects = 0;
    m_staticUploaded = false;

    m_clusterSet = {};
    m_clusterArgsBuffer = nullptr;
    m_clusterTerrainArgsBuffer = nullptr;
    m_clusterArgsBuffer2 = nullptr;
    m_clusterTerrainArgsBuffer2 = nullptr;
    m_clusterEntryData.clear();
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

    m_megaVertexBuffer = nullptr;
    m_megaIndexBuffer = nullptr;
    m_megaVertices.clear();
    m_megaIndices.clear();
    m_staticInstanceData.clear();
    m_dynamicInstanceData.clear();
    m_staticObjectFlags.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_staticBatchVertexCounts.clear();
    m_staticBatchKeys.clear();
    m_dynamicObjectFlags.clear();
    m_dynamicDrawArgsData.clear();
    m_dynamicMaterialIDData.clear();
    m_dynamicBatchKeys.clear();
    m_dynamicIdentity.clear();
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_maxMegaVertices = 0;
    m_maxMegaIndices = 0;
    m_megaBuffersReady = false;
    m_levelLoadInProgress = false;

    m_terrainInstanceBuffer = nullptr;
    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainBatchKeys.clear();
    m_terrainObjectCount = 0;

    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();

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
        m_cullingStats.clusterVisible = std::min(counts[0] + counts[5], m_clusterSet.staticEntryCount);
        m_cullingStats.clusterTerrainVisible = std::min(counts[1] + counts[6], m_clusterSet.terrainEntryCount);
        m_cullingStats.clusterTrianglesDrawn = counts[2] + counts[7];
        m_cullingStats.clusterTerrainTrianglesDrawn = counts[3] + counts[8];
        m_cullingStats.clusterCandidates = counts[4];
        m_cullingStats.clusterRetestVisible = counts[5] + counts[6];

        nvDevice->unmapBuffer(oldest);
    }
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SCENE OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::UploadSceneObjects(fg::RenderContext* ctx, const GeometryCollector* geometry)
{
    ZoneScopedN("GPUCull::UploadSceneObjects");

    if (!m_computeEnabled || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    const u32 totalBatches = static_cast<u32>(batches.size());

    if (totalBatches == 0) {
        m_staticObjectCount = 0;
        m_dynamicObjectCount = 0;
        m_transparentResidualCount = 0;
        m_clusterSet.dynamicEntryCount = 0;
        m_clusterSet.dynamicResidualCount = 0;
        return;
    }

    R_ASSERT2(m_staticInstanceBuffer && m_dynamicInstanceBuffer, "GPU culling instance buffers not initialized");

    if (!m_staticDataCached) {
        m_staticObjectFlags.clear();
        m_staticObjectFlags.reserve(totalBatches);
        m_staticDrawArgsData.clear();
        m_staticDrawArgsData.reserve(totalBatches);
        m_staticMaterialIDData.clear();
        m_staticMaterialIDData.reserve(totalBatches);
        m_staticInstanceData.clear();
        m_staticInstanceData.reserve(totalBatches);
        m_staticBatchVertexCounts.clear();
        m_staticBatchVertexCounts.reserve(totalBatches);
        m_staticBatchKeys.clear();
        m_staticBatchKeys.reserve(totalBatches);
    }

    m_dynamicObjectFlags.clear();
    m_dynamicObjectFlags.reserve(totalBatches);
    m_dynamicDrawArgsData.clear();
    m_dynamicDrawArgsData.reserve(totalBatches);
    m_dynamicMaterialIDData.clear();
    m_dynamicMaterialIDData.reserve(totalBatches);
    m_dynamicInstanceData.clear();
    m_dynamicInstanceData.reserve(totalBatches);
    m_dynamicBatchKeys.clear();
    m_dynamicBatchKeys.reserve(totalBatches);
    m_dynamicIdentity.clear();
    m_dynamicIdentity.reserve(totalBatches);

    if (!m_terrainDataCached) {
        m_terrainDrawArgsData.clear();
        m_terrainDrawArgsData.reserve(totalBatches / 4);
        m_terrainMaterialIDData.clear();
        m_terrainMaterialIDData.reserve(totalBatches / 4);
        m_terrainInstanceData.clear();
        m_terrainInstanceData.reserve(totalBatches / 4);
        m_terrainBatchKeys.clear();
        m_terrainBatchKeys.reserve(totalBatches / 4);
    }

    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();
    m_transparentResidualCount = 0;

    auto batchFlags = [](const GeometryBatch& batch) -> u32 {
        if (const auto* mat = bindless::MaterialBuffer::Instance().GetMaterial(batch.bindlessMaterialID)) {
            if (mat->shaderVariant != 0 || (mat->flags & bindless::MAT_FLAG_ALPHA_BLEND))
                return GPU_OBJECT_NO_RESOLVE;
        }
        return 0u;
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

        materialIDData.push_back(materialID);

        GPUInstanceData inst;
        inst.world = batch.worldMatrix;
        inst.materialID = materialID;
        inst.flags = flags;
        inst.pad0 = 0.0f;
        inst.pad1 = 0.0f;
        instanceData.push_back(inst);
    };

    {
    ZoneScopedN("Upload::Rebuild");
    for (u32 i = 0; i < totalBatches; i++) {
        const auto& batch = batches[i];

        if (batch.isSkinned)
            continue;

        if (batch.isTerrain) {
            if (m_terrainDataCached)
                continue;
            appendBatch(batch, 0u, batch.terrainMaterialID,
                m_terrainDrawArgsData, m_terrainMaterialIDData, m_terrainInstanceData);
            m_terrainBatchKeys.push_back(batchKey(batch));
            continue;
        }

        if (batch.IsStrictB2F()) {
            if (!batch.megaBufferAlloc.valid)
                ++m_transparentResidualCount;
            appendBatch(batch, batchFlags(batch), batch.bindlessMaterialID,
                m_transparentDrawArgsData, m_transparentMaterialIDData, m_transparentInstanceData);
            continue;
        }

        if (batch.isStatic) {
            if (m_staticDataCached)
                continue;
            const u32 flags = batchFlags(batch);
            m_staticObjectFlags.push_back(flags);
            appendBatch(batch, flags, batch.bindlessMaterialID,
                m_staticDrawArgsData, m_staticMaterialIDData, m_staticInstanceData);
            m_staticBatchVertexCounts.push_back(batch.megaBufferAlloc.valid ? batch.megaBufferAlloc.vertexCount : 0);
            m_staticBatchKeys.push_back(batchKey(batch));
        } else {
            const u32 flags = batchFlags(batch);
            m_dynamicObjectFlags.push_back(flags);
            appendBatch(batch, flags, batch.bindlessMaterialID,
                m_dynamicDrawArgsData, m_dynamicMaterialIDData, m_dynamicInstanceData);
            m_dynamicBatchKeys.push_back(batchKey(batch));
            m_dynamicIdentity.push_back(std::make_pair(static_cast<const void*>(batch.visual), static_cast<const void*>(batch.renderable)));
        }
    }
    }

    const u32 staticCount = std::min(static_cast<u32>(m_staticInstanceData.size()), m_maxObjects);
    if (m_staticInstanceData.size() > staticCount) {
        m_staticObjectFlags.resize(staticCount);
        m_staticDrawArgsData.resize(staticCount);
        m_staticMaterialIDData.resize(staticCount);
        m_staticInstanceData.resize(staticCount);
        m_staticBatchVertexCounts.resize(staticCount);
        m_staticBatchKeys.resize(staticCount);
    }

    const u32 dynamicCapacity = (staticCount < m_maxObjects) ? (m_maxObjects - staticCount) : 0;
    const u32 dynamicCount = std::min(static_cast<u32>(m_dynamicInstanceData.size()), dynamicCapacity);
    if (m_dynamicInstanceData.size() > dynamicCount) {
        m_dynamicObjectFlags.resize(dynamicCount);
        m_dynamicDrawArgsData.resize(dynamicCount);
        m_dynamicMaterialIDData.resize(dynamicCount);
        m_dynamicInstanceData.resize(dynamicCount);
        m_dynamicBatchKeys.resize(dynamicCount);
        m_dynamicIdentity.resize(dynamicCount);
    }

    m_staticObjectCount = staticCount;
    m_dynamicObjectCount = dynamicCount;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (m_neutralFadeBuffer && !m_neutralFadeZeroed) {
        u32 neutral = 0;
        cmdList->writeBuffer(m_neutralFadeBuffer, &neutral, sizeof(u32));
        m_neutralFadeZeroed = true;
    }

    if (!m_megaDataUploaded && m_megaBuffersReady &&
        !m_megaVertices.empty() && !m_megaIndices.empty() &&
        m_megaVertexBuffer && m_megaIndexBuffer) {

        cmdList->writeBuffer(m_megaVertexBuffer,
            m_megaVertices.data(),
            m_megaVertices.size() * sizeof(bindless::UnifiedVertex));

        cmdList->writeBuffer(m_megaIndexBuffer,
            m_megaIndices.data(),
            m_megaIndices.size() * sizeof(u32));

        m_megaDataUploaded = true;

        Msg("* [GPUCulling] Mega-buffer data uploaded: %zu vertices, %zu indices",
            m_megaVertices.size(), m_megaIndices.size());

        m_megaVertices.clear();
        m_megaVertices.shrink_to_fit();
        m_megaIndices.clear();
        m_megaIndices.shrink_to_fit();
    }

    if (m_staticObjectCount > 0 && !m_staticUploaded) {
        BuildClusterEntries();
        UploadClusterEntries(cmdList, m_device->GetNVRHIDevice());

        R_ASSERT2(m_staticInstanceData.size() >= m_staticObjectCount, "Static instance data smaller than count");
        cmdList->writeBuffer(m_staticInstanceBuffer,
            m_staticInstanceData.data(),
            m_staticObjectCount * sizeof(GPUInstanceData));

        m_staticUploaded = true;
        m_staticDataCached = true;

        Msg("* [GPUCulling] Static object data uploaded: %u objects", m_staticObjectCount);
    }

    BuildDynamicClusterEntries(cmdList);

    if (m_dynamicObjectCount > 0) {
        ZoneScopedN("Upload::DynamicWrite");
        R_ASSERT2(m_dynamicInstanceData.size() >= m_dynamicObjectCount, "Dynamic instance data smaller than count");
        cmdList->writeBuffer(m_dynamicInstanceBuffer,
            m_dynamicInstanceData.data(),
            m_dynamicObjectCount * sizeof(GPUInstanceData));
    }

    m_terrainObjectCount = std::min(static_cast<u32>(m_terrainInstanceData.size()), m_maxTerrainObjects);

    if (m_terrainObjectCount > 0 && !m_terrainDataCached && m_terrainInstanceBuffer) {
        ZoneScopedN("Upload::TerrainWrite");
        cmdList->writeBuffer(m_terrainInstanceBuffer,
            m_terrainInstanceData.data(),
            m_terrainObjectCount * sizeof(GPUInstanceData));
        cmdList->setBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::ShaderResource);

        m_terrainDataCached = true;
        Msg("* [GPUCulling] Terrain data cached: %u objects", m_terrainObjectCount);
    }

    m_transparentObjectCount = std::min(static_cast<u32>(m_transparentInstanceData.size()), m_maxTransparentObjects);

    if (m_transparentObjectCount > 0 && m_transparentInstanceBuffer && m_transparentDrawArgsBuffer) {
        ZoneScopedN("Upload::TransparentWrite");
        cmdList->writeBuffer(m_transparentDrawArgsBuffer,
            m_transparentDrawArgsData.data(),
            m_transparentObjectCount * sizeof(IndirectDrawArgs));
        cmdList->setBufferState(m_transparentDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

        cmdList->writeBuffer(m_transparentInstanceBuffer,
            m_transparentInstanceData.data(),
            m_transparentObjectCount * sizeof(GPUInstanceData));
        cmdList->setBufferState(m_transparentInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
    }
}

void GPUCullingManager::InvalidateStaticCullingData()
{
    m_staticDataCached = false;
    m_staticUploaded = false;
    m_staticObjectCount = 0;

    m_staticObjectFlags.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_staticInstanceData.clear();
    m_staticBatchVertexCounts.clear();
    m_staticBatchKeys.clear();

    m_terrainDataCached = false;
    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainBatchKeys.clear();
    m_terrainObjectCount = 0;

    m_clusterSet = {};
    m_clusterEntryData.clear();

    Msg("* [GPUCulling] Static culling data invalidated");
}

void GPUCullingManager::InvalidateShadersAndPipelines()
{
    m_clusterCullPipeline = nullptr;
    m_clusterCullLayout = nullptr;
    m_clusterArgsPipeline = nullptr;
    m_clusterArgsLayout = nullptr;
    m_clusterRetestPipeline = nullptr;
    m_clusterRetestLayout = nullptr;

    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_initialized = false;
    m_computeEnabled = false;
    m_skinnedEnabled = false;

    m_staticDataCached = false;
    m_terrainDataCached = false;
    m_staticUploaded = false;

    Msg("* [GPUCulling] Shaders and pipelines invalidated for hot-reload");
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SKINNED OBJECTS
// ═══════════════════════════════════════════════════════

static const u32 kSkinnedKindOrder[3] = { 0u, 2u, 1u };

void GPUCullingManager::UploadSkinnedObjects(fg::RenderContext* ctx, const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches, decals::OverlayManager* overlayMgr)
{
    ZoneScopedN("GPUCull::UploadSkinnedObjects");

    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        SkinnedBucket& bucket = m_skinnedBuckets[f];
        bucket.args.clear();
        bucket.records.clear();
        bucket.materialIDs.clear();
        bucket.kinds.clear();
        bucket.srcVertexBases.clear();
        bucket.vertexCounts.clear();
        bucket.visuals.clear();
    }
    m_skinnedObjectCount = 0;
    m_skinnedEntryCount = 0;
    m_skinnedVisibleEntryCount = 0;
    m_skinnedHudEntryCount = 0;

    if (!IsSkinnedEnabled() || !geometry)
        return;

    auto cmdList = ctx->GetCommandList();
    if (overlayMgr)
        overlayMgr->UploadSplats(cmdList);
    m_skinnedPools.FlushUploads(m_device->GetNVRHIDevice(), cmdList);
    m_boneBatching = true;
    m_boneBatchStart = m_currentBoneOffset;

    u32 residual = 0;
    u32 hudPooled = 0;
    auto addBatch = [&](const GeometryBatch& batch, u8 kind) {
        const u32 variantIdx = bindless::MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
        const bool pooled = variantIdx == 0
            && batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
            && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
        if (!pooled) {
            residual += kind == 2u ? 0u : 1u;
            return;
        }
        hudPooled += kind == 2u ? 1u : 0u;

        SkinnedBucket& bucket = m_skinnedBuckets[batch.skinnedPoolFormat];

        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 1;
        args.startIndexLocation = batch.skinnedPoolFirstIndex;
        args.baseVertexLocation = batch.skinnedPoolBaseVertex;
        args.startInstanceLocation = 0;
        bucket.args.push_back(args);

        CKinematics* skeleton = nullptr;
        u32 visualType = batch.visual ? batch.visual->getType() : 0;
        if (visualType == MT_SKELETON_GEOMDEF_ST)
            skeleton = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
        else if (visualType == MT_SKELETON_GEOMDEF_PM)
            skeleton = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();

        SkinnedDrawRecord rec;
        rec.world = batch.worldMatrix;
        rec.boneOffset = GetOrUploadSkeleton(cmdList, skeleton);
        rec.splatOffset = 0;
        rec.splatCount = 0;
        if (overlayMgr && skeleton) {
            auto sr = overlayMgr->GetSplatRange(skeleton);
            rec.splatOffset = sr.offset;
            rec.splatCount = sr.count;
        }
        rec.prevFirstVertex = 0xFFFFFFFFu;
        rec.bounds.set(batch.worldBoundsCenter.x, batch.worldBoundsCenter.y, batch.worldBoundsCenter.z, batch.worldBoundsRadius);
        bucket.records.push_back(rec);
        bucket.materialIDs.push_back(batch.bindlessMaterialID);
        bucket.kinds.push_back(kind);
        bucket.srcVertexBases.push_back(u32(batch.skinnedPoolBaseVertex));
        bucket.vertexCounts.push_back(batch.vertexCount);
        bucket.visuals.push_back(batch.visual);
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

    FlushBoneBatch(cmdList);

    u32 pooledTotal = 0;
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
        pooledTotal += static_cast<u32>(m_skinnedBuckets[f].records.size());
    m_skinnedObjectCount = pooledTotal - hudPooled + residual;

    if (pooledTotal == 0)
        return;

    EnsureSkinnedCapacity(pooledTotal);

    u32 vertexDemand = 0;
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
        for (u32 count : m_skinnedBuckets[f].vertexCounts)
            vertexDemand += count;
    const bool preVBRecreated = EnsurePreskinBuffers(m_device->GetNVRHIDevice(), vertexDemand);
    const bool historyValid = !preVBRecreated && m_skinnedHistoryFrame + 1u == Device.dwFrame;
    const auto& prevHistory = m_skinnedHistory[m_skinnedHistoryIndex];
    auto& nextHistory = m_skinnedHistory[m_skinnedHistoryIndex ^ 1u];
    nextHistory.clear();

    m_skinnedRecordsData.clear();
    m_skinnedMaterialIDData.clear();
    m_skinnedChunkData.clear();
    m_skinnedEntryData.clear();
    m_skinnedShadowEntryData.clear();
    m_skinnedHudEntryData.clear();
    bool hudBoundsValid = false;
    u32 vertexTotal = 0;
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        SkinnedBucket& bucket = m_skinnedBuckets[f];
        m_skinnedChunkBase[f] = static_cast<u32>(m_skinnedChunkData.size());
        for (u32 kind : kSkinnedKindOrder) {
            for (u32 i = 0; i < static_cast<u32>(bucket.records.size()); ++i) {
                if (bucket.kinds[i] != kind)
                    continue;
                const u32 slot = static_cast<u32>(m_skinnedRecordsData.size());
                const u32 vertexCount = bucket.vertexCounts[i];
                const IndirectDrawArgs& args = bucket.args[i];
                m_skinnedRecordsData.push_back(bucket.records[i]);
                u32 prevBase = 0xFFFFFFFFu;
                if (historyValid) {
                    auto it = prevHistory.find(bucket.visuals[i]);
                    if (it != prevHistory.end())
                        prevBase = it->second;
                }
                m_skinnedRecordsData.back().prevFirstVertex = prevBase;
                if (kind == 2u) {
                    const Fvector4& b = bucket.records[i].bounds;
                    if (!hudBoundsValid) {
                        m_skinnedHudBounds = b;
                        hudBoundsValid = true;
                    } else {
                        Fvector c0;
                        c0.set(m_skinnedHudBounds.x, m_skinnedHudBounds.y, m_skinnedHudBounds.z);
                        Fvector c1;
                        c1.set(b.x, b.y, b.z);
                        Fvector d;
                        d.sub(c1, c0);
                        const float dist = d.magnitude();
                        if (dist + b.w > m_skinnedHudBounds.w) {
                            if (dist + m_skinnedHudBounds.w <= b.w) {
                                m_skinnedHudBounds = b;
                            } else {
                                const float r = 0.5f * (dist + m_skinnedHudBounds.w + b.w);
                                c0.mad(d, (r - m_skinnedHudBounds.w) / dist);
                                m_skinnedHudBounds.set(c0.x, c0.y, c0.z, r);
                            }
                        }
                    }
                }
                nextHistory[bucket.visuals[i]] = vertexTotal;
                m_skinnedMaterialIDData.push_back(bucket.materialIDs[i]);
                for (u32 v0 = 0; v0 < vertexCount; v0 += SKINNED_CHUNK_VERTICES) {
                    SkinnedChunk chunk;
                    chunk.slot = slot;
                    chunk.srcVertex = bucket.srcVertexBases[i] + v0;
                    chunk.dstVertex = vertexTotal + v0;
                    chunk.count = std::min(SKINNED_CHUNK_VERTICES, vertexCount - v0);
                    m_skinnedChunkData.push_back(chunk);
                }
                {
                    const u32 ibBase = m_skinnedPools.GetFormatIndexBase(f) + args.startIndexLocation;
                    for (u32 i0 = 0; i0 < args.indexCountPerInstance; i0 += SKINNED_ENTRY_INDICES) {
                        GPUClusterEntry e = {};
                        e.sphere.set(bucket.records[i].bounds.x, bucket.records[i].bounds.y, bucket.records[i].bounds.z, bucket.records[i].bounds.w);
                        e.extent.set(bucket.records[i].bounds.w, bucket.records[i].bounds.w, bucket.records[i].bounds.w, 0.0f);
                        e.indexCount = std::min(SKINNED_ENTRY_INDICES, args.indexCountPerInstance - i0);
                        e.ibFirst = ibBase + i0;
                        e.firstVertex = vertexTotal;
                        e.batchIndex = slot;
                        e.materialID = bucket.materialIDs[i];
                        e.flags = GPU_CLUSTER_ENTRY_SKINNED | (kind == 2u ? GPU_CLUSTER_ENTRY_HUD : 0u) | (kind == 1u ? GPU_CLUSTER_ENTRY_SHADOW_ONLY : 0u);
                        if (kind == 2u)
                            m_skinnedHudEntryData.push_back(static_cast<u32>(m_skinnedEntryData.size()));
                        (kind == 1u ? m_skinnedShadowEntryData : m_skinnedEntryData).push_back(e);
                    }
                }
                vertexTotal += vertexCount;
            }
        }
        m_skinnedChunkCount[f] = static_cast<u32>(m_skinnedChunkData.size()) - m_skinnedChunkBase[f];
    }

    m_skinnedVisibleEntryCount = static_cast<u32>(m_skinnedEntryData.size());
    m_skinnedEntryData.insert(m_skinnedEntryData.end(), m_skinnedShadowEntryData.begin(), m_skinnedShadowEntryData.end());

    cmdList->writeBuffer(m_skinnedRecordsBuffer, m_skinnedRecordsData.data(), u64(pooledTotal) * sizeof(SkinnedDrawRecord));

    m_skinnedEntryCount = static_cast<u32>(m_skinnedEntryData.size());
    if (m_skinnedEntryCount > m_skinnedEntryCapacity) {
        static bool s_warned = false;
        if (!s_warned) {
            Msg("! [GPUCulling] %u skinned entries exceed the %u capacity, dropping the rest", m_skinnedEntryCount, m_skinnedEntryCapacity);
            s_warned = true;
        }
        m_skinnedEntryCount = m_skinnedEntryCapacity;
        m_skinnedVisibleEntryCount = std::min(m_skinnedVisibleEntryCount, m_skinnedEntryCapacity);
    }
    if (m_skinnedEntryCount > 0)
        cmdList->writeBuffer(m_skinnedEntryBuffer, m_skinnedEntryData.data(), u64(m_skinnedEntryCount) * sizeof(GPUClusterEntry));
    m_skinnedHudEntryCount = std::min(static_cast<u32>(m_skinnedHudEntryData.size()), SKINNED_HUD_ENTRY_CAPACITY);
    if (m_skinnedHudEntryCount > 0)
        cmdList->writeBuffer(m_skinnedHudEntryBuffer, m_skinnedHudEntryData.data(), u64(m_skinnedHudEntryCount) * sizeof(u32));

    if (DispatchPreskin(cmdList, overlayMgr, vertexTotal)) {
        m_skinnedHistoryIndex ^= 1u;
        m_skinnedHistoryFrame = Device.dwFrame;
    } else {
        nextHistory.clear();
    }
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
    u32 capacity = std::max(m_skinnedPreVBCapacity * 2u, 65536u);
    while (capacity < vertexTotal)
        capacity *= 2;
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
    }
    m_skinnedPreVBCapacity = capacity;
    return true;
}

bool GPUCullingManager::DispatchPreskin(nvrhi::ICommandList* cmdList, decals::OverlayManager* overlayMgr, u32 vertexTotal)
{
    if (vertexTotal == 0 || m_skinnedChunkData.empty())
        return false;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!EnsurePreskinPipeline(nvDevice))
        return false;

    const u32 chunkCount = static_cast<u32>(m_skinnedChunkData.size());
    if (m_skinnedChunkCapacity < chunkCount) {
        u32 capacity = std::max(m_skinnedChunkCapacity * 2u, 1024u);
        while (capacity < chunkCount)
            capacity *= 2;
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedChunks";
        desc.byteSize = u64(capacity) * sizeof(SkinnedChunk);
        desc.structStride = sizeof(SkinnedChunk);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedChunkBuffer = nvDevice->createBuffer(desc);
        m_skinnedChunkCapacity = capacity;
    }
    if (!m_skinnedPreVB[0] || !m_skinnedPreVB[1] || !m_skinnedChunkBuffer)
        return false;

    m_skinnedPreVBIndex ^= 1u;
    nvrhi::IBuffer* dstVB = m_skinnedPreVB[m_skinnedPreVBIndex];
    cmdList->writeBuffer(m_skinnedChunkBuffer, m_skinnedChunkData.data(), u64(chunkCount) * sizeof(SkinnedChunk));

    auto& cache = framegraph::GetPassResourceCache();
    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("skinned_preskin", ".cs");
    if (!refl)
        return false;
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(fg::passes::StaticGlobals), m_device);
    auto paramsCB = cache.GetOrCreateVolatileCB("GPUCull", "PreskinParams", 16, m_device, 64);
    nvrhi::IBuffer* splatBuffer = overlayMgr ? overlayMgr->GetSplatBuffer() : nullptr;

    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        if (m_skinnedChunkCount[f] == 0)
            continue;
        nvrhi::IBuffer* srcVB = m_skinnedPools.GetVertexBuffer(f);
        if (!srcVB)
            continue;

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

        framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.SkinnedPreskin");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB)
           .ConstantBuffer("PreskinParams", paramsCB)
           .BufferSRV("g_SrcVB", srcVB)
           .BufferSRV("g_Chunks", m_skinnedChunkBuffer)
           .BufferSRV("g_SkinnedRecords", m_skinnedRecordsBuffer)
           .BufferSRV("g_BoneMatrices", m_globalBoneBuffer)
           .BufferSRV("g_PaintSplats", splatBuffer)
           .BufferUAV("g_DstVB", dstVB);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), m_preskinLayout, nvDevice);
        if (!bindingSet)
            continue;

        nvrhi::ComputeState cs;
        cs.pipeline = m_preskinPipeline;
        cs.bindings = { bindingSet };
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
    // Reset bone buffer allocations for new frame
    ++m_boneUploadFrameId;
    m_currentBoneOffset = 0;
}

u32 GPUCullingManager::GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton)
{
    if (!m_boneBufferInitialized || !skeleton || !cmdList)
        return 0;

    // Check if already uploaded this frame
    if (skeleton->fg_bone_upload_frame == m_boneUploadFrameId) {
        return skeleton->fg_bone_upload_offset;
    }

    // Allocate space for this skeleton
    u32 boneCount = skeleton->LL_BoneCount();
    if (boneCount == 0)
        return 0;

    // Check capacity
    if (m_currentBoneOffset + boneCount > MAX_TOTAL_BONES) {
        Msg("! [GPUCulling] Bone buffer full: need %u bones, have %u remaining",
            boneCount, MAX_TOTAL_BONES - m_currentBoneOffset);
        return 0;  // Return 0 offset, will render with identity bones
    }

    // Record the offset for this skeleton
    u32 boneOffset = m_currentBoneOffset;
    skeleton->fg_bone_upload_frame = m_boneUploadFrameId;
    skeleton->fg_bone_upload_offset = boneOffset;

    // Upload bones
    UploadSkeletonBones(cmdList, skeleton, boneOffset);

    // Advance allocation pointer
    m_currentBoneOffset += boneCount;

    return boneOffset;
}

void GPUCullingManager::UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset)
{
    u32 boneCount = skeleton->LL_BoneCount();

    // Slang uses column_major — raw row-major Fmatrix bytes are naturally transposed.
    // No explicit transpose needed.
    for (u32 i = 0; i < boneCount; i++) {
        m_boneStagingBuffer[boneOffset + i] = skeleton->LL_GetTransform_R(u16(i));
    }

    if (m_boneBatching)
        return;

    u64 byteOffset = static_cast<u64>(boneOffset) * BONE_STRIDE;
    u64 byteSize = static_cast<u64>(boneCount) * BONE_STRIDE;

    cmdList->writeBuffer(m_globalBoneBuffer, m_boneStagingBuffer.data() + boneOffset, byteSize, byteOffset);
}

void GPUCullingManager::FlushBoneBatch(nvrhi::ICommandList* cmdList)
{
    if (!m_boneBatching)
        return;
    m_boneBatching = false;
    if (!cmdList || m_currentBoneOffset <= m_boneBatchStart)
        return;

    const u64 byteOffset = static_cast<u64>(m_boneBatchStart) * BONE_STRIDE;
    const u64 byteSize = static_cast<u64>(m_currentBoneOffset - m_boneBatchStart) * BONE_STRIDE;
    cmdList->writeBuffer(m_globalBoneBuffer, m_boneStagingBuffer.data() + m_boneBatchStart, byteSize, byteOffset);
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

framegraph::VirtualResourceHandle GPUCullingManager::SetupCullingPass(
    framegraph::FrameGraph& fg,
    const GeometryCollector* geometry,
    framegraph::VirtualResourceHandle prevHiZ,
    const Fmatrix& prevViewProj,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels)
{
    using namespace framegraph;

    if (!m_computeEnabled || !geometry || geometry->GetBatches().empty() || !m_clusterArgsBuffer)
        return VirtualResourceHandle();

    struct GPUCullPassData {
        VirtualResourceHandle clusterArgs;
        VirtualResourceHandle prevHiZ;
        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        Fmatrix prevViewProj;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    ResourceDesc argsDesc;
    argsDesc.type = ResourceDesc::Type::Buffer;
    argsDesc.debugName = "ClusterCull_Args";
    argsDesc.bufferSize = sizeof(u32) * 4;
    argsDesc.isUAV = true;
    argsDesc.isTransient = false;
    VirtualResourceHandle argsHandle = fg.ImportBuffer("cluster_args", m_clusterArgsBuffer, argsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",
        [&, argsHandle, geometry, prevHiZ, prevViewProj, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();

            data.manager = this;
            data.geometry = geometry;
            data.prevViewProj = prevViewProj;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;
            data.clusterArgs = passBuilder.write(argsHandle, ResourceState::UnorderedAccess);
            if (prevHiZ.is_valid())
                data.prevHiZ = passBuilder.read(prevHiZ, ResourceState::ShaderResource);
        },
        [](const GPUCullPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            mgr->UploadSceneObjects(ctx, data.geometry);

            bindless::MaterialBuffer::Instance().Upload(ctx);

            if (mgr->m_rtAccelMgr) {
                mgr->m_rtAccelMgr->BuildIfNeeded(cmdList, mgr);
                if (mgr->m_rtAccelMgr->IsReady()) {
                    if (!mgr->m_rtAccelMgr->GetMaterialBuffer())
                        mgr->m_rtAccelMgr->SetMaterialBuffer(bindless::MaterialBuffer::Instance().GetBuffer());
                    if (!mgr->m_rtAccelMgr->GetTerrainMaterialBuffer())
                        mgr->m_rtAccelMgr->SetTerrainMaterialBuffer(bindless::TerrainMaterialBuffer::Instance().GetBuffer());
                }
            }

            nvrhi::ITexture* prevHiZ = data.prevHiZ.is_valid() ? fg.GetPhysicalTexture(data.prevHiZ) : nullptr;
            mgr->DispatchClusterCull(cmdList, mgr->m_device->GetNVRHIDevice(), prevHiZ, data.prevViewProj,
                data.hizWidth, data.hizHeight, data.hizMipLevels);
        }
    );

    return passData.clusterArgs;
}

framegraph::VirtualResourceHandle GPUCullingManager::SetupClusterRetestPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels)
{
    using namespace framegraph;

    if (!m_computeEnabled || !hizPyramid.is_valid() || m_clusterSet.entryCount == 0 || !m_clusterArgsBuffer2)
        return VirtualResourceHandle();

    struct ClusterRetestPassData {
        VirtualResourceHandle hiz;
        VirtualResourceHandle args;
        GPUCullingManager* manager;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    ResourceDesc argsDesc;
    argsDesc.type = ResourceDesc::Type::Buffer;
    argsDesc.debugName = "ClusterCull_RetestArgs";
    argsDesc.bufferSize = sizeof(u32) * 4;
    argsDesc.isUAV = true;
    argsDesc.isTransient = false;
    VirtualResourceHandle argsHandle = fg.ImportBuffer("cluster_retest_args", m_clusterArgsBuffer2, argsDesc);

    auto& passData = fg.addCallbackPass<ClusterRetestPassData>(
        "Cluster Retest",
        [&, hizPyramid, argsHandle, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, ClusterRetestPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.manager = this;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;
            data.hiz = passBuilder.read(hizPyramid, ResourceState::ShaderResource);
            data.args = passBuilder.write(argsHandle, ResourceState::UnorderedAccess);
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
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    decals::OverlayManager* overlayMgr)
{
    using namespace framegraph;

    if (!IsSkinnedEnabled())
        return VirtualResourceHandle{};

    struct SkinnedUploadPassData {
        VirtualResourceHandle drawArgsBuffer;
        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        const xr_vector<GeometryBatch>* hudBatches;
        decals::OverlayManager* overlayMgr;
    };

    ResourceDesc entryBufferDesc;
    entryBufferDesc.type = ResourceDesc::Type::Buffer;
    entryBufferDesc.debugName = "GPUCull_SkinnedEntries";
    entryBufferDesc.bufferSize = u64(m_skinnedEntryCapacity) * sizeof(GPUClusterEntry);
    entryBufferDesc.structStride = sizeof(GPUClusterEntry);
    entryBufferDesc.isTransient = false;

    VirtualResourceHandle argsBufferHandle = fg.ImportBuffer(
        "skinned_entries", m_skinnedEntryBuffer, entryBufferDesc);

    auto& passData = fg.addCallbackPass<SkinnedUploadPassData>(
        "Skinned Upload",
        [&, geometry, hudBatches, argsBufferHandle, overlayMgr](FrameGraph& builder, PassHandle passHandle, SkinnedUploadPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.manager = this;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.overlayMgr = overlayMgr;
            data.drawArgsBuffer = passBuilder.write(argsBufferHandle, ResourceState::CopyDest);
        },
        [](const SkinnedUploadPassData& data, const FrameGraph&, fg::RenderContext* ctx) {
            data.manager->UploadSkinnedObjects(ctx, data.geometry, data.hudBatches, data.overlayMgr);
        }
    );

    return passData.drawArgsBuffer;
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
                particle.position = batch.visual->vis.sphere.P;
                particle.radius = batch.visual->vis.sphere.R;
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
            nvrhi::FramebufferHandle framebuffer = nvDevice->createFramebuffer(fbDesc);

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

    m_levelLoadInProgress = true;
    m_megaBuffersReady = false;
    m_megaDataUploaded = false;

    InvalidateStaticCullingData();

    // Clear and pre-allocate mega-buffers
    m_megaVertices.clear();
    m_megaVertices.reserve(estimatedVertices);
    m_megaIndices.clear();
    m_megaIndices.reserve(estimatedIndices);

    // Clear VB/IB pool tracking
    m_vbPools.clear();
    m_ibPools.clear();
    m_vbPoolsAlt.clear();
    m_ibPoolsAlt.clear();

    m_totalVertexCount = 0;
    m_totalIndexCount = 0;

    Msg("* [GPUCulling] BeginLevelLoad - estimated %u vertices, %u indices", estimatedVertices, estimatedIndices);
}

MeshAllocation GPUCullingManager::RegisterMesh(
    const void* vertices,
    u32 vertexCount,
    u32 vertexStride,
    bindless::SourceVertexFormat format,
    const u16* indices,
    u32 indexCount)
{
    MeshAllocation alloc;

    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterMesh called outside of level load");
        return alloc;
    }

    if (!vertices || vertexCount == 0 || !indices || indexCount == 0) {
        return alloc;
    }

    if (format == bindless::SourceVertexFormat::Unknown) {
        Msg("! [GPUCulling] RegisterMesh: Unknown vertex format (stride=%u)", vertexStride);
        return alloc;
    }

    // Record offsets before adding
    alloc.vertexOffset = m_totalVertexCount;
    alloc.indexOffset = m_totalIndexCount;
    alloc.vertexCount = vertexCount;
    alloc.indexCount = indexCount;

    // Convert vertices to unified format
    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterMesh: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);  // Rollback
        return alloc;
    }

    // Copy indices, converting from 16-bit to 32-bit and adjusting for vertex offset
    u32 prevIndexSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevIndexSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
        // Note: We store raw indices without adding vertexOffset here
        // The offset will be handled via baseVertexLocation in draw args
        m_megaIndices[prevIndexSize + i] = static_cast<u32>(indices[i]);
    }

    m_totalVertexCount += vertexCount;
    m_totalIndexCount += indexCount;
    alloc.valid = true;

    return alloc;
}

void GPUCullingManager::CreateMegaBuffers()
{
    if (!m_device) {
        Msg("! [GPUCulling] CreateMegaBuffers: No device");
        return;
    }

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    // Create vertex buffer
    // NOTE: Vertex buffers cannot use structStride (structured buffer) - D3D11 restriction
    if (m_totalVertexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaVertexBuffer";
        desc.byteSize = m_totalVertexCount * sizeof(bindless::UnifiedVertex);
        desc.isVertexBuffer = true;  // Required for D3D11 vertex buffer binding
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct);

        m_megaVertexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaVertexBuffer) {
            Msg("! [GPUCulling] Failed to create mega vertex buffer (%u vertices, %zu bytes)",
                m_totalVertexCount, desc.byteSize);
            return;
        }

        m_maxMegaVertices = m_totalVertexCount;
    }

    // Create index buffer (32-bit indices)
    if (m_totalIndexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaIndexBuffer";
        desc.byteSize = m_totalIndexCount * sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.isIndexBuffer = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct);

        m_megaIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaIndexBuffer) {
            Msg("! [GPUCulling] Failed to create mega index buffer (%u indices, %zu bytes)",
                m_totalIndexCount, desc.byteSize);
            return;
        }

        m_maxMegaIndices = m_totalIndexCount;
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DynamicPrevWorld";
        desc.byteSize = m_maxObjects * sizeof(Fmatrix);
        desc.structStride = sizeof(Fmatrix);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_dynamicPrevWorldBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_dynamicPrevWorldBuffer, "Failed to create dynamic previous-world buffer");
    }

    Msg("* [GPUCulling] Mega-buffers created: VB=%u verts (%.1f MB), IB=%u indices (%.1f MB)",
        m_totalVertexCount,
        (m_totalVertexCount * sizeof(bindless::UnifiedVertex)) / (1024.0f * 1024.0f),
        m_totalIndexCount,
        (m_totalIndexCount * sizeof(u32)) / (1024.0f * 1024.0f));
}

void GPUCullingManager::EndLevelLoad()
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] EndLevelLoad called without BeginLevelLoad");
        return;
    }

    Msg("* [GPUCulling] EndLevelLoad - collected %u vertices, %u indices",
        m_totalVertexCount, m_totalIndexCount);

    // Create GPU buffers
    CreateMegaBuffers();

    if (!m_megaVertexBuffer || !m_megaIndexBuffer) {
        Msg("! [GPUCulling] EndLevelLoad: Failed to create mega-buffers");
        m_levelLoadInProgress = false;
        return;
    }

    // Upload data to GPU (need a context - will be done on first frame)
    // Mark as ready - upload will happen in UploadSceneObjects on first frame
    m_megaBuffersReady = true;
    m_levelLoadInProgress = false;

    Msg("* [GPUCulling] Mega-buffers ready for GPU upload");
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

    const bool cacheEnabled = ps_r_cluster_cache != 0;

    if (!cacheEnabled || !m_clusterDAG.TryLoadCache(cachePath, geomStamp, ranges)) {
        m_clusterDAG.Bake(ranges, m_megaVertices.data(), m_megaIndices.data());
        if (cacheEnabled)
            m_clusterDAG.SaveCache(cachePath, geomStamp, ranges);
    }

    const xr_vector<u32>& baked = m_clusterDAG.BakedIndices();
    if (baked.empty())
        return;

    m_clusterDAG.SetMegaIndexBase(m_totalIndexCount);
    m_megaIndices.insert(m_megaIndices.end(), baked.begin(), baked.end());
    m_totalIndexCount += u32(baked.size());
    m_clusterDAG.ReleaseIndexData();

    Msg("* [GPUCulling] cluster indices appended at %u, mega-IB now %u indices (%.1f MB)",
        m_clusterDAG.MegaIndexBase(), m_totalIndexCount,
        (m_totalIndexCount * sizeof(u32)) / (1024.0f * 1024.0f));
}

u32 GPUCullingManager::GetStaticResidualCount() const
{
    if (m_clusterSet.entryCount > 0)
        return m_clusterSet.residualStaticCount;
    return m_staticObjectCount;
}

u32 GPUCullingManager::GetTerrainResidualCount() const
{
    if (m_clusterSet.entryCount > 0)
        return m_clusterSet.residualTerrainCount;
    return m_terrainObjectCount;
}

static void EmitClusterEntry(const ClusterDAG& dag, u32 megaBase, const ClusterMetaProto& p, const ClusterUnitRecord& rec,
    u32 batchIndex, const Fmatrix& world, u32 materialID, u32 extraFlags, xr_vector<GPUClusterEntry>& out)
{
    const float scale = std::max(world.i.magnitude(), std::max(world.j.magnitude(), world.k.magnitude()));

    GPUClusterEntry e;
    Fvector c;

    world.transform_tiny(c, Fvector().set(p.sphere[0], p.sphere[1], p.sphere[2]));
    e.sphere.set(c.x, c.y, c.z, p.sphere[3] * scale);
    e.extent.set(
        _abs(world.i.x) * p.extent[0] + _abs(world.j.x) * p.extent[1] + _abs(world.k.x) * p.extent[2],
        _abs(world.i.y) * p.extent[0] + _abs(world.j.y) * p.extent[1] + _abs(world.k.y) * p.extent[2],
        _abs(world.i.z) * p.extent[0] + _abs(world.j.z) * p.extent[1] + _abs(world.k.z) * p.extent[2],
        0.0f);
    world.transform_tiny(c, Fvector().set(p.lodSelf[0], p.lodSelf[1], p.lodSelf[2]));
    e.lodSelf.set(c.x, c.y, c.z, p.lodSelf[3] * scale);
    world.transform_tiny(c, Fvector().set(p.lodParent[0], p.lodParent[1], p.lodParent[2]));
    e.lodParent.set(c.x, c.y, c.z, p.lodParent[3] * scale);

    e.selfError = p.selfError * scale;
    float pe = p.parentError * scale;
    if (!(pe < 1e30f))
        pe = 1e30f;
    e.parentError = pe;

    e.indexCount = p.indexCount;
    e.ibFirst = megaBase + rec.firstIndex + p.ibFirst;
    e.firstVertex = rec.isComponent
        ? 0
        : dag.MemberKeys()[rec.firstMember + p.member].vertexOffset;
    e.batchIndex = batchIndex;
    e.materialID = materialID;
    e.flags = ((p.flags & CLUSTER_PROTO_FLAG_AT) ? GPU_CLUSTER_ENTRY_AT : 0) |
              ((p.flags & CLUSTER_PROTO_FLAG_TERRAIN) ? GPU_CLUSTER_ENTRY_TERRAIN : 0) |
              extraFlags;

    u32 errClass = 3;
    if (e.parentError < 1.0f) errClass = 0;
    else if (e.parentError < 10.0f) errClass = 1;
    else if (e.parentError < 1e30f) errClass = 2;
    e.flags |= (std::min(p.depth, 15u) << 8) | (errClass << 12);

    out.push_back(e);
}

void GPUCullingManager::BuildDynamicClusterEntries(nvrhi::ICommandList* cmdList)
{
    m_dynamicEntryData.clear();
    m_clusterSet.dynamicEntryCount = 0;
    const u32 dynamicCount = m_dynamicObjectCount;
    const bool historyValid = m_dynamicHistoryFrame + 1u == Device.dwFrame;
    const auto& prevHistory = m_dynamicHistory[m_dynamicHistoryIndex];
    auto& nextHistory = m_dynamicHistory[m_dynamicHistoryIndex ^ 1u];
    nextHistory.clear();
    m_dynamicPrevWorldData.resize(dynamicCount);
    u32 clustered = 0;

    const bool clusterReady = m_clusterSet.uploaded && m_clusterSet.entryBuffer;
    const xr_vector<ClusterMetaProto>& protos = m_clusterDAG.Protos();
    const u32 megaBase = m_clusterDAG.MegaIndexBase();

    for (u32 i = 0; i < dynamicCount; ++i) {
        const Fmatrix& world = m_dynamicInstanceData[i].world;
        const auto& id = m_dynamicIdentity[i];
        Fmatrix prevWorld = world;
        if (historyValid) {
            auto it = prevHistory.find(id);
            if (it != prevHistory.end())
                prevWorld = it->second;
        }
        nextHistory[id] = world;
        m_dynamicPrevWorldData[i] = prevWorld;

        if (!clusterReady)
            continue;
        const ClusterMeshKey& key = m_dynamicBatchKeys[i];
        if (key.indexCount == 0)
            continue;
        if (m_dynamicObjectFlags[i] & GPU_OBJECT_NO_RESOLVE)
            continue;
        u32 member = 0;
        const ClusterUnitRecord* rec = m_clusterDAG.FindRecord(key, member);
        if (!rec)
            continue;
        if (m_dynamicEntryData.size() + rec->protoCount > kDynamicClusterEntryCapacity)
            continue;
        for (u32 p = 0; p < rec->protoCount; ++p) {
            const ClusterMetaProto& proto = protos[rec->firstProto + p];
            if (rec->isComponent && proto.member != member)
                continue;
            EmitClusterEntry(m_clusterDAG, megaBase, proto, *rec, i, world,
                m_dynamicMaterialIDData[i], GPU_CLUSTER_ENTRY_DYNAMIC, m_dynamicEntryData);
        }
        ++clustered;
    }
    m_clusterSet.dynamicResidualCount = dynamicCount - clustered;
    m_dynamicHistoryIndex ^= 1u;
    m_dynamicHistoryFrame = Device.dwFrame;

    m_clusterSet.dynamicEntryCount = static_cast<u32>(m_dynamicEntryData.size());
    if (m_clusterSet.dynamicEntryCount > 0)
        cmdList->writeBuffer(m_clusterSet.entryBuffer, m_dynamicEntryData.data(),
            u64(m_clusterSet.dynamicEntryCount) * sizeof(GPUClusterEntry),
            u64(m_clusterSet.entryCount) * sizeof(GPUClusterEntry));
    if (dynamicCount > 0 && m_dynamicPrevWorldBuffer)
        cmdList->writeBuffer(m_dynamicPrevWorldBuffer, m_dynamicPrevWorldData.data(), u64(dynamicCount) * sizeof(Fmatrix));
}

void GPUCullingManager::BuildClusterEntries()
{
    m_clusterEntryData.clear();
    m_clusterSet.entryCount = 0;
    m_clusterSet.staticEntryCount = 0;
    m_clusterSet.terrainEntryCount = 0;
    m_clusterSet.residualStaticCount = 0;
    m_clusterSet.residualTerrainCount = 0;
    m_clusterSet.uploaded = false;

    if (m_clusterDAG.Empty())
        return;

    const u32 staticCount = m_staticObjectCount;
    if (staticCount == 0 || m_staticBatchKeys.size() < staticCount)
        return;

    const xr_vector<ClusterMetaProto>& protos = m_clusterDAG.Protos();
    const xr_vector<ClusterUnitRecord>& records = m_clusterDAG.Records();
    const u32 megaBase = m_clusterDAG.MegaIndexBase();

    xr_vector<xr_vector<u32>> componentBatches(records.size());

    auto emitEntry = [&](const ClusterMetaProto& p, const ClusterUnitRecord& rec,
                         u32 batchIndex, const Fmatrix& world, u32 materialID, u32 extraFlags = 0u) {
        EmitClusterEntry(m_clusterDAG, megaBase, p, rec, batchIndex, world, materialID, extraFlags, m_clusterEntryData);
    };

    auto emitStatic = [&](const ClusterMetaProto& p, const ClusterUnitRecord& rec, u32 batchIndex, u32 extraFlags) {
        emitEntry(p, rec, batchIndex, m_staticInstanceData[batchIndex].world,
            m_staticMaterialIDData[batchIndex], extraFlags);
    };

    u32 clusteredBatches = 0;
    u32 shadowOnlyBatches = 0;
    u32 unclusteredBatches = 0;
    xr_vector<u8> shadowOnly(staticCount, 0);
    for (u32 i = 0; i < staticCount; ++i) {
        const ClusterMeshKey& key = m_staticBatchKeys[i];
        if (key.indexCount == 0)
            continue;
        if (m_staticObjectFlags[i] & GPU_OBJECT_NO_RESOLVE) {
            const auto* mat = bindless::MaterialBuffer::Instance().GetMaterial(m_staticMaterialIDData[i]);
            if (!mat || (mat->flags & bindless::MAT_FLAG_ALPHA_BLEND))
                continue;
            shadowOnly[i] = 1;
        }

        u32 member = 0;
        const ClusterUnitRecord* rec = m_clusterDAG.FindRecord(key, member);
        if (!rec) {
            unclusteredBatches++;
            continue;
        }

        if (shadowOnly[i])
            shadowOnlyBatches++;
        else
            clusteredBatches++;
        const u32 extraFlags = shadowOnly[i] ? u32(GPU_CLUSTER_ENTRY_SHADOW_ONLY) : 0u;

        if (!rec->isComponent) {
            for (u32 p = 0; p < rec->protoCount; ++p)
                emitStatic(protos[rec->firstProto + p], *rec, i, extraFlags);
        } else {
            const u32 recIdx = u32(rec - records.data());
            xr_vector<u32>& memberBatch = componentBatches[recIdx];
            if (memberBatch.empty())
                memberBatch.resize(rec->memberCount, UINT32_MAX);
            if (memberBatch[member] == UINT32_MAX)
                memberBatch[member] = i;
        }
    }

    for (u32 r = 0; r < u32(records.size()); ++r) {
        const xr_vector<u32>& memberBatch = componentBatches[r];
        if (memberBatch.empty())
            continue;

        const ClusterUnitRecord& rec = records[r];
        for (u32 p = 0; p < rec.protoCount; ++p) {
            const ClusterMetaProto& proto = protos[rec.firstProto + p];
            if (proto.member >= memberBatch.size() || memberBatch[proto.member] == UINT32_MAX)
                continue;
            const u32 batch = memberBatch[proto.member];
            emitStatic(proto, rec, batch, shadowOnly[batch] ? u32(GPU_CLUSTER_ENTRY_SHADOW_ONLY) : 0u);
        }
    }

    m_clusterSet.staticEntryCount = u32(m_clusterEntryData.size());
    m_clusterSet.residualStaticCount = staticCount - std::min(clusteredBatches, staticCount);

    u32 clusteredTerrain = 0;
    const u32 terrainCount = std::min(u32(m_terrainInstanceData.size()), u32(m_terrainBatchKeys.size()));
    xr_vector<xr_vector<u32>> terrainComponentBatches(records.size());

    for (u32 i = 0; i < terrainCount; ++i) {
        const ClusterMeshKey& key = m_terrainBatchKeys[i];
        if (key.indexCount == 0)
            continue;

        u32 member = 0;
        const ClusterUnitRecord* rec = m_clusterDAG.FindRecord(key, member);
        if (!rec)
            continue;

        clusteredTerrain++;

        if (!rec->isComponent) {
            for (u32 p = 0; p < rec->protoCount; ++p)
                emitEntry(protos[rec->firstProto + p], *rec, i,
                    m_terrainInstanceData[i].world, m_terrainMaterialIDData[i]);
        } else {
            const u32 recIdx = u32(rec - records.data());
            xr_vector<u32>& memberBatch = terrainComponentBatches[recIdx];
            if (memberBatch.empty())
                memberBatch.resize(rec->memberCount, UINT32_MAX);
            if (memberBatch[member] == UINT32_MAX)
                memberBatch[member] = i;
        }
    }

    for (u32 r = 0; r < u32(records.size()); ++r) {
        const xr_vector<u32>& memberBatch = terrainComponentBatches[r];
        if (memberBatch.empty())
            continue;

        const ClusterUnitRecord& rec = records[r];
        for (u32 p = 0; p < rec.protoCount; ++p) {
            const ClusterMetaProto& proto = protos[rec.firstProto + p];
            if (proto.member >= memberBatch.size() || memberBatch[proto.member] == UINT32_MAX)
                continue;
            const u32 batch = memberBatch[proto.member];
            emitEntry(proto, rec, batch,
                m_terrainInstanceData[batch].world, m_terrainMaterialIDData[batch]);
        }
    }

    m_clusterSet.entryCount = u32(m_clusterEntryData.size());
    m_clusterSet.terrainEntryCount = m_clusterSet.entryCount - m_clusterSet.staticEntryCount;
    m_clusterSet.residualTerrainCount = terrainCount - std::min(clusteredTerrain, terrainCount);
    Msg("* [GPUCulling] cluster coverage: %u static batches shadow-only (variant materials), %u without a DAG record", shadowOnlyBatches, unclusteredBatches);
    {
        u32 giants = 0;
        float maxR = 0.0f;
        u32 maxIdx = 0;
        for (u32 i = 0; i < u32(m_clusterEntryData.size()); ++i) {
            const float r = m_clusterEntryData[i].sphere.w;
            if (r > 100.0f) {
                ++giants;
                if (r > maxR) { maxR = r; maxIdx = i; }
            }
        }
        if (giants) {
            const GPUClusterEntry& g = m_clusterEntryData[maxIdx];
            Msg("! [GPUCulling] %u cluster entries with radius > 100 m; largest r=%.1f entry=%u batch=%u flags=0x%x tris=%u selfErr=%.3f parentErr=%.3f",
                giants, maxR, maxIdx, g.batchIndex, g.flags, g.indexCount / 3u, g.selfError, g.parentError);
        }
    }
    Msg("* [GPUCulling] cluster entries: %u (%u static + %u terrain) from %u+%u clustered batches",
        m_clusterSet.entryCount, m_clusterSet.staticEntryCount, m_clusterSet.terrainEntryCount,
        clusteredBatches, clusteredTerrain);
}

void GPUCullingManager::UploadClusterEntries(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice)
{
    if (m_clusterSet.entryCount == 0 || m_clusterSet.uploaded)
        return;

    const u32 n = m_clusterSet.entryCount;

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "ClusterCull_Entries";
        desc.byteSize = u64(n + kDynamicClusterEntryCapacity) * sizeof(GPUClusterEntry);
        desc.structStride = sizeof(GPUClusterEntry);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_clusterSet.entryBuffer = nvDevice->createBuffer(desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "ClusterCull_Count";
        desc.byteSize = sizeof(u32) * kClusterCountWords;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_clusterSet.countBuffer = nvDevice->createBuffer(desc);
    }

    auto makeStreamBuffer = [&](const char* name, u32 elems) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = u64(std::max(elems, 1u)) * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };
    m_clusterSet.visibleEntryBuffer = makeStreamBuffer("ClusterCull_VisibleEntries", m_clusterSet.staticEntryCount + kDynamicClusterEntryCapacity);
    m_clusterSet.fadeBuffer = makeStreamBuffer("ClusterCull_Fades", m_clusterSet.staticEntryCount + kDynamicClusterEntryCapacity);
    m_clusterSet.terrainVisibleEntryBuffer = makeStreamBuffer("ClusterCull_TerrainVisibleEntries", m_clusterSet.terrainEntryCount);
    m_clusterSet.terrainFadeBuffer = makeStreamBuffer("ClusterCull_TerrainFades", m_clusterSet.terrainEntryCount);
    m_clusterSet.candidateBuffer = makeStreamBuffer("ClusterCull_Candidates", n + kDynamicClusterEntryCapacity);
    m_clusterSet.visibleEntryBuffer2 = makeStreamBuffer("ClusterCull_RetestVisibleEntries", m_clusterSet.staticEntryCount + kDynamicClusterEntryCapacity);
    m_clusterSet.fadeBuffer2 = makeStreamBuffer("ClusterCull_RetestFades", m_clusterSet.staticEntryCount + kDynamicClusterEntryCapacity);
    m_clusterSet.terrainVisibleEntryBuffer2 = makeStreamBuffer("ClusterCull_RetestTerrainVisibleEntries", m_clusterSet.terrainEntryCount);
    m_clusterSet.terrainFadeBuffer2 = makeStreamBuffer("ClusterCull_RetestTerrainFades", m_clusterSet.terrainEntryCount);

    if (!m_clusterSet.entryBuffer || !m_clusterSet.countBuffer ||
        !m_clusterSet.visibleEntryBuffer || !m_clusterSet.fadeBuffer ||
        !m_clusterSet.terrainVisibleEntryBuffer || !m_clusterSet.terrainFadeBuffer ||
        !m_clusterSet.candidateBuffer || !m_clusterSet.visibleEntryBuffer2 || !m_clusterSet.fadeBuffer2 ||
        !m_clusterSet.terrainVisibleEntryBuffer2 || !m_clusterSet.terrainFadeBuffer2) {
        Msg("! [GPUCulling] cluster buffer creation failed, disabling cluster path");
        m_clusterSet = {};
        return;
    }

    cmdList->writeBuffer(m_clusterSet.entryBuffer,
        m_clusterEntryData.data(), u64(n) * sizeof(GPUClusterEntry));

    {
        ClusterBvh bvh;
        BuildClusterShadowBVH(m_clusterEntryData.data(), n, bvh);
        m_clusterSet.bvhNodeCount = u32(bvh.nodes.size());
        nvrhi::BufferDesc desc;
        desc.debugName = "ClusterCull_ShadowBvhNodes";
        desc.byteSize = u64(m_clusterSet.bvhNodeCount) * sizeof(ClusterBvhNode);
        desc.structStride = sizeof(ClusterBvhNode);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_clusterSet.bvhNodeBuffer = nvDevice->createBuffer(desc);
        desc.debugName = "ClusterCull_ShadowBvhIndices";
        desc.byteSize = u64(bvh.indices.size()) * sizeof(u32);
        desc.structStride = sizeof(u32);
        m_clusterSet.bvhIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_clusterSet.bvhNodeBuffer || !m_clusterSet.bvhIndexBuffer) {
            Msg("! [GPUCulling] shadow BVH buffer creation failed, disabling cluster path");
            m_clusterSet = {};
            return;
        }
        cmdList->writeBuffer(m_clusterSet.bvhNodeBuffer, bvh.nodes.data(),
            u64(m_clusterSet.bvhNodeCount) * sizeof(ClusterBvhNode));
        cmdList->writeBuffer(m_clusterSet.bvhIndexBuffer, bvh.indices.data(),
            u64(bvh.indices.size()) * sizeof(u32));
        Msg("* [GPUCulling] shadow BVH: %u nodes, %u leaves, depth %u (%.1f MB)",
            m_clusterSet.bvhNodeCount, bvh.leafCount, bvh.maxDepth,
            (u64(m_clusterSet.bvhNodeCount) * sizeof(ClusterBvhNode) + u64(n) * sizeof(u32)) / (1024.0f * 1024.0f));
    }

    u32 zeroCount[kClusterCountWords] = {};
    cmdList->writeBuffer(m_clusterSet.countBuffer, zeroCount, sizeof(zeroCount));
    u32 zeroArgs[4] = { 384, 0, 0, 0 };
    cmdList->writeBuffer(m_clusterArgsBuffer, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterTerrainArgsBuffer, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterArgsBuffer2, zeroArgs, sizeof(zeroArgs));
    cmdList->writeBuffer(m_clusterTerrainArgsBuffer2, zeroArgs, sizeof(zeroArgs));

    m_clusterSet.uploaded = true;
    m_clusterEntryData.clear();
    m_clusterEntryData.shrink_to_fit();

    Msg("* [GPUCulling] cluster entry buffers uploaded: %u entries (%.1f MB)",
        n, (u64(n) * (sizeof(GPUClusterEntry) + 2 * sizeof(u32))) / (1024.0f * 1024.0f));
}

struct ClusterCullParamsCB {
    Fmatrix hizViewProj;
    Fvector4 frustumPlanes[6];
    Fvector4 cameraPos;
    Fvector4 viewDir;
    Fvector4 lodParams;
    u32 entryCount;
    u32 useHiZ;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    float ssaCull;
    u32 pad[2];
};

bool GPUCullingManager::EnsureClusterCullPipeline(nvrhi::IDevice* nvDevice)
{
    if (m_clusterCullPipeline && m_clusterCullLayout && m_clusterArgsPipeline &&
        m_clusterArgsLayout && m_clusterRetestPipeline && m_clusterRetestLayout &&
        m_clusterCullParamsCB.IsValid() && m_clusterArgsParamsCB.IsValid())
        return true;

    auto result = GEnv.Render->GetShaderLoader()->LoadComputeShader("cluster_cull");
    auto retestResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("cluster_cull_retest");
    auto argsResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("cluster_draw_args");
    if (!result.handle || !retestResult.handle || !argsResult.handle) {
        Msg("! [GPUCulling] cluster cull shaders failed to load");
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_cull", ".cs");
    auto* retestRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_cull_retest", ".cs");
    auto* argsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_draw_args", ".cs");
    if (!refl || !retestRefl || !argsRefl)
        return false;

    m_clusterCullLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterCull", *refl, nvDevice);
    m_clusterRetestLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterRetest", *retestRefl, nvDevice);
    m_clusterArgsLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ClusterArgs", *argsRefl, nvDevice);
    if (!m_clusterCullLayout || !m_clusterRetestLayout || !m_clusterArgsLayout)
        return false;

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = result.handle;
    pipeDesc.bindingLayouts = { m_clusterCullLayout };
    m_clusterCullPipeline = nvDevice->createComputePipeline(pipeDesc);

    nvrhi::ComputePipelineDesc retestPipeDesc;
    retestPipeDesc.CS = retestResult.handle;
    retestPipeDesc.bindingLayouts = { m_clusterRetestLayout };
    m_clusterRetestPipeline = nvDevice->createComputePipeline(retestPipeDesc);

    nvrhi::ComputePipelineDesc argsPipeDesc;
    argsPipeDesc.CS = argsResult.handle;
    argsPipeDesc.bindingLayouts = { m_clusterArgsLayout };
    m_clusterArgsPipeline = nvDevice->createComputePipeline(argsPipeDesc);

    if (!m_clusterCullPipeline || !m_clusterRetestPipeline || !m_clusterArgsPipeline)
        return false;

    if (!m_clusterArgsParamsCB.IsValid()) {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "ClusterCull_ArgsParams";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 64;
        m_clusterArgsParamsCB = m_device->CreateBuffer(desc);
        if (!m_clusterArgsParamsCB.IsValid())
            return false;
    }

    if (!m_clusterCullParamsCB.IsValid()) {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "ClusterCull_Params";
        desc.byteSize = sizeof(ClusterCullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 512;
        m_clusterCullParamsCB = m_device->CreateBuffer(desc);
    }

    return m_clusterCullParamsCB.IsValid();
}

void GPUCullingManager::DispatchClusterArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice, u32 countBase,
    nvrhi::IBuffer* args, nvrhi::IBuffer* terrainArgs)
{
    struct ClusterArgsParamsCB {
        u32 countBase;
        u32 pad[3];
    };
    ClusterArgsParamsCB cb = {};
    cb.countBase = countBase;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterArgsParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(args, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(terrainArgs, nvrhi::ResourceStates::UnorderedAccess);

    auto* argsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_draw_args", ".cs");
    framegraph::BindingSetBuilder argsBsb(*argsRefl, nvDevice, "GPUCull.ClusterArgs");
    argsBsb.ConstantBuffer("ClusterArgsParams", m_device->GetNativeBuffer(m_clusterArgsParamsCB))
           .BufferSRV("g_Count", m_clusterSet.countBuffer)
           .BufferUAV("g_Args", args)
           .BufferUAV("g_TerrainArgs", terrainArgs);

    nvrhi::BindingSetHandle argsBindingSet = nvDevice->createBindingSet(argsBsb.Build(), m_clusterArgsLayout);
    if (!argsBindingSet)
        return;

    nvrhi::ComputeState argsState;
    argsState.pipeline = m_clusterArgsPipeline;
    argsState.bindings = { argsBindingSet };
    cmdList->setComputeState(argsState);
    cmdList->dispatch(1, 1, 1);

    cmdList->setBufferState(args, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(terrainArgs, nvrhi::ResourceStates::IndirectArgument);
}

void GPUCullingManager::FillClusterCullParams(ClusterCullParamsCB& cb, const Fmatrix& hizViewProj, u32 entryCount,
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
    cb.entryCount = entryCount;
    cb.useHiZ = useHiZ ? 1u : 0u;
    cb.hizWidth = useHiZ ? hizWidth : 1u;
    cb.hizHeight = useHiZ ? hizHeight : 1u;
    cb.hizMipLevels = useHiZ ? hizMipLevels : 1u;
    cb.ssaCull = 0.0f;
}

void GPUCullingManager::DispatchClusterCull(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* prevHiZ, const Fmatrix& prevViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels)
{
    if (m_clusterSet.entryCount == 0 || !m_clusterSet.uploaded)
        return;
    if (!EnsureClusterCullPipeline(nvDevice)) {
        Msg("! [GPUCulling] cluster cull pipeline unavailable, disabling cluster path");
        m_clusterSet.entryCount = 0;
        return;
    }

    u32 zero[kClusterCountWords] = {};
    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(m_clusterSet.countBuffer, zero, sizeof(zero));

    const u32 entryCount = m_clusterSet.entryCount + m_clusterSet.dynamicEntryCount;
    ClusterCullParamsCB cb;
    FillClusterCullParams(cb, prevHiZ ? prevViewProj : Device.mFullTransform, entryCount, prevHiZ != nullptr,
        hizWidth, hizHeight, hizMipLevels);
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.visibleEntryBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.fadeBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.terrainVisibleEntryBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.terrainFadeBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.candidateBuffer, nvrhi::ResourceStates::UnorderedAccess);

    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_cull", ".cs");
    framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.ClusterCull");
    bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
       .BufferSRV("g_Entries", m_clusterSet.entryBuffer)
       .Texture("g_HiZPyramid", prevHiZ ? prevHiZ : m_dummyHiZ.Get())
       .BufferUAV("g_OutCount", m_clusterSet.countBuffer)
       .BufferUAV("g_OutEntryIndices", m_clusterSet.visibleEntryBuffer)
       .BufferUAV("g_OutFades", m_clusterSet.fadeBuffer)
       .BufferUAV("g_OutTerrainEntryIndices", m_clusterSet.terrainVisibleEntryBuffer)
       .BufferUAV("g_OutTerrainFades", m_clusterSet.terrainFadeBuffer)
       .BufferUAV("g_OutCandidates", m_clusterSet.candidateBuffer);

    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterCullLayout);
    if (!bindingSet)
        return;

    nvrhi::ComputeState state;
    state.pipeline = m_clusterCullPipeline;
    state.bindings = { bindingSet };
    cmdList->setComputeState(state);
    cmdList->dispatch((entryCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE, 1, 1);

    cmdList->setBufferState(m_clusterSet.visibleEntryBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.fadeBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.terrainVisibleEntryBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.terrainFadeBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.candidateBuffer, nvrhi::ResourceStates::ShaderResource);

    DispatchClusterArgs(cmdList, nvDevice, 0, m_clusterArgsBuffer, m_clusterTerrainArgsBuffer);
}

void GPUCullingManager::DispatchClusterRetest(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* hiz, u32 hizWidth, u32 hizHeight, u32 hizMipLevels)
{
    if (m_clusterSet.entryCount == 0 || !m_clusterSet.uploaded || !hiz)
        return;
    if (!m_clusterRetestPipeline || !m_clusterRetestLayout || !m_clusterArgsPipeline)
        return;

    const u32 entryCount = m_clusterSet.entryCount + m_clusterSet.dynamicEntryCount;
    ClusterCullParamsCB cb;
    FillClusterCullParams(cb, Device.mFullTransform, entryCount, true,
        hizWidth, hizHeight, hizMipLevels);
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_clusterCullParamsCB), &cb, sizeof(cb));

    cmdList->setBufferState(m_clusterSet.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.candidateBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.visibleEntryBuffer2, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.fadeBuffer2, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.terrainVisibleEntryBuffer2, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(m_clusterSet.terrainFadeBuffer2, nvrhi::ResourceStates::UnorderedAccess);

    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_cull_retest", ".cs");
    framegraph::BindingSetBuilder bsb(*refl, nvDevice, "GPUCull.ClusterRetest");
    bsb.ConstantBuffer("ClusterCullParams", m_device->GetNativeBuffer(m_clusterCullParamsCB))
       .BufferSRV("g_Entries", m_clusterSet.entryBuffer)
       .Texture("g_HiZPyramid", hiz)
       .BufferSRV("g_Candidates", m_clusterSet.candidateBuffer)
       .BufferUAV("g_OutCount", m_clusterSet.countBuffer)
       .BufferUAV("g_OutEntryIndices", m_clusterSet.visibleEntryBuffer2)
       .BufferUAV("g_OutFades", m_clusterSet.fadeBuffer2)
       .BufferUAV("g_OutTerrainEntryIndices", m_clusterSet.terrainVisibleEntryBuffer2)
       .BufferUAV("g_OutTerrainFades", m_clusterSet.terrainFadeBuffer2);

    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), m_clusterRetestLayout);
    if (!bindingSet)
        return;

    nvrhi::ComputeState state;
    state.pipeline = m_clusterRetestPipeline;
    state.bindings = { bindingSet };
    cmdList->setComputeState(state);
    cmdList->dispatch((entryCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE, 1, 1);

    cmdList->setBufferState(m_clusterSet.visibleEntryBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.fadeBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.terrainVisibleEntryBuffer2, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_clusterSet.terrainFadeBuffer2, nvrhi::ResourceStates::ShaderResource);

    DispatchClusterArgs(cmdList, nvDevice, 20, m_clusterArgsBuffer2, m_clusterTerrainArgsBuffer2);
}

// ═══════════════════════════════════════════════════════
//  VB/IB POOL REGISTRATION (for level geometry)
// ═══════════════════════════════════════════════════════

bindless::SourceVertexFormat GPUCullingManager::DetectFormatFromDecl(
    const VertexElement* decl,
    u32 stride)
{
    if (!decl)
        return bindless::SourceVertexFormat::Unknown;

    bool hasColor = false;
    bool hasTexCoord1 = false;
    bool hasShort4TexCoord = false;
    bool hasFloat2TexCoord = false;
    u32 texcoord0Type = 0;

    for (int i = 0; decl[i].Stream != 0xFF; i++) {
        const VertexElement& elem = decl[i];

        if (elem.Usage == VS_COLOR && elem.UsageIndex == 0) {
            hasColor = true;
        }
        else if (elem.Usage == VS_TEXCOORD) {
            if (elem.UsageIndex == 0) {
                texcoord0Type = elem.Type;
                if (elem.Type == VF_SHORT4) hasShort4TexCoord = true;
                if (elem.Type == VF_FLOAT2) hasFloat2TexCoord = true;
            }
            else if (elem.UsageIndex == 1) {
                hasTexCoord1 = true;
            }
        }
    }

    // Match against known formats
    if (stride == 12) {
        return bindless::SourceVertexFormat::X_Vert;
    }
    else if (stride == 28) {
        // Unpacked formats
        if (hasColor) return bindless::SourceVertexFormat::R1_Vert_Unpacked;
        return bindless::SourceVertexFormat::MU_Model_Unpacked;
    }
    else if (stride == 32) {
        if (hasFloat2TexCoord && hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap_Unpacked;
        }
        if (hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap;  // Lightmapped
        }
        if (hasColor) {
            return bindless::SourceVertexFormat::R1_Vert;  // Vertex-lit
        }
        if (hasShort4TexCoord) {
            return bindless::SourceVertexFormat::MU_Model;  // Trees/models
        }
        // Default to R1_Lmap for 32-byte without clear indicators
        return bindless::SourceVertexFormat::R1_Lmap;
    }

    Msg("! [GPUCulling] Unknown vertex format: stride=%u, hasColor=%d, hasTexCoord1=%d, hasShort4=%d",
        stride, hasColor, hasTexCoord1, hasShort4TexCoord);
    return bindless::SourceVertexFormat::Unknown;
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

    // Detect vertex format from declaration
    bindless::SourceVertexFormat format = DetectFormatFromDecl(decl, vertexStride);
    if (format == bindless::SourceVertexFormat::Unknown) {
        // Still register a placeholder pool to keep indices in sync
        // (FVisual stores the raw VB ID, so pool[i] must correspond to VB[i])
        Msg("! [GPUCulling] RegisterVBPool: Unknown vertex format (stride=%u) - registering placeholder", vertexStride);

        VBPoolInfo placeholder;
        placeholder.megaBufferVertexOffset = 0;
        placeholder.vertexCount = 0;  // Empty placeholder
        placeholder.format = bindless::SourceVertexFormat::Unknown;

        xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;  // Return valid index but pool has 0 vertices
    }

    // Create pool info
    VBPoolInfo poolInfo;
    poolInfo.megaBufferVertexOffset = m_totalVertexCount;
    poolInfo.vertexCount = vertexCount;
    poolInfo.format = format;

    // Convert vertices to unified format and add to mega-buffer
    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterVBPool: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);  // Rollback
        return UINT32_MAX;
    }

    m_totalVertexCount += vertexCount;

    // Store pool info
    xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
    u32 poolID = static_cast<u32>(pools.size());
    pools.push_back(poolInfo);

    Msg("* [GPUCulling] RegisterVBPool[%u]: %u verts (format=%d, offset=%u)%s",
        poolID, vertexCount, static_cast<int>(format), poolInfo.megaBufferVertexOffset,
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
