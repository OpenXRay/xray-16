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
    RENDER_NAMESPACE::dxRender_Visual* visual = nullptr;  // CParticleEffect or CParticleGroup
    Fmatrix worldMatrix;                                  // Transform (usually identity for world, camera-relative for HUD)
    IRenderable* renderable = nullptr;                    // Owner object
    bool isHUDMode = false;                               // True if HUD particle (needs FOV adjustment)
};

// ═══════════════════════════════════════════════════════
//  PARTICLE VERTEX FORMAT
// ═══════════════════════════════════════════════════════
// Matches FVF::LIT format used by vanilla particle rendering
struct ParticleVertex {
    Fvector p;       // Position (12 bytes)
    u32 color;       // RGBA color (4 bytes)
    Fvector2 t;      // Texcoord (8 bytes)
    // Total: 24 bytes per vertex
};

// ═══════════════════════════════════════════════════════
//  PARTICLE PASS SETUP
// ═══════════════════════════════════════════════════════
// Lambda-based particle pass setup
// Renders particle systems (effects and groups) as dynamic billboards/sprites
// Renders AFTER forward color and HUD passes (particles on top of world+HUD)
// Supports both world and HUD particles with proper FOV handling
framegraph::DefaultOutputLayout setupParticlePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes
