// xrRender/GPUCullingManager.h
#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/Bindless/VertexConverter.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/ClusterDAG.h"
#include "Layers/xrRender/ClusterShadowBVH.h"
#include "Layers/xrRender/GeometryResidency.h"

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
    class dxRender_Visual;  // Forward declaration for visual pointer map
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


// Object flags
enum GPUObjectFlags : u32 {
    GPU_OBJECT_NO_RESOLVE = 0x1,
    GPU_OBJECT_SHADOW_ONLY = 0x2,
    GPU_OBJECT_SKINNED_FORWARD = 0x4,
};

struct TransparentDrawRange {
    u32 key;
    u32 first;
    u32 count;
};

enum TransparentKeyBits : u32 {
    TRANSPARENT_KEY_DST_SHIFT = 8,
    TRANSPARENT_KEY_DEPTH_WRITE = 1u << 16,
    TRANSPARENT_KEY_DISTORT = 1u << 17,
    TRANSPARENT_KEY_NO_COLOR = 1u << 18,
    TRANSPARENT_KEY_UNLIT = 1u << 19,
    TRANSPARENT_KEY_WMARK = 1u << 20,
};

// Typed skinned/HUD cluster entry (matches HLSL ClusterEntry). ibFirst and
// firstVertex address the typed skinned streams; page, payloadOffset and
// vertexCount address the compact cluster payload arenas.
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
    u32 page;
    u32 payloadOffset;
    u32 vertexCount;
    u32 geoPad;
};
static_assert(sizeof(GPUClusterEntry) == 112, "GPUClusterEntry must be 112 bytes");

enum GPUClusterEntryFlags : u32 {
    GPU_CLUSTER_ENTRY_AT      = 0x1,
    GPU_CLUSTER_ENTRY_PLAIN   = 0x2,
    GPU_CLUSTER_ENTRY_SHADOW_ONLY = 0x8,
    GPU_CLUSTER_ENTRY_TERRAIN = 0x4,
    GPU_CLUSTER_ENTRY_SKINNED = 0x10,
    GPU_CLUSTER_ENTRY_HUD = 0x20,
    GPU_CLUSTER_ENTRY_DYNAMIC = 0x40,
    GPU_CLUSTER_ENTRY_NO_SHADOW = 0x80,
};

class GPUGeoInstance
{
public:
    Fmatrix world;
    Fmatrix prevWorld;
    u32 assetMember;
    u32 materialID;
    u32 flags;
    float scaleBound;
    u32 firstRef;
    u32 refCount;
    u32 firstPage;
    u32 historyValid;
    float prevScaleBound;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};
static_assert(sizeof(GPUGeoInstance) == 176, "GPUGeoInstance is shader-visible");

class GeometryInstanceKey
{
public:
    u64 renderable = 0;
    u64 visual = 0;
    u32 subset = 0;

    bool operator<(const GeometryInstanceKey& o) const;
};

// ═══════════════════════════════════════════════════════
//  DEBUG VISUALIZATION DATA (matches HLSL CullDebugData struct)
// ═══════════════════════════════════════════════════════

// Culling result states
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
    Fvector position;       // World-space sphere center (12 bytes)
    float radius;           // Sphere radius (4 bytes)
    u32 cullState;          // One of CullState values (4 bytes)
    float objectDepth;      // Normalized depth (4 bytes)
    float hiZDepth;         // Hi-Z depth sampled (4 bytes)
    u32 objectIndex;        // Original object index (4 bytes)
};
static_assert(sizeof(CullDebugData) == 32, "CullDebugData must be 32 bytes for GPU alignment");

// ═══════════════════════════════════════════════════════
//  INDIRECT DRAW ARGUMENTS (matches D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)
// ═══════════════════════════════════════════════════════

struct IndirectDrawArgs {
    u32 indexCountPerInstance;
    u32 instanceCount;          // Set to 1 by culling shader if visible, 0 if culled
    u32 startIndexLocation;
    s32 baseVertexLocation;
    u32 startInstanceLocation;
};
static_assert(sizeof(IndirectDrawArgs) == 20, "IndirectDrawArgs must be 20 bytes");

// ═══════════════════════════════════════════════════════
//  MESH ALLOCATION (for mega-buffer system)
// ═══════════════════════════════════════════════════════

struct MeshAllocation {
    u32 vertexOffset;   // Offset in mega-VB (in vertices)
    u32 indexOffset;    // Offset in mega-IB (in indices)
    u32 vertexCount;
    u32 indexCount;
    bool valid;

    MeshAllocation() : vertexOffset(0), indexOffset(0), vertexCount(0), indexCount(0), valid(false) {}
};

class ForwardVertex
{
public:
    bindless::UnifiedVertex vertex;
    Fvector3 normal;
};

static_assert(sizeof(ForwardVertex) == 60);

constexpr u32 GEOMETRY_SOURCE_FORWARD = 1u << 31;

struct GPUInstanceData {
    Fmatrix world;          // World transform (64 bytes)
    u32 materialID;         // Bindless material ID
    u32 flags;              // Instance flags
    float pad0, pad1;       // Padding to 80 bytes
};
static_assert(sizeof(GPUInstanceData) == 80, "GPUInstanceData must be 80 bytes for GPU alignment");

// ═══════════════════════════════════════════════════════
//  GPU CULLING MANAGER
// ═══════════════════════════════════════════════════════
//
// Manages GPU-driven frustum and occlusion culling for world geometry.
// Uses Hi-Z pyramid from depth prepass for conservative occlusion testing.
//
// USAGE:
// 1. Call Initialize() once at startup
// 2. Call PrepareSceneGeometry() each frame, then the geometry prepare pass uploads
// 3. Call SetupCullingPass() to add culling pass to FrameGraph
// 4. Forward pass reads visible indices from culling output
//
// PERFORMANCE:
// - GPU culling: ~0.3-0.5ms for 100K objects
// - 10-100x faster than CPU culling for large scenes

