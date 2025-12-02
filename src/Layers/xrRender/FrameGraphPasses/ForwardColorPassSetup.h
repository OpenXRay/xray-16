// xrRender/FrameGraphPasses/ForwardColorPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

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
// GPU CULLING INTEGRATION (Phase 3.5):
// - When drawArgsBuffer is valid, uses DrawIndexedIndirect for GPU-driven rendering
// - Draw args buffer is passed through FrameGraph for proper dependency tracking
// - This ensures correct state transition (UAV -> IndirectArgument)
// - Falls back to standard DrawIndexed if drawArgsBuffer is invalid
//
// ═══════════════════════════════════════════════════════
//  BINDLESS FORWARD CONFIG
// ═══════════════════════════════════════════════════════
// Configuration for bindless GPU-driven rendering mode

struct BindlessForwardConfig {
    // Compact draw args buffer (only visible batches)
    nvrhi::IBuffer* compactDrawArgsBuffer = nullptr;

    // Material IDs parallel to compact draw args
    nvrhi::IBuffer* compactMaterialIDBuffer = nullptr;

    // Number of visible objects from previous frame (for multi-draw count)
    u32 previousFrameVisibleCount = 0;

    // Enable bindless rendering mode
    bool enabled = false;

    // ═══════════════════════════════════════════════════════
    //  MEGA-BUFFER SYSTEM (GPU-Driven Rendering)
    // ═══════════════════════════════════════════════════════
    // Unified vertex/index buffers containing all level geometry
    // When available, render path uses these instead of per-batch buffers

    nvrhi::IBuffer* megaVertexBuffer = nullptr;   // UnifiedVertex format (48-byte)
    nvrhi::IBuffer* megaIndexBuffer = nullptr;    // 32-bit indices
    bool megaBuffersReady = false;

    bool IsValid() const {
        return enabled && compactDrawArgsBuffer && compactMaterialIDBuffer && previousFrameVisibleCount > 0;
    }

    bool UseMegaBuffers() const {
        return megaBuffersReady && megaVertexBuffer && megaIndexBuffer;
    }
};

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
    u32 height,                                    // Render target height
    framegraph::VirtualResourceHandle drawArgsBuffer = framegraph::VirtualResourceHandle(),  // GPU culling draw args (optional)
    const BindlessForwardConfig& bindlessConfig = BindlessForwardConfig()  // Bindless rendering config
);

} // namespace xray::render::passes
