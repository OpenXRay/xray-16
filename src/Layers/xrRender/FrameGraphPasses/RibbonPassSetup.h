// xrRender/FrameGraphPasses/RibbonPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "PassVertexFormats.h"

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
//  Configuration enums (matching Stride ShapeBuilderRibbon)
// ═══════════════════════════════════════════════════════

enum class RibbonSmoothingMode : u8 {
    CatmullRom,       // Standard Catmull-Rom spline (Stride: Fast)
    Circumcircle,     // Circumcircle arc blending  (Stride: Best)
};

enum class RibbonUVPolicy : u8 {
    DistanceBased,    // V accumulates arc-length distance * texCoordsFactor
    Stretched,        // V = pointIndex / totalPoints * texCoordsFactor
    AsIs,             // V resets 0->1 per original segment * texCoordsFactor
};

struct RibbonUVTransform {
    bool flipX = false;
    bool flipY = false;
    bool rotate90 = false;   // swap U and V after flip
};

// ═══════════════════════════════════════════════════════
//  Data structures
// ═══════════════════════════════════════════════════════

static constexpr u32 RIBBON_MAX_POINTS = 256;
static constexpr u32 RIBBON_SUBDIVISIONS = 12;
static constexpr float RIBBON_MIN_SEGMENT_DIST = 0.025f;

struct RibbonPoint {
    Fvector position;
    float age;       // Seconds since emission
    float size;      // Per-point half-width (world units)
    u32 order;       // Upper 16 = group ID, lower 16 = spawn order
};

struct RibbonPassState {
    // GPU resources
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    nvrhi::BufferHandle ribbonVB;
    u32 ribbonVBCapacity = 0;
    nvrhi::BufferHandle ribbonIB;
    u32 ribbonIBCapacity = 0;
    bool initialized = false;

    // CPU-side trail state (persists across frames)
    RibbonPoint points[RIBBON_MAX_POINTS] = {};
    u32 pointCount = 0;
    float maxAge = 3.0f;

    // Configuration
    float defaultHalfWidth = 0.15f;
    float texCoordsFactor = 1.0f;
    RibbonSmoothingMode smoothing = RibbonSmoothingMode::Circumcircle;
    RibbonUVPolicy uvPolicy = RibbonUVPolicy::DistanceBased;
    RibbonUVTransform uvTransform;
    bool enableTailFade = true;
    bool useScreenSpaceWidth = false;

    // Order field tracking
    u16 currentGroupID = 0;
    u16 nextSpawnOrder = 0;
};

struct RibbonPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle depth;
    ng::RenderDevice* device;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    RibbonPassState* passState;
};

struct RibbonPassOutput {
    framegraph::DefaultOutputLayout layout;
};

void InitializeRibbonResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, RibbonPassState& state);

// ═══════════════════════════════════════════════════════
//  RIBBON PASS SETUP
// ═══════════════════════════════════════════════════════
// Renders a ribbon trail: camera-facing quad strip connecting
// control points emitted over time. Points age out after maxAge seconds.
// Supports Stride-compatible smoothing, UV policies, and group splitting.
RibbonPassOutput setupRibbonPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state = nullptr
);

} // namespace xray::render::RENDER_NAMESPACE::passes
