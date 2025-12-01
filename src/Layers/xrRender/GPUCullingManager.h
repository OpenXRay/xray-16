// xrRender/GPUCullingManager.h
#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

// Forward declarations
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
    Fvector position;       // World-space bounding sphere center (12 bytes)
    float radius;           // Bounding sphere radius (4 bytes)
    u32 batchIndex;         // Index into original batch array (4 bytes)
    u32 flags;              // Object flags (4 bytes)
    float pad0, pad1;       // Padding to 32 bytes (8 bytes)
};
static_assert(sizeof(GPUObjectData) == 32, "GPUObjectData must be 32 bytes for GPU alignment");

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
    CULL_STATE_VISIBLE           = 0,  // Passed all tests - GREEN
    CULL_STATE_OCCLUDER          = 1,  // Visible and close (writing Hi-Z) - BLUE
    CULL_STATE_CULLED_DISTANCE   = 2,  // Failed distance test - RED (dark)
    CULL_STATE_CULLED_FRUSTUM    = 3,  // Failed frustum test - RED (medium)
    CULL_STATE_CULLED_OCCLUSION  = 4,  // Failed Hi-Z occlusion test - YELLOW
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
//  GPU CULLING OUTPUT
// ═══════════════════════════════════════════════════════

struct GPUCullOutput {
    framegraph::VirtualResourceHandle visibleIndices;   // Buffer of visible batch indices (debug)
    framegraph::VirtualResourceHandle visibleCount;     // Atomic counter buffer
    framegraph::VirtualResourceHandle drawArgsBuffer;   // Indirect draw arguments (one per batch)
    u32 maxObjects;                                     // Maximum objects that can be visible
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

    // Setup GPU culling pass in FrameGraph
    // Returns handles to culled results for forward pass
    // NOTE: geometry is captured and used during execute - must remain valid
    GPUCullOutput SetupCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const GeometryCollector* geometry
    );

    // Get number of objects uploaded this frame
    u32 GetObjectCount() const { return m_objectCount; }

    // Check if culling is enabled and ready
    bool IsEnabled() const { return m_initialized && m_computeEnabled; }

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
        u32 hizMipLevels
    );

    // Check if debug visualization is enabled
    bool IsDebugEnabled() const;

private:
    void CreateBuffers(ng::RenderDevice* device);
    void CreateComputePipeline(ng::RenderDevice* device);
    void CreateDebugResources(ng::RenderDevice* device);

    // Extract frustum planes from view-projection matrix
    void ExtractFrustumPlanes(Fmatrix& viewProj, Fvector4* outPlanes);

    // NVRHI resources
    nvrhi::BufferHandle m_objectBuffer;         // All objects (GPU read)
    nvrhi::BufferHandle m_visibleIndexBuffer;   // Visible object indices (GPU write)
    nvrhi::BufferHandle m_visibleCountBuffer;   // Atomic counter (GPU write)
    nvrhi::BufferHandle m_drawArgsBuffer;       // Indirect draw arguments (GPU read/write)
    nvrhi::BufferHandle m_cullParamsCB;         // Constant buffer

    // Compute pipelines
    nvrhi::ComputePipelineHandle m_cullPipeline;      // Main culling pass
    nvrhi::ComputePipelineHandle m_clearArgsPipeline; // Clear draw args pass
    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_clearArgsLayout;
    nvrhi::SamplerHandle m_pointSampler;

    // ───────────────────────────────────────────────────────
    //  DEBUG VISUALIZATION RESOURCES
    // ───────────────────────────────────────────────────────
    nvrhi::BufferHandle m_debugBuffer;                // CullDebugData for all objects
    nvrhi::BufferHandle m_debugComputeParamsCB;       // Constant buffer for compute shader
    nvrhi::BufferHandle m_debugGraphicsParamsCB;      // Constant buffer for graphics shaders

    // Debug compute pipeline (object_cull_debug.cs)
    nvrhi::ComputePipelineHandle m_debugComputePipeline;
    nvrhi::BindingLayoutHandle m_debugComputeLayout;

    // Debug graphics pipeline (cull_debug.vs + cull_debug.ps)
    nvrhi::GraphicsPipelineHandle m_debugGraphicsPipeline;
    nvrhi::BindingLayoutHandle m_debugGraphicsLayout;
    nvrhi::InputLayoutHandle m_debugInputLayout;

    // State
    ng::RenderDevice* m_device = nullptr;
    u32 m_objectCount = 0;
    u32 m_maxObjects = 0;
    bool m_initialized = false;
    bool m_computeEnabled = false;

    // CPU-side object data (for upload)
    xr_vector<GPUObjectData> m_objectData;
    xr_vector<IndirectDrawArgs> m_drawArgsData;  // Draw arguments (geometry info)

public:
    // Get draw args buffer for forward pass (indirect draw)
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer.Get(); }
};

} // namespace xray::render::RENDER_NAMESPACE
