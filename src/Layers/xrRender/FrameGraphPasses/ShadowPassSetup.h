#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
}

namespace xray::render
{
class MaterialCache;
}

namespace xray::render::fg
{
class FGDetailManager;
}

namespace xray::render::fg::passes
{

static constexpr u32 kCSMCascadeCount = 3;

// Defined in ShadowPassSetup.cpp — reads r2_smap_size (classic default 2048).
u32 GetCSMResolution();
// Per-cascade ladder: c0=full, c1=half, c2=quarter (clamped ≥512).
u32 GetCSMCascadeResolution(u32 cascade);

struct alignas(16) ShadowCascadeCB
{
    Fmatrix lightVP;
};
static_assert(sizeof(ShadowCascadeCB) % 16 == 0);

struct alignas(16) GrassShadowCB
{
    float grassBladeHeight;
    u32 buildDetailsIndex; // bindless atlas for billboard alpha clip
    float pad0, pad1;
};
static_assert(sizeof(GrassShadowCB) % 16 == 0);

struct ShadowPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle foliagePipeline; // transparent-set foliage caster (reuses layout)
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::GraphicsPipelineHandle grassPipeline;
    nvrhi::BindingLayoutHandle grassLayout;
    nvrhi::GraphicsPipelineHandle billboardGrassPipeline;
    nvrhi::BindingLayoutHandle billboardGrassLayout;
    nvrhi::GraphicsPipelineHandle skinnedPipeline;
    nvrhi::BindingLayoutHandle skinnedLayout;
    nvrhi::GraphicsPipelineHandle skinned4wPipeline;
    nvrhi::BindingLayoutHandle skinned4wLayout;
    nvrhi::GraphicsPipelineHandle skinnedHqPipeline; // 1-bone HQ (36B)
    nvrhi::BindingLayoutHandle skinnedHqLayout;
    nvrhi::GraphicsPipelineHandle skinned2wPipeline; // 2-bone HQ (44B)
    nvrhi::BindingLayoutHandle skinned2wLayout;
    nvrhi::GraphicsPipelineHandle skinned3wPipeline; // 3-bone HQ (44B)
    nvrhi::BindingLayoutHandle skinned3wLayout;
    nvrhi::GraphicsPipelineHandle skinnedMdiPipeline[6];
    nvrhi::BindingLayoutHandle skinnedMdiLayout;
    nvrhi::InputLayoutHandle skinnedMdiInputLayout[6];
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::InputLayoutHandle grassInputLayout;
    nvrhi::InputLayoutHandle skinnedInputLayout;
    nvrhi::InputLayoutHandle skinned4wInputLayout;
    nvrhi::InputLayoutHandle skinnedHqInputLayout;
    nvrhi::InputLayoutHandle skinned2wInputLayout;
    nvrhi::InputLayoutHandle skinned3wInputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle foliagePs;
    nvrhi::ShaderHandle terrainPs;
    nvrhi::ShaderHandle grassVs;
    nvrhi::ShaderHandle grassPs;
    nvrhi::ShaderHandle billboardGrassVs;
    nvrhi::ShaderHandle billboardGrassPs;
    nvrhi::ShaderHandle skinnedVs;
    nvrhi::ShaderHandle skinned4wVs;
    nvrhi::ShaderHandle skinnedHqVs;
    nvrhi::ShaderHandle skinned2wVs;
    nvrhi::ShaderHandle skinned3wVs;
    nvrhi::BufferHandle cascadeCB;
    nvrhi::BufferHandle grassCB;
    u32 cascadeCBMaxVersions = 0; // tracks volatile version budget (CSM+local tiles)

    xray::render::fg::TextureHandle shadowArrayHandle; // legacy alias → cascade 0
    nvrhi::ITexture* shadowArray = nullptr; // cascade 0 (bind slot t23)
    xray::render::fg::TextureHandle shadowCascadeHandle[kCSMCascadeCount];
    nvrhi::ITexture* shadowCascades[kCSMCascadeCount] = {};
    u32 cascadeResolution[kCSMCascadeCount] = {};

    // Dedicated HUD/weapon shadow map (not world cascade 0)
    xray::render::fg::TextureHandle hudShadowHandle;
    nvrhi::ITexture* hudShadowMap = nullptr;
    Fmatrix hudClipVP;
    Fmatrix hudSampleVP;
    static constexpr u32 kHUDShadowResolution = 1024;

    Fmatrix cascadeClipVP[kCSMCascadeCount];   // light VP for depth raster
    Fmatrix cascadeSampleVP[kCSMCascadeCount]; // clip→UV for sampling
    Fvector4 cascadeSplits;
    bool initialized = false;
    bool enabled = false;
};

struct ShadowCascadeOutputs
{
    framegraph::VirtualResourceHandle shadowArray;
    bool valid = false;
};

ShadowCascadeOutputs setupCascadedShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    fg::FGDetailManager* detailManager,
    const Fvector& sunDirection,
    const xr_vector<xray::render::GeometryBatch>* hudBatches,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches,
    fg::GPUCullingManager* gpuCulling,
    ShadowPassState& state,
    framegraph::VirtualResourceHandle skinnedDrawArgs = {});

void InitializeShadowPass(fg::RenderDevice* device, ShadowPassState& state);
void ShutdownShadowPass(fg::RenderDevice* device, ShadowPassState& state);

} // namespace xray::render::fg::passes
