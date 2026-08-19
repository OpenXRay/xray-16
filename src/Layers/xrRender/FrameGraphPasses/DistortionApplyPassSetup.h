#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::fg::passes {

struct DistortionApplyPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::ITexture* waterUnderColor = nullptr;
    bool initialized = false;
    u32 version = 0;
};

void InitializeDistortionApplyPass(nvrhi::IDevice* device, DistortionApplyPassState& state);

framegraph::VirtualResourceHandle setupDistortionApplyPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle distortionRT,
    framegraph::VirtualResourceHandle worldPos,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    DistortionApplyPassState& state
);

} // namespace xray::render::fg::passes
