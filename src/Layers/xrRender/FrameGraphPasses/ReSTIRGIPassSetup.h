#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h"
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
    nvrhi::ComputePipelineHandle spatialPipeline;
    nvrhi::BindingLayoutHandle spatialLayout;
    nvrhi::ComputePipelineHandle compositePipeline;
    nvrhi::BindingLayoutHandle compositeLayout;
    nvrhi::ComputePipelineHandle wetPipeline;
    nvrhi::BindingLayoutHandle wetLayout;
    nvrhi::ComputePipelineHandle sunshaftsPipeline;
    nvrhi::BindingLayoutHandle sunshaftsLayout;
    nvrhi::ComputePipelineHandle ddgiPipeline;
    nvrhi::BindingLayoutHandle ddgiLayout;
    nvrhi::ComputePipelineHandle waterPipeline;
    nvrhi::BindingLayoutHandle waterLayout;
    nvrhi::ComputePipelineHandle diTemporalPipeline;
    nvrhi::BindingLayoutHandle diTemporalLayout;
    nvrhi::ComputePipelineHandle diSpatialPipeline;
    nvrhi::BindingLayoutHandle diSpatialLayout;
    nvrhi::ComputePipelineHandle diShadePipeline;
    nvrhi::BindingLayoutHandle diShadeLayout;
    nvrhi::ComputePipelineHandle blurPipeline;
    nvrhi::BindingLayoutHandle blurLayout;
    nvrhi::ComputePipelineHandle temporalFilterPipeline;
    nvrhi::BindingLayoutHandle temporalFilterLayout;
    nvrhi::ComputePipelineHandle specTemporalPipeline;
    nvrhi::BindingLayoutHandle specTemporalLayout;
    nvrhi::ComputePipelineHandle ptInitialPipeline;
    nvrhi::BindingLayoutHandle ptInitialLayout;
    nvrhi::ComputePipelineHandle ptTemporalPipeline;
    nvrhi::BindingLayoutHandle ptTemporalLayout;
    nvrhi::ComputePipelineHandle ptSpatialPipeline;
    nvrhi::BindingLayoutHandle ptSpatialLayout;
    nvrhi::ComputePipelineHandle ptDupPipeline;
    nvrhi::BindingLayoutHandle ptDupLayout;
    u32 currTemporalIdx = 0;

    nvrhi::IBuffer* cb = nullptr;
    nvrhi::ITexture* waterUnderWorldPos = nullptr;
    nvrhi::ITexture* waterUnderColor = nullptr;

    bool initialized = false;
    bool enabled = false;
    u32 pipeVersion = 0;
};

struct ReSTIRGIOutput {
    framegraph::VirtualResourceHandle sceneColor;
    nvrhi::ITexture* noisyDiffuse = nullptr;
    nvrhi::ITexture* noisySpecular = nullptr;
    nvrhi::ITexture* hitDistance = nullptr;
};

ReSTIRGIOutput setupReSTIRGIPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle worldPos,
    framegraph::VirtualResourceHandle prevNormals,
    framegraph::VirtualResourceHandle prevDepth,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fmatrix& prevInvViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData,
    const GrassShadowOutputs& grassShadow = {}
);

void ShutdownReSTIRGI(ReSTIRGIPassState& state);

}
