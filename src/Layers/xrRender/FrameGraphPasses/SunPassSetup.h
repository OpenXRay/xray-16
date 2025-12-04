// xrRender/FrameGraphPasses/SunPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

// Forward declarations
class CEnvironment;

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════
//  SUN PASS (Sun disc rendering)
// ═══════════════════════════════════════════════════════
//
// Renders the sun disc as a camera-facing billboard quad.
// Uses the CLensFlare system for position/color calculation.
// Runs AFTER sky pass with additive blending.
//
// Components:
// - Sun source (bright disc)
// - Gradient (glow around sun)
// - Lens flares (optional, along sun-screen axis)

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,  // Color buffer from sky pass
    CEnvironment* environment,
    u32 width,
    u32 height
);

// Initialize sun pass resources (call once at startup)
void InitializeSunPass(ng::RenderDevice* device);

// Shutdown sun pass resources
void ShutdownSunPass();

} // namespace xray::render::RENDER_NAMESPACE::passes
