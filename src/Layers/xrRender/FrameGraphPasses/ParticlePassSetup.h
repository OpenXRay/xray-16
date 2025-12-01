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

struct ParticleBatch {
    RENDER_NAMESPACE::dxRender_Visual* visual = nullptr;
    Fmatrix worldMatrix;
    IRenderable* renderable = nullptr;
    bool isHUDMode = false;
    u32 particleCount = 0;
    u32 vertexOffset = 0;
};

struct ParticleVertex {
    Fvector p;
    u32 color;
    Fvector2 t;
};

framegraph::DefaultOutputLayout setupParticlePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    nvrhi::IBuffer* particleDrawArgsBuffer = nullptr
);

}
