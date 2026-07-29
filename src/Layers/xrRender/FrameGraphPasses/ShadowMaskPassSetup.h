#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowHZBPassSetup.h"
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

struct ShadowMaskPassState
{
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::TextureHandle mask;
    u32 width = 0;
    u32 height = 0;
    bool initialized = false;
    bool computeEnabled = false;
};

struct ShadowMaskOutput
{
    nvrhi::ITexture* mask = nullptr;
    framegraph::VirtualResourceHandle handle;
    bool valid = false;
};

ShadowMaskOutput setupShadowMaskPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    u32 fullWidth,
    u32 fullHeight,
    const BindlessForwardConfig& bindlessConfig,
    const ShadowHZBOutput& hzb,
    ShadowMaskPassState& state);

} // namespace xray::render::fg::passes
