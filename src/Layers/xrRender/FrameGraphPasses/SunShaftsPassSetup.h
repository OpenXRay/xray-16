#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
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

struct SunShaftsPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;       // half-res march (shafts-only)
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle combinePipeline; // full-res upsample + add to scene
    nvrhi::BindingLayoutHandle combineLayout;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupSunShaftsPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle worldPos,
    nvrhi::ITexture* shadowMapArray,
    framegraph::VirtualResourceHandle shadowMapHandle,
    u32 width,
    u32 height,
    SunShaftsPassState& state,
    nvrhi::ITexture* const* shadowCascades = nullptr);

void InitializeSunShaftsPass(nvrhi::IDevice* device, SunShaftsPassState& state);

} // namespace xray::render::fg::passes
