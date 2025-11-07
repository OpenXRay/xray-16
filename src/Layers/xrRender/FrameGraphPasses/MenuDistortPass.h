#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::passes {

struct MenuDistortPassConfig {
    u32 width = 0;
    u32 height = 0;

    float clearColor[4] = {127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 127.0f / 255.0f};
};

class MenuDistortPass : public framegraph::IPass {
public:
    MenuDistortPass(ng::RenderDevice* device, const MenuDistortPassConfig& config = MenuDistortPassConfig());
    ~MenuDistortPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;
    }

    void SetOutputs(framegraph::VirtualResourceHandle menuDistort, framegraph::VirtualResourceHandle depth);

    struct MenuDistortStats {
        u32 numEffects = 0;
        float cpuTimeMs = 0.0f;
    };

    const MenuDistortStats& GetMenuDistortStats() const { return m_menuDistortStats; }

private:
    ng::RenderDevice* m_device;
    MenuDistortPassConfig m_config;
    MenuDistortStats m_menuDistortStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;
};

}
