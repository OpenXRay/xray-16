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
    // World-only PatchList variants (same VS/PS, + bindless_skinned_tess HS/DS).
    SkinningPipelineVariant tessNonHQ;
    SkinningPipelineVariant tessHQ1w;
    SkinningPipelineVariant tessHQ2w;
    SkinningPipelineVariant tessHQ3w;
    SkinningPipelineVariant tessHQ4w;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle hudLayout;
    nvrhi::BindingLayoutHandle hudScopeLayout;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle hudPS;
    nvrhi::ShaderHandle hudScopePS;
    nvrhi::ShaderHandle tessHS;
    nvrhi::ShaderHandle tessDS;
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
    SkinningPipelineVariant hudScopeNonHQ;
    SkinningPipelineVariant hudScopeHQ1w;
    SkinningPipelineVariant hudScopeHQ2w;
    SkinningPipelineVariant hudScopeHQ3w;
    SkinningPipelineVariant hudScopeHQ4w;
    SkinningPipelineVariant depthNonHQ;
    SkinningPipelineVariant depthHQ1w;
    SkinningPipelineVariant depthHQ2w;
    SkinningPipelineVariant depthHQ3w;
    SkinningPipelineVariant depthHQ4w;
    nvrhi::BindingLayoutHandle depthLayout;
    nvrhi::ShaderHandle depthPS;
    SkinningPipelineVariant velNonHQ;
    SkinningPipelineVariant velHQ1w;
    SkinningPipelineVariant velHQ2w;
    SkinningPipelineVariant velHQ3w;
    SkinningPipelineVariant velHQ4w;
    SkinningPipelineVariant velMdiNonHQ;
    SkinningPipelineVariant velMdiHQ1w;
    SkinningPipelineVariant velMdiHQ2w;
    SkinningPipelineVariant velMdiHQ3w;
    SkinningPipelineVariant velMdiHQ4w;
    nvrhi::BindingLayoutHandle velocityLayout;
    nvrhi::BindingLayoutHandle velocityMdiLayout;
    nvrhi::ShaderHandle velocityPS;
    nvrhi::TextureHandle secondVP;
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
    framegraph::VirtualResourceHandle shadowMap;
    fg::RenderDevice* device;
    const GeometryCollector* geometry;
    const xr_vector<GeometryBatch>* hudBatches;
    MaterialCache* materialCache;
    fg::GPUCullingManager* gpuCulling;
    u32 width, height;
    framegraph::DefaultOutputLayout outputs;
    SkinningPassState* passState;
    decals::OverlayManager* overlayMgr;
    nvrhi::ITexture* shadowMapArray = nullptr;
    nvrhi::ITexture* shadowCascades[3] = {};
    nvrhi::ITexture* hudShadowMap = nullptr;
    nvrhi::ITexture* localShadowAtlas = nullptr;
    nvrhi::ITexture* localShadowESM = nullptr;
    nvrhi::ITexture* shadowHZB[3] = {};
    nvrhi::ITexture* shadowMask = nullptr;
    nvrhi::ITexture* contactDepth = nullptr;
    nvrhi::ITexture* contactHistory = nullptr;
    nvrhi::ITexture* envSky0 = nullptr;
    nvrhi::ITexture* envSky1 = nullptr;
};

framegraph::VirtualResourceHandle setupSkinnedDepthPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    decals::OverlayManager* overlayMgr = nullptr
);

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,       // Contains world skinned batches
    const xr_vector<GeometryBatch>* hudBatches,  // unused when HUD is deferred; pass nullptr
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling = nullptr,
    framegraph::VirtualResourceHandle skinnedDrawArgs = {},
    SkinningPassState* state = nullptr,
    decals::OverlayManager* overlayMgr = nullptr,
    nvrhi::ITexture* shadowMapArray = nullptr,
    framegraph::VirtualResourceHandle shadowMapHandle = {},
    nvrhi::ITexture* contactDepth = nullptr,
    nvrhi::ITexture* contactHistory = nullptr,
    nvrhi::ITexture* envSky0 = nullptr,
    nvrhi::ITexture* envSky1 = nullptr,
    nvrhi::ITexture* hudShadowMap = nullptr,
    nvrhi::ITexture* const* shadowCascades = nullptr,
    nvrhi::ITexture* localShadowAtlas = nullptr,
    nvrhi::ITexture* const* shadowHZB = nullptr,
    nvrhi::ITexture* shadowMask = nullptr,
    nvrhi::ITexture* localShadowESM = nullptr
);

framegraph::VirtualResourceHandle setupHudOverlayPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle worldPos,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    nvrhi::ITexture* shadowMapArray = nullptr,
    nvrhi::ITexture* contactDepth = nullptr,
    nvrhi::ITexture* contactHistory = nullptr,
    nvrhi::ITexture* envSky0 = nullptr,
    nvrhi::ITexture* envSky1 = nullptr,
    nvrhi::ITexture* hudShadowMap = nullptr,
    nvrhi::ITexture* const* shadowCascades = nullptr,
    nvrhi::ITexture* localShadowAtlas = nullptr,
    nvrhi::ITexture* const* shadowHZB = nullptr,
    nvrhi::ITexture* shadowMask = nullptr,
    nvrhi::ITexture* localShadowESM = nullptr,
    bool rtgiGuidePass = false
);

framegraph::VirtualResourceHandle setupSkinnedVelocityPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle depth,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    const Fmatrix& viewProj,
    const Fmatrix& prevViewProj,
    u32 width,
    u32 height
);

} // namespace xray::render::fg::passes
