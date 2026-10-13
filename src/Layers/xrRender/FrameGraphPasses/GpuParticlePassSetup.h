#pragma once

#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/LightingMode.h"
#include "ParticlePassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
class FrameGraph;
struct ExtractedReflection;
}

namespace xray::render {
class MaterialCache;
}

namespace xray::render::fg {
class RenderDevice;
struct GpuParticleDrawResources;
}

namespace xray::render::fg::passes {

struct GpuParticlePassState {
    MaterialCache* materialCache = nullptr;
    u32 materialEpoch = 0;
    u32 registeredPrograms = 0;
    nvrhi::GraphicsPipelineHandle pipelines[PARTICLE_BLEND_COUNT];
    nvrhi::GraphicsPipelineHandle distortPipeline;
    nvrhi::FramebufferInfo framebuffer;
    nvrhi::Format distortionFormat = nvrhi::Format::UNKNOWN;
    const framegraph::ExtractedReflection* vertexReflection = nullptr;
    const framegraph::ExtractedReflection* setReflection = nullptr;
    const framegraph::ExtractedReflection* pixelReflection = nullptr;
    const framegraph::ExtractedReflection* distortReflection = nullptr;
};

void WarmGpuParticlePipelines(fg::RenderDevice* device, GpuParticlePassState& state);

framegraph::DefaultOutputLayout setupGpuParticlePass(
    framegraph::FrameGraph& graph,
    fg::RenderDevice* device,
    const GpuParticleDrawResources& resources,
    framegraph::VirtualResourceHandle materials,
    const framegraph::DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    GpuParticlePassState& state,
    const LightingFrameState* lighting = nullptr);

}
