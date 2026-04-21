#pragma once
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; }
namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::RENDER_NAMESPACE::passes {

struct MotionVectorPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::IBuffer* cb = nullptr;
    bool initialized = false;
};

struct MotionVectorOutput {
    framegraph::VirtualResourceHandle motionVectors;
};

MotionVectorOutput setupMotionVectorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    u32 width, u32 height,
    MotionVectorPassState& state);

} // namespace
