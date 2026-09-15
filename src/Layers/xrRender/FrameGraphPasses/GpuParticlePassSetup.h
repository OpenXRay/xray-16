#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
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

struct GpuParticlePassOutputs {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle distortion;
};

GpuParticlePassOutputs setupGpuParticlePass(
    framegraph::FrameGraph& graph,
    fg::RenderDevice* device,
    const GpuParticleDrawResources& resources,
    MaterialCache* materialCache,
    framegraph::VirtualResourceHandle color,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle distortion,
    u32 width,
    u32 height,
    GpuParticlePassState& state);

}
