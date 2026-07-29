#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
class GeometryCollector;
class MaterialCache;
namespace fg {
class RenderDevice;
}
}

namespace xray::render::framegraph {
class FrameGraph;
}

namespace xray::render::fg::passes {

struct DepthPrepassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::GraphicsPipelineHandle tessPipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::BindingLayoutHandle tessLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::InputLayoutHandle tessInputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle terrainPs;
    nvrhi::ShaderHandle tessVs;
    nvrhi::ShaderHandle tessHs;
    nvrhi::ShaderHandle tessDs;
    nvrhi::ShaderHandle tessPs;
    bool initialized = false;
};

// Opaque depth fill for early-Z in Forward+. Returns the depth handle written.
framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const BindlessForwardConfig& bindlessConfig,
    DepthPrepassState* state,
    framegraph::VirtualResourceHandle cullDrawArgs = {});

} // namespace xray::render::fg::passes
