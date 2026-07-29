#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct SSSSSPassState
{
    nvrhi::GraphicsPipelineHandle blurPipeline;
    nvrhi::BindingLayoutHandle blurLayout;
    u32 pipeVersion = 0;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupSSSSSPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    u32 width,
    u32 height,
    SSSSSPassState& state);

} // namespace
