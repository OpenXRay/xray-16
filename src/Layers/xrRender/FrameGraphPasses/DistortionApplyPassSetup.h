#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct DistortionApplyPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupDistortionApplyPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle distortionRT,
    framegraph::VirtualResourceHandle worldPos,
    u32 width,
    u32 height,
    DistortionApplyPassState& state
);

} // namespace xray::render::RENDER_NAMESPACE::passes
