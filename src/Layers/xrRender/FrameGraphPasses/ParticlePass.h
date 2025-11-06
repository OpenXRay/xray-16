// xrRender/FrameGraphPasses/ParticlePass.h
#pragma once

#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/xrCore.h"

namespace xray::render::RENDER_NAMESPACE {
    class dxRender_Visual;
    namespace PS {
        class CParticleEffect;
        class CParticleGroup;
    }
}

namespace xray::render::passes {

using namespace framegraph;

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
//  PARTICLE PASS CONFIGURATION
// ═══════════════════════════════════════════════════════
struct ParticlePassConfig {
    u32 width = 0;
    u32 height = 0;
};

// ═══════════════════════════════════════════════════════
//  PARTICLE PASS
// ═══════════════════════════════════════════════════════
//
// Renders particle systems (effects and groups) as dynamic billboards/sprites.
// Supports both world and HUD particles with proper FOV handling.
//
// Rendering technique:
// - World particles: Normal projection, render to G-Buffer
// - HUD particles: Apply Unreal Engine view-space FOV transformation
//
// Particles use dynamic vertex buffers (billboards generated per-frame)
// and render after all solid geometry to avoid overdraw.
//
// Execution order: GBuffer → HUD → Particles
// Depth testing: Enabled (particles test against world+HUD depth)
//
class ParticlePass : public framegraph::IPass {
public:
    ParticlePass(ng::RenderDevice* device, const ParticlePassConfig& config);
    ~ParticlePass() override;

    // ═══════════════════════════════════════════════════
    //  IPass Interface
    // ═══════════════════════════════════════════════════

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;
    framegraph::RenderPhase GetPhase() const override { return framegraph::RenderPhase::Custom; }

    // ═══════════════════════════════════════════════════
    //  Configuration
    // ═══════════════════════════════════════════════════

    // Set particle batches to render (world particles)
    void SetWorldParticleBatches(const xr_vector<ParticleBatch>* batches) {
        m_worldParticleBatches = batches;
    }

    // Set HUD particle batches (need FOV transformation)
    void SetHUDParticleBatches(const xr_vector<ParticleBatch>* batches) {
        m_hudParticleBatches = batches;
    }

    // Set render target outputs (from GBufferPass)
    void SetOutputs(const framegraph::DefaultOutputLayout& outputs) {
        m_outputs = outputs;
    }

private:
    ng::RenderDevice* m_device = nullptr;
    ParticlePassConfig m_config;

    // Particle batches (provided by FrameGraphRenderer)
    const xr_vector<ParticleBatch>* m_worldParticleBatches = nullptr;
    const xr_vector<ParticleBatch>* m_hudParticleBatches = nullptr;

    // Render targets (shared with GBufferPass)
    framegraph::DefaultOutputLayout m_outputs;

    // Dynamic vertex buffer for particle billboards (per-frame)
    nvrhi::BufferHandle m_particleVB;
    u32 m_particleVBSize = 0;  // Current VB size in bytes

    // Shared quad index buffer (static, created once)
    nvrhi::BufferHandle m_quadIB;
    u32 m_maxQuads = 0;  // Max number of quads supported by IB

    // Particle PSO cache (shader name -> PSO)
    xr_map<shared_str, ng::PipelineState*> m_particlePSOCache;

    // ═══════════════════════════════════════════════════
    //  Rendering Helpers
    // ═══════════════════════════════════════════════════

    // Apply HUD FOV adjustment to world matrix (Unreal Engine technique)
    Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix);

    // Create or resize shared quad index buffer
    void EnsureQuadIndexBuffer(u32 maxQuads);

    // Create or resize dynamic particle vertex buffer
    void EnsureParticleVertexBuffer(u32 sizeBytes);

    // Get or create PSO for particle shader
    ng::PipelineState* GetOrCreateParticlePSO(
        RENDER_NAMESPACE::Shader* shader,
        const RENDER_NAMESPACE::PS::CPEDef* pDef,
        const framegraph::FrameGraph& fg
    );

    // Render a single particle system (effect or group)
    // Returns true if rendered, false if skipped
    bool RenderParticleSystem(
        ng::RenderContext& ctx,
        const ParticleBatch& batch,
        bool applyFOV,
        const framegraph::FrameGraph& fg
    );

    // ═══════════════════════════════════════════════════
    //  Billboard Generation (ported from ParticleEffect.cpp)
    // ═══════════════════════════════════════════════════

    // Particle vertex format (matches FVF::LIT)
    struct ParticleVertex {
        Fvector p;       // Position (12 bytes)
        u32 color;       // RGBA color (4 bytes)
        Fvector2 t;      // Texcoord (8 bytes)
        // Total: 24 bytes per vertex
    };

    // Fill a billboard sprite (4 vertices forming a quad)
    void FillSprite(
        ParticleVertex*& pv,
        const Fvector& T, const Fvector& R,
        const Fvector& pos,
        const Fvector2& lt, const Fvector2& rb,
        float r1, float r2,
        u32 clr,
        float sina, float cosa
    );

    // Fill a billboard sprite (aligned to direction)
    void FillSprite(
        ParticleVertex*& pv,
        const Fvector& pos, const Fvector& dir,
        const Fvector2& lt, const Fvector2& rb,
        float r1, float r2,
        u32 clr,
        float sina, float cosa
    );
};

} // namespace xray::render::passes
