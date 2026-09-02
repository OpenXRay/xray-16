#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

enum class RibbonSmoothingMode : u8 {
    CatmullRom,
    Circumcircle,
};

enum class RibbonUVPolicy : u8 {
    DistanceBased,
    Stretched,
    AsIs,
};

struct RibbonUVTransform {
    bool flipX = false;
    bool flipY = false;
    bool rotate90 = false;
};

static constexpr u32 RIBBON_MAX_POINTS = 256;
static constexpr u32 RIBBON_SUBDIVISIONS = 12;
static constexpr float RIBBON_MIN_SEGMENT_DIST = 0.025f;

struct RibbonPoint {
    Fvector position;
    float age;
    float size;
    u32 order;
};

struct GPURibbonControlPoint {
    float posX, posY, posZ;
    float halfWidth;
    float ageNorm;
    float cumDist;
};
static_assert(sizeof(GPURibbonControlPoint) == 24, "GPURibbonControlPoint must be 24 bytes");

struct RibbonParamsCB {
    u32   controlPointCount;
    u32   subdivisions;
    float texCoordsFactor;
    u32   uvPolicy;

    float totalDist;
    u32   enableTailFade;
    u32   smoothingMode;
    u32   useScreenSpaceWidth;

    u32   flipX;
    u32   flipY;
    u32   rotate90;
    u32   materialID;

    Fvector4 invViewX;

    Fvector4 invViewY;
};
static_assert(sizeof(RibbonParamsCB) == 80, "RibbonParamsCB must be 80 bytes (5 rows x 16)");

struct RibbonPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    nvrhi::BufferHandle controlPointBuffer;
    u32 controlPointCapacity = 0;
    bool initialized = false;

    RibbonPoint points[RIBBON_MAX_POINTS] = {};
    u32 pointCount = 0;
    float maxAge = 3.0f;

    float defaultHalfWidth = 0.15f;
    float texCoordsFactor = 1.0f;
    RibbonSmoothingMode smoothing = RibbonSmoothingMode::Circumcircle;
    RibbonUVPolicy uvPolicy = RibbonUVPolicy::DistanceBased;
    RibbonUVTransform uvTransform;
    bool enableTailFade = true;
    bool useScreenSpaceWidth = false;

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

RibbonPassOutput setupRibbonPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state = nullptr
);

}
