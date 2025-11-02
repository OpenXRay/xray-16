// xrRender/r_FrameGraphRenderer.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
#include "Layers/xrRender/FrameGraphPasses/LightingPass.h"
#include "Layers/xrRender/FrameGraphPasses/TonemapPass.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

// Forward declarations
struct Fmatrix;
namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
}

namespace xray::render {
using RENDER_NAMESPACE::dxRender_Visual;

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

    // Final output texture (for copying to backbuffer)
    framegraph::VirtualResourceHandle m_finalOutput;

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

    // Frame setup
    void SetupFrame();
    void BuildFrameGraph();

    // Copy final output to game backbuffer
    void PresentToBackbuffer();

    // Visibility & culling (CPU-based for now, will move to GPU later)
    void CollectVisibleGeometry();

    // Helper functions for geometry collection
    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform);
    void ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs);
};

} // namespace xray::render
