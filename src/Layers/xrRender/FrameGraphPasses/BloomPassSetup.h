#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

static constexpr u32 kBloomMips = 5;

struct BloomPassState
{
    nvrhi::GraphicsPipelineHandle extractPipe;
    nvrhi::GraphicsPipelineHandle downPipe;
    nvrhi::GraphicsPipelineHandle upPipe;
    nvrhi::GraphicsPipelineHandle compPipe;
    nvrhi::BindingLayoutHandle extractLayout;
    nvrhi::BindingLayoutHandle downLayout;
    nvrhi::BindingLayoutHandle upLayout;
    nvrhi::BindingLayoutHandle compLayout;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupBloomPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    u32 width,
    u32 height,
    BloomPassState& state);

} // namespace
