#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::passes {

struct MenuCompositePassConfig {
    u32 width = 0;
    u32 height = 0;
};

class MenuCompositePass : public framegraph::IPass {
public:
    MenuCompositePass(const MenuCompositePassConfig& config = MenuCompositePassConfig());
    ~MenuCompositePass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::PostProcess;
    }

    void SetInputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle menuDistort);
    void SetOutput(framegraph::VirtualResourceHandle finalOutput);

    struct MenuCompositeStats {
        float cpuTimeMs = 0.0f;
    };

    const MenuCompositeStats& GetMenuCompositeStats() const { return m_menuCompositeStats; }

private:
    
    MenuCompositePassConfig m_config;
    MenuCompositeStats m_menuCompositeStats;

    framegraph::VirtualResourceHandle m_inputScene;
    framegraph::VirtualResourceHandle m_inputUI;
    framegraph::VirtualResourceHandle m_inputMenuMain;
    framegraph::VirtualResourceHandle m_inputMenuDistort;

    framegraph::VirtualResourceHandle m_outputRT;

    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BindingSetHandle m_bindingSet;
    nvrhi::SamplerHandle m_linearSampler;

    bool m_initialized = false;
    bool Initialize();
};

}
