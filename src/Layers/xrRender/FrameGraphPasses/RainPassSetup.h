#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/LightingMode.h"

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class FGRainRender; }

namespace xray::render::fg::passes {

struct RainPassData {
    framegraph::VirtualResourceHandle output;
    framegraph::VirtualResourceHandle depth;
    FGRainRender* renderer = nullptr;
    const LightingFrameState* lighting = nullptr;
};

framegraph::VirtualResourceHandle setupRainPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget,
    FGRainRender* renderer,
    const LightingFrameState* lighting = nullptr);

}
