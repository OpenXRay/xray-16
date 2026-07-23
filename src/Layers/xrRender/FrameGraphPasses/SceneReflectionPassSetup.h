#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

// Snapshot of fully-lit scene color BEFORE wet/AO darkening.
// This is the shared reflection source for:
//   - wet surface specular (screen-space + sky IBL fallback)
//   - fullscreen SSR
// Named RT: "rt_SceneReflection"
framegraph::VirtualResourceHandle setupSceneReflectionCapture(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle litSceneColor,
    u32 width,
    u32 height);

} // namespace
