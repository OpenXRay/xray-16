#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/RainGPUSimManager.h"

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class FGRainRender; }

namespace xray::render::fg::passes {

struct RainPassData {
    framegraph::VirtualResourceHandle output;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle rainSM;
    FGRainRender* renderer = nullptr;
    RainHeightmapInfo heightmap{};
    Fmatrix rainSampleVP;
    bool rainSMValid = false;
};

framegraph::VirtualResourceHandle setupRainPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle worldPosTarget,
    framegraph::VirtualResourceHandle rainSMTarget,
    FGRainRender* renderer,
    const Fmatrix& rainSampleVP,
    bool rainSMValid,
    const RainHeightmapInfo& heightmap);

}
