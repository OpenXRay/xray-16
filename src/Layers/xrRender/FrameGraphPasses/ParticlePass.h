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

struct ParticleBatch {
    RENDER_NAMESPACE::dxRender_Visual* visual = nullptr;
    Fmatrix worldMatrix;
    IRenderable* renderable = nullptr;
    bool isHUDMode = false;
};

struct ParticlePassConfig {
    u32 width = 0;
    u32 height = 0;
};

class ParticlePass : public framegraph::IPass {
public:
    ParticlePass(ng::RenderDevice* device, const ParticlePassConfig& config);
    ~ParticlePass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;
    framegraph::RenderPhase GetPhase() const override { return framegraph::RenderPhase::Custom; }

    void SetWorldParticleBatches(const xr_vector<ParticleBatch>* batches) {
        m_worldParticleBatches = batches;
    }

    void SetHUDParticleBatches(const xr_vector<ParticleBatch>* batches) {
        m_hudParticleBatches = batches;
    }

    void SetOutputs(const framegraph::DefaultOutputLayout& outputs) {
        m_outputs = outputs;
    }

private:
    ng::RenderDevice* m_device = nullptr;
    ParticlePassConfig m_config;

    const xr_vector<ParticleBatch>* m_worldParticleBatches = nullptr;
    const xr_vector<ParticleBatch>* m_hudParticleBatches = nullptr;

    framegraph::DefaultOutputLayout m_outputs;

    nvrhi::BufferHandle m_particleVB;
    u32 m_particleVBSize = 0;

    nvrhi::BufferHandle m_quadIB;
    u32 m_maxQuads = 0;

    xr_map<shared_str, ng::PipelineState*> m_particlePSOCache;

    Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix);

    void EnsureQuadIndexBuffer(u32 maxQuads);

    void EnsureParticleVertexBuffer(u32 sizeBytes);

    ng::PipelineState* GetOrCreateParticlePSO(
        RENDER_NAMESPACE::Shader* shader,
        const RENDER_NAMESPACE::PS::CPEDef* pDef,
        const framegraph::FrameGraph& fg
    );

    bool RenderParticleSystem(
        ng::RenderContext& ctx,
        const ParticleBatch& batch,
        bool applyFOV,
        const framegraph::FrameGraph& fg
    );

    struct ParticleVertex {
        Fvector p;
        u32 color;
        Fvector2 t;
    };

    void FillSprite(
        ParticleVertex*& pv,
        const Fvector& T, const Fvector& R,
        const Fvector& pos,
        const Fvector2& lt, const Fvector2& rb,
        float r1, float r2,
        u32 clr,
        float sina, float cosa
    );

    void FillSprite(
        ParticleVertex*& pv,
        const Fvector& pos, const Fvector& dir,
        const Fvector2& lt, const Fvector2& rb,
        float r1, float r2,
        u32 clr,
        float sina, float cosa
    );
};

}
