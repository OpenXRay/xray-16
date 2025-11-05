// xrRender/Geometry/GeometryBatch.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"  // For RenderPhase

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

    // Visibility/culling (for front-to-back sorting)
    bool isVisible = true;
    float distanceToCamera = 0.0f;

    // Debug
    shared_str debugName;
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
