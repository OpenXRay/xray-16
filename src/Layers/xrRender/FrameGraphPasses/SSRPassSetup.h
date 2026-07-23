#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct SSRPassState
{
    nvrhi::GraphicsPipelineHandle pipeline; // resolve (SSR buffer only)
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle blurPipeline;
    nvrhi::BindingLayoutHandle blurLayout;
    nvrhi::GraphicsPipelineHandle applyPipeline;
    nvrhi::BindingLayoutHandle applyLayout;
    nvrhi::TextureHandle colorCopy;  // pre-wet reflection source
    nvrhi::TextureHandle litCopy;    // post-wet lit scene for apply (never R/W same as out)
    nvrhi::TextureHandle depthCopy;  // typeless D32 SRV copy (Metal-safe, like water SSR)
    nvrhi::TextureHandle outputTex;  // persistent composite out (never FG-aliased)
    nvrhi::TextureHandle blurTex;    // persistent blur RT
    nvrhi::TextureHandle history[2];
    u32 historyW = 0;
    u32 historyH = 0;
    u32 historyIndex = 0;
    u32 pipeVersion = 0;
    bool hasHistory = false;
    bool historyNeedsClear = false;
    bool initialized = false;
};

// sceneColor: composite target (may be post-wet/AO)
// reflectionColor: lit scene to sample for reflections (pre-wet)
framegraph::VirtualResourceHandle setupSSRPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle reflectionColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle worldPos,
    framegraph::VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    SSRPassState& state);

} // namespace
