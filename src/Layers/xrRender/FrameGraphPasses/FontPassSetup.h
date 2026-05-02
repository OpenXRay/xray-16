#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupFontPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget);

}
