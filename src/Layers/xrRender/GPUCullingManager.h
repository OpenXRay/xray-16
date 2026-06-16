// xrRender/GPUCullingManager.h
#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"

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
}

namespace xray::render::fg {

// ═══════════════════════════════════════════════════════
//  GPU OBJECT DATA (matches HLSL GPUObjectData struct)
// ═══════════════════════════════════════════════════════

struct GPUObjectData {
    Fvector position;
    float radius;
    u32 batchIndex;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUObjectData) == 32, "GPUObjectData must be 32 bytes for GPU alignment");

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
    GPU_OBJECT_OPAQUE      = 0x1,
    GPU_OBJECT_ALPHA_TEST  = 0x2,
    GPU_OBJECT_TRANSPARENT = 0x4,
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

struct GPUInstanceData {
    Fmatrix world;          // World transform (64 bytes)
    u32 materialID;         // Bindless material ID
    u32 flags;              // Instance flags
    float pad0, pad1;       // Padding to 80 bytes
};
static_assert(sizeof(GPUInstanceData) == 80, "GPUInstanceData must be 80 bytes for GPU alignment");

struct GPUCullOutput {
    framegraph::VirtualResourceHandle visibleIndices;
    framegraph::VirtualResourceHandle visibleCount;
    framegraph::VirtualResourceHandle drawArgsBuffer;
    framegraph::VirtualResourceHandle staticDrawArgsBuffer;
    framegraph::VirtualResourceHandle dynamicDrawArgsBuffer;
    framegraph::VirtualResourceHandle staticCompactDrawArgs;
    framegraph::VirtualResourceHandle staticCompactBatchIndices;
    framegraph::VirtualResourceHandle dynamicCompactDrawArgs;
    framegraph::VirtualResourceHandle dynamicCompactBatchIndices;
    u32 maxObjects;
    u32 staticObjectCount;
    u32 dynamicObjectCount;

    // Terrain-specific outputs (separate draw call)
    framegraph::VirtualResourceHandle terrainDrawArgsBuffer;
    framegraph::VirtualResourceHandle terrainCompactDrawArgs;
    framegraph::VirtualResourceHandle terrainCompactBatchIndices;
    framegraph::VirtualResourceHandle terrainCompactMaterialIDs;
    framegraph::VirtualResourceHandle terrainCompactCount;
    u32 terrainObjectCount;

    // Transparent-specific outputs (alpha-blended geometry)
    framegraph::VirtualResourceHandle transparentCompactDrawArgs;
    framegraph::VirtualResourceHandle transparentCompactBatchIndices;
    framegraph::VirtualResourceHandle transparentCompactMaterialIDs;
    framegraph::VirtualResourceHandle transparentCompactCount;
    u32 transparentObjectCount;

};

struct GPUParticleCullOutput {
    framegraph::VirtualResourceHandle drawArgsBuffer;
    u32 maxParticles;
};

// ═══════════════════════════════════════════════════════
//  GPU CULLING MANAGER
// ═══════════════════════════════════════════════════════
//
// Manages GPU-driven frustum and occlusion culling for world geometry.
// Uses Hi-Z pyramid from depth prepass for conservative occlusion testing.
//
// USAGE:
// 1. Call Initialize() once at startup
// 2. Call UploadSceneObjects() each frame with geometry batches
// 3. Call SetupCullingPass() to add culling pass to FrameGraph
// 4. Forward pass reads visible indices from culling output
//
// PERFORMANCE:
// - GPU culling: ~0.3-0.5ms for 100K objects
// - 10-100x faster than CPU culling for large scenes

class GPUCullingManager {
public:
    GPUCullingManager();
    ~GPUCullingManager();

    // Initialize GPU resources (call once at startup)
    void Initialize(fg::RenderDevice* device);

    // Shutdown and release resources
    void Shutdown();

    // Upload scene objects to GPU (call once per frame before culling)
    // Extracts bounding sphere data from geometry batches
    void UploadSceneObjects(fg::RenderContext* ctx, const GeometryCollector* geometry);

