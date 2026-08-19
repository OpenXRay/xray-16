#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes {

void setupDlssFgPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorWithUI,
    framegraph::VirtualResourceHandle hudlessColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle motionVectors,
    const Fmatrix& viewToClip,
    const Fmatrix& prevViewProj,
    const Fmatrix& currViewProj,
    u32 renderW,
    u32 renderH,
    u32 displayW,
    u32 displayH,
    bool reset);

}
