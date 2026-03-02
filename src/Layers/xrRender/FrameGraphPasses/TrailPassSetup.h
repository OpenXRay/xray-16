#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "RibbonPassSetup.h"

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

enum class TrailEdgePolicy : u8 {
    Edge,
    Center,
};

static constexpr u32 TRAIL_MAX_POINTS = 256;
static constexpr u32 TRAIL_SUBDIVISIONS = 12;
static constexpr float TRAIL_MIN_SEGMENT_DIST = 0.025f;

struct TrailPoint {
    Fvector position;
    Fvector direction;
    float age;
    u32 order;
};

struct GPUTrailControlPoint {
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float ageNorm;
    float cumDist;
};
static_assert(sizeof(GPUTrailControlPoint) == 32, "GPUTrailControlPoint must be 32 bytes");

struct TrailParamsCB {
    u32   controlPointCount;
    u32   subdivisions;
    float texCoordsFactor;
    u32   uvPolicy;

    float totalDist;
    u32   enableTailFade;
    u32   smoothingMode;
    u32   edgePolicy;

    u32   flipX;
    u32   flipY;
    u32   rotate90;
    u32   materialID;

    u32   useGPUState;
    u32   pad0, pad1, pad2;
};
static_assert(sizeof(TrailParamsCB) == 64, "TrailParamsCB must be 64 bytes (4 rows x 16)");

struct TrailPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    nvrhi::BufferHandle controlPointBuffer;
    nvrhi::BufferHandle dummyStateBuffer;
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

    TrailPoint points[TRAIL_MAX_POINTS] = {};
    u32 pointCount = 0;
    float maxAge = 3.0f;

    float defaultHalfWidth = 0.15f;
    float texCoordsFactor = 1.0f;
    RibbonSmoothingMode smoothing = RibbonSmoothingMode::Circumcircle;
    RibbonUVPolicy uvPolicy = RibbonUVPolicy::DistanceBased;
    RibbonUVTransform uvTransform;
    bool enableTailFade = true;
    TrailEdgePolicy edgePolicy = TrailEdgePolicy::Center;

    u16 currentGroupID = 0;
    u16 nextSpawnOrder = 0;
};

struct TrailPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle depth;
    ng::RenderDevice* device;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    TrailPassState* passState;
};

struct TrailPassOutput {
    framegraph::DefaultOutputLayout layout;
};

void InitializeTrailResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, TrailPassState& state);

TrailPassOutput setupTrailPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    TrailPassState* state = nullptr
);

}
