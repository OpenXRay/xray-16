// xrRender/FrameGraphPasses/SkinningPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>

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
// Renders all skinned meshes in two phases:
//   1. World Phase - NPCs, monsters, etc. with normal depth [0.0, 1.0]
//   2. HUD Phase - First-person weapons/hands with compressed depth [0.9, 1.0]
//
// This pass consolidates all skinned mesh rendering that was previously split
// between ForwardColorPass (world skinned) and HUDPass (HUD skinned).

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

struct SkinningPassData {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
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

// Main skinning pass setup function
// Renders world skinned meshes followed by HUD skinned meshes
framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,       // Contains world skinned batches
    const xr_vector<GeometryBatch>* hudBatches,  // HUD skinned batches (weapons, hands)
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling = nullptr,
    framegraph::VirtualResourceHandle skinnedDrawArgs = {},
    SkinningPassState* state = nullptr,
    decals::OverlayManager* overlayMgr = nullptr
);

} // namespace xray::render::fg::passes
