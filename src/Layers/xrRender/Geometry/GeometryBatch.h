// xrRender/Geometry/GeometryBatch.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"  // For RenderPhase
#include "Layers/xrRender/Shader.h"  // For ShaderElement flags
#include "Layers/xrRender/FBasicVisual.h"  // For dxRender_Visual

namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;  // Forward declaration
}

namespace xray::render {

using RENDER_NAMESPACE::dxRender_Visual;

struct MaterialPSO;  // Forward declaration

// ══════════════════════════════════════════════════════════
//  GEOMETRY BATCH (SINGLE DRAW CALL)
// ══════════════════════════════════════════════════════════

struct GeometryBatch {
    // Vertex/index buffers (NVRHI handles for wrapped legacy buffers)
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;

    // Draw parameters
    u32 indexCount = 0;
    u32 startIndex = 0;
    s32 baseVertex = 0;

    // Material
    u32 materialID = 0;

    // Textures
    ng::TextureHandle albedoTexture;
    ng::TextureHandle normalTexture;
    ng::TextureHandle materialTexture;  // Metallic/roughness

    // Transform
    Fmatrix worldMatrix;

    // Pre-computed world-space bounding sphere for GPU culling
    // Computed at batch creation time to handle different visual types:
    // - Static geometry: sphere already in world space (identity transform)
    // - Trees: sphere already in world space (level compiler pre-transforms)
    // - Dynamic objects: sphere transformed by worldMatrix
    Fvector worldBoundsCenter;
    float worldBoundsRadius = 0.0f;

    // Shader
    nvrhi::IGraphicsPipeline* pipeline = nullptr;
    nvrhi::IBindingSet* bindingSet = nullptr;

    // ═══════════════════════════════════════════════════
    //  WEEK 16: DYNAMIC ROUTING DATA
    // ═══════════════════════════════════════════════════

    // MaterialPSO contains shader RT bindings and full PSO
    // Created lazily during Execute() (after FrameGraph compilation)
    MaterialPSO* materialPSO = nullptr;

    // Rendering phase (from shader reflection via ShaderPhaseCache)
    // Populated during ScanRequiredPhases() (before compilation)
    // Used by routing system to assign batches to correct pass
    framegraph::RenderPhase renderPhase = framegraph::RenderPhase::Geometry;

    // Source visual (for material system)
    dxRender_Visual* visual = nullptr;
    IRenderable* renderable = nullptr; // For skinned meshes

    // Visibility/culling
    bool isVisible = true;

    // SSA (Screen Space Area) for sorting - matches vanilla CalcSSA()
    // SSA = R / distSQ where R = bounding sphere radius, distSQ = distance squared to camera
    // Larger SSA = closer/bigger = should render first (front-to-back for opaque)
    float ssa = 0.0f;

    // Debug
    shared_str debugName;

    // ═══════════════════════════════════════════════════
    //  SHADER FLAG HELPERS
    // ═══════════════════════════════════════════════════
    // Uses ShaderElement::Sflags set during blender compilation:
    //   - bAlphaTest: Alpha-tested (uses clip/discard in shader)
    //   - bStrictB2F: Requires back-to-front sorting (transparent)

    // Check if batch is alpha-tested (bAlphaTest flag set by blender)
    bool IsAlphaTested() const {
        RENDER_NAMESPACE::ShaderElement* elem = visual->shader->E[0]._get();
        if (elem) {
            return elem->flags.bAlphaTest != 0;
        }
        return false;
    }

    // Check if batch requires back-to-front sorting (transparent/alpha-blended)
    bool IsStrictB2F() const {
        RENDER_NAMESPACE::ShaderElement* elem = visual->shader->E[0]._get();
        if (elem) {
            return elem->flags.bStrictB2F != 0;
        }
        return false;
    }

    // Check if batch is opaque (no alpha-test and no strict B2F)
    bool IsOpaque() const {
        return !IsAlphaTested();
    }
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

    // Get batches for rendering (const)
    const xr_vector<GeometryBatch>& GetBatches() const { return m_batches; }

    // Get batches for routing (non-const, for Week 16 dynamic routing)
    xr_vector<GeometryBatch>& GetBatchesMutable() { return m_batches; }

    // Sort batches for optimal rendering
    void Sort();

    // Statistics
    struct Stats {
        u32 numBatches = 0;
        u32 numTriangles = 0;
        u32 numVertices = 0;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    xr_vector<GeometryBatch> m_batches;
    Stats m_stats;

    // Sorting key
    static u64 ComputeSortKey(const GeometryBatch& batch);
};

// Global geometry collector instance (to be initialized by renderer)
extern GeometryCollector* g_geometryCollector;

} // namespace xray::render
