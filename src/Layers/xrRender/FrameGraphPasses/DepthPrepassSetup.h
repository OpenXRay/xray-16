#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

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

struct BindlessDepthConfig {
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    bool megaBuffersReady = false;

    bool UseMegaBuffers() const {
        return megaBuffersReady && megaVertexBuffer && megaIndexBuffer;
    }
};

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const BindlessDepthConfig& bindlessConfig = {}
);

}
