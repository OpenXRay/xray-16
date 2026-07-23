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

struct SunPassData {
    framegraph::VirtualResourceHandle colorOutput;
    framegraph::VirtualResourceHandle depth;
    FGEnvironmentRender* renderer;
    u32 width;
    u32 height;
};

// Draw sun disc with depth test so geometry occludes it (classic behaviour).
framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    FGEnvironmentRender* renderer,
    u32 width,
    u32 height
);

} // namespace xray::render::fg::passes
