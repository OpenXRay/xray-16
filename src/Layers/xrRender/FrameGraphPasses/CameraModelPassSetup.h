#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct CameraModelPassState
{
    nvrhi::GraphicsPipelineHandle cameraPipeline;
    nvrhi::BindingLayoutHandle cameraLayout;
    nvrhi::GraphicsPipelineHandle mblurPipeline;
    nvrhi::BindingLayoutHandle mblurLayout;
    nvrhi::Format pipelineFormat = nvrhi::Format::UNKNOWN;
    bool initialized = false;
};

framegraph::VirtualResourceHandle setupCameraModelPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle ldrInput,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CameraModelPassState& state);

} // namespace
