#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::passes {

framegraph::DefaultOutputLayout setupGBufferPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

}