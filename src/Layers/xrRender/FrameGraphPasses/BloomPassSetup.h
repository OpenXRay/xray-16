#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
}

namespace xray::render::fg::passes
{
class BloomPassState
{
public:
    nvrhi::ComputePipelineHandle downsamplePipeline;
    nvrhi::BindingLayoutHandle downsampleLayout;
    nvrhi::ComputePipelineHandle upsamplePipeline;
    nvrhi::BindingLayoutHandle upsampleLayout;
    bool pipelinesFailed = false;
};

class BloomConstants
{
public:
    Fvector2 sourceTexelSize = {};
    u32 targetWidth = 0;
    u32 targetHeight = 0;
    u32 firstLevel = 0;
    u32 pad0 = 0;
    u32 pad1 = 0;
    u32 pad2 = 0;
};

class BloomPassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle exposure;
    framegraph::VirtualResourceHandle bloom;
    fg::RenderDevice* device = nullptr;
    BloomPassState* state = nullptr;
    u32 width = 0;
    u32 height = 0;
    u32 levels = 0;
};

class BloomOutput
{
public:
    framegraph::VirtualResourceHandle texture;
    u32 levels = 0;
};

u32 CalculateBloomLevels(u32 width, u32 height);

BloomOutput setupBloomPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle exposure,
    u32 width,
    u32 height,
    BloomPassState& state);
}
