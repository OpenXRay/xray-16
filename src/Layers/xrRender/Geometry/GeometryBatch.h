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

struct GeometryBatch {
    u32 indexCount = 0;
    u32 startIndex = 0;
    s32 baseVertex = 0;
    u32 vertexCount = 0;

    u32 bindlessMaterialID = UINT32_MAX;

    Fmatrix worldMatrix;

    Fvector worldBoundsCenter;
    float worldBoundsRadius = 0.0f;

    dxRender_Visual* visual = nullptr;
    IRenderable* renderable = nullptr;

    u64 visualLifetimeID = 0;
    u64 renderableLifetimeID = 0;
    u32 geometrySubset = 0;

    bool isStatic = false;

    bool isSkinned = false;

    bool isShadowOnly = false;

    bool isTransparent = false;
    bool isAlphaTested = false;

    u16 skinningRenderMode = 0;

    u32 skinnedPoolFormat = UINT32_MAX;
    s32 skinnedPoolBaseVertex = 0;
    u32 skinnedPoolFirstIndex = 0;

    bool isTerrain = false;
    u32 terrainMaterialID = UINT32_MAX;

    float ssa = 0.0f;

    fg::MeshAllocation megaBufferAlloc;

    bool IsAlphaTested() const { return isAlphaTested; }

    bool IsStrictB2F() const { return isTransparent; }

    bool IsOpaque() const { return !isAlphaTested && !isTransparent; }
};

class GeometryCollector {
public:
    GeometryCollector();
    ~GeometryCollector();

    void BeginFrame();
    void EndFrame();

    void Submit(const GeometryBatch& batch);

    const xr_vector<GeometryBatch>& GetBatches() const { return m_batches; }
    const xr_vector<GeometryBatch>& GetStaticBatches() const { return m_staticBatches; }
    const xr_vector<u32>& GetStaticTransparentIndices() const { return m_staticTransparentIndices; }

    bool IsStaticBuildComplete() const { return m_staticBuildComplete; }
    bool HasBatches() const { return !m_batches.empty() || !m_staticBatches.empty(); }

    void EndStaticBuild();
    void ClearStatic();

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

extern GeometryCollector* g_geometryCollector;

}
