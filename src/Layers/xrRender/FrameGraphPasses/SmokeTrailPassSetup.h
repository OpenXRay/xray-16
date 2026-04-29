#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "SmokeTrailManager.h"
#include "TrailPassSetup.h"

namespace xray::render {
namespace fg {
    class RenderDevice;
}
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::fg::passes {

struct SmokeTrailPassState
{
    nvrhi::ComputePipelineHandle  emitPipeline;
    nvrhi::ComputePipelineHandle  simPipeline;
    nvrhi::ComputePipelineHandle  compactPipeline;
    nvrhi::BindingLayoutHandle    emitLayout;
    nvrhi::BindingLayoutHandle    simLayout;
    nvrhi::BindingLayoutHandle    compactLayout;
    nvrhi::ShaderHandle           emitCS;
    nvrhi::ShaderHandle           simCS;
    nvrhi::ShaderHandle           compactCS;

    nvrhi::GraphicsPipelineHandle drawPipeline;
    nvrhi::BindingLayoutHandle    drawLayout;
    nvrhi::ShaderHandle           drawVS;
    nvrhi::ShaderHandle           drawPS;
    nvrhi::SamplerHandle          sampler;

    bool initialized = false;
};

framegraph::DefaultOutputLayout setupSmokeTrailPass(
    framegraph::FrameGraph&              fg,
    fg::RenderDevice*                    device,
    const framegraph::DefaultOutputLayout& inputs,
    SmokeTrailManager*                   manager,
    u32                                  width,
    u32                                  height,
    SmokeTrailPassState&                 state,
    nvrhi::ITexture*                     perlin4dVolume = nullptr);

}
