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

framegraph::DefaultOutputLayout setupHUDPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& gbufferInputs,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

}