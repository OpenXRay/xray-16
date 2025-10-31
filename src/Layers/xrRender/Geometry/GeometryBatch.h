// xrRender/Geometry/GeometryBatch.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render {

// ══════════════════════════════════════════════════════════
//  GEOMETRY BATCH (SINGLE DRAW CALL)
// ══════════════════════════════════════════════════════════

struct GeometryBatch {
    // Vertex/index buffers
    ng::BufferHandle vertexBuffer;
    ng::BufferHandle indexBuffer;

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

    // Get batches for rendering
    const xr_vector<GeometryBatch>& GetBatches() const { return m_batches; }

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
