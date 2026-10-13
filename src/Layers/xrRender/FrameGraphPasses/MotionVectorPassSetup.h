#pragma once
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ViewFrame.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; }
namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::fg::passes {

struct MotionVectorPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::IBuffer* cb = nullptr;
    nvrhi::TextureHandle dummyVisId;
    bool initialized = false;
};

struct MotionVectorOutput {
    framegraph::VirtualResourceHandle motionVectors;
};

MotionVectorOutput setupMotionVectorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle visId,
    framegraph::VirtualResourceHandle visDepth,
    framegraph::VirtualResourceHandle motionVectors,
    const ViewFrame& view,
    MotionVectorPassState& state);

}
