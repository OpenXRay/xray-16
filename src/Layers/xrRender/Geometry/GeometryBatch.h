#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render {

struct GeometryBatch {
    ng::BufferHandle vertexBuffer;
    ng::BufferHandle indexBuffer;

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
