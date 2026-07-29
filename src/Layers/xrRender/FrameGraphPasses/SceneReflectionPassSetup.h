#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct SceneReflectionPassState
{
    nvrhi::TextureHandle texture;
    u32 width = 0;
    u32 height = 0;
};

framegraph::VirtualResourceHandle setupSceneReflectionCapture(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle litSceneColor,
    u32 width,
    u32 height,
    SceneReflectionPassState& state);

} // namespace
