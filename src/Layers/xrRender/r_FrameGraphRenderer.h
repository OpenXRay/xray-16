// xrRender/r_FrameGraphRenderer.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderPhaseCache.h"
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

    // Access RenderContext
    ng::RenderContext* GetRenderContext() const { return m_renderContext.get(); }

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

    // Shader phase cache (Week 16 - for precompilation phase detection)
    xr_unique_ptr<framegraph::ShaderPhaseCache> m_shaderPhaseCache;

    // Final output texture (for copying to backbuffer)
    framegraph::VirtualResourceHandle m_finalOutput;

    // Vanilla RT handles (created once in BuildFrameGraphStructure)
    framegraph::VirtualResourceHandle m_rt_Position;
    framegraph::VirtualResourceHandle m_rt_Normal;
    framegraph::VirtualResourceHandle m_rt_Albedo;
    framegraph::VirtualResourceHandle m_rt_Depth;
    framegraph::VirtualResourceHandle m_rt_Accumulator;
    framegraph::VirtualResourceHandle m_rt_Generic_0;
    framegraph::VirtualResourceHandle m_rt_Generic_1;
    framegraph::VirtualResourceHandle m_rt_Generic_2;
    framegraph::VirtualResourceHandle m_backbuffer;

    // Passes
    xr_unique_ptr<passes::GBufferPass> m_gbufferPass;
    xr_unique_ptr<passes::LightingPass> m_lightingPass;
    xr_unique_ptr<passes::TonemapPass> m_tonemapPass;

    // Geometry collector
    xr_unique_ptr<GeometryCollector> m_geometryCollector;

    // RenderContext for execution
    xr_unique_ptr<ng::RenderContext> m_renderContext;

    // Buffer handle cache (D3D11 buffer ptr → NVRHI handle)
    xr_map<ID3D11Buffer*, nvrhi::BufferHandle> m_bufferHandleCache;

    // Statistics
    Stats m_stats;

    // Frame setup
    void SetupFrame();

    // FrameGraph structure (called once in Initialize)
    void BuildFrameGraphStructure();

    // FrameGraph passes (called per-frame in Render)
    void SetupFrameGraphPasses();

    // Copy final output to game backbuffer
    void PresentToBackbuffer();

    // Visibility & culling (CPU-based for now, will move to GPU later)
    void CollectVisibleGeometry();

    // Helper: Create render target (DRY helper for BuildFrameGraph)
    framegraph::VirtualResourceHandle CreateRT(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format,
        bool isDepthStencil = false
    );

    // Helper functions for geometry collection
    bool ProcessVisualGeometry(dxRender_Visual* visual, const Fmatrix& worldTransform);
    void ExtractStaticLeafVisuals(dxRender_Visual* pVisual, xr_vector<dxRender_Visual*>& outLeafs);

    // ═══════════════════════════════════════════════════
    //  DYNAMIC PASS ROUTING (Week 16)
    // ═══════════════════════════════════════════════════

    // Pass registry entry
    struct PassEntry {
        framegraph::RenderPhase phase;
        xr_unique_ptr<framegraph::IPass> pass;
        xr_vector<GeometryBatch*> assignedBatches;
    };

    // Active passes for this frame (dynamically created)
    xr_vector<PassEntry> m_activePasses;

    // Scan materials to determine required phases
    xr_set<framegraph::RenderPhase> ScanRequiredPhases() const;

    // Create passes based on required phases
    void CreatePhasePass(framegraph::RenderPhase phase);

    // Route batches to appropriate passes
    void RouteBatchesToPasses();

    // Create all required passes dynamically
    void CreateAllRequiredPasses();
};

} // namespace xray::render
