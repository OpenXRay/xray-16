#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

framegraph::VirtualResourceHandle setupDistortionApplyPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneSnapshot,
    framegraph::VirtualResourceHandle distortionRT,
    framegraph::VirtualResourceHandle worldPos,
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes
