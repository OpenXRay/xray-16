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
}

namespace xray::render::RENDER_NAMESPACE::passes {

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

}
