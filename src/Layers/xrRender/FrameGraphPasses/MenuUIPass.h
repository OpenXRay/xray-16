#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

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

struct MenuUIPassConfig {
    u32 width = 0;
    u32 height = 0;

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth = 1.0f;
    u8 clearStencil = 0;
};

class MenuUIPass : public framegraph::IPass {
public:
    MenuUIPass(ng::RenderDevice* device, const MenuUIPassConfig& config = MenuUIPassConfig());
    ~MenuUIPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;
    }

    void SetOutputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle depth);

    struct MenuStats {
        u32 numDialogs = 0;
        float cpuTimeMs = 0.0f;
    };

    const MenuStats& GetMenuStats() const { return m_menuStats; }

private:
    ng::RenderDevice* m_device;
    MenuUIPassConfig m_config;
    MenuStats m_menuStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;

    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;
    bool m_nvrhiUIInitialized{false};
};

}
