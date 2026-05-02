#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct UIPassData {
    framegraph::VirtualResourceHandle sceneInput;
    framegraph::VirtualResourceHandle sceneOutput;
    u32 width;
    u32 height;
};

struct CursorPassData {
    framegraph::VirtualResourceHandle uiTarget;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneTarget,
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