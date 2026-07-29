#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct DofPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupDofPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    DofPassState& state);

} // namespace
