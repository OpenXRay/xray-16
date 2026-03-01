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

struct RibbonPassState {
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

RibbonPassOutput setupRibbonPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state = nullptr
);

}
