#pragma once

#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/LightingMode.h"

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class FGLensFlareRender;
}

namespace xray::render::fg::passes
{
struct LensFlarePassData
{
    framegraph::VirtualResourceHandle output;
    framegraph::VirtualResourceHandle depth;
    FGLensFlareRender* renderer = nullptr;
    const LightingFrameState* lighting = nullptr;
};

framegraph::VirtualResourceHandle setupLensFlarePass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGLensFlareRender* renderer,
    const LightingFrameState* lighting = nullptr);
} // namespace xray::render::fg::passes
