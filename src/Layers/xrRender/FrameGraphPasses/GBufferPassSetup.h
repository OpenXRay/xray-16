// xrRender/FrameGraphPasses/GBufferPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

// Forward declarations
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

namespace xray::render::RENDER_NAMESPACE::passes {

// Lambda-based GBufferPass setup function (Frostbite pattern)
// This replaces the class-based GBufferPass with a free function
// that uses lambdas for setup and execute phases
framegraph::DefaultOutputLayout setupGBufferPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,                      // Render device for buffer access
    framegraph::VirtualResourceHandle depthInput,  // Optional existing depth buffer
    const GeometryCollector* geometry,             // Geometry batches to render
    MaterialCache* materialCache,                  // Material cache for shaders
    u32 width,                                     // Render target width
    u32 height                                     // Render target height
);

} // namespace xray::render::passes