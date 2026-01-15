// xrRender/GPUCullingManager.h
#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"

namespace xray::render::RENDER_NAMESPACE::passes {
    struct ParticleBatch;
}

namespace xray::render {
    class GeometryCollector;
    struct GeometryBatch;
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
    namespace framegraph {
        class FrameGraph;
    }
}

namespace xray::render::RENDER_NAMESPACE {

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

// ═══════════════════════════════════════════════════════
//  INSTANCE DATA (matches HLSL InstanceData struct)
// ═══════════════════════════════════════════════════════

struct GPUInstanceData {
    Fmatrix world;          // World transform (64 bytes)
    u32 materialID;         // Bindless material ID
    u32 flags;              // Instance flags
    float pad0, pad1;       // Padding to 80 bytes
};
static_assert(sizeof(GPUInstanceData) == 80, "GPUInstanceData must be 80 bytes for GPU alignment");

// ═══════════════════════════════════════════════════════
//  GPU CULLING OUTPUT
// ═══════════════════════════════════════════════════════

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
    void Initialize(ng::RenderDevice* device);

    // Shutdown and release resources
    void Shutdown();

    // Upload scene objects to GPU (call once per frame before culling)
    // Extracts bounding sphere data from geometry batches
    void UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry);

    // Invalidate static GPU culling data (call on level load/unload)
    void InvalidateStaticCullingData();

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

    // Check if mega-buffers are ready for rendering
    bool AreMegaBuffersReady() const { return m_megaBuffersReady; }

    // Get mega-buffers for rendering
    nvrhi::IBuffer* GetMegaVertexBuffer() const { return m_megaVertexBuffer.Get(); }
    nvrhi::IBuffer* GetMegaIndexBuffer() const { return m_megaIndexBuffer.Get(); }
    // Get compact count buffer (contains actual visible draw count from GPU culling)
    bool IsCompactionEnabled() const { return m_compactEnabled; }

    // Upload instance data (transforms) for current frame
    void UploadInstanceData(ng::RenderContext* ctx, const GeometryCollector* geometry);

    // Get mega-buffer stats
    u32 GetTotalVertexCount() const { return m_totalVertexCount; }
    u32 GetTotalIndexCount() const { return m_totalIndexCount; }

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

    void UploadParticleBatches(ng::RenderContext* ctx, const xr_vector<passes::ParticleBatch>* batches);

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
    nvrhi::IBuffer* GetTerrainInstanceBuffer() const { return m_terrainInstanceBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainBatchIndicesBuffer() const { return m_terrainBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactDrawArgsBuffer() const { return m_terrainCompactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactBatchIndicesBuffer() const { return m_terrainCompactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactCountBuffer() const { return m_terrainCompactCountBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactMaterialIDBuffer() const { return m_terrainCompactMaterialIDBuffer.Get(); }

private:
    void CreateBuffers(ng::RenderDevice* device);
    void CreateComputePipeline(ng::RenderDevice* device);
    void CreateCompactionResources(ng::RenderDevice* device);
    void CreateDebugResources(ng::RenderDevice* device);
    void CreateParticleResources(ng::RenderDevice* device);
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
    nvrhi::BufferHandle m_cullParamsCB;         // Constant buffer

    // Compute pipelines
    nvrhi::ComputePipelineHandle m_cullPipeline;
    nvrhi::ComputePipelineHandle m_clearArgsPipeline;
    nvrhi::ComputePipelineHandle m_compactPipeline;
    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_clearArgsLayout;
    nvrhi::BindingLayoutHandle m_compactLayout;
    nvrhi::SamplerHandle m_pointSampler;

    nvrhi::BufferHandle m_compactParamsCB;

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
    nvrhi::BufferHandle m_terrainCompactMaterialIDBuffer;
    nvrhi::BufferHandle m_terrainMaterialIDBuffer;       // Terrain material IDs (for bindless)

    // Terrain visibility apply pass (copies visibility → instanceCount in draw args)
    nvrhi::ComputePipelineHandle m_terrainApplyVisibilityPipeline;
    nvrhi::BindingLayoutHandle m_terrainApplyVisibilityLayout;

    // ───────────────────────────────────────────────────────
    //  DEBUG VISUALIZATION RESOURCES
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_debugBuffer;                // CullDebugData for all objects
    nvrhi::BufferHandle m_debugComputeParamsCB;       // Constant buffer for compute shader
    nvrhi::BufferHandle m_debugGraphicsParamsCB;      // Constant buffer for graphics shaders

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
    nvrhi::BufferHandle m_particleCullParamsCB;
    nvrhi::ComputePipelineHandle m_particleCullPipeline;
    nvrhi::BindingLayoutHandle m_particleCullLayout;

    u32 m_particleCount = 0;
    u32 m_maxParticles = 0;
    bool m_particleCullEnabled = false;
    xr_vector<GPUParticleData> m_particleData;
    xr_vector<IndirectDrawArgs> m_particleDrawArgsData;
    ng::RenderDevice* m_device = nullptr;
    u32 m_objectCount = 0;
    u32 m_maxObjects = 0;
    bool m_initialized = false;
    bool m_computeEnabled = false;

    // CPU-side object data (for upload)
    xr_vector<GPUObjectData> m_staticObjectData;
    xr_vector<IndirectDrawArgs> m_staticDrawArgsData;  // Draw arguments (geometry info)
    xr_vector<u32> m_staticMaterialIDData;             // Material IDs per batch (for bindless)
    xr_vector<GPUInstanceData> m_staticInstanceData;

    xr_vector<GPUObjectData> m_dynamicObjectData;
    xr_vector<IndirectDrawArgs> m_dynamicDrawArgsData;  // Draw arguments (geometry info)
    xr_vector<u32> m_dynamicMaterialIDData;             // Material IDs per batch (for bindless)
    xr_vector<GPUInstanceData> m_dynamicInstanceData;

    // Terrain-specific CPU data (separate from regular geometry)
    u32 m_terrainObjectCount = 0;
    u32 m_maxTerrainObjects = 0;
    xr_vector<GPUObjectData> m_terrainObjectData;
    xr_vector<IndirectDrawArgs> m_terrainDrawArgsData;
    xr_vector<u32> m_terrainMaterialIDData;      // Terrain material IDs (index into TerrainMaterialBuffer)
    xr_vector<GPUInstanceData> m_terrainInstanceData;  // Terrain world transforms

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
        const D3DVERTEXELEMENT9* decl,
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
        const D3DVERTEXELEMENT9* decl,
        u32 stride
    );
};

} // namespace xray::render::RENDER_NAMESPACE
