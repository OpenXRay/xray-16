#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

class UIPassData
{
public:
    framegraph::VirtualResourceHandle target;
    u32 width = 0;
    u32 height = 0;
};

struct CursorPassData {
    framegraph::VirtualResourceHandle uiTarget;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle interfaceLayer,
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
