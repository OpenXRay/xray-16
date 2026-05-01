#pragma once

#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"

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
};

framegraph::VirtualResourceHandle setupLensFlarePass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGLensFlareRender* renderer);
} // namespace xray::render::fg::passes
