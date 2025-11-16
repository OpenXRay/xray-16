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

struct CursorPassConfig {
    u32 width = 0;
    u32 height = 0;
};

class CursorPass : public framegraph::IPass {
public:
    CursorPass(const CursorPassConfig& config = CursorPassConfig());
    ~CursorPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;
    }

    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    struct CursorStats {
        u32 numBatches = 0;
        float cpuTimeMs = 0.0f;
    };

    const CursorStats& GetCursorStats() const { return m_cursorStats; }

private:

    
    CursorPassConfig m_config;
    CursorStats m_cursorStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;
    xr_unique_ptr<ui::UIRenderCollector> m_uiCollector;
    xr_unique_ptr<ui::NVRHIUIRenderer> m_uiRenderer;

    bool m_nvrhiUIInitialized{false};
};

}
