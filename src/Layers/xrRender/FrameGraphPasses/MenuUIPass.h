// xrRender/FrameGraphPasses/MenuUIPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  MENU UI PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct MenuUIPassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)

    // Clear values
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // Black background
    float clearDepth = 1.0f;
    u8 clearStencil = 0;
};

// ══════════════════════════════════════════════════════════
//  MENU UI PASS (Phase 3: Render legacy UI to rt_MenuMain)
// ══════════════════════════════════════════════════════════
// Renders main menu UI elements to rt_MenuMain
// This is STEP 1 of the 3-step menu rendering pipeline:
//   1. MenuUIPass:       Render UI dialogs to rt_MenuMain
//   2. MenuDistortPass:  Render distortion mask to rt_MenuDistort
//   3. MenuCompositePass: Composite both to final output

class MenuUIPass : public framegraph::IPass {
public:
    MenuUIPass(ng::RenderDevice* device, const MenuUIPassConfig& config = MenuUIPassConfig());
    ~MenuUIPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;  // Menu rendering is a custom phase
    }

    // Set output render targets (called by FrameGraphRenderer)
    void SetOutputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle depth);

    // Menu-specific statistics
    struct MenuStats {
        u32 numDialogs = 0;
        float cpuTimeMs = 0.0f;
    };

    const MenuStats& GetMenuStats() const { return m_menuStats; }

private:
    ng::RenderDevice* m_device;
    MenuUIPassConfig m_config;
    MenuStats m_menuStats;

    // Output render targets
    framegraph::VirtualResourceHandle m_outputRT;      // rt_MenuMain
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth (reuse existing depth buffer)
};

} // namespace xray::render::passes
