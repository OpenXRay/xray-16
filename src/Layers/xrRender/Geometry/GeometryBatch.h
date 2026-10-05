// xrRender/Geometry/GeometryBatch.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FBasicVisual.h"  // For dxRender_Visual
#include "Layers/xrRender/GPUCullingManager.h"  // For MeshAllocation
#include "Layers/xrRender/Materials/MaterialSystem.h"  // For D3D12 material info

namespace xray::render::fg {
    class dxRender_Visual;  // Forward declaration
}

namespace xray::render {

using fg::dxRender_Visual;

// ══════════════════════════════════════════════════════════
//  GEOMETRY BATCH (SINGLE DRAW CALL)
// ══════════════════════════════════════════════════════════

struct GeometryBatch {
    // Draw parameters
    u32 indexCount = 0;
    u32 startIndex = 0;
    s32 baseVertex = 0;
    u32 vertexCount = 0;

    u32 bindlessMaterialID = UINT32_MAX;  // Index into g_Materials for GPU-driven rendering

    // Transform
    Fmatrix worldMatrix;

    // Pre-computed world-space bounding sphere for GPU culling
    // Computed at batch creation time to handle different visual types:
    // - Static geometry: sphere already in world space (identity transform)
    // - Trees: sphere already in world space (level compiler pre-transforms)
    // - Dynamic objects: sphere transformed by worldMatrix
    Fvector worldBoundsCenter;
    float worldBoundsRadius = 0.0f;

    // Source visual (for material system)
    dxRender_Visual* visual = nullptr;
    IRenderable* renderable = nullptr; // For skinned meshes

    u64 visualLifetimeID = 0;
    u64 renderableLifetimeID = 0;
    u32 geometrySubset = 0;

    // Static vs dynamic classification (used for GPU culling uploads)
    bool isStatic = false;

    bool isSkinned = false;

    bool isShadowOnly = false;

    bool isTransparent = false;
    bool isAlphaTested = false;

    // Skinning render mode from CSkeletonX::RenderMode
    // Used to select correct shader (1B, 2B, 3B, 4B variants)
    u16 skinningRenderMode = 0;

    u32 skinnedPoolFormat = UINT32_MAX;
    s32 skinnedPoolBaseVertex = 0;
    u32 skinnedPoolFirstIndex = 0;

    // Terrain flag - determines which pipeline to use
    // Terrain uses bindless_terrain.ps with 4-layer detail blending
    bool isTerrain = false;
    u32 terrainMaterialID = UINT32_MAX;  // Index into g_TerrainMaterials for terrain rendering
    float hemiScale = 0.0f;
    float hemiBias = 1.0f;
    u32 lightmapTexture = UINT32_MAX;

    // SSA (Screen Space Area) for sorting - matches vanilla CalcSSA()
    // SSA = R / distSQ where R = bounding sphere radius, distSQ = distance squared to camera
    // Larger SSA = closer/bigger = should render first (front-to-back for opaque)
    float ssa = 0.0f;

    // ═══════════════════════════════════════════════════
    //  GPU-DRIVEN RENDERING: Mega-buffer allocation
    // ═══════════════════════════════════════════════════
    // Contains offsets into the unified mega vertex/index buffers
    // Set during ProcessVisualGeometry using pool IDs from mesh
    fg::MeshAllocation megaBufferAlloc;

    // ═══════════════════════════════════════════════════
    //  SHADER FLAG HELPERS
    // ═══════════════════════════════════════════════════
    // Uses MaterialSystem for material flags

    bool IsAlphaTested() const { return isAlphaTested; }

    bool IsStrictB2F() const { return isTransparent; }

    bool IsOpaque() const { return !isAlphaTested && !isTransparent; }
};

// ══════════════════════════════════════════════════════════
//  GEOMETRY COLLECTOR
// ══════════════════════════════════════════════════════════

class GeometryCollector {
public:
    GeometryCollector();
    ~GeometryCollector();

    // Begin/end frame
    void BeginFrame();
    void EndFrame();

    // Submit geometry for rendering
    void Submit(const GeometryBatch& batch);

    const xr_vector<GeometryBatch>& GetBatches() const { return m_batches; }
    const xr_vector<GeometryBatch>& GetStaticBatches() const { return m_staticBatches; }
    const xr_vector<u32>& GetStaticTransparentIndices() const { return m_staticTransparentIndices; }

    bool IsStaticBuildComplete() const { return m_staticBuildComplete; }
    bool HasBatches() const { return !m_batches.empty() || !m_staticBatches.empty(); }

    void EndStaticBuild();
    void ClearStatic();

    // Statistics
    struct Stats {
        u32 numBatches = 0;
        u32 numTriangles = 0;
        u32 numVertices = 0;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    xr_vector<GeometryBatch> m_batches;
    xr_vector<GeometryBatch> m_staticBatches;
    xr_vector<u32> m_staticTransparentIndices;
    bool m_staticBuildComplete = false;
    Stats m_stats;
};

// Global geometry collector instance (to be initialized by renderer)
extern GeometryCollector* g_geometryCollector;

} // namespace xray::render
