#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h"
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

struct ShadowHZBPassState
{
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::TextureHandle hzb[kCSMCascadeCount];
    u32 cascadeRes[kCSMCascadeCount] = {};
    u32 mipLevels[kCSMCascadeCount] = {};
    bool initialized = false;
    bool computeEnabled = false;
};

struct ShadowHZBOutput
{
    nvrhi::ITexture* hzb[kCSMCascadeCount] = {};
    framegraph::VirtualResourceHandle handles[kCSMCascadeCount];
    u32 mipLevels[kCSMCascadeCount] = {};
    bool valid = false;
};

ShadowHZBOutput setupShadowHZBPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    ShadowPassState& shadowState,
    ShadowHZBPassState& state,
    framegraph::VirtualResourceHandle shadowMapHandle = {});

} // namespace xray::render::fg::passes
