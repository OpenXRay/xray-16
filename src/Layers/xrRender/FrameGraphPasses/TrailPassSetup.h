// xrRender/FrameGraphPasses/TrailPassSetup.h
// Trail renderer — Stride ShapeBuilderTrail parity.
// Per-control-point stored direction vector, no camera dependency.
// Parallel to RibbonPassSetup.h (which implements ShapeBuilderRibbon).
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "RibbonPassSetup.h"  // Shared enums: RibbonSmoothingMode, RibbonUVPolicy, RibbonUVTransform

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
//  Trail-specific enums (Stride ShapeBuilderTrail)
// ═══════════════════════════════════════════════════════

enum class TrailEdgePolicy : u8 {
    Edge,     // Position is one edge, direction points to the other edge
    Center,   // Position is center, direction is half-width both sides
};

// ═══════════════════════════════════════════════════════
//  Data structures
// ═══════════════════════════════════════════════════════

static constexpr u32 TRAIL_MAX_POINTS = 256;
static constexpr u32 TRAIL_SUBDIVISIONS = 12;
static constexpr float TRAIL_MIN_SEGMENT_DIST = 0.025f;

// CPU-side trail point — includes 3D direction vector (Stride: ParticleFields.Direction)
struct TrailPoint {
    Fvector position;
    Fvector direction;   // Pre-scaled width direction (world-space, magnitude = halfWidth)
    float age;           // Seconds since emission
    u32 order;           // Upper 16 = group ID, lower 16 = spawn order
};

// GPU control point — uploaded to StructuredBuffer (32 bytes, all floats, Vulkan std430 safe)
// Direction is PRE-SCALED to half-width (Stride parity: directions[i] = dir * particleSize).
// VS reads and uses directly — no separate halfWidth field needed.
struct GPUTrailControlPoint {
    float posX, posY, posZ;
    float dirX, dirY, dirZ;  // Pre-scaled width direction (world-space, magnitude = halfWidth)
    float ageNorm;            // age / maxAge (0=head, 1=tail)
    float cumDist;            // Cumulative distance from head
};
static_assert(sizeof(GPUTrailControlPoint) == 32, "GPUTrailControlPoint must be 32 bytes");

// Constant buffer matching trail.vs cbuffer TrailParams at b5
struct TrailParamsCB {
    // Row 0
    u32   controlPointCount;
    u32   subdivisions;
    float texCoordsFactor;
    u32   uvPolicy;              // 0=DistanceBased, 1=Stretched, 2=AsIs

    // Row 1
    float totalDist;
    u32   enableTailFade;
    u32   smoothingMode;         // 0=CatmullRom, 1=Circumcircle
    u32   edgePolicy;            // 0=Edge, 1=Center

    // Row 2
    u32   flipX;
    u32   flipY;
    u32   rotate90;
    u32   materialID;

    // Row 3
    u32   useGPUState;           // 1 = read liveCount/totalDist from t11 state buffer
    float turbAmount;            // per-instance turbulence displacement amount
    float turbFrequency;         // spatial frequency (1/size)
    float turbEvolution;         // time parameter (4th noise dimension)

    // Row 4
    float sphereCenterX;
    float sphereCenterY;
    float sphereCenterZ;
    float sphereRadius;

    // Row 5 (reserved)
    u32   pad5_0, pad5_1, pad5_2, pad5_3;
};
static_assert(sizeof(TrailParamsCB) == 96, "TrailParamsCB must be 96 bytes (6 rows x 16)");

struct TrailPassState {
    // GPU resources
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    nvrhi::BufferHandle controlPointBuffer;
    nvrhi::BufferHandle dummyStateBuffer;   // Tiny buffer for t11 when CPU-driven (useGPUState=0)
    u32 controlPointCapacity = 0;
    bool initialized = false;

    void ResetGPUState()
    {
        pipeline = nullptr;
        layout = nullptr;
        vs = nullptr;
        ps = nullptr;
        sampler = nullptr;
        controlPointBuffer = nullptr;
        dummyStateBuffer = nullptr;
        controlPointCapacity = 0;
        initialized = false;
    }

    // CPU-side trail state (persists across frames)
    TrailPoint points[TRAIL_MAX_POINTS] = {};
    u32 pointCount = 0;
    float maxAge = 3.0f;

    // Configuration
    float defaultHalfWidth = 0.15f;
    float texCoordsFactor = 1.0f;
    RibbonSmoothingMode smoothing = RibbonSmoothingMode::Circumcircle;
    RibbonUVPolicy uvPolicy = RibbonUVPolicy::DistanceBased;
    RibbonUVTransform uvTransform;
    bool enableTailFade = true;
    TrailEdgePolicy edgePolicy = TrailEdgePolicy::Center;

    // Order field tracking
    u16 currentGroupID = 0;
    u16 nextSpawnOrder = 0;
};

struct TrailPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle depth;
    fg::RenderDevice* device;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    TrailPassState* passState;
};

struct TrailPassOutput {
    framegraph::DefaultOutputLayout layout;
};

void InitializeTrailResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TrailPassState& state);

// ═══════════════════════════════════════════════════════
//  TRAIL PASS SETUP
// ═══════════════════════════════════════════════════════
// Renders a trail: GPU-generated quad strip from control points.
// VS reads StructuredBuffer, does Catmull-Rom subdivision + stored direction width.
// Supports Stride-compatible smoothing, UV policies, EdgePolicy, and group splitting.
TrailPassOutput setupTrailPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    TrailPassState* state = nullptr
);

} // namespace xray::render::fg::passes
