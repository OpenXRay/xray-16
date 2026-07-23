#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::fg::passes {

struct WetSurfacesPassState {
    nvrhi::GraphicsPipelineHandle pipeline;      // apply
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle patchPipeline;
    nvrhi::BindingLayoutHandle patchLayout;
    nvrhi::GraphicsPipelineHandle writeNormalPipeline;
    nvrhi::BindingLayoutHandle writeNormalLayout;
    bool initialized = false;
    u32 pipeVersion = 0;
};

void InitializeWetSurfacesPass(nvrhi::IDevice* device, WetSurfacesPassState& state);

// Phase 1 wet + phase 2 SSR source (rt_SceneReflection, non-aliased).
struct WetSurfacesExtras
{
    framegraph::VirtualResourceHandle rainSM;
    nvrhi::ITexture* rainSMTex = nullptr;
    Fmatrix rainSampleVP;
    bool rainSMValid = false;

    framegraph::VirtualResourceHandle sceneReflection;
    bool sceneReflectionValid = false;
};

framegraph::DefaultOutputLayout setupWetSurfacesPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    WetSurfacesPassState& state,
    const WetSurfacesExtras& extras = {});

} // namespace xray::render::fg::passes
