// xrRender/FrameGraphPasses/ParticlePassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "PassVertexFormats.h"
#include "ParticleGPUCullingManager.h"

namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
    namespace PS {
        class CParticleEffect;
        class CParticleGroup;
        class CPEDef;
    }
}

namespace xray::render {
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════
//  PARTICLE BATCH
// ═══════════════════════════════════════════════════════
// Represents a single particle system to render
// Particles are dynamic billboards/sprites, not static geometry
struct ParticleBatch {
    RENDER_NAMESPACE::dxRender_Visual* visual = nullptr;
    Fmatrix worldMatrix;
    IRenderable* renderable = nullptr;
    bool isHUDMode = false;
    u32 particleCount = 0;
    u32 vertexOffset = 0;
    u32 bindlessMaterialID = 0;  // Bindless material ID for texture lookup
};

struct ParticlePassState {
    nvrhi::GraphicsPipelineHandle pipelineBlend;
    nvrhi::GraphicsPipelineHandle pipelineAdd;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::SamplerHandle sampler;
    bool initialized = false;
    nvrhi::BufferHandle particleVB;
    u32 particleVBSize = 0;
    nvrhi::BufferHandle quadIB;
    u32 maxQuads = 0;
    xr_unique_ptr<ParticleGPUCullingManager> gpuCullingManager;
};

struct ParticlePassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle outputNormal;
    framegraph::VirtualResourceHandle hiZPyramid;
    ng::RenderDevice* device;
    const xr_vector<ParticleBatch>* worldParticleBatches;
    const xr_vector<ParticleBatch>* hudParticleBatches;
    MaterialCache* materialCache;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
    ParticlePassState* passState;
};

void InitializeParticleResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, ParticlePassState& state);

// ═══════════════════════════════════════════════════════
//  PARTICLE PASS SETUP
// ═══════════════════════════════════════════════════════
// Lambda-based particle pass setup with GPU culling support
// Renders particle systems (effects and groups) as dynamic billboards/sprites
// Renders AFTER forward color and skinning passes (particles on top of world+HUD)
// Supports both world and HUD particles with proper FOV handling
// When hiZPyramid is valid, uses GPU frustum + occlusion culling
framegraph::DefaultOutputLayout setupParticlePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle hiZPyramid = {},  // Hi-Z pyramid for GPU culling
    u32 hiZWidth = 0,
    u32 hiZHeight = 0,
    u32 hiZMipLevels = 0,
    ParticlePassState* state = nullptr
);

} // namespace xray::render::RENDER_NAMESPACE::passes
