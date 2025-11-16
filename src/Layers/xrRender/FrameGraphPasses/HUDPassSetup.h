// xrRender/FrameGraphPasses/HUDPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::passes {

// Lambda-based HUD pass setup
// Renders HUD geometry (weapons, world-space UI) on top of GBuffer
framegraph::DefaultOutputLayout setupHUDPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& gbufferInputs,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

} // namespace xray::render::passes