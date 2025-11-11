// xrRender/FrameGraphPasses/MenuCompositePass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  MENU COMPOSITE PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct MenuCompositePassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)
};

// ══════════════════════════════════════════════════════════
//  MENU COMPOSITE PASS (Phase 5: Composite menu RTs to output)
// ══════════════════════════════════════════════════════════
// Composites rt_MenuMain + rt_MenuDistort to final output
// This is STEP 3 of the 3-step menu rendering pipeline:
//   1. MenuUIPass:       Render UI dialogs to rt_MenuMain
//   2. MenuDistortPass:  Render distortion mask to rt_MenuDistort
//   3. MenuCompositePass: Composite both to final output
//
// Uses fullscreen triangle with alpha-blend shader (industry standard optimization)

class MenuCompositePass : public framegraph::IPass {
public:
    MenuCompositePass(ng::RenderDevice* device, const MenuCompositePassConfig& config = MenuCompositePassConfig());
    ~MenuCompositePass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::PostProcess;  // Compositing is post-process
    }

    // Set input/output render targets (called by FrameGraphRenderer)
    void SetInputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle menuDistort);
    void SetOutput(framegraph::VirtualResourceHandle finalOutput);

    // Menu-specific statistics
    struct MenuCompositeStats {
        float cpuTimeMs = 0.0f;
    };

    const MenuCompositeStats& GetMenuCompositeStats() const { return m_menuCompositeStats; }

private:
    ng::RenderDevice* m_device;
    MenuCompositePassConfig m_config;
    MenuCompositeStats m_menuCompositeStats;

    // Input render targets (textures to sample from)
    framegraph::VirtualResourceHandle m_inputScene;        // Base 3D scene layer
    framegraph::VirtualResourceHandle m_inputUI;           // UI layer with alpha
    framegraph::VirtualResourceHandle m_inputMenuMain;     // Legacy (for compatibility)
    framegraph::VirtualResourceHandle m_inputMenuDistort;  // Legacy (for compatibility)

    // Output render target (composite result)
    framegraph::VirtualResourceHandle m_outputRT;  // Final output

    // Rendering resources for alpha-blend compositing
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BindingSetHandle m_bindingSet;  // Updated per-frame with textures
    nvrhi::SamplerHandle m_linearSampler;

    bool m_initialized = false;
    bool Initialize();
};

} // namespace xray::render::passes
