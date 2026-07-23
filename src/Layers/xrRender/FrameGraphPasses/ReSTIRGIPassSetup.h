#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; }
namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RTAccelStructManager; }

namespace xray::render::fg::passes {

struct ReSTIRGIPassState {
    nvrhi::ComputePipelineHandle initialPipeline;
    nvrhi::BindingLayoutHandle initialLayout;
    nvrhi::ComputePipelineHandle temporalPipeline;
    nvrhi::BindingLayoutHandle temporalLayout;
    nvrhi::ComputePipelineHandle compositePipeline;
    nvrhi::BindingLayoutHandle compositeLayout;

    nvrhi::IBuffer* cb = nullptr;
    nvrhi::SamplerHandle sampler;

    nvrhi::TextureHandle reservoirA[2];
    nvrhi::TextureHandle reservoirB[2];
    nvrhi::TextureHandle directLighting;
    u32 currTemporalIdx = 0;
    u32 texWidth = 0;
    u32 texHeight = 0;

    bool initialized = false;
    bool enabled = false;
};

struct ReSTIRGIOutput {
    framegraph::VirtualResourceHandle sceneColor;
};

ReSTIRGIOutput setupReSTIRGIPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle prevNormals,
    framegraph::VirtualResourceHandle prevDepth,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData
);

void ShutdownReSTIRGI(ReSTIRGIPassState& state);

}
