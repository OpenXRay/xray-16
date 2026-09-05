#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/ClusterDAG.h"

namespace xray::render::fg::passes {
    struct ParticleBatch;
}

namespace xray::render {
    class GeometryCollector;
    struct GeometryBatch;
    namespace fg {
        class RenderDevice;
        class RenderContext;
    }
    namespace framegraph {
        class FrameGraph;
    }
}

namespace xray::render::fg {
    class dxRender_Visual;
    class CKinematics;
    class RTAccelStructManager;
    namespace decals {
        class OverlayManager;
    }
}

namespace xray::render::fg {

struct ClusterCullParamsCB;

struct GPUParticleData {
    Fvector position;
    float radius;
    u32 batchIndex;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUParticleData) == 32, "GPUParticleData must be 32 bytes for GPU alignment");


enum GPUObjectFlags : u32 {
    GPU_OBJECT_NO_RESOLVE = 0x1,
};

struct GPUClusterEntry {
    Fvector4 sphere;
    Fvector4 lodSelf;
    Fvector4 lodParent;
    u32 indexCount;
    u32 ibFirst;
    u32 firstVertex;
    u32 batchIndex;
    u32 materialID;
    u32 flags;
    float selfError;
    float parentError;
    Fvector4 extent;
};
static_assert(sizeof(GPUClusterEntry) == 96, "GPUClusterEntry must be 96 bytes");

enum GPUClusterEntryFlags : u32 {
    GPU_CLUSTER_ENTRY_AT      = 0x1,
    GPU_CLUSTER_ENTRY_PLAIN   = 0x2,
    GPU_CLUSTER_ENTRY_SHADOW_ONLY = 0x8,
    GPU_CLUSTER_ENTRY_TERRAIN = 0x4,
    GPU_CLUSTER_ENTRY_SKINNED = 0x10,
    GPU_CLUSTER_ENTRY_HUD = 0x20,
    GPU_CLUSTER_ENTRY_DYNAMIC = 0x40,
};

enum CullState : u32 {
    CULL_STATE_VISIBLE           = 0,
    CULL_STATE_OCCLUDER          = 1,
    CULL_STATE_CULLED_DISTANCE   = 2,
    CULL_STATE_CULLED_FRUSTUM    = 3,
    CULL_STATE_CULLED_OCCLUSION  = 4,
    CULL_STATE_PARTICLE_VISIBLE  = 5,
    CULL_STATE_PARTICLE_CULLED   = 6,
};

struct CullDebugData {
    Fvector position;
    float radius;
    u32 cullState;
    float objectDepth;
    float hiZDepth;
    u32 objectIndex;
};
static_assert(sizeof(CullDebugData) == 32, "CullDebugData must be 32 bytes for GPU alignment");

struct IndirectDrawArgs {
    u32 indexCountPerInstance;
    u32 instanceCount;
    u32 startIndexLocation;
    s32 baseVertexLocation;
    u32 startInstanceLocation;
};
static_assert(sizeof(IndirectDrawArgs) == 20, "IndirectDrawArgs must be 20 bytes");

struct MeshAllocation {
    u32 vertexOffset;
    u32 indexOffset;
    u32 vertexCount;
    u32 indexCount;
    bool valid;

    MeshAllocation() : vertexOffset(0), indexOffset(0), vertexCount(0), indexCount(0), valid(false) {}
};

struct GPUInstanceData {
    Fmatrix world;
    u32 materialID;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUInstanceData) == 80, "GPUInstanceData must be 80 bytes for GPU alignment");

class GPUCullingManager {
public:
    GPUCullingManager();
    ~GPUCullingManager();

    void Initialize(fg::RenderDevice* device);

    void Shutdown();

    void UploadSceneObjects(fg::RenderContext* ctx, const GeometryCollector* geometry);

    void InvalidateStaticCullingData();
    void InvalidateShadersAndPipelines();

    framegraph::VirtualResourceHandle SetupCullingPass(
        framegraph::FrameGraph& fg,
        const GeometryCollector* geometry,
        framegraph::VirtualResourceHandle prevHiZ,
        const Fmatrix& prevViewProj,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels
    );