class GeometrySourceKey
{
public:
    u64 vertexSource = 0;
    u64 indexSource = 0;
    u32 vertexBase = 0;
    u32 vertexCount = 0;
    u32 indexBase = 0;
    u32 indexCount = 0;
    u32 flags = 0;
    u32 vertexStride = 0;
    u32 vertexFormat = 0;

    bool operator<(const GeometrySourceKey& o) const;
};

class GeometryFrameResources
{
public:
    framegraph::VirtualResourceHandle clusterMeta;
    framegraph::VirtualResourceHandle assetMembers;
    framegraph::VirtualResourceHandle assetNodes;
    framegraph::VirtualResourceHandle clusterRefs;
    framegraph::VirtualResourceHandle instances;
    framegraph::VirtualResourceHandle counts;
    framegraph::VirtualResourceHandle clusterPages;
    framegraph::VirtualResourceHandle clusterPayload;
    framegraph::VirtualResourceHandle clusterVertices;
    framegraph::VirtualResourceHandle clusterGroups;
    framegraph::VirtualResourceHandle rtVertices;
    framegraph::VirtualResourceHandle rtIndices;
    framegraph::VirtualResourceHandle boneMatrices;
    framegraph::VirtualResourceHandle groupResidency;
    framegraph::VirtualResourceHandle shadowCuts;
    framegraph::VirtualResourceHandle arenaCopySourceVertices;
    framegraph::VirtualResourceHandle arenaCopySourcePayload;
    framegraph::VirtualResourceHandle megaVertices;
    framegraph::VirtualResourceHandle megaIndices;
    framegraph::VirtualResourceHandle megaCopySourceVertices;
    framegraph::VirtualResourceHandle megaCopySourceIndices;
    framegraph::VirtualResourceHandle forwardArgs;
    framegraph::VirtualResourceHandle forwardInstances;
    framegraph::VirtualResourceHandle forwardDrawIndices;
    framegraph::VirtualResourceHandle shadowBvhNodes;
    framegraph::VirtualResourceHandle shadowBvhIndices;
    framegraph::VirtualResourceHandle visibleEntries;
    framegraph::VirtualResourceHandle fades;
    framegraph::VirtualResourceHandle terrainVisibleEntries;
    framegraph::VirtualResourceHandle terrainFades;
    framegraph::VirtualResourceHandle swEntries;
    framegraph::VirtualResourceHandle candidates;
    framegraph::VirtualResourceHandle retestVisibleEntries;
    framegraph::VirtualResourceHandle retestFades;
    framegraph::VirtualResourceHandle retestTerrainVisibleEntries;
    framegraph::VirtualResourceHandle retestTerrainFades;
    framegraph::VirtualResourceHandle retestSwEntries;
    framegraph::VirtualResourceHandle drawArgs;
    framegraph::VirtualResourceHandle terrainArgs;
    framegraph::VirtualResourceHandle swArgs;
    framegraph::VirtualResourceHandle retestDrawArgs;
    framegraph::VirtualResourceHandle retestTerrainArgs;
    framegraph::VirtualResourceHandle retestSwArgs;
    framegraph::VirtualResourceHandle retestDispatchArgs;
    framegraph::VirtualResourceHandle queueArgs;
    framegraph::VirtualResourceHandle nodeQueueA;
    framegraph::VirtualResourceHandle nodeQueueB;
    framegraph::VirtualResourceHandle deferredNodes;
    framegraph::VirtualResourceHandle deferredInstances;
    framegraph::VirtualResourceHandle leafQueue;
    framegraph::VirtualResourceHandle skinnedEntries;
    framegraph::VirtualResourceHandle deformedVertices;
    framegraph::VirtualResourceHandle previousDeformedVertices;
    framegraph::VirtualResourceHandle skinnedHudEntries;
    framegraph::VirtualResourceHandle neutralFades;
    framegraph::VirtualResourceHandle paintSplats;
    framegraph::VirtualResourceHandle skinnedRecords;
    framegraph::VirtualResourceHandle skinnedIndices;
    framegraph::VirtualResourceHandle skinnedForwardArgs;
    framegraph::VirtualResourceHandle skinnedForwardInstances;
    framegraph::VirtualResourceHandle materials;
    framegraph::VirtualResourceHandle terrainMaterials;
    framegraph::VirtualResourceHandle variants;
    framegraph::VirtualResourceHandle variantTextures;
    bool valid = false;
};

class GeometryFrameBuffers
{
public:
    nvrhi::IBuffer* clusterMeta = nullptr;
    nvrhi::IBuffer* clusterRefs = nullptr;
    nvrhi::IBuffer* instances = nullptr;
    nvrhi::IBuffer* clusterPages = nullptr;
    nvrhi::IBuffer* clusterPayload = nullptr;
    nvrhi::IBuffer* clusterVertices = nullptr;
    nvrhi::IBuffer* clusterGroups = nullptr;
    nvrhi::IBuffer* groupResidency = nullptr;
    nvrhi::IBuffer* shadowBvhNodes = nullptr;
    nvrhi::IBuffer* shadowBvhIndices = nullptr;
    nvrhi::IBuffer* skinnedEntries = nullptr;
    nvrhi::IBuffer* deformedVertices = nullptr;
    nvrhi::IBuffer* skinnedIndices = nullptr;
    nvrhi::IBuffer* skinnedHudEntries = nullptr;
    nvrhi::IBuffer* neutralFades = nullptr;
    nvrhi::IBuffer* materials = nullptr;
};

