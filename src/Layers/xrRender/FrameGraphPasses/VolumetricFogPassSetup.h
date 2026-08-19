#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; class RTAccelStructManager; }
namespace xray::render::framegraph { class FrameGraph; }

namespace xray::render::fg::passes {

struct VolumetricFogPassState {
    nvrhi::ComputePipelineHandle densityPipeline;
    nvrhi::BindingLayoutHandle densityLayout;
    nvrhi::ComputePipelineHandle injectPipeline;
    nvrhi::BindingLayoutHandle injectLayout;
    nvrhi::ComputePipelineHandle accumulatePipeline;
    nvrhi::BindingLayoutHandle accumulateLayout;
    nvrhi::ComputePipelineHandle applyPipeline;
    nvrhi::BindingLayoutHandle applyLayout;
    nvrhi::IBuffer* cb = nullptr;
    nvrhi::SamplerHandle sampler;
    bool initialized = false;
    bool enabled = false;
    u32 pipeVersion = 0;
};

framegraph::VirtualResourceHandle setupVolumetricFogPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle worldPos,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height,
    VolumetricFogPassState& state,
    RTAccelStructManager* accelMgr = nullptr);

void ShutdownVolumetricFog(VolumetricFogPassState& state);

}
