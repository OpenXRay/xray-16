#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class FGEnvironmentRender;
}

namespace xray::render::fg::passes {

struct SkyPassData {
    framegraph::VirtualResourceHandle colorOutput;
    framegraph::VirtualResourceHandle depthOutput;
    FGEnvironmentRender* renderer;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    FGEnvironmentRender* renderer,
    u32 width,
    u32 height
);

} // namespace xray::render::fg::passes
