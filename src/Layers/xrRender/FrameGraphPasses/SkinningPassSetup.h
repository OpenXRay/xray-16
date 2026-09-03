// xrRender/FrameGraphPasses/SkinningPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
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

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNING PASS - Consolidated skinned mesh rendering
// ═══════════════════════════════════════════════════════════════════════════

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
    SkinningPipelineVariant mdi;
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
    MaterialCache* materialCache;
    fg::GPUCullingManager* gpuCulling;
    u32 width, height;
    framegraph::DefaultOutputLayout outputs;
    SkinningPassState* passState;
    decals::OverlayManager* overlayMgr;
};

// Main skinning pass setup function
framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,       // Contains world skinned batches
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling = nullptr,
    framegraph::VirtualResourceHandle skinnedDrawArgs = {},
    SkinningPassState* state = nullptr,
    decals::OverlayManager* overlayMgr = nullptr
);

} // namespace xray::render::fg::passes
