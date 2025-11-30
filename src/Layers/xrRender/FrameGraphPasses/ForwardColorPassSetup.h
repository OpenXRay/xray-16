// xrRender/FrameGraphPasses/ForwardColorPassSetup.h
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

// ═══════════════════════════════════════════════════════
//  FORWARD COLOR PASS (Phase 1: Single-RT Forward Rendering)
// ═══════════════════════════════════════════════════════
//
// CHANGES FROM GBUFFER PASS:
// - Single HDR color buffer (RGBA16_FLOAT) instead of 3 RTs
// - 60% bandwidth reduction (128 bits/pixel → 64 bits/pixel)
// - Simpler, cleaner architecture
// - Foundation for Forward+ lighting (Phase 3+)
//
// Lambda-based ForwardColorPass setup function (Frostbite pattern)
// Replaces the wasteful 3-RT G-buffer with single color output
// NOTE: Does NOT clear color buffer - sky pass renders background first
framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,                      // Render device for buffer access
    framegraph::VirtualResourceHandle depthInput,  // Existing depth buffer from prepass
    framegraph::VirtualResourceHandle colorInput,  // Color buffer from sky pass
    const GeometryCollector* geometry,             // Geometry batches to render
    MaterialCache* materialCache,                  // Material cache for shaders
    u32 width,                                     // Render target width
    u32 height                                     // Render target height
);

} // namespace xray::render::passes
