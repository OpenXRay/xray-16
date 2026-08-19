#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace fg {
    class RenderDevice;
}
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::fg::decals {
    class DecalManager;
}

namespace xray::render::fg::passes {

struct DecalPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    bool initialized = false;
    u32 pipeVersion = 0;
    u32 colorFormat = 0;
};

framegraph::DefaultOutputLayout setupDecalPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    decals::DecalManager* decalMgr,
    u32 width, u32 height,
    DecalPassState& state);

} // namespace xray::render::fg::passes
