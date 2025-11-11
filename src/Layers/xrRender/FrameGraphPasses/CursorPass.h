// xrRender/FrameGraphPasses/CursorPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render {
    class MaterialCache;
    namespace ui {
        class UIRenderCollector;
        class NVRHIUIRenderer;
    }
    namespace framegraph {
        class VolatileConstantBufferPool;
    }
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  CURSOR PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct CursorPassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)
};

// ══════════════════════════════════════════════════════════
//  CURSOR PASS (Render mouse cursor on top of all UI)
// ══════════════════════════════════════════════════════════
// Renders mouse cursor on top of all UI and text layers
// This is STEP 3 of the 4-step UI rendering pipeline:
//   1. UIPass:           Render UI sprites/widgets to rt_MenuMain
//   2. TextPass:         Render text/fonts on top
//   3. CursorPass:       Render cursor on top (THIS PASS)
//   4. MenuCompositePass: Composite UI layer over scene
//
// This pass:
// - Collects cursor geometry from g_pGamePersistent->OnRenderCursor()
// - Renders cursor using NVRHIUIRenderer (same as UIPass)
// - Renders to rt_MenuMain with alpha blending (no clear)
// - Ensures cursor appears on top of all UI elements and text

class CursorPass : public framegraph::IPass {
public:
    CursorPass(ng::RenderDevice* device, const CursorPassConfig& config = CursorPassConfig());
    ~CursorPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;  // Cursor rendering is a custom phase
    }

    // Set output render targets (called by FrameGraphRenderer)
    // NOTE: CursorPass renders ON TOP of TextPass output (same RT)
    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    // Cursor rendering statistics
    struct CursorStats {
        u32 numBatches = 0;
        float cpuTimeMs = 0.0f;
    };

    const CursorStats& GetCursorStats() const { return m_cursorStats; }

private:
    // ═══════════════════════════════════════════════════════
    //  MEMBER VARIABLES
    // ═══════════════════════════════════════════════════════

    ng::RenderDevice* m_device;
    CursorPassConfig m_config;
    CursorStats m_cursorStats;

    // Output render targets (same as UIPass/TextPass - we composite on top)
    framegraph::VirtualResourceHandle m_outputRT;      // rt_MenuMain
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth

    // UI rendering infrastructure (same as UIPass)
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;
    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;

    bool m_nvrhiUIInitialized{false};
};

} // namespace xray::render::passes
