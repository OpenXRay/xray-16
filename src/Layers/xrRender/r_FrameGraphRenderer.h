#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderPhaseCache.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
#include "Layers/xrRender/FrameGraphPasses/LightingPass.h"
#include "Layers/xrRender/FrameGraphPasses/TonemapPass.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

struct Fmatrix;
namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
}

namespace xray::render {
using RENDER_NAMESPACE::dxRender_Visual;

class FrameGraphRenderer {
public:
    FrameGraphRenderer();
    ~FrameGraphRenderer();

    bool Initialize(ng::RenderDevice* device);
    void Shutdown();

    void Render();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    struct Stats {
        float totalFrameMs = 0.0f;
        float gbufferMs = 0.0f;
        float lightingMs = 0.0f;
        float tonemapMs = 0.0f;
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void PrintStats() const;

private:
    bool m_enabled = false;
    ng::RenderDevice* m_device = nullptr;

    xr_unique_ptr<framegraph::FrameGraph> m_framegraph;

    xr_unique_ptr<framegraph::ShaderPhaseCache> m_shaderPhaseCache;

    framegraph::VirtualResourceHandle m_finalOutput;

    xr_unique_ptr<passes::GBufferPass> m_gbufferPass;
    xr_unique_ptr<passes::LightingPass> m_lightingPass;
    xr_unique_ptr<passes::TonemapPass> m_tonemapPass;

    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    xr_unique_ptr<ng::RenderContext> m_renderContext;

    Stats m_stats;

    void SetupFrame();
    void BuildFrameGraph();

    void PresentToBackbuffer();

    void CollectVisibleGeometry();

    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform);
    void ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs);

    struct PassEntry {
        framegraph::RenderPhase phase;
        xr_unique_ptr<framegraph::IPass> pass;
        xr_vector<GeometryBatch*> assignedBatches;
    };

    xr_vector<PassEntry> m_activePasses;

    xr_set<framegraph::RenderPhase> ScanRequiredPhases() const;

    void CreatePhasePass(framegraph::RenderPhase phase);

    void RouteBatchesToPasses();

    void CreateAllRequiredPasses();
};

}
