#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DebugDrawPassData {
    framegraph::VirtualResourceHandle output;
    u32 width  = 0;
    u32 height = 0;
};

framegraph::VirtualResourceHandle setupDebugDrawPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    u32 width,
    u32 height);

}
