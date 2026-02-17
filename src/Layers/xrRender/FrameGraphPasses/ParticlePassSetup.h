// xrRender/FrameGraphPasses/ParticlePassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/xrCore.h"

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
        class PipelineState;
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

// ═══════════════════════════════════════════════════════
//  PARTICLE VERTEX FORMAT
// ═══════════════════════════════════════════════════════
// Extended format with material ID for batched rendering
struct ParticleVertex {
    Fvector p;       // Position (12 bytes)
    u32 color;       // RGBA color (4 bytes)
    Fvector2 t;      // Texcoord (8 bytes)
    u32 materialID;  // Bindless material ID (4 bytes)
    u32 _pad;        // Padding to match Vulkan std430 stride (4 bytes)
    // Total: 32 bytes per vertex (std430: vec3 alignment=16 → struct alignment=16 → stride=32)
};
static_assert(sizeof(ParticleVertex) == 32, "ParticleVertex must be 32 bytes to match Vulkan std430 stride");

// ═══════════════════════════════════════════════════════
//  GPU PARTICLE DATA (for GPU culling)
// ═══════════════════════════════════════════════════════
// Compact particle data uploaded to GPU for culling + billboard generation
struct GPUParticleData {
    Fvector position;   // 12 bytes - World position (billboard center)
    float rotation;     //  4 bytes - Rotation angle (radians)
    Fvector2 size;      //  8 bytes - Billboard half-extents
    u32 color;          //  4 bytes - BGRA8 packed color
    u32 materialID;     //  4 bytes - Bindless material index
    Fvector2 uvMin;     //  8 bytes - UV top-left
    Fvector2 uvMax;     //  8 bytes - UV bottom-right
    // Total: 48 bytes per particle
};
static_assert(sizeof(GPUParticleData) == 48, "GPUParticleData must be 48 bytes");

// ═══════════════════════════════════════════════════════
//  PARTICLE PIPELINE MANAGEMENT
// ═══════════════════════════════════════════════════════
// Initialize particle pipelines (call once at startup)
void InitializeParticlePipelines(ng::RenderDevice* device);

// Shutdown particle pipelines (call at cleanup)
void ShutdownParticlePipelines();

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
    u32 hiZMipLevels = 0
);

} // namespace xray::render::RENDER_NAMESPACE::passes
