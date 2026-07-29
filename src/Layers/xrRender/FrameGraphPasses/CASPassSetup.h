#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct CASPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::Format pipelineFormat = nvrhi::Format::UNKNOWN;
    bool initialized = false;
};

// LDR sharpen: input → outputTarget (usually backbuffer)
framegraph::VirtualResourceHandle setupCASPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle ldrInput,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CASPassState& state);

} // namespace
