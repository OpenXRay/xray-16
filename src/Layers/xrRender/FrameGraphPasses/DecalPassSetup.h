#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace ng {
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
    nvrhi::BindingLayoutHandle globalsLayout;
    nvrhi::BindingLayoutHandle decalLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    bool initialized = false;
};

framegraph::DefaultOutputLayout setupDecalPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    decals::DecalManager* decalMgr,
    u32 width, u32 height,
    DecalPassState& state);

} // namespace xray::render::RENDER_NAMESPACE::passes
