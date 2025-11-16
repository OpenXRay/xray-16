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

struct UIPassConfig {
    u32 width = 0;
    u32 height = 0;

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float clearDepth = 1.0f;
    u8 clearStencil = 0;
};

class UIPass : public framegraph::IPass {
public:
    UIPass(const UIPassConfig& config = UIPassConfig());
    ~UIPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;
    }

    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    struct UIStats {
        u32 numBatches = 0;
        float cpuTimeMs = 0.0f;
    };

    const UIStats& GetUIStats() const { return m_uiStats; }

private:
    UIPassConfig m_config;
    UIStats m_uiStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;

    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;
    bool m_nvrhiUIInitialized{false};
};

}
