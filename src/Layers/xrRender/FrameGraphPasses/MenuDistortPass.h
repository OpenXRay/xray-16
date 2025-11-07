// xrRender/FrameGraphPasses/MenuDistortPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  MENU DISTORT PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct MenuDistortPassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)

    // Clear value for distortion mask (127, 127, 0, 127) = neutral distortion
    float clearColor[4] = {127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 127.0f / 255.0f};
};

// ══════════════════════════════════════════════════════════
//  MENU DISTORT PASS (Phase 4: Render distortion to rt_MenuDistort)
// ══════════════════════════════════════════════════════════
// Renders menu post-process distortion mask to rt_MenuDistort
// This is STEP 2 of the 3-step menu rendering pipeline:
//   1. MenuUIPass:       Render UI dialogs to rt_MenuMain
//   2. MenuDistortPass:  Render distortion mask to rt_MenuDistort
//   3. MenuCompositePass: Composite both to final output

class MenuDistortPass : public framegraph::IPass {
public:
    MenuDistortPass(ng::RenderDevice* device, const MenuDistortPassConfig& config = MenuDistortPassConfig());
    ~MenuDistortPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;  // Menu rendering is a custom phase
    }

    // Set output render target (called by FrameGraphRenderer)
    void SetOutputs(framegraph::VirtualResourceHandle menuDistort, framegraph::VirtualResourceHandle depth);

    // Menu-specific statistics
    struct MenuDistortStats {
        u32 numEffects = 0;
        float cpuTimeMs = 0.0f;
    };

    const MenuDistortStats& GetMenuDistortStats() const { return m_menuDistortStats; }

private:
    ng::RenderDevice* m_device;
    MenuDistortPassConfig m_config;
    MenuDistortStats m_menuDistortStats;

    // Output render targets
    framegraph::VirtualResourceHandle m_outputRT;      // rt_MenuDistort
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth (reuse)
};

} // namespace xray::render::passes