class GeometryMemoryStats
{
public:
    u64 sharedMetadataBytes = 0;
    u64 instanceTableBytes = 0;
    u64 payloadBytes = 0;
    u64 vertexBytes = 0;
    u64 retainedSourceBytes = 0;
    u64 forwardDrawBytes = 0;
    u64 retiringSourceBytes = 0;
    u64 sourceStagingBytes = 0;
    u64 hostSourceBytes = 0;
    u32 forwardUploadLeases = 0;
    u64 residencyArenaBytes = 0;
    u64 residencyUsedBytes = 0;
    u64 residencyPinnedBytes = 0;
    u64 residencyStagingBytes = 0;
};

GeometryFrameBuffers ResolveGeometryResources(const framegraph::FrameGraph& fg, const GeometryFrameResources& res);

class GPUCullingManager {
public:
    GPUCullingManager();
    ~GPUCullingManager();

    // Initialize GPU resources (call once at startup)
    void Initialize(fg::RenderDevice* device);

    // Shutdown and release resources
    void Shutdown();

    void PrepareSceneGeometry(const GeometryCollector* geometry);
    void UploadSceneObjects(fg::RenderContext* ctx);

    void InvalidateStaticCullingData();
    void InvalidateShadersAndPipelines();

    GeometryFrameResources ImportGeometryResources(framegraph::FrameGraph& fg);

    framegraph::VirtualResourceHandle SetupGeometryPreparePass(
        framegraph::FrameGraph& fg,
        const GeometryCollector* geometry,
        GeometryFrameResources& resources
    );

    void SetupCullingPass(
        framegraph::FrameGraph& fg,
        const GeometryCollector* geometry,
        framegraph::VirtualResourceHandle prevHiZ,
        const Fmatrix& prevViewProj,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        GeometryFrameResources& resources
    );

    // Re-tests the entries phase one held back against this frame's Hi-Z and
    // returns the imported retest args buffer as the token for the second raster.
    framegraph::VirtualResourceHandle SetupClusterRetestPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        GeometryFrameResources& resources
    );

    u32 GetStaticObjectCount() const { return m_staticObjectCount; }
    u32 GetDynamicObjectCount() const { return m_dynamicObjectCount; }

    // Check if culling is enabled and ready
    bool IsEnabled() const { return m_initialized && m_computeEnabled; }


    // Begin level load - prepare to receive mesh data
    // estimatedVertices/Indices help pre-allocate, but will grow if needed
    void BeginLevelLoad(u32 estimatedVertices = 1000000, u32 estimatedIndices = 3000000);

    // End level load - upload all data to GPU
    void EndLevelLoad();
    void UnloadLevel();
    void RetainForwardGeometry(const MeshAllocation& allocation);

    bool AreMegaBuffersReady() const { return m_megaBuffersReady; }
    bool IsMegaDataUploaded() const { return m_megaDataUploaded; }

    nvrhi::IBuffer* GetMegaVertexBuffer() const { return m_megaVertexBuffer.Get(); }
    nvrhi::IBuffer* GetRTVertexBuffer() const { return m_rtVertexBuffer.Get(); }
    nvrhi::IBuffer* GetRTIndexBuffer() const { return m_rtIndexBuffer.Get(); }
    u32 GetRTVertexCount() const { return m_rtVertexCount; }
    u32 GetRTIndexCount() const { return m_rtIndexCount; }
    static constexpr u32 RT_VERTEX_STRIDE = 32u;
    u32 GetPreparedSkeletonOffset(CKinematics* skeleton) const;
    const Fmatrix* GetPreparedSkeletonMatrices(CKinematics* skeleton, u32& count) const;
    nvrhi::IBuffer* GetMegaIndexBuffer() const { return m_megaIndexBuffer.Get(); }
    nvrhi::IBuffer* GetClusterPageBuffer() const { return m_residency.GetPageTableBuffer(); }
    nvrhi::IBuffer* GetClusterPayloadBuffer() const { return m_residency.GetPayloadArenaBuffer(); }
    nvrhi::IBuffer* GetClusterVertexBuffer() const { return m_residency.GetVertexArenaBuffer(); }
    nvrhi::IBuffer* GetClusterGroupBuffer() const { return m_residency.GetClusterGroupBuffer(); }
    nvrhi::IBuffer* GetGroupResidencyBuffer() const { return m_residency.GetGroupBitsBuffer(); }
    GeometryResidencyManager& GetResidency() { return m_residency; }
    const GeometryResidencyManager& GetResidency() const { return m_residency; }
    void BeginGeometryResidencyFrame();
    void EndGeometryResidencyFrame();
    void AddShadowGeometryDemand(const GeometryDemandView& view);
    u32 GetGeometryCutRevision() const { return m_residency.CutRevision(); }
    GeometryMemoryStats GetGeometryMemoryStats() const;
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

    // ───────────────────────────────────────────────────────
    //  DEBUG VISUALIZATION
    // ───────────────────────────────────────────────────────

    // Setup debug visualization pass (renders colored bounding spheres)
    // Call AFTER main rendering, renders as overlay
    // Only executes if r_debug_gpu_culling is enabled
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

    u32 GetStaticResidualCount() const;
    u32 GetTerrainResidualCount() const;

    u32 GetTerrainObjectCount() const { return m_terrainObjectCount; }

    u32 GetTransparentObjectCount() const { return m_transparentObjectCount; }
    u32 GetTransparentResidualCount() const { return m_transparentResidualCount; }
    const xr_vector<TransparentDrawRange>& GetTransparentRanges() const { return m_transparentRanges; }
    u32 GetSkinnedForwardCount() const { return m_skinnedForwardCount; }
    nvrhi::IBuffer* GetSkinnedForwardArgsBuffer() const { return m_skinnedForwardArgsBuffer.Get(); }
    nvrhi::IBuffer* GetSkinnedForwardInstanceBuffer() const { return m_skinnedForwardInstanceBuffer.Get(); }
    const xr_vector<TransparentDrawRange>& GetSkinnedForwardRanges() const { return m_skinnedForwardRanges; }
    u32 GetSkinnedEntryCapacity() const { return m_skinnedEntryCapacity; }
    u32 GetSkinnedHudEntryCapacity() const { return m_skinnedHudEntryCapacity; }
    nvrhi::IBuffer* GetSkinnedChunkBuffer() const { return m_skinnedChunkBuffer.Get(); }

    // ───────────────────────────────────────────────────────
    //  CULLING STATS READBACK (for profiling overlay)
    // ───────────────────────────────────────────────────────
    // Returns previous frame's visible counts (1-frame latency to avoid GPU stall)
    struct CullingStats {
        u32 clusterVisible = 0;
        u32 clusterTerrainVisible = 0;
        u32 clusterTrianglesDrawn = 0;
        u32 clusterTerrainTrianglesDrawn = 0;
        u32 clusterCandidates = 0;
        u32 clusterRetestVisible = 0;
        u32 clusterInstanceVisits = 0;
        u32 clusterNodeVisits = 0;
        u32 clusterLeafVisits = 0;
        u32 clusterDeferredInstances = 0;
        u32 clusterDeferredNodes = 0;
        u32 clusterOverflow = 0;
    };
    const CullingStats& GetCullingStats() const { return m_cullingStats; }

    // Schedule readback of visible counts (call after culling pass)
    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);

    // ───────────────────────────────────────────────────────
    //  SKINNED MESH UPLOAD
    // ───────────────────────────────────────────────────────
    // Pooled skinned batches are concatenated per vertex format into global
    // draw-args, record and material-ID arrays; each draw's startInstanceLocation
    // is its global slot so DRAWINDEX indexes the records directly.

    void PrepareSkinnedGeometry(const GeometryCollector* geometry,
        const xr_vector<GeometryBatch>* hudBatches, decals::OverlayManager* overlayMgr);

    void UploadSkinnedObjects(fg::RenderContext* ctx, decals::OverlayManager* overlayMgr);

    // Returns the imported draw-args buffer handle (invalid if disabled) so the
    // consumers can declare a read dependency on the upload.
    framegraph::VirtualResourceHandle SetupSkinnedUploadPass(
        framegraph::FrameGraph& fg,
        decals::OverlayManager* overlayMgr,
        GeometryFrameResources& resources
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
        struct Batch {
            const GeometryBatch* source;
            u32 boneOffset;
            u32 splatOffset;
            u32 splatCount;
        };
        xr_vector<Batch> kinds[4];
    };

    struct TransparentDrawScratch {
        xr_vector<u32> order;
        xr_vector<IndirectDrawArgs> args;
        xr_vector<GPUInstanceData> instances;
        xr_vector<u32> keys;
        xr_vector<u32> materialIDs;
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
    static constexpr u32 kSkinnedEntryLimit = (1u << 25) - 1u;

    // ───────────────────────────────────────────────────────
    //  SKELETON BONE BUFFER (for GPU-driven skinned rendering)
    // ───────────────────────────────────────────────────────
    // Global bone buffer pool - all skeleton bones are uploaded here each frame.
    // Each skeleton gets a contiguous range: g_BoneMatrices[offset + boneIndex]

    // Call at frame start to reset bone buffer allocations
    void BeginSkinnedFrame();

    // Get bone offset for a skeleton, uploading if not already done this frame
    // Returns offset (in bone count) into global buffer
    u32 GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton);

    // Get the global bone buffer for shader binding
    nvrhi::IBuffer* GetGlobalBoneBuffer() const { return m_globalBoneBuffer.Get(); }

    // Process readback results from previous frame (call at frame start)
    void ProcessStatsReadback();

    u32 GetClusterCullRefCount() const { return m_clusterSet.refCount + m_clusterSet.dynamicRefCount; }
    u32 GetDynamicClusterRefCount() const { return m_clusterSet.dynamicRefCount; }
    u32 GetDynamicResidualCount() const { return m_clusterSet.dynamicResidualCount; }
    u32 GetClusterRefCapacity() const { return m_clusterSet.refCapacity; }
    nvrhi::IBuffer* GetClusterRefBuffer() const { return m_clusterSet.refBuffer.Get(); }
    nvrhi::IBuffer* GetClusterMetaBuffer() const { return m_clusterSet.metaBuffer.Get(); }
    nvrhi::IBuffer* GetGeoInstanceBuffer() const { return m_clusterSet.instanceBuffer.Get(); }
    nvrhi::IBuffer* GetAssetMemberBuffer() const { return m_clusterSet.memberBuffer.Get(); }
    nvrhi::IBuffer* GetAssetNodeBuffer() const { return m_clusterSet.assetNodeBuffer.Get(); }
    nvrhi::IBuffer* GetShadowBvhNodeBuffer() const { return m_clusterSet.bvhNodeBuffer.Get(); }
    nvrhi::IBuffer* GetShadowBvhIndexBuffer() const { return m_clusterSet.bvhIndexBuffer.Get(); }
    u32 GetShadowBvhNodeCount() const { return m_clusterSet.bvhNodeCount; }
    bool GetShadowPairCapacity(float pageWidth, float errorThreshold, u32 pagesAxis, u32* capacity);
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
    nvrhi::IBuffer* GetClusterSwEntryBuffer() const { return m_clusterSet.swEntryBuffer.Get(); }
    nvrhi::IBuffer* GetClusterSwArgsBuffer() const { return m_clusterSwArgsBuffer.Get(); }
    nvrhi::IBuffer* GetClusterRetestSwEntryBuffer() const { return m_clusterSet.swEntryBuffer2.Get(); }
    nvrhi::IBuffer* GetClusterRetestSwArgsBuffer() const { return m_clusterSwArgsBuffer2.Get(); }
    void SetClusterSwCull(float swCull, float swNearZ) { m_clusterSwCull = swCull; m_clusterSwNearZ = swNearZ; }
    u32 GetClusterRefCount() const { return m_clusterSet.refCount; }
    u32 GetClusterStaticRefCount() const { return m_clusterSet.staticRefCount; }
    u32 GetClusterTerrainRefCount() const { return m_clusterSet.terrainRefCount; }
    u32 GetGeoInstanceCount() const { return m_clusterSet.instanceCount + m_clusterSet.dynamicInstanceCount; }
    nvrhi::IBuffer* GetNeutralFadeBuffer() const { return m_neutralFadeBuffer.Get(); }
    nvrhi::ITexture* GetDummyHiZ() const { return m_dummyHiZ.Get(); }

private:
    void CreateBuffers(fg::RenderDevice* device);
    void CreateDebugResources(fg::RenderDevice* device);
    void CreateParticleResources(fg::RenderDevice* device);
    void CreateMegaBuffers();  // Called by EndLevelLoad

    // Extract frustum planes from view-projection matrix
    void ExtractFrustumPlanes(Fmatrix& viewProj, Fvector4* outPlanes);

    nvrhi::BufferHandle m_transparentInstanceBuffer;
    nvrhi::BufferHandle m_transparentDrawArgsBuffer;
    u32 m_staticObjectCount = 0;
    u32 m_dynamicObjectCount = 0;
    u32 m_transparentObjectCount = 0;
    u32 m_transparentResidualCount = 0;
    u32 m_maxTransparentObjects = 0;
    bool m_staticUploaded = false;

    // ───────────────────────────────────────────────────────
    //  CLUSTER LOD CULLING SET
    // ───────────────────────────────────────────────────────
    struct ClusterCullBuffers {
        nvrhi::BufferHandle metaBuffer;
        nvrhi::BufferHandle memberBuffer;
        nvrhi::BufferHandle assetNodeBuffer;
        nvrhi::BufferHandle refBuffer;
        nvrhi::BufferHandle instanceBuffer;
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
        nvrhi::BufferHandle swEntryBuffer;
        nvrhi::BufferHandle swEntryBuffer2;
        nvrhi::BufferHandle nodeQueue[2];
        nvrhi::BufferHandle deferredNodeBuffer;
        nvrhi::BufferHandle deferredInstanceBuffer;
        nvrhi::BufferHandle nodeSinkBuffer;
        nvrhi::BufferHandle u32SinkBuffer;
        nvrhi::BufferHandle leafQueueBuffer;
        nvrhi::BufferHandle bvhNodeBuffer;
        nvrhi::BufferHandle bvhIndexBuffer;
        u32 bvhNodeCount = 0;
        xr_vector<ClusterShadowCaster> shadowCasters;
        float shadowCapacityWidth = 0.0f;
        float shadowCapacityError = 0.0f;
        u32 shadowCapacityAxis = 0;
        u32 shadowCapacity[3] = {};
        bool shadowCapacityValid = false;
        u32 refCount = 0;
        u32 staticRefCount = 0;
        u32 terrainRefCount = 0;
        u32 dynamicRefCount = 0;
        u32 instanceCount = 0;
        u32 dynamicInstanceCount = 0;
        u32 refCapacity = 0;
        u32 instanceCapacity = 0;
        u32 nodeRefCapacity = 0;
        u32 staticNodeRefs = 0;
        u32 dynamicNodeRefs = 0;
        u32 traversalDepth = 0;
        u32 dynamicResidualCount = 0;
        u32 residualStaticCount = 0;
        u32 residualTerrainCount = 0;
        bool uploaded = false;
    };
    ClusterCullBuffers m_clusterSet;
    ClusterBvh m_shadowBvh;
    nvrhi::BufferHandle m_clusterArgsBuffer;
    nvrhi::BufferHandle m_clusterTerrainArgsBuffer;
    nvrhi::BufferHandle m_clusterArgsBuffer2;
    nvrhi::BufferHandle m_clusterTerrainArgsBuffer2;
    nvrhi::BufferHandle m_clusterSwArgsBuffer;
    nvrhi::BufferHandle m_clusterSwArgsBuffer2;
    nvrhi::BufferHandle m_clusterRetestDispatchArgs;
    nvrhi::BufferHandle m_clusterQueueArgsBuffer;
    float m_clusterSwCull = 0.0f;
    float m_clusterSwNearZ = 0.0f;
    static constexpr u32 kClusterCountWords = 20;
    static constexpr u32 kClusterQueueArgsSlots = 4;
    static constexpr u32 kClusterTraversalGroup = 64;
    static constexpr u32 kMaxClusterTraversalDepth = 64;
    nvrhi::BufferHandle m_neutralFadeBuffer;
    bool m_neutralFadeZeroed = false;
    xr_vector<GPUGeoInstance> m_geoInstanceData;
    xr_vector<u32> m_clusterRefData;
    xr_vector<ClusterMeshKey> m_staticBatchKeys;
    xr_vector<GPUGeoInstance> m_dynamicGeoInstanceData;
    xr_vector<u32> m_dynamicRefData;
    xr_vector<ClusterMeshKey> m_dynamicBatchKeys;
    xr_vector<GeometryInstanceKey> m_dynamicIdentity;
    class DynamicHistoryEntry
    {
    public:
        Fmatrix world;
        u32 assetMember;
    };
    xr_map<GeometryInstanceKey, DynamicHistoryEntry> m_dynamicHistory[2];
    u32 m_dynamicHistoryIndex = 0;
    u32 m_dynamicHistoryFrame = 0;
    u32 m_geometryHistoryFrame = 0;
    bool m_staticHistoryValid = false;
    void BuildDynamicGeometryInstances();
    nvrhi::ComputePipelineHandle m_clusterInstancePipeline;
    nvrhi::BindingLayoutHandle m_clusterInstanceLayout;
    nvrhi::ComputePipelineHandle m_clusterNodePipeline;
    nvrhi::BindingLayoutHandle m_clusterNodeLayout;
    nvrhi::ComputePipelineHandle m_clusterLeafPipeline;
    nvrhi::BindingLayoutHandle m_clusterLeafLayout;
    nvrhi::ComputePipelineHandle m_clusterQueueArgsPipeline;
    nvrhi::BindingLayoutHandle m_clusterQueueArgsLayout;
    nvrhi::ComputePipelineHandle m_clusterArgsPipeline;
    nvrhi::BindingLayoutHandle m_clusterArgsLayout;
    fg::BufferHandle m_clusterCullParamsCB;
    fg::BufferHandle m_clusterArgsParamsCB;
    fg::BufferHandle m_clusterQueueParamsCB;

    void BuildStaticGeometryInstances();
    bool EnsureGeometryTableBuffers(nvrhi::IDevice* nvDevice);
    void UploadGeometryTables(nvrhi::ICommandList* cmdList);
    bool EnsureClusterStreamBuffers(nvrhi::IDevice* nvDevice);
    bool EnsureClusterCullPipeline(nvrhi::IDevice* nvDevice);
    void DispatchClusterCull(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* prevHiZ, const Fmatrix& prevViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);
    void DispatchClusterRetest(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* hiz, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);
    void DispatchClusterArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice, u32 countBase, u32 swCountOffset,
        nvrhi::IBuffer* args, nvrhi::IBuffer* terrainArgs, nvrhi::IBuffer* swArgs, nvrhi::IBuffer* retestDispatchArgs);
    void DispatchQueueArgs(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        u32 srcCountOffset, u32 resetCountOffset, u32 slot, u32 groupSize);
    void DispatchClusterTraversal(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* hiz, const Fmatrix& hizViewProj, u32 hizWidth, u32 hizHeight, u32 hizMipLevels,
        bool late);
    void AccumulateGeometryDemand();
    void DispatchClusterLeaves(nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice,
        nvrhi::ITexture* hiz, nvrhi::IBuffer* queue, u32 argsSlot, bool late, bool allowDefer);
    void FillClusterCullParams(ClusterCullParamsCB& cb, const Fmatrix& hizViewProj,
        bool useHiZ, u32 hizWidth, u32 hizHeight, u32 hizMipLevels);

    nvrhi::TextureHandle m_dummyHiZ;

    bool m_staticDataCached = false;

    u32 m_terrainObjectCount = 0;
    u32 m_maxTerrainObjects = 0;

    // ───────────────────────────────────────────────────────
    //  DEBUG VISUALIZATION RESOURCES
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_debugBuffer;                // CullDebugData for all objects
    fg::BufferHandle m_debugComputeParamsCB;       // Constant buffer for compute shader
    fg::BufferHandle m_debugGraphicsParamsCB;      // Constant buffer for graphics shaders

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
    xr_vector<u32> m_dynamicMaterialIDData;             // Material IDs per batch (for bindless)
    xr_vector<GPUInstanceData> m_dynamicInstanceData;

    // Terrain-specific CPU data (separate from regular geometry)
    xr_vector<IndirectDrawArgs> m_terrainDrawArgsData;
    xr_vector<u32> m_terrainMaterialIDData;
    xr_vector<GPUInstanceData> m_terrainInstanceData;
    xr_vector<ClusterMeshKey> m_terrainBatchKeys;
    bool m_terrainDataCached = false;

    // Transparent-specific CPU data
    xr_vector<IndirectDrawArgs> m_transparentDrawArgsData;
    xr_vector<IndirectDrawArgs> m_transparentNativeArgs;
    xr_vector<u32> m_transparentMaterialIDData;
    xr_vector<GPUInstanceData> m_transparentInstanceData;
    xr_vector<u32> m_transparentKeys;
    xr_vector<TransparentDrawRange> m_transparentRanges;
    TransparentDrawScratch m_transparentDrawScratch;
    nvrhi::BufferHandle m_forwardDrawIndexBuffer;
    u32 m_forwardDrawIndexCapacity = 0;
    xr_vector<u32> m_forwardDrawIndices;
    void EnsureForwardBuffers(nvrhi::IDevice* device);

    // ───────────────────────────────────────────────────────
    //  SKINNED MESH UPLOAD
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_skinnedRecordsBuffer;
    u32 m_skinnedObjectCount = 0;
    u32 m_maxSkinnedObjects = 0;
    bool m_skinnedEnabled = false;

    xr_vector<SkinnedDrawRecord> m_skinnedRecordsData;
    xr_vector<SkinnedChunk> m_skinnedChunkData;
    xr_vector<GPUClusterEntry> m_skinnedEntryData;
    xr_vector<GPUClusterEntry> m_skinnedShadowEntryData;
    nvrhi::BufferHandle m_skinnedEntryBuffer;
    u32 m_skinnedEntryCapacity = 0;
    u32 m_skinnedEntryCount = 0;
    u32 m_skinnedVisibleEntryCount = 0;
    u32 m_skinnedForwardCapacity = 0;
    xr_vector<IndirectDrawArgs> m_skinnedForwardArgsData;
    xr_vector<GPUInstanceData> m_skinnedForwardInstanceData;
    xr_vector<u32> m_skinnedForwardKeys;
    xr_vector<float> m_skinnedForwardSort;
    xr_vector<TransparentDrawRange> m_skinnedForwardRanges;
    nvrhi::BufferHandle m_skinnedForwardArgsBuffer;
    nvrhi::BufferHandle m_skinnedForwardInstanceBuffer;
    u32 m_skinnedForwardCount = 0;
    void EnsureSkinnedForwardBuffers(nvrhi::IDevice* nvDevice);
    xr_vector<u32> m_skinnedHudEntryData;
    nvrhi::BufferHandle m_skinnedHudEntryBuffer;
    u32 m_skinnedHudEntryCapacity = 0;
    u32 m_skinnedHudEntryCount = 0;
    bool m_skinnedPreVBRecreated = false;
    bool m_skinnedPrepared = false;
    u32 m_skinnedPreparedVertexCount = 0;
    class SkinnedUploadPassData
    {
    public:
        framegraph::VirtualResourceHandle entries;
        GPUCullingManager* manager;
        decals::OverlayManager* overlayMgr;
    };
    Fvector4 m_skinnedHudBounds = {};
    u32 m_skinnedChunkBase[SkinnedGeometryPools::FORMAT_COUNT] = {};
    u32 m_skinnedChunkCount[SkinnedGeometryPools::FORMAT_COUNT] = {};
    nvrhi::BufferHandle m_skinnedChunkBuffer;
    u32 m_skinnedChunkCapacity = 0;
    nvrhi::BufferHandle m_skinnedPreVB[2];
    u32 m_skinnedPreVBCapacity = 0;
    u32 m_skinnedPreVBIndex = 0;
    class SkinnedHistoryEntry
    {
    public:
        u64 visual;
        u64 renderable;
        u32 firstVertex;
        u32 slot;

        bool SameIdentity(const SkinnedHistoryEntry& o) const;
        bool operator<(const SkinnedHistoryEntry& o) const;
    };
    xr_vector<SkinnedHistoryEntry> m_skinnedHistory[2];
    u32 m_skinnedHistoryIndex = 0;
    u32 m_skinnedHistoryFrame = 0;
    bool EnsurePreskinBuffers(nvrhi::IDevice* nvDevice, u32 vertexTotal);
    void EnsureSkinnedChunkBuffer(nvrhi::IDevice* nvDevice, u32 chunkTotal);
    void EnsureSkinnedEntryBuffers(nvrhi::IDevice* nvDevice, u32 entryTotal, u32 hudEntryTotal);
    nvrhi::ComputePipelineHandle m_preskinPipeline;
    nvrhi::BindingLayoutHandle m_preskinLayout;
    struct PreskinBindingSet {
        nvrhi::IBuffer* resources[8] = {};
        nvrhi::BindingSetHandle handle;
    };
    PreskinBindingSet m_preskinBindingSets[SkinnedGeometryPools::FORMAT_COUNT][2];
    bool m_preskinFailed = false;
    bool EnsurePreskinPipeline(nvrhi::IDevice* nvDevice);
    bool DispatchPreskin(nvrhi::ICommandList* cmdList, decals::OverlayManager* overlayMgr, u32 vertexTotal);

    SkinnedGeometryPools m_skinnedPools;
    SkinnedBucket m_skinnedBuckets[SkinnedGeometryPools::FORMAT_COUNT];

    // Global bone buffer for GPU-driven skinned rendering
    // All skeleton bones are uploaded here each frame, indexed by per-instance offset
    static constexpr u32 MAX_TOTAL_BONES = 16384;  // ~200 skeletons * 78 bones
    static constexpr u32 BONE_STRIDE = sizeof(Fmatrix);  // 64 bytes
    nvrhi::BufferHandle m_globalBoneBuffer;
    u32 m_boneUploadFrameId = 0;
    xr_vector<Fmatrix> m_boneStagingBuffer;
    u32 m_currentBoneOffset = 0;
    bool m_boneBufferInitialized = false;
    u32 m_boneBatchStart = 0;

    void CreateSkinnedBuffers(fg::RenderDevice* device);
    void EnsureSkinnedCapacity(u32 count);
    static CKinematics* GetBatchSkeleton(const GeometryBatch& batch);
    u32 PrepareSkeletonPalette(CKinematics* skeleton);
    void FlushBoneBatch(nvrhi::ICommandList* cmdList);

    // ───────────────────────────────────────────────────────
    //  MEGA-BUFFER SYSTEM
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_megaVertexBuffer;     // Unified vertex buffer (UnifiedVertex format)
    nvrhi::BufferHandle m_megaIndexBuffer;      // Unified index buffer (32-bit indices)

    // CPU-side staging data (during level load)
    xr_vector<bindless::UnifiedVertex> m_megaVertices;
    xr_vector<u32> m_megaIndices;
    xr_vector<ForwardVertex> m_forwardVertices;
    xr_vector<u32> m_forwardIndices;
    xr_map<ClusterMeshKey, MeshAllocation> m_forwardAllocations;
    u32 m_forwardVertexBase = 0;
    u32 m_forwardIndexBase = 0;
    void AppendForwardGeometry(const MeshAllocation& allocation, const ClusterSourceView& source);
    class ForwardUpload
    {
    public:
        u64 lease = 0;
        nvrhi::BufferHandle vertices;
        nvrhi::BufferHandle indices;
        nvrhi::BufferHandle drawIndices;
        nvrhi::BufferHandle copyVertices;
        nvrhi::BufferHandle copyIndices;
        xr_vector<ForwardVertex> vertexStaging;
        xr_vector<u32> indexStaging;
        xr_vector<u32> drawIndexStaging;
    };
    xr_vector<ForwardUpload> m_forwardUploads;
    void RetireForwardUploads(bool discard);
    // Exact float normals for sources that author a normal without a tangent
    // frame, kept in lockstep with m_megaVertices while cooking needs them.
    xr_vector<Fvector3> m_megaSourceNormals;
    bool m_megaSourceNormalsActive = false;

    // Tracking
    u32 m_totalVertexCount = 0;
    u32 m_totalIndexCount = 0;
    u32 m_maxMegaVertices = 0;
    u32 m_maxMegaIndices = 0;
    u32 m_megaVertexCapacity = 0;
    u32 m_megaIndexCapacity = 0;
    xr_vector<bindless::UnifiedVertex> m_runtimeVertices;
    xr_vector<Fvector3> m_runtimeSourceNormals;
    xr_vector<u32> m_runtimeIndices;
    nvrhi::BufferHandle m_megaCopySourceVB;
    nvrhi::BufferHandle m_megaCopySourceIB;
    u32 m_megaCopyVertexCount = 0;
    u32 m_megaCopyIndexCount = 0;
    bool m_geometryTablesDirty = false;
    xr_map<GeometrySourceKey, MeshAllocation> m_runtimeSourceLookup;
    bool EnsureMegaCapacity(u32 vertexTotal, u32 indexTotal);
    void EnsureSourceNormalStorage(u32 vertexTotal);
    Fvector3* ActivateSourceNormals(u32 firstVertex, u32 vertexCount);
    bool m_megaBuffersReady = false;
    bool m_megaDataUploaded = false;
    bool m_levelLoadInProgress = false;

    ClusterDAG m_clusterDAG;

    // ───────────────────────────────────────────────────────
    //  VB POOL REGISTRATION (for level geometry)
    // ───────────────────────────────────────────────────────
    // During LoadBuffers(), we register entire VB pools
    // Each pool's vertices are converted and stored in mega-buffer
    // Meshes later lookup their allocation by (vbID, vBase)

    struct VBPoolInfo {
        u32 megaBufferVertexOffset;  // Start offset in mega-VB
        u32 vertexCount;              // Total vertices in pool
        bindless::SourceVertexLayout layout;
    };

    struct IBPoolInfo {
        u32 megaBufferIndexOffset;   // Start offset in mega-IB
        u32 indexCount;               // Total indices in pool
    };

    xr_vector<VBPoolInfo> m_vbPools;       // VB ID -> pool info
    xr_vector<IBPoolInfo> m_ibPools;       // IB ID -> pool info
    xr_vector<VBPoolInfo> m_vbPoolsAlt;    // Alternative (fast) geometry
    xr_vector<IBPoolInfo> m_ibPoolsAlt;

    // ───────────────────────────────────────────────────────
    //  STATS READBACK (for profiling)
    // ───────────────────────────────────────────────────────
    static constexpr u32 STATS_READBACK_SLOTS = 6;
    nvrhi::BufferHandle m_statsReadbackBuffers[STATS_READBACK_SLOTS];
    CullingStats m_cullingStats;                 // Previous frame's stats
    u32 m_statsWriteSlot = 0;
    u32 m_statsScheduled = 0;

