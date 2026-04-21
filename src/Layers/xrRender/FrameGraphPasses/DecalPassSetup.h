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

namespace xray::render::RENDER_NAMESPACE::decals {
    class DecalManager;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct DecalPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    bool initialized = false;
};

framegraph::DefaultOutputLayout setupDecalPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    decals::DecalManager* decalMgr,
    u32 width, u32 height,
    DecalPassState& state);

} // namespace xray::render::RENDER_NAMESPACE::passes
