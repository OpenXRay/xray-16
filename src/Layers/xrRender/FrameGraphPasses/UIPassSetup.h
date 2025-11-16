#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::passes {

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
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

framegraph::VirtualResourceHandle setupCompositePass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneInput,
    framegraph::VirtualResourceHandle uiInput,
    u32 width,
    u32 height
);

}