public:
    // ───────────────────────────────────────────────────────
    //  VB POOL REGISTRATION API
    // ───────────────────────────────────────────────────────

    // Register a vertex buffer pool during level load (called from LoadBuffers)
    // Returns pool index for later mesh allocation lookup
    u32 RegisterVBPool(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        const VertexElement* decl,
        bool alternative = false
    );

    // Register an index buffer pool during level load
    u32 RegisterIBPool(
        const u16* indices,
        u32 indexCount,
        bool alternative = false
    );

    MeshAllocation RegisterRuntimeGeometry(
        const GeometrySourceKey& source,
        const void* vertices,
        u32 vertexStride,
        const VertexElement* decl,
        const u16* indices
    );

    const MeshAllocation* FindRuntimeGeometry(const GeometrySourceKey& source) const;

    MeshAllocation GetMeshAllocation(
        u32 vbID, u32 vBase, u32 vCount,
        u32 ibID, u32 iBase, u32 iCount,
        bool alternative = false
    ) const;

    void BakeClusterDAG(const xr_vector<ClusterBakeRange>& ranges,
        const char* cachePath, u64 geomStamp);
    ClusterDAG& GetClusterDAG() { return m_clusterDAG; }
private:
    class GeometryPreparePassData
    {
    public:
        framegraph::VirtualResourceHandle instances;
        framegraph::VirtualResourceHandle clusterRefs;
        framegraph::VirtualResourceHandle megaVertices;
        framegraph::VirtualResourceHandle megaIndices;
        GPUCullingManager* manager;
    };

    class GPUCullPassData
    {
    public:
        framegraph::VirtualResourceHandle prevHiZ;
        GPUCullingManager* manager;
        Fmatrix prevViewProj;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    class ClusterRetestPassData
    {
    public:
        framegraph::VirtualResourceHandle hiz;
        framegraph::VirtualResourceHandle args;
        GPUCullingManager* manager;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    GeometryResidencyManager m_residency;
    nvrhi::BufferHandle m_rtVertexBuffer;
    nvrhi::BufferHandle m_rtIndexBuffer;
    xr_vector<u8> m_rtVertexStaging;
    xr_vector<u32> m_rtIndexStaging;
    u32 m_rtVertexCount = 0;
    u32 m_rtIndexCount = 0;
    bool m_rtSourceUploaded = false;
    u64 m_rtSourceLease = 0;
    void RetireRTSourceUpload(bool discard);
    void CaptureRTSource();
    void UploadRTSource(nvrhi::ICommandList* cmdList);
    string_path m_pageStorePath = {};
};


} // namespace xray::render::fg
