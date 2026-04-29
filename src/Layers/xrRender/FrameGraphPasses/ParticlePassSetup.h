// xrRender/FrameGraphPasses/ParticlePassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "PassVertexFormats.h"
#include "ParticleGPUCullingManager.h"

namespace xray::render::fg {
    class dxRender_Visual;
    namespace PS {
        class CParticleEffect;
        class CParticleGroup;
        class CPEDef;
    }
}

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

enum class ParticleShaderVariant : u8 {
    Standard,
    Emissive,
    Distort,
    Count
};

// Matches CBlender_Particle::oBlend token IDs exactly
enum ParticleBlendMode : u8 {
    PARTICLE_BLEND_SET       = 0,
    PARTICLE_BLEND_BLEND     = 1,
    PARTICLE_BLEND_ADD       = 2,
    PARTICLE_BLEND_MUL       = 3,
    PARTICLE_BLEND_MUL_2X    = 4,
    PARTICLE_BLEND_ALPHA_ADD = 5,
    PARTICLE_BLEND_COUNT     = 6,
};

struct ParticleBatch {
    fg::dxRender_Visual* visual = nullptr;
    Fmatrix worldMatrix;
    IRenderable* renderable = nullptr;
    bool isHUDMode = false;
    u32 particleCount = 0;
    u32 vertexOffset = 0;
    u32 bindlessMaterialID = 0;
    u8 blendMode = PARTICLE_BLEND_BLEND;
    ParticleShaderVariant shaderVariant = ParticleShaderVariant::Standard;
};

struct ParticlePassState {
    nvrhi::GraphicsPipelineHandle pipelines[PARTICLE_BLEND_COUNT];
    nvrhi::GraphicsPipelineHandle distortPipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle distortLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle distortPS;
    nvrhi::SamplerHandle sampler;
    bool initialized = false;
    bool distortInitialized = false;
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
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle hiZPyramid;
    framegraph::VirtualResourceHandle distortionRT;
    framegraph::VirtualResourceHandle worldPosCopy;
    fg::RenderDevice* device;
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
    bool hasDistortion;
};

struct ParticlePassOutput {
    framegraph::DefaultOutputLayout layout;
    framegraph::VirtualResourceHandle distortionRT;
};

void InitializeParticleResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ParticlePassState& state);

// ═══════════════════════════════════════════════════════
//  PARTICLE PASS SETUP
// ═══════════════════════════════════════════════════════
// Lambda-based particle pass setup with GPU culling support
// Renders particle systems (effects and groups) as dynamic billboards/sprites
// Renders AFTER forward color and skinning passes (particles on top of world+HUD)
// Supports both world and HUD particles with proper FOV handling
// When hiZPyramid is valid, uses GPU frustum + occlusion culling
ParticlePassOutput setupParticlePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
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

} // namespace xray::render::fg::passes
