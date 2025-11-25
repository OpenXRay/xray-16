#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneTarget,
    u32 width,
    u32 height
);

framegraph::VirtualResourceHandle setupTextPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height
);

framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height
);

}