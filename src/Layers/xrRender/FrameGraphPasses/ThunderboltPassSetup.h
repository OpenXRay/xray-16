#pragma once

#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class FGThunderboltRender;
}

namespace xray::render::fg::passes
{
struct ThunderboltPassData
{
    framegraph::VirtualResourceHandle output;
    framegraph::VirtualResourceHandle depth;
    FGThunderboltRender* renderer = nullptr;
};

framegraph::VirtualResourceHandle setupThunderboltPass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGThunderboltRender* renderer);
} // namespace xray::render::fg::passes
