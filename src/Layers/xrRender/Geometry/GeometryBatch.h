#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/GPUCullingManager.h"

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
    u32 bindlessMaterialID = UINT32_MAX;

    ng::TextureHandle albedoTexture;
    ng::TextureHandle normalTexture;
    ng::TextureHandle materialTexture;

    Fmatrix worldMatrix;

    Fvector worldBoundsCenter;
    float worldBoundsRadius = 0.0f;

    nvrhi::IGraphicsPipeline* pipeline = nullptr;
    nvrhi::IBindingSet* bindingSet = nullptr;

    MaterialPSO* materialPSO = nullptr;

    framegraph::RenderPhase renderPhase = framegraph::RenderPhase::Geometry;

    dxRender_Visual* visual = nullptr;
    IRenderable* renderable = nullptr;

    bool isVisible = true;

    float ssa = 0.0f;

    shared_str debugName;

    RENDER_NAMESPACE::MeshAllocation megaBufferAlloc;

    bool IsAlphaTested() const {
        if (!visual || !visual->shader)
            return false;
        RENDER_NAMESPACE::ShaderElement* elem = visual->shader->E[0]._get();
        if (elem) {
            return elem->flags.bAlphaTest != 0;
        }
        return false;
    }

    bool IsStrictB2F() const {
        if (!visual || !visual->shader)
            return false;
        RENDER_NAMESPACE::ShaderElement* elem = visual->shader->E[0]._get();
        if (elem) {
            return elem->flags.bStrictB2F != 0;
        }
        return false;
    }

    bool IsOpaque() const {
        return !IsAlphaTested();
    }
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
