// SmokeTrailPassSetup.h
// FrameGraph pass setup for GPU smoke trail:
// emit CS → simulate CS → compact CS → draw (reuses trail.vs pipeline)
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "SmokeTrailManager.h"
#include "TrailPassSetup.h"  // TrailPassState (reused for draw)

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

// Persistent pass state — compute pipelines only (draw reuses TrailPassState)
struct SmokeTrailPassState
{
    nvrhi::ComputePipelineHandle  emitPipeline;
    nvrhi::ComputePipelineHandle  simPipeline;
    nvrhi::ComputePipelineHandle  compactPipeline;
    nvrhi::BindingLayoutHandle    emitLayout;      // b5 + u0(sim) + u1(state)
    nvrhi::BindingLayoutHandle    simLayout;       // b5 + u0(sim)
    nvrhi::BindingLayoutHandle    compactLayout;   // b5 + u0(compact) + u1(state) + u2(drawArgs) + u3(sim)
    nvrhi::ShaderHandle           emitCS;
    nvrhi::ShaderHandle           simCS;
    nvrhi::ShaderHandle           compactCS;
    bool initialized = false;
};

// Setup: inserts 4 passes (3 compute + 1 draw) into the framegraph.
// Draw pass reuses TrailPassState's pipeline/layout for stored-direction rendering.
framegraph::DefaultOutputLayout setupSmokeTrailPass(
    framegraph::FrameGraph&              fg,
    ng::RenderDevice*                    device,
    const framegraph::DefaultOutputLayout& inputs,
    SmokeTrailManager*                   manager,
    TrailPassState*                      trailState,  // reused for draw
    u32                                  width,
    u32                                  height,
    SmokeTrailPassState&                 state);

} // namespace xray::render::RENDER_NAMESPACE::passes
