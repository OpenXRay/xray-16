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

struct AmbientOcclusionPassState {
    nvrhi::GraphicsPipelineHandle ssaoPipeline;
    nvrhi::GraphicsPipelineHandle hbaoPipeline;
    nvrhi::GraphicsPipelineHandle hdaoPipeline;
    nvrhi::GraphicsPipelineHandle gtaoRenderPipeline;
    nvrhi::GraphicsPipelineHandle gtaoFilterPipeline;
    nvrhi::GraphicsPipelineHandle blurPipeline;
    nvrhi::GraphicsPipelineHandle applyPipeline;

    nvrhi::BindingLayoutHandle genLayout;      // depth+normal+worldPos+AOConstants
    nvrhi::BindingLayoutHandle gtaoRenderLayout;
    nvrhi::BindingLayoutHandle gtaoFilterLayout;
    nvrhi::BindingLayoutHandle blurLayout;
    nvrhi::BindingLayoutHandle applyLayout;

    bool initialized = false;
};

void InitializeAmbientOcclusionPass(nvrhi::IDevice* device, AmbientOcclusionPassState& state);

framegraph::VirtualResourceHandle setupAmbientOcclusionPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle worldPos,
    u32 width,
    u32 height,
    AmbientOcclusionPassState& state);

} // namespace xray::render::fg::passes
