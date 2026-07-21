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
    SkinningPipelineVariant hudNonHQ;
    SkinningPipelineVariant hudHQ1w;
    SkinningPipelineVariant hudHQ2w;
    SkinningPipelineVariant hudHQ3w;
    SkinningPipelineVariant hudHQ4w;
    nvrhi::SamplerHandle linearSampler;
    bool initialized = false;
};

void InitializeSkinningResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, SkinningPassState& state);

// Callback to update skinned culling stats
using SkinnedStatsCallback = void(*)(u32 rendered, u32 culled, void* userData);

// Callback type for visual-based visibility lookup (handles batch reordering)
using VisibilityByVisualCallback = u32(*)(const dxRender_Visual* visual, void* userData);

// Skinned mesh visibility data (from GPU culling)
struct SkinnedVisibilityData {
    bool enabled = false;  // Whether GPU culling is active

    // Visual-based visibility lookup (handles batch reordering correctly)
    VisibilityByVisualCallback visibilityByVisualCallback = nullptr;
    void* visibilityByVisualUserData = nullptr;

    // Stats callback (called at end of skinning pass)
    SkinnedStatsCallback statsCallback = nullptr;
    void* statsUserData = nullptr;
};

struct SkinningPassData {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle depth;
    fg::RenderDevice* device;
    const GeometryCollector* geometry;
    const xr_vector<GeometryBatch>* hudBatches;
    MaterialCache* materialCache;
    u32 width, height;
    framegraph::DefaultOutputLayout outputs;
    SkinnedVisibilityData visibilityData;
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
    const SkinnedVisibilityData& visibilityData = {},
    SkinningPassState* state = nullptr,
    decals::OverlayManager* overlayMgr = nullptr
);

} // namespace xray::render::fg::passes
