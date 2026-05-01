#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class FGStatGraphRender;
}

namespace xray::render::fg::passes {

struct StatGraphPassData {
    framegraph::VirtualResourceHandle output;
    FGStatGraphRender* renderer = nullptr;
    u32 width  = 0;
    u32 height = 0;
};

framegraph::VirtualResourceHandle setupStatGraphPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    FGStatGraphRender* renderer,
    u32 width,
    u32 height);

}
