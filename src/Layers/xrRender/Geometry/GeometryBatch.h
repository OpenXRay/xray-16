#pragma once

#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"

namespace xray::render::fg {
    class dxRender_Visual;
}

namespace xray::render {

using fg::dxRender_Visual;

struct MaterialPSO;

struct GeometryBatch {
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;

    u32 indexCount = 0;
    u32 startIndex = 0;
    s32 baseVertex = 0;
    u32 vertexCount = 0;
    u32 vertexStride = 0;

    u32 materialID = 0;
    u32 bindlessMaterialID = UINT32_MAX;

    fg::TextureHandle albedoTexture;
    fg::TextureHandle normalTexture;
    fg::TextureHandle materialTexture;

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

    bool isStatic = false;

    bool isSkinned = false;

    bool isShadowOnly = false;

    u16 skinningRenderMode = 0;

    u32 skinnedPoolFormat = UINT32_MAX;
    s32 skinnedPoolBaseVertex = 0;
    u32 skinnedPoolFirstIndex = 0;

    bool isTerrain = false;
    u32 terrainMaterialID = UINT32_MAX;

    float ssa = 0.0f;

    shared_str debugName;

    fg::MeshAllocation megaBufferAlloc;

    bool IsAlphaTested() const {
        if (!visual || !visual->shaderName.size())
            return false;

        return MaterialSystem::Instance()
            .GetMaterialInfo(visual->shaderName)
            .alphaTest;
    }

    bool IsStrictB2F() const {
        if (!visual || !visual->shaderName.size())
            return false;

        return MaterialSystem::Instance()
            .GetMaterialInfo(visual->shaderName)
            .transparent;
    }

    bool IsOpaque() const {
        return !IsAlphaTested() && !IsStrictB2F();
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
