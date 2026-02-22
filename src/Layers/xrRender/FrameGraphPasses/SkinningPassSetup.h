#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
    class GeometryCollector;
    class dxRender_Visual;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct SkinningPipelineVariant {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
};

struct SkinningPassState {
    SkinningPipelineVariant nonHQ;
    SkinningPipelineVariant hq1w;
    SkinningPipelineVariant hq2w;
    SkinningPipelineVariant hq3w;
    SkinningPipelineVariant hq4w;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle linearSampler;
    bool initialized = false;
};

void InitializeSkinningResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, SkinningPassState& state);

using SkinnedStatsCallback = void(*)(u32 rendered, u32 culled, void* userData);

using VisibilityByVisualCallback = u32(*)(const dxRender_Visual* visual, void* userData);

struct SkinnedVisibilityData {
    bool enabled = false;

    VisibilityByVisualCallback visibilityByVisualCallback = nullptr;
    void* visibilityByVisualUserData = nullptr;

    SkinnedStatsCallback statsCallback = nullptr;
    void* statsUserData = nullptr;
};

struct SkinningPassData {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle depth;
    ng::RenderDevice* device;
    const GeometryCollector* geometry;
    const xr_vector<GeometryBatch>* hudBatches;
    MaterialCache* materialCache;
    u32 width, height;
    framegraph::DefaultOutputLayout outputs;
    SkinnedVisibilityData visibilityData;
    SkinningPassState* passState;
};

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const SkinnedVisibilityData& visibilityData = {},
    SkinningPassState* state = nullptr
);

}
