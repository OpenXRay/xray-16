#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct SSGIPassState
{
    nvrhi::GraphicsPipelineHandle tracePipeline;
    nvrhi::GraphicsPipelineHandle blurPipeline;
    nvrhi::GraphicsPipelineHandle temporalPipeline;
    nvrhi::GraphicsPipelineHandle applyPipeline;
    nvrhi::BindingLayoutHandle traceLayout;
    nvrhi::BindingLayoutHandle blurLayout;
    nvrhi::BindingLayoutHandle temporalLayout;
    nvrhi::BindingLayoutHandle applyLayout;
    nvrhi::TextureHandle colorCopy;
    nvrhi::TextureHandle history[2];
    u32 historyW = 0;
    u32 historyH = 0;
    u32 historyIndex = 0;
    bool hasHistory = false;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupSSGIPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    SSGIPassState& state);

} // namespace
