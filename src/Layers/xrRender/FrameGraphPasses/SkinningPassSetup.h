#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>
#include "SunShadowPassSetup.h"
#include "Layers/xrRender/Decals/OverlayManager.h"

namespace xray::render {
    struct GeometryBatch;
    class MaterialCache;
    class GeometryCollector;
    namespace fg {
        class dxRender_Visual;
        class RenderDevice;
        class GPUCullingManager;
    }
}

namespace xray::render::fg::decals {
    class OverlayManager;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

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
    nvrhi::BindingLayoutHandle hudLayout;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle hudPS;
    SkinningPipelineVariant mdiNonHQ;
    SkinningPipelineVariant mdiHQ1w;
    SkinningPipelineVariant mdiHQ2w;
    SkinningPipelineVariant mdiHQ3w;
    SkinningPipelineVariant mdiHQ4w;
    nvrhi::BindingLayoutHandle mdiLayout;
    nvrhi::ShaderHandle mdiPS;
    SkinningPipelineVariant hudNonHQ;
    SkinningPipelineVariant hudHQ1w;
    SkinningPipelineVariant hudHQ2w;
    SkinningPipelineVariant hudHQ3w;
    SkinningPipelineVariant hudHQ4w;
    nvrhi::SamplerHandle linearSampler;
    bool initialized = false;
};

void InitializeSkinningResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, SkinningPassState& state);
decals::OverlayManager::SplatRange GetSplatRange(const GeometryBatch& batch, decals::OverlayManager* overlayMgr);

struct SkinningPassData {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle skinnedDrawArgs;
    fg::RenderDevice* device;
    const GeometryCollector* geometry;
    const xr_vector<GeometryBatch>* hudBatches;
    MaterialCache* materialCache;
    fg::GPUCullingManager* gpuCulling;
    u32 width, height;
    framegraph::DefaultOutputLayout outputs;
    SkinningPassState* passState;
    decals::OverlayManager* overlayMgr;
};

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling = nullptr,
    framegraph::VirtualResourceHandle skinnedDrawArgs = {},
    SkinningPassState* state = nullptr,
    decals::OverlayManager* overlayMgr = nullptr
);

}
