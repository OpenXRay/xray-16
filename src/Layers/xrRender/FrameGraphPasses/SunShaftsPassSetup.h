#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
struct ExtractedReflection;
}

namespace xray::render::fg
{
class RenderDevice;
}

namespace xray::render::fg::passes
{

struct SunShaftsPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle combinePipeline;
    nvrhi::BindingLayoutHandle combineLayout;
    nvrhi::TextureHandle depthCopy;
    framegraph::ExtractedReflection* marchVsReflection = nullptr;
    framegraph::ExtractedReflection* marchPsReflection = nullptr;
    framegraph::ExtractedReflection* combinePsReflection = nullptr;
    bool initialized = false;
    u32 pipeVersion = 0;
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

float ResolveSunShaftsIntensity();

} // namespace xray::render::fg::passes
