// SmokeTrailPassSetup.h
// FrameGraph pass setup for GPU smoke trail:
// emit CS → simulate CS → compact CS → draw (smoke_trail.vs)
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "SmokeTrailManager.h"
#include "TrailPassSetup.h"  // TrailParamsCB, shared structs

namespace xray::render {
namespace ng {
    class RenderDevice;
}
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Persistent pass state — compute pipelines + dedicated draw pipeline
struct SmokeTrailPassState
{
    // Compute
    nvrhi::ComputePipelineHandle  emitPipeline;
    nvrhi::ComputePipelineHandle  simPipeline;
    nvrhi::ComputePipelineHandle  compactPipeline;
    nvrhi::BindingLayoutHandle    emitLayout;      // b5 + u0(sim) + u1(state)
    nvrhi::BindingLayoutHandle    simLayout;       // b5 + u0(sim)
    nvrhi::BindingLayoutHandle    compactLayout;   // b5 + u0(compact) + u1(state) + u2(drawArgs) + u3(sim)
    nvrhi::ShaderHandle           emitCS;
    nvrhi::ShaderHandle           simCS;
    nvrhi::ShaderHandle           compactCS;

    // Draw (own VS + PS pipeline — smoke_trail.vs, smoke_trail.ps)
    nvrhi::GraphicsPipelineHandle drawPipeline;
    nvrhi::BindingLayoutHandle    drawLayout;
    nvrhi::ShaderHandle           drawVS;
    nvrhi::ShaderHandle           drawPS;
    nvrhi::SamplerHandle          sampler;

    bool initialized = false;
};

// Setup: inserts 4 passes (3 compute + 1 draw) into the framegraph.
framegraph::DefaultOutputLayout setupSmokeTrailPass(
    framegraph::FrameGraph&              fg,
    ng::RenderDevice*                    device,
    const framegraph::DefaultOutputLayout& inputs,
    SmokeTrailManager*                   manager,
    u32                                  width,
    u32                                  height,
    SmokeTrailPassState&                 state,
    nvrhi::ITexture*                     perlin4dVolume = nullptr);

} // namespace xray::render::RENDER_NAMESPACE::passes