    framegraph::VirtualResourceHandle SetupClusterRetestPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels
    );

    u32 GetStaticObjectCount() const { return m_staticObjectCount; }
    u32 GetDynamicObjectCount() const { return m_dynamicObjectCount; }

    bool IsEnabled() const { return m_initialized && m_computeEnabled; }

    void BeginLevelLoad(u32 estimatedVertices = 1000000, u32 estimatedIndices = 3000000);

    MeshAllocation RegisterMesh(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        bindless::SourceVertexFormat format,
        const u16* indices,
        u32 indexCount
    );

    void EndLevelLoad();

    bool AreMegaBuffersReady() const { return m_megaBuffersReady; }
    bool IsMegaDataUploaded() const { return m_megaDataUploaded; }

    nvrhi::IBuffer* GetMegaVertexBuffer() const { return m_megaVertexBuffer.Get(); }
    nvrhi::IBuffer* GetMegaIndexBuffer() const { return m_megaIndexBuffer.Get(); }
    u32 GetTotalVertexCount() const { return m_totalVertexCount; }
    u32 GetTotalIndexCount() const { return m_totalIndexCount; }

    const xr_vector<IndirectDrawArgs>& GetStaticDrawArgsData() const { return m_staticDrawArgsData; }
    const xr_vector<u32>& GetStaticMaterialIDData() const { return m_staticMaterialIDData; }
    const xr_vector<u32>& GetStaticBatchVertexCounts() const { return m_staticBatchVertexCounts; }
    const xr_vector<IndirectDrawArgs>& GetTerrainDrawArgsData() const { return m_terrainDrawArgsData; }
    const xr_vector<u32>& GetTerrainMaterialIDData() const { return m_terrainMaterialIDData; }
    const xr_vector<IndirectDrawArgs>& GetTransparentDrawArgsData() const { return m_transparentDrawArgsData; }
    const xr_vector<u32>& GetTransparentMaterialIDData() const { return m_transparentMaterialIDData; }
    const xr_vector<GPUInstanceData>& GetStaticInstanceData() const { return m_staticInstanceData; }

    void SetRTAccelStructManager(RTAccelStructManager* mgr) { m_rtAccelMgr = mgr; }
    RTAccelStructManager* GetRTAccelStructManager() const { return m_rtAccelMgr; }

    void SetupDebugVisualizationPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        framegraph::VirtualResourceHandle colorTarget,
        framegraph::VirtualResourceHandle depthTarget,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const Fmatrix& prevViewProj,
        const xr_vector<passes::ParticleBatch>* particleBatches = nullptr
    );

    bool IsDebugEnabled() const;

    nvrhi::IBuffer* GetStaticInstanceBuffer() const { return m_staticInstanceBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicInstanceBuffer() const { return m_dynamicInstanceBuffer.Get(); }
    u32 GetStaticResidualCount() const;
    u32 GetTerrainResidualCount() const;

    u32 GetTerrainObjectCount() const { return m_terrainObjectCount; }
    nvrhi::IBuffer* GetTerrainInstanceBuffer() const { return m_terrainInstanceBuffer.Get(); }

    u32 GetTransparentObjectCount() const { return m_transparentObjectCount; }
    u32 GetTransparentResidualCount() const { return m_transparentResidualCount; }
    nvrhi::IBuffer* GetTransparentInstanceBuffer() const { return m_transparentInstanceBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentDrawArgsBuffer() const { return m_transparentDrawArgsBuffer.Get(); }

    struct CullingStats {
        u32 clusterVisible = 0;
        u32 clusterTerrainVisible = 0;
        u32 clusterTrianglesDrawn = 0;
        u32 clusterTerrainTrianglesDrawn = 0;
        u32 clusterCandidates = 0;
        u32 clusterRetestVisible = 0;
    };
    const CullingStats& GetCullingStats() const { return m_cullingStats; }

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);

    void UploadSkinnedObjects(fg::RenderContext* ctx, const GeometryCollector* geometry,
        const xr_vector<GeometryBatch>* hudBatches, decals::OverlayManager* overlayMgr);

    framegraph::VirtualResourceHandle SetupSkinnedUploadPass(
        framegraph::FrameGraph& fg,
        const GeometryCollector* geometry,
        const xr_vector<GeometryBatch>* hudBatches,
        decals::OverlayManager* overlayMgr
    );

    u32 GetSkinnedObjectCount() const { return m_skinnedObjectCount; }
    bool IsSkinnedEnabled() const { return m_initialized && m_skinnedEnabled; }
    nvrhi::IBuffer* GetSkinnedRecordsBuffer() const { return m_skinnedRecordsBuffer.Get(); }
    SkinnedGeometryPools& GetSkinnedPools() { return m_skinnedPools; }

    struct SkinnedDrawRecord {
        Fmatrix world;
        u32 boneOffset;
        u32 splatOffset;
        u32 splatCount;
        u32 prevFirstVertex;
        Fvector4 bounds;
    };
    static_assert(sizeof(SkinnedDrawRecord) == 96, "SkinnedDrawRecord must be 96 bytes");

    struct SkinnedBucket {
        xr_vector<IndirectDrawArgs> args;
        xr_vector<SkinnedDrawRecord> records;
        xr_vector<u32> materialIDs;
        xr_vector<u8> kinds;
        xr_vector<u32> srcVertexBases;
        xr_vector<u32> vertexCounts;
        xr_vector<const void*> visuals;
    };

    struct SkinnedChunk {
        u32 slot;
        u32 srcVertex;
        u32 dstVertex;
        u32 count;
    };
    static constexpr u32 SKINNED_CHUNK_VERTICES = 256;

    nvrhi::IBuffer* GetSkinnedPreVertexBuffer() const { return m_skinnedPreVB[m_skinnedPreVBIndex].Get(); }
    nvrhi::IBuffer* GetSkinnedPrevVertexBuffer() const { return m_skinnedPreVB[m_skinnedPreVBIndex ^ 1u].Get(); }
    nvrhi::IBuffer* GetSkinnedEntryBuffer() const { return m_skinnedEntryBuffer.Get(); }
    u32 GetSkinnedEntryCount() const { return m_skinnedEntryCount; }
    u32 GetSkinnedVisibleEntryCount() const { return m_skinnedVisibleEntryCount; }
    nvrhi::IBuffer* GetSkinnedHudEntryBuffer() const { return m_skinnedHudEntryBuffer.Get(); }
    u32 GetSkinnedHudEntryCount() const { return m_skinnedHudEntryCount; }
    const Fvector4& GetSkinnedHudBounds() const { return m_skinnedHudBounds; }
    static constexpr u32 SKINNED_ENTRY_INDICES = 384;
    static constexpr u32 SKINNED_ENTRY_CAPACITY = 32768;
    static constexpr u32 SKINNED_HUD_ENTRY_CAPACITY = 4096;

    void BeginSkinnedFrame();

    u32 GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton);

    nvrhi::IBuffer* GetGlobalBoneBuffer() const { return m_globalBoneBuffer.Get(); }

    void ProcessStatsReadback();

    static constexpr u32 kDynamicClusterEntryCapacity = 16384;
    u32 GetClusterCullEntryCount() const { return m_clusterSet.entryCount + m_clusterSet.dynamicEntryCount; }
    u32 GetDynamicClusterEntryCount() const { return m_clusterSet.dynamicEntryCount; }
    u32 GetDynamicResidualCount() const { return m_clusterSet.dynamicResidualCount; }
    u32 GetClusterEntryCapacity() const { return m_clusterSet.entryCount + kDynamicClusterEntryCapacity; }
    nvrhi::IBuffer* GetDynamicPrevWorldBuffer() const { return m_dynamicPrevWorldBuffer.Get(); }
    nvrhi::IBuffer* GetClusterEntryBuffer() const { return m_clusterSet.entryBuffer.Get(); }
    nvrhi::IBuffer* GetShadowBvhNodeBuffer() const { return m_clusterSet.bvhNodeBuffer.Get(); }
    nvrhi::IBuffer* GetShadowBvhIndexBuffer() const { return m_clusterSet.bvhIndexBuffer.Get(); }
    u32 GetShadowBvhNodeCount() const { return m_clusterSet.bvhNodeCount; }
    nvrhi::IBuffer* GetClusterVisibleEntryBuffer() const { return m_clusterSet.visibleEntryBuffer.Get(); }
    nvrhi::IBuffer* GetClusterArgsBuffer() const { return m_clusterArgsBuffer.Get(); }
    nvrhi::IBuffer* GetClusterFadeBuffer() const { return m_clusterSet.fadeBuffer.Get(); }
    nvrhi::IBuffer* GetClusterTerrainVisibleEntryBuffer() const { return m_clusterSet.terrainVisibleEntryBuffer.Get(); }
    nvrhi::IBuffer* GetClusterTerrainArgsBuffer() const { return m_clusterTerrainArgsBuffer.Get(); }
    nvrhi::IBuffer* GetClusterTerrainFadeBuffer() const { return m_clusterSet.terrainFadeBuffer.Get(); }
    nvrhi::IBuffer* GetClusterRetestVisibleEntryBuffer() const { return m_clusterSet.visibleEntryBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestFadeBuffer() const { return m_clusterSet.fadeBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestArgsBuffer() const { return m_clusterArgsBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestTerrainVisibleEntryBuffer() const { return m_clusterSet.terrainVisibleEntryBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestTerrainFadeBuffer() const { return m_clusterSet.terrainFadeBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestTerrainArgsBuffer() const { return m_clusterTerrainArgsBuffer2.Get(); }
    u32 GetClusterEntryCount() const { return m_clusterSet.entryCount; }
    u32 GetClusterStaticEntryCount() const { return m_clusterSet.staticEntryCount; }
    u32 GetClusterTerrainEntryCount() const { return m_clusterSet.terrainEntryCount; }
    nvrhi::IBuffer* GetNeutralFadeBuffer() const { return m_neutralFadeBuffer.Get(); }
    nvrhi::ITexture* GetDummyHiZ() const { return m_dummyHiZ.Get(); }

private:
    void CreateBuffers(fg::RenderDevice* device);
    void CreateDebugResources(fg::RenderDevice* device);
    void CreateParticleResources(fg::RenderDevice* device);
    void CreateMegaBuffers();

    void ExtractFrustumPlanes(Fmatrix& viewProj, Fvector4* outPlanes);

    nvrhi::BufferHandle m_staticInstanceBuffer;
    nvrhi::BufferHandle m_dynamicInstanceBuffer;
    nvrhi::BufferHandle m_transparentInstanceBuffer;
    nvrhi::BufferHandle m_transparentDrawArgsBuffer;
    u32 m_staticObjectCount = 0;
    u32 m_dynamicObjectCount = 0;
    u32 m_transparentObjectCount = 0;
    u32 m_transparentResidualCount = 0;
    u32 m_maxTransparentObjects = 0;
    bool m_staticUploaded = false;

    struct ClusterCullBuffers {
        nvrhi::BufferHandle entryBuffer;
        nvrhi::BufferHandle countBuffer;
        nvrhi::BufferHandle visibleEntryBuffer;
        nvrhi::BufferHandle fadeBuffer;
        nvrhi::BufferHandle terrainVisibleEntryBuffer;
        nvrhi::BufferHandle terrainFadeBuffer;
        nvrhi::BufferHandle candidateBuffer;
        nvrhi::BufferHandle visibleEntryBuffer2;
        nvrhi::BufferHandle fadeBuffer2;
        nvrhi::BufferHandle terrainVisibleEntryBuffer2;
        nvrhi::BufferHandle terrainFadeBuffer2;
        nvrhi::BufferHandle bvhNodeBuffer;
        nvrhi::BufferHandle bvhIndexBuffer;
        u32 bvhNodeCount = 0;
        u32 entryCount = 0;
        u32 dynamicEntryCount = 0;
        u32 dynamicResidualCount = 0;
        u32 staticEntryCount = 0;
        u32 terrainEntryCount = 0;
        u32 residualStaticCount = 0;
        u32 residualTerrainCount = 0;
        bool uploaded = false;
    };
    ClusterCullBuffers m_clusterSet;
    nvrhi::BufferHandle m_clusterArgsBuffer;
    nvrhi::BufferHandle m_clusterTerrainArgsBuffer;
    nvrhi::BufferHandle m_clusterArgsBuffer2;
    nvrhi::BufferHandle m_clusterTerrainArgsBuffer2;
    static constexpr u32 kClusterCountWords = 10;
    nvrhi::BufferHandle m_neutralFadeBuffer;
    bool m_neutralFadeZeroed = false;
    xr_vector<GPUClusterEntry> m_clusterEntryData;
    xr_vector<ClusterMeshKey> m_staticBatchKeys;
    xr_vector<GPUClusterEntry> m_dynamicEntryData;
    xr_vector<ClusterMeshKey> m_dynamicBatchKeys;
    xr_vector<std::pair<const void*, const void*>> m_dynamicIdentity;
    xr_vector<Fmatrix> m_dynamicPrevWorldData;
    nvrhi::BufferHandle m_dynamicPrevWorldBuffer;
    xr_map<std::pair<const void*, const void*>, Fmatrix> m_dynamicHistory[2];
    u32 m_dynamicHistoryIndex = 0;
    u32 m_dynamicHistoryFrame = 0;
    void BuildDynamicClusterEntries(nvrhi::ICommandList* cmdList);
    nvrhi::ComputePipelineHandle m_clusterCullPipeline;
    nvrhi::BindingLayoutHandle m_clusterCullLayout;
    nvrhi::ComputePipelineHandle m_clusterArgsPipeline;
    nvrhi::BindingLayoutHandle m_clusterArgsLayout;
    nvrhi::ComputePipelineHandle m_clusterRetestPipeline;
    nvrhi::BindingLayoutHandle m_clusterRetestLayout;
    fg::BufferHandle m_clusterCullParamsCB;
    fg::BufferHandle m_clusterArgsParamsCB;

    void BuildClusterEntries();
    void UploadClusterEntries(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice);
    bool EnsureClusterCullPipeline(nvrhi::IDevice* nvDevice);
    void DispatchClusterCull(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* prevHiZ, const Fmatrix& prevViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);
    void DispatchClusterRetest(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* hiz, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);
    void DispatchClusterArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice, u32 countBase,
        nvrhi::IBuffer* args, nvrhi::IBuffer* terrainArgs);
    void FillClusterCullParams(ClusterCullParamsCB& cb, const Fmatrix& hizViewProj, u32 entryCount,
        bool useHiZ, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);

    nvrhi::TextureHandle m_dummyHiZ;

    bool m_staticDataCached = false;

    u32 m_terrainObjectCount = 0;
    u32 m_maxTerrainObjects = 0;
    nvrhi::BufferHandle m_terrainInstanceBuffer;

    nvrhi::BufferHandle m_debugBuffer;
    fg::BufferHandle m_debugComputeParamsCB;
    fg::BufferHandle m_debugGraphicsParamsCB;

    nvrhi::ComputePipelineHandle m_particleDebugComputePipeline;
    nvrhi::BindingLayoutHandle m_debugComputeLayout;

    nvrhi::GraphicsPipelineHandle m_debugGraphicsPipeline;
    nvrhi::BindingLayoutHandle m_debugGraphicsLayout;
    nvrhi::InputLayoutHandle m_debugInputLayout;

    nvrhi::BufferHandle m_particleBuffer;

    u32 m_maxParticles = 0;
    xr_vector<GPUParticleData> m_particleData;
    fg::RenderDevice* m_device = nullptr;
    RTAccelStructManager* m_rtAccelMgr = nullptr;
    u32 m_maxObjects = 0;
    bool m_initialized = false;
    bool m_computeEnabled = false;

    xr_vector<u32> m_staticObjectFlags;
    xr_vector<IndirectDrawArgs> m_staticDrawArgsData;
    xr_vector<u32> m_staticMaterialIDData;
    xr_vector<GPUInstanceData> m_staticInstanceData;
    xr_vector<u32> m_staticBatchVertexCounts;

    xr_vector<u32> m_dynamicObjectFlags;
    xr_vector<IndirectDrawArgs> m_dynamicDrawArgsData;
    xr_vector<u32> m_dynamicMaterialIDData;
    xr_vector<GPUInstanceData> m_dynamicInstanceData;

    xr_vector<IndirectDrawArgs> m_terrainDrawArgsData;
    xr_vector<u32> m_terrainMaterialIDData;
    xr_vector<GPUInstanceData> m_terrainInstanceData;
    xr_vector<ClusterMeshKey> m_terrainBatchKeys;
    bool m_terrainDataCached = false;

    xr_vector<IndirectDrawArgs> m_transparentDrawArgsData;
    xr_vector<u32> m_transparentMaterialIDData;
    xr_vector<GPUInstanceData> m_transparentInstanceData;

    nvrhi::BufferHandle m_skinnedRecordsBuffer;
    u32 m_skinnedObjectCount = 0;
    u32 m_maxSkinnedObjects = 0;
    bool m_skinnedEnabled = false;

    xr_vector<SkinnedDrawRecord> m_skinnedRecordsData;
    xr_vector<u32> m_skinnedMaterialIDData;
    xr_vector<SkinnedChunk> m_skinnedChunkData;
    xr_vector<GPUClusterEntry> m_skinnedEntryData;
    xr_vector<GPUClusterEntry> m_skinnedShadowEntryData;
    nvrhi::BufferHandle m_skinnedEntryBuffer;
    u32 m_skinnedEntryCapacity = 0;
    u32 m_skinnedEntryCount = 0;
    u32 m_skinnedVisibleEntryCount = 0;
    xr_vector<u32> m_skinnedHudEntryData;
    nvrhi::BufferHandle m_skinnedHudEntryBuffer;
    u32 m_skinnedHudEntryCount = 0;
    Fvector4 m_skinnedHudBounds = {};
    u32 m_skinnedChunkBase[SkinnedGeometryPools::FORMAT_COUNT] = {};
    u32 m_skinnedChunkCount[SkinnedGeometryPools::FORMAT_COUNT] = {};
    nvrhi::BufferHandle m_skinnedChunkBuffer;
    u32 m_skinnedChunkCapacity = 0;
    nvrhi::BufferHandle m_skinnedPreVB[2];
    u32 m_skinnedPreVBCapacity = 0;
    u32 m_skinnedPreVBIndex = 0;
    xr_map<const void*, u32> m_skinnedHistory[2];
    u32 m_skinnedHistoryIndex = 0;
    u32 m_skinnedHistoryFrame = 0;
    bool EnsurePreskinBuffers(nvrhi::IDevice* nvDevice, u32 vertexTotal);
    nvrhi::ComputePipelineHandle m_preskinPipeline;
    nvrhi::BindingLayoutHandle m_preskinLayout;
    bool m_preskinFailed = false;
    bool EnsurePreskinPipeline(nvrhi::IDevice* nvDevice);
    bool DispatchPreskin(nvrhi::ICommandList* cmdList, decals::OverlayManager* overlayMgr, u32 vertexTotal);

    SkinnedGeometryPools m_skinnedPools;
    SkinnedBucket m_skinnedBuckets[SkinnedGeometryPools::FORMAT_COUNT];

    static constexpr u32 MAX_TOTAL_BONES = 16384;
    static constexpr u32 BONE_STRIDE = sizeof(Fmatrix);
    nvrhi::BufferHandle m_globalBoneBuffer;
    u32 m_boneUploadFrameId = 0;
    xr_vector<Fmatrix> m_boneStagingBuffer;
    u32 m_currentBoneOffset = 0;
    bool m_boneBufferInitialized = false;
    bool m_boneBatching = false;
    u32 m_boneBatchStart = 0;

    void CreateSkinnedBuffers(fg::RenderDevice* device);
    void EnsureSkinnedCapacity(u32 count);
    void UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset);
    void FlushBoneBatch(nvrhi::ICommandList* cmdList);

    nvrhi::BufferHandle m_megaVertexBuffer;
    nvrhi::BufferHandle m_megaIndexBuffer;

    xr_vector<bindless::UnifiedVertex> m_megaVertices;
    xr_vector<u32> m_megaIndices;

    u32 m_totalVertexCount = 0;
    u32 m_totalIndexCount = 0;
    u32 m_maxMegaVertices = 0;
    u32 m_maxMegaIndices = 0;
    bool m_megaBuffersReady = false;
    bool m_megaDataUploaded = false;
    bool m_levelLoadInProgress = false;

    ClusterDAG m_clusterDAG;

    struct VBPoolInfo {
        u32 megaBufferVertexOffset;
        u32 vertexCount;
        bindless::SourceVertexFormat format;
    };

    struct IBPoolInfo {
        u32 megaBufferIndexOffset;
        u32 indexCount;
    };

    xr_vector<VBPoolInfo> m_vbPools;
    xr_vector<IBPoolInfo> m_ibPools;
    xr_vector<VBPoolInfo> m_vbPoolsAlt;
    xr_vector<IBPoolInfo> m_ibPoolsAlt;

    static constexpr u32 STATS_READBACK_SLOTS = 6;
    nvrhi::BufferHandle m_statsReadbackBuffers[STATS_READBACK_SLOTS];
    CullingStats m_cullingStats;
    u32 m_statsWriteSlot = 0;
    u32 m_statsScheduled = 0;

public:

    u32 RegisterVBPool(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        const VertexElement* decl,
        bool alternative = false
    );

    u32 RegisterIBPool(
        const u16* indices,
        u32 indexCount,
        bool alternative = false
    );

    MeshAllocation GetMeshAllocation(
        u32 vbID, u32 vBase, u32 vCount,
        u32 ibID, u32 iBase, u32 iCount,
        bool alternative = false
    ) const;

    void BakeClusterDAG(const xr_vector<ClusterBakeRange>& ranges,
        const char* cachePath, u64 geomStamp);
    ClusterDAG& GetClusterDAG() { return m_clusterDAG; }

    static bindless::SourceVertexFormat DetectFormatFromDecl(
        const VertexElement* decl,
        u32 stride
    );
};

}
