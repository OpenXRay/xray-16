// xrRender/FrameGraphPasses/RibbonPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

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

// GPU control point — uploaded to StructuredBuffer (24 bytes, no float3, Vulkan std430 safe)
struct GPURibbonControlPoint {
    float posX, posY, posZ;
    float halfWidth;
    float ageNorm;   // age / maxAge (0=head, 1=tail)
    float cumDist;   // cumulative distance from head
};
static_assert(sizeof(GPURibbonControlPoint) == 24, "GPURibbonControlPoint must be 24 bytes");

// Constant buffer matching ribbon.vs cbuffer RibbonParams at b5
struct RibbonParamsCB {
    // Row 0
    u32   controlPointCount;
    u32   subdivisions;
    float texCoordsFactor;
    u32   uvPolicy;

    // Row 1
    float totalDist;
    u32   enableTailFade;
    u32   smoothingMode;
    u32   useScreenSpaceWidth;

    // Row 2
    u32   flipX;
    u32   flipY;
    u32   rotate90;
    u32   materialID;

    // Row 3: InvView X (camera right), w unused
    Fvector4 invViewX;

    // Row 4: InvView Y (camera up), w unused
    Fvector4 invViewY;
};
static_assert(sizeof(RibbonParamsCB) == 80, "RibbonParamsCB must be 80 bytes (5 rows x 16)");

struct RibbonPassState {
    // GPU resources
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    nvrhi::BufferHandle controlPointBuffer;
    u32 controlPointCapacity = 0;
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
    fg::RenderDevice* device;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    RibbonPassState* passState;
};

struct RibbonPassOutput {
    framegraph::DefaultOutputLayout layout;
};

void InitializeRibbonResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, RibbonPassState& state);

// ═══════════════════════════════════════════════════════
//  RIBBON PASS SETUP
// ═══════════════════════════════════════════════════════
// Renders a ribbon trail: GPU-generated quad strip from control points.
// VS reads StructuredBuffer, does Catmull-Rom subdivision + camera-facing width.
// Supports Stride-compatible smoothing, UV policies, and group splitting.
RibbonPassOutput setupRibbonPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state = nullptr
);

} // namespace xray::render::fg::passes
