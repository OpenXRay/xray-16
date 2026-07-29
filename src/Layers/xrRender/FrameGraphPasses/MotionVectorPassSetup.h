#pragma once
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; }
namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::fg::passes {

struct MotionVectorPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::IBuffer* cb = nullptr;
    bool initialized = false;
    u32 pipeVersion = 0;
    Fvector prevCameraPos = {0, 0, 0};
    bool hasPrevCamera = false;
};

struct MotionVectorOutput {
    framegraph::VirtualResourceHandle motionVectors;
};

MotionVectorOutput setupMotionVectorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle worldPosInput,
    const Fmatrix& viewProj,
    const Fmatrix& prevViewProj,
    u32 width, u32 height,
    MotionVectorPassState& state);

} // namespace
