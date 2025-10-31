// xrRender/r_FrameGraphRenderer.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
#include "Layers/xrRender/FrameGraphPasses/LightingPass.h"
#include "Layers/xrRender/FrameGraphPasses/TonemapPass.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

namespace xray::render {

// ══════════════════════════════════════════════════════════
//  FRAMEGRAPH RENDERER
// ══════════════════════════════════════════════════════════

class FrameGraphRenderer {
public:
    FrameGraphRenderer();
    ~FrameGraphRenderer();

    // Initialize
    bool Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Main render function
    void Render();

    // Toggle FrameGraph rendering
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // Statistics
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

    // FrameGraph
    xr_unique_ptr<framegraph::FrameGraph> m_framegraph;

    // Passes
    xr_unique_ptr<passes::GBufferPass> m_gbufferPass;
    xr_unique_ptr<passes::LightingPass> m_lightingPass;
    xr_unique_ptr<passes::TonemapPass> m_tonemapPass;

    // Geometry collector
    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    // RenderContext for execution
    xr_unique_ptr<ng::RenderContext> m_renderContext;

    // Statistics
    Stats m_stats;

    // Test geometry (temporary - for pipeline verification)
    ng::BufferHandle m_testVertexBuffer;
    ng::BufferHandle m_testIndexBuffer;
    bool m_testGeometryCreated = false;

    // Frame setup
    void SetupFrame();
    void BuildFrameGraph();

    // Test geometry
    void CreateTestGeometry();
    void SubmitTestGeometry();
};

} // namespace xray::render
