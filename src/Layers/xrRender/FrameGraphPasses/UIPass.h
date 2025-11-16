// xrRender/FrameGraphPasses/UIPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

// Forward declarations
namespace xray::render {
    class MaterialCache;
}

namespace xray::render::framegraph {
    class VolatileConstantBufferPool;
}

namespace xray::render::ui {
    class UIRenderCollector;
    class NVRHIUIRenderer;
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  UI PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct UIPassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)

    // Clear values
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Transparent background
    float clearDepth = 1.0f;
    u8 clearStencil = 0;
};

// ══════════════════════════════════════════════════════════
//  UI PASS (Render UI sprites/widgets)
// ══════════════════════════════════════════════════════════
// Renders UI elements (sprites, backgrounds, widgets) to rt_UIMain
// This is STEP 1 of the 4-step UI rendering pipeline:
//   1. UIPass:           Render UI sprites/widgets to rt_UIMain
//   2. TextPass:         Render text/fonts on top
//   3. UIDistortPass:    Render distortion mask to rt_UIDistort
//   4. UICompositePass:  Composite all layers to final output
//
// This pass runs during BOTH menu and in-game rendering.
// If no UI geometry is present, it's a fast clear operation.

class UIPass : public framegraph::IPass {
public:
    UIPass(const UIPassConfig& config = UIPassConfig());
    ~UIPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;  // UI rendering is a custom phase
    }

    // Set output render targets (called by FrameGraphRenderer)
    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    // UI statistics
    struct UIStats {
        u32 numBatches = 0;
        float cpuTimeMs = 0.0f;
    };

    const UIStats& GetUIStats() const { return m_uiStats; }

private:
    UIPassConfig m_config;
    UIStats m_uiStats;

    // Output render targets
    framegraph::VirtualResourceHandle m_outputRT;      // rt_UIMain
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth (reuse existing depth buffer)

    // Material system (UIPass owns its own MaterialCache + VCB pool)
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;

    // NVRHI UI rendering backend
    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;
    bool m_nvrhiUIInitialized{false};
};

} // namespace xray::render::passes
