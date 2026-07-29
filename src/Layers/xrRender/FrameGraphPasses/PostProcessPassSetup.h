#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; class CRenderTarget; }

namespace xray::render::fg::passes
{

struct PostProcessPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::Format pipelineFormat = nvrhi::Format::UNKNOWN;
    nvrhi::TextureHandle noisePlaceholder;
    nvrhi::ITexture* noiseTexture = nullptr;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupPostProcessPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle ldrInput,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CRenderTarget* target,
    PostProcessPassState& state);

} // namespace