    void InvalidateStaticCullingData();
    void InvalidateShadersAndPipelines();

    // Setup GPU culling pass in FrameGraph
    // Returns handles to culled results for forward pass
    // NOTE: geometry is captured and used during execute - must remain valid
    GPUCullOutput SetupCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const GeometryCollector* geometry,
        const Fmatrix& prevViewProj  // Previous frame's viewProj for temporal Hi-Z
    );

    // Get number of objects uploaded this frame (static + dynamic)
    u32 GetObjectCount() const { return m_objectCount; }
    u32 GetStaticObjectCount() const { return m_staticSet.objectCount; }
    u32 GetDynamicObjectCount() const { return m_dynamicSet.objectCount; }

    // Check if culling is enabled and ready
    bool IsEnabled() const { return m_initialized && m_computeEnabled; }

    // ───────────────────────────────────────────────────────
    //  MEGA-BUFFER SYSTEM (GPU-Driven Rendering)
    // ───────────────────────────────────────────────────────
    // Unified vertex/index buffers for all geometry
    // Enables true MultiDrawIndirect with single VB/IB binding

    // Begin level load - prepare to receive mesh data
    // estimatedVertices/Indices help pre-allocate, but will grow if needed
    void BeginLevelLoad(u32 estimatedVertices = 1000000, u32 estimatedIndices = 3000000);

    // Register a mesh's geometry into mega-buffers
    // Converts from X-Ray format to UnifiedVertex and stores in mega-buffer
    // Returns allocation info with offsets for draw args
    MeshAllocation RegisterMesh(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        bindless::SourceVertexFormat format,
        const u16* indices,
        u32 indexCount
    );

    // End level load - upload all data to GPU
    void EndLevelLoad();

    bool AreMegaBuffersReady() const { return m_megaBuffersReady; }
    bool IsMegaDataUploaded() const { return m_megaDataUploaded; }

    // Get mega-buffers for rendering
    nvrhi::IBuffer* GetMegaVertexBuffer() const { return m_megaVertexBuffer.Get(); }
    nvrhi::IBuffer* GetMegaIndexBuffer() const { return m_megaIndexBuffer.Get(); }
    // Get compact count buffer (contains actual visible draw count from GPU culling)
    bool IsCompactionEnabled() const { return m_compactEnabled; }

    // Upload instance data (transforms) for current frame
    void UploadInstanceData(fg::RenderContext* ctx, const GeometryCollector* geometry);

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
        const xr_vector<passes::ParticleBatch>* particleBatches = nullptr
    );

    bool IsDebugEnabled() const;

    void UploadParticleBatches(fg::RenderContext* ctx, const xr_vector<passes::ParticleBatch>* batches);

    GPUParticleCullOutput SetupParticleCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const xr_vector<passes::ParticleBatch>* batches
    );

    u32 GetParticleCount() const { return m_particleCount; }
    bool IsParticleCullingEnabled() const { return m_initialized && m_particleCullEnabled; }
    nvrhi::IBuffer* GetParticleDrawArgsBuffer() const { return m_particleDrawArgsBuffer.Get(); }

    nvrhi::IBuffer* GetStaticCompactDrawArgsBuffer() const { return m_staticSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactBatchIndicesBuffer() const { return m_staticSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactMaterialIDBuffer() const { return m_staticSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactCountBuffer() const { return m_staticSet.compactCountBuffer.Get(); }
    nvrhi::IBuffer* GetStaticInstanceBuffer() const { return m_staticSet.instanceBuffer.Get(); }

    nvrhi::IBuffer* GetDynamicCompactDrawArgsBuffer() const { return m_dynamicSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactBatchIndicesBuffer() const { return m_dynamicSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactMaterialIDBuffer() const { return m_dynamicSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactCountBuffer() const { return m_dynamicSet.compactCountBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicInstanceBuffer() const { return m_dynamicSet.instanceBuffer.Get(); }

    nvrhi::IBuffer* GetStaticDrawArgsBuffer() const { return m_staticSet.drawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicDrawArgsBuffer() const { return m_dynamicSet.drawArgsBuffer.Get(); }

    // ───────────────────────────────────────────────────────
    //  TERRAIN-SPECIFIC BUFFERS
    // ───────────────────────────────────────────────────────
    u32 GetTerrainObjectCount() const { return m_terrainObjectCount; }
    nvrhi::IBuffer* GetTerrainDrawArgsBuffer() const { return m_terrainDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainMaterialIDBuffer() const { return m_terrainMaterialIDBuffer.Get(); }

    // ───────────────────────────────────────────────────────
    //  TRANSPARENT-SPECIFIC BUFFERS
    // ───────────────────────────────────────────────────────
    u32 GetTransparentObjectCount() const { return m_transparentSet.objectCount; }
    nvrhi::IBuffer* GetTransparentInstanceBuffer() const { return m_transparentSet.instanceBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactDrawArgsBuffer() const { return m_transparentSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactBatchIndicesBuffer() const { return m_transparentSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactMaterialIDBuffer() const { return m_transparentSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactCountBuffer() const { return m_transparentSet.compactCountBuffer.Get(); }

    // ───────────────────────────────────────────────────────
    //  VARIANT PARTITIONING (multi-PSO rendering)
    // ───────────────────────────────────────────────────────
    static constexpr u32 MAX_SHADER_VARIANTS = 32;

    struct VariantPartitionBuffers {
        nvrhi::BufferHandle variantCountBuffer;
        nvrhi::BufferHandle reorderedDrawArgsBuffer;
        nvrhi::BufferHandle reorderedBatchIndicesBuffer;
        nvrhi::BufferHandle reorderedMaterialIDsBuffer;
        nvrhi::BufferHandle drawIndexBuffer;
        u32 binCapacity = 0;
        u32 variantCount = 0;

        VariantPartitionConfig ToConfig() const {
            return {
                variantCountBuffer.Get(), reorderedDrawArgsBuffer.Get(),
                reorderedBatchIndicesBuffer.Get(), reorderedMaterialIDsBuffer.Get(),
                drawIndexBuffer.Get(), binCapacity, variantCount
            };
        }
    };

    bool IsVariantPartitionEnabled() const { return m_variantPartitionEnabled; }
    const VariantPartitionBuffers& GetStaticPartition() const { return m_staticPartition; }
    const VariantPartitionBuffers& GetTransparentPartition() const { return m_transparentPartition; }

    // ───────────────────────────────────────────────────────
    //  CULLING STATS READBACK (for profiling overlay)
    // ───────────────────────────────────────────────────────
    // Returns previous frame's visible counts (1-frame latency to avoid GPU stall)
    struct CullingStats {
        u32 staticVisible = 0;
        u32 dynamicVisible = 0;
        u32 terrainVisible = 0;
        u32 transparentVisible = 0;
        u32 totalVisible() const { return staticVisible + dynamicVisible + terrainVisible + transparentVisible; }
    };
    const CullingStats& GetCullingStats() const { return m_cullingStats; }

    // Schedule readback of visible counts (call after culling pass)
    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);

    // ───────────────────────────────────────────────────────
    //  SKINNED MESH CULLING
    // ───────────────────────────────────────────────────────
    // Skinned meshes use per-draw rendering (bone matrices) and cannot
    // use multi-draw compaction. Instead, we cull them and provide a
    // visibility buffer that the skinning pass checks before each draw.

    // Upload skinned mesh bounding spheres (call from UploadSceneObjects)
    void UploadSkinnedObjects(fg::RenderContext* ctx, const GeometryCollector* geometry);

    // Setup skinned culling pass (uses same Hi-Z pyramid as static culling)
    void SetupSkinnedCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const GeometryCollector* geometry,
        const Fmatrix& prevViewProj
    );

    // Get skinned visibility by visual pointer (handles batch reordering)
    // Returns 0 (culled) if visual not found, non-zero (visible) otherwise
    u32 GetSkinnedVisibilityByVisual(const dxRender_Visual* visual) const;

    // Check if skinned visibility data is available
    bool HasSkinnedVisibilityData() const { return !m_skinnedVisibilityValues.empty(); }

    u32 GetSkinnedObjectCount() const { return m_skinnedObjectCount; }
    bool IsSkinnedCullingEnabled() const { return m_initialized && m_skinnedCullEnabled; }


    // Skinned culling stats (for profiler display)
    struct SkinnedCullingStats {
        u32 submitted = 0;
        u32 visible = 0;
        u32 culled = 0;
    };
    const SkinnedCullingStats& GetSkinnedCullingStats() const { return m_skinnedCullingStats; }
    void UpdateSkinnedCullingStats(u32 rendered, u32 culled);

    // Schedule skinned visibility readback (call after skinned culling pass)
    void ScheduleSkinnedVisibilityReadback(nvrhi::ICommandList* cmdList);

    // Process skinned visibility readback (call at frame start or before skinning pass)
    void ProcessSkinnedVisibilityReadback();

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

    // Clear skeleton visibility data (call on level unload to prevent dangling pointers)
    void ClearSkinnedVisibilityData();

    // Process readback results from previous frame (call at frame start)
    void ProcessStatsReadback();
    nvrhi::IBuffer* GetTerrainInstanceBuffer() const { return m_terrainInstanceBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainBatchIndicesBuffer() const { return m_terrainBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactDrawArgsBuffer() const { return m_terrainCompactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactBatchIndicesBuffer() const { return m_terrainCompactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactCountBuffer() const { return m_terrainCompactCountBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactMaterialIDBuffer() const { return m_terrainCompactMaterialIDBuffer.Get(); }

private:
    void CreateBuffers(fg::RenderDevice* device);
    void CreateComputePipeline(fg::RenderDevice* device);
    void CreateCompactionResources(fg::RenderDevice* device);
    void CreateDebugResources(fg::RenderDevice* device);
    void CreateParticleResources(fg::RenderDevice* device);
    void CreateMegaBuffers();  // Called by EndLevelLoad

    // Extract frustum planes from view-projection matrix
    void ExtractFrustumPlanes(Fmatrix& viewProj, Fvector4* outPlanes);

    struct CullSetBuffers {
        nvrhi::BufferHandle objectBuffer;               // All objects (GPU read)
        nvrhi::BufferHandle visibleIndexBuffer;         // Visible object indices (GPU write)
        nvrhi::BufferHandle visibleCountBuffer;         // Atomic counter (GPU write)
        nvrhi::BufferHandle drawArgsBuffer;             // Indirect draw arguments (GPU read/write)
        nvrhi::BufferHandle materialIDBuffer;           // Material IDs per batch (input)
        nvrhi::BufferHandle visibilityBuffer;           // Visibility buffer (1 uint per object)
        nvrhi::BufferHandle compactDrawArgsBuffer;      // Compacted draw args (output)
        nvrhi::BufferHandle compactBatchIndicesBuffer;  // Compacted batch indices (output)
        nvrhi::BufferHandle compactMaterialIDBuffer;    // Compacted material IDs (output)
        nvrhi::BufferHandle compactCountBuffer;         // Compacted visible count (output)
        nvrhi::BufferHandle compactDispatchArgsBuffer;  // DispatchIndirect args derived from compact count
        nvrhi::BufferHandle compactLocalPrefixBuffer;   // Local prefix per batch (scratch)
        nvrhi::BufferHandle compactGroupCountsBuffer;   // Visible count per group (scratch)
        nvrhi::BufferHandle compactGroupOffsetsBuffer;  // Prefix offsets per group (scratch)
        nvrhi::BufferHandle instanceBuffer;             // Instance data buffer (GPUInstanceData)
        u32 objectCount = 0;
        u32 maxObjects = 0;
        bool drawArgsUploaded = false;
        bool objectsUploaded = false;
    };

    // Static/dynamic culling sets
    CullSetBuffers m_staticSet;
    CullSetBuffers m_dynamicSet;

    // Shared constant buffer
    fg::BufferHandle m_cullParamsCB;         // Constant buffer

    // Compute pipelines
    nvrhi::ComputePipelineHandle m_cullPipeline;
    nvrhi::ComputePipelineHandle m_clearArgsPipeline;
    nvrhi::ComputePipelineHandle m_compactCountPipeline;
    nvrhi::ComputePipelineHandle m_compactScanPipeline;
    nvrhi::ComputePipelineHandle m_compactScatterPipeline;
    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_clearArgsLayout;
    nvrhi::BindingLayoutHandle m_compactCountLayout;
    nvrhi::BindingLayoutHandle m_compactScanLayout;
    nvrhi::BindingLayoutHandle m_compactScatterLayout;
    nvrhi::SamplerHandle m_pointSampler;

    fg::BufferHandle m_compactParamsCB;

    // Variant partition pipeline
    nvrhi::ComputePipelineHandle m_variantPartitionPipeline;
    nvrhi::BindingLayoutHandle m_variantPartitionLayout;
    fg::BufferHandle m_variantPartitionParamsCB;
    VariantPartitionBuffers m_staticPartition;
    VariantPartitionBuffers m_transparentPartition;
    bool m_variantPartitionEnabled = false;

    void CreateVariantPartitionResources(fg::RenderDevice* device);
    void InitPartitionBuffers(nvrhi::IDevice* nvDevice, VariantPartitionBuffers& part,
        const char* prefix, u32 variantCount, u32 maxObjects);
    void DispatchVariantPartition(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* nvDevice,
        const CullSetBuffers& set,
        VariantPartitionBuffers& partition);

    bool m_staticTerrainDrawArgsUploaded = false;  // True after first upload (terrain)

    bool m_compactEnabled = false;

    bool m_staticDataCached = false;

    // ───────────────────────────────────────────────────────
    //  TERRAIN-SPECIFIC BUFFERS
    // ───────────────────────────────────────────────────────
    // Terrain uses separate TerrainMaterialBuffer (t9) with 4-layer detail blending
    // Rendered in separate draw call after regular geometry
    nvrhi::BufferHandle m_terrainObjectBuffer;           // Terrain objects (GPU read)
    nvrhi::BufferHandle m_terrainDrawArgsBuffer;         // Terrain indirect draw args
    nvrhi::BufferHandle m_terrainVisibleIndexBuffer;     // Terrain visible indices
    nvrhi::BufferHandle m_terrainVisibleCountBuffer;     // Terrain atomic counter
    nvrhi::BufferHandle m_terrainVisibilityBuffer;       // Terrain visibility (1 uint per object, like regular geometry)
    nvrhi::BufferHandle m_terrainInstanceBuffer;         // Terrain world transforms (like m_instanceBuffer)
    nvrhi::BufferHandle m_terrainBatchIndicesBuffer;     // Identity mapping (0,1,2,3...) for direct indexing
    nvrhi::BufferHandle m_terrainCompactDrawArgsBuffer;  // Terrain compacted draw args
    nvrhi::BufferHandle m_terrainCompactBatchIndicesBuffer;
    nvrhi::BufferHandle m_terrainCompactCountBuffer;
    nvrhi::BufferHandle m_terrainCompactDispatchArgsBuffer;
    nvrhi::BufferHandle m_terrainCompactMaterialIDBuffer;
    nvrhi::BufferHandle m_terrainCompactLocalPrefixBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupCountsBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupOffsetsBuffer;
    nvrhi::BufferHandle m_terrainMaterialIDBuffer;       // Terrain material IDs (for bindless)

    // Terrain visibility apply pass (copies visibility → instanceCount in draw args)
    nvrhi::ComputePipelineHandle m_terrainApplyVisibilityPipeline;
    nvrhi::BindingLayoutHandle m_terrainApplyVisibilityLayout;

    // ───────────────────────────────────────────────────────
    //  TRANSPARENT CULLING SET (alpha-blended geometry)
    // ───────────────────────────────────────────────────────
    CullSetBuffers m_transparentSet;

    // ───────────────────────────────────────────────────────
    //  DEBUG VISUALIZATION RESOURCES
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_debugBuffer;                // CullDebugData for all objects
    fg::BufferHandle m_debugComputeParamsCB;       // Constant buffer for compute shader
    fg::BufferHandle m_debugGraphicsParamsCB;      // Constant buffer for graphics shaders

    // Debug compute pipeline (object_cull_debug.cs)
    nvrhi::ComputePipelineHandle m_debugComputePipeline;
    nvrhi::ComputePipelineHandle m_particleDebugComputePipeline;
    nvrhi::BindingLayoutHandle m_debugComputeLayout;

    nvrhi::GraphicsPipelineHandle m_debugGraphicsPipeline;
    nvrhi::BindingLayoutHandle m_debugGraphicsLayout;
    nvrhi::InputLayoutHandle m_debugInputLayout;

    nvrhi::BufferHandle m_particleBuffer;
    nvrhi::BufferHandle m_particleDrawArgsBuffer;
    nvrhi::BufferHandle m_particleVisibleCountBuffer;
    fg::BufferHandle m_particleCullParamsCB;
    nvrhi::ComputePipelineHandle m_particleCullPipeline;
    nvrhi::BindingLayoutHandle m_particleCullLayout;

    u32 m_particleCount = 0;
    u32 m_maxParticles = 0;
    bool m_particleCullEnabled = false;
    xr_vector<GPUParticleData> m_particleData;
    xr_vector<IndirectDrawArgs> m_particleDrawArgsData;
    fg::RenderDevice* m_device = nullptr;
    RTAccelStructManager* m_rtAccelMgr = nullptr;
    u32 m_objectCount = 0;
    u32 m_maxObjects = 0;
    bool m_initialized = false;
    bool m_computeEnabled = false;

    xr_vector<GPUObjectData> m_staticObjectData;
    xr_vector<IndirectDrawArgs> m_staticDrawArgsData;
    xr_vector<u32> m_staticMaterialIDData;
    xr_vector<GPUInstanceData> m_staticInstanceData;
    xr_vector<u32> m_staticBatchVertexCounts;

    xr_vector<GPUObjectData> m_dynamicObjectData;
    xr_vector<IndirectDrawArgs> m_dynamicDrawArgsData;  // Draw arguments (geometry info)
    xr_vector<u32> m_dynamicMaterialIDData;             // Material IDs per batch (for bindless)
    xr_vector<GPUInstanceData> m_dynamicInstanceData;

    // Terrain-specific CPU data (separate from regular geometry)
    u32 m_terrainObjectCount = 0;
    u32 m_maxTerrainObjects = 0;
    xr_vector<GPUObjectData> m_terrainObjectData;
    xr_vector<IndirectDrawArgs> m_terrainDrawArgsData;
    xr_vector<u32> m_terrainMaterialIDData;
    xr_vector<GPUInstanceData> m_terrainInstanceData;

    // Transparent-specific CPU data
    xr_vector<GPUObjectData> m_transparentObjectData;
    xr_vector<IndirectDrawArgs> m_transparentDrawArgsData;
    xr_vector<u32> m_transparentMaterialIDData;
    xr_vector<GPUInstanceData> m_transparentInstanceData;

    // ───────────────────────────────────────────────────────
    //  SKINNED MESH CULLING (GPU-Driven)
    // ───────────────────────────────────────────────────────
    // GPU-driven skinned mesh rendering with indirect draws.
    // Uses global bone buffer with per-instance offset.

    nvrhi::BufferHandle m_skinnedObjectBuffer;           // GPUObjectData for skinned batches
    nvrhi::BufferHandle m_skinnedVisibilityBuffer;       // Frame stamp per batch (u32)

    // Double-buffer for async readback (no fence, trust GPU pipelining)
    // By frame N, frame N-2's GPU work is guaranteed complete
    // This gives n-2 latency (1 frame fresher than original n-3)
    static constexpr u32 SKINNED_READBACK_FRAMES = 6;
    nvrhi::BufferHandle m_skinnedReadbackBuffers[SKINNED_READBACK_FRAMES];
    u32 m_skinnedReadbackWriteIndex = 0;   // Which buffer to write to next
    u32 m_skinnedReadbackFrameCount = 0;   // Frames accumulated (0, 1, or 2)

    u32 m_skinnedSubmitFrameId = 0;
    u32 m_skinnedReadbackSubmitFrame[SKINNED_READBACK_FRAMES] = {};
    u32 m_skinnedReadbackCounts[SKINNED_READBACK_FRAMES] = {};
    xr_vector<u32> m_skinnedVisibilityValues;
    u32 m_skinnedVisibilityFrame = 0;

    u32 m_skinnedObjectCount = 0;
    u32 m_maxSkinnedObjects = 0;
    bool m_skinnedCullEnabled = false;

    // CPU-side data
    xr_vector<GPUObjectData> m_skinnedObjectData;
    xr_vector<const GeometryBatch*> m_skinnedBatchPointers;  // Batch pointers (parallel to object data)
    u32 m_skinnedFrameId = 0;
    SkinnedCullingStats m_skinnedCullingStats;

    // Global bone buffer for GPU-driven skinned rendering
    // All skeleton bones are uploaded here each frame, indexed by per-instance offset
    static constexpr u32 MAX_TOTAL_BONES = 8192;  // ~100 skeletons * 78 bones
    static constexpr u32 BONE_STRIDE = sizeof(Fmatrix);  // 64 bytes
    nvrhi::BufferHandle m_globalBoneBuffer;
    u32 m_boneUploadFrameId = 0;
    xr_vector<Fmatrix> m_boneStagingBuffer;
    u32 m_currentBoneOffset = 0;
    bool m_boneBufferInitialized = false;

    void CreateSkinnedCullingBuffers(fg::RenderDevice* device);
    void EnsureSkinnedBufferCapacity(u32 count);
    void UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset);

    // ───────────────────────────────────────────────────────
    //  MEGA-BUFFER SYSTEM
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_megaVertexBuffer;     // Unified vertex buffer (UnifiedVertex format)
    nvrhi::BufferHandle m_megaIndexBuffer;      // Unified index buffer (32-bit indices)

    // CPU-side staging data (during level load)
    xr_vector<bindless::UnifiedVertex> m_megaVertices;
    xr_vector<u32> m_megaIndices;

    // Tracking
    u32 m_totalVertexCount = 0;
    u32 m_totalIndexCount = 0;
    u32 m_maxMegaVertices = 0;
    u32 m_maxMegaIndices = 0;
    bool m_megaBuffersReady = false;
    bool m_megaDataUploaded = false;
    bool m_levelLoadInProgress = false;

    // ───────────────────────────────────────────────────────
    //  VB POOL REGISTRATION (for level geometry)
    // ───────────────────────────────────────────────────────
    // During LoadBuffers(), we register entire VB pools
    // Each pool's vertices are converted and stored in mega-buffer
    // Meshes later lookup their allocation by (vbID, vBase)

    struct VBPoolInfo {
        u32 megaBufferVertexOffset;  // Start offset in mega-VB
        u32 vertexCount;              // Total vertices in pool
        bindless::SourceVertexFormat format;
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

    // Get mesh allocation from VB/IB pool + offsets
    // Called from FVisual::Load() to get mega-buffer offsets
    MeshAllocation GetMeshAllocation(
        u32 vbID, u32 vBase, u32 vCount,
        u32 ibID, u32 iBase, u32 iCount,
        bool alternative = false
    ) const;

    // Detect vertex format from vertex declaration
    static bindless::SourceVertexFormat DetectFormatFromDecl(
        const VertexElement* decl,
        u32 stride
    );
};

} // namespace xray::render::fg
