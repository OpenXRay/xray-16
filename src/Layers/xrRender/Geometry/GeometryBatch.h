#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
}

namespace xray::render {

using RENDER_NAMESPACE::dxRender_Visual;

struct MaterialPSO;

struct GeometryBatch {
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;

    u32 indexCount = 0;
    u32 startIndex = 0;
    s32 baseVertex = 0;

    u32 materialID = 0;

    ng::TextureHandle albedoTexture;
    ng::TextureHandle normalTexture;
    ng::TextureHandle materialTexture;

    Fmatrix worldMatrix;

    nvrhi::IGraphicsPipeline* pipeline = nullptr;
    nvrhi::IBindingSet* bindingSet = nullptr;

    MaterialPSO* materialPSO = nullptr;

    framegraph::RenderPhase renderPhase = framegraph::RenderPhase::Geometry;

    dxRender_Visual* visual = nullptr;

    bool isVisible = true;
    float distanceToCamera = 0.0f;

    shared_str debugName;
};

class GeometryCollector {
public:
    GeometryCollector();
    ~GeometryCollector();

    void BeginFrame();
    void EndFrame();

    void Submit(const GeometryBatch& batch);

    const xr_vector<GeometryBatch>& GetBatches() const { return m_batches; }

    xr_vector<GeometryBatch>& GetBatchesMutable() { return m_batches; }

    void Sort();

    struct Stats {
        u32 numBatches = 0;
        u32 numTriangles = 0;
        u32 numVertices = 0;
    };

    const Stats& GetStats() const { return m_stats; }

private:
    xr_vector<GeometryBatch> m_batches;
    Stats m_stats;

    static u64 ComputeSortKey(const GeometryBatch& batch);
};

extern GeometryCollector* g_geometryCollector;

}
