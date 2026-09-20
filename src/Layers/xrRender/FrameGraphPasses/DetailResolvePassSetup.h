#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "MaterialResolvePassSetup.h"
#include "DetailCullPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class FGDetailManager;
    class RenderDevice;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DetailResolvePassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle sourceLayout;
    nvrhi::ShaderHandle shader;
    bool initialized = false;
    bool failed = false;
};

bool EnsureDetailResolveResources(fg::RenderDevice* device, DetailResolvePassState& state,
    const DetailPassResources& details);

MaterialResolveOutput setupDetailResolvePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle visId,
    framegraph::VirtualResourceHandle depth,
    const MaterialResolveOutput& inputs,
    fg::FGDetailManager* detailManager,
    const DetailPassResources& details,
    const Fmatrix& prevView,
    const Fmatrix& prevProj,
    bool motionValid,
    float prevTime,
    u32 width,
    u32 height,
    DetailResolvePassState* state);

}
