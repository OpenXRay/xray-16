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
    nvrhi::ComputePipelineHandle spatialPipeline;
    nvrhi::BindingLayoutHandle spatialLayout;
    nvrhi::ComputePipelineHandle diTemporalPipeline;
    nvrhi::BindingLayoutHandle diTemporalLayout;
    nvrhi::ComputePipelineHandle diSpatialPipeline;
    nvrhi::BindingLayoutHandle diSpatialLayout;
    nvrhi::ComputePipelineHandle diShadePipeline;
    nvrhi::BindingLayoutHandle diShadeLayout;
    nvrhi::ComputePipelineHandle waterPipeline;
    nvrhi::BindingLayoutHandle waterLayout;
    nvrhi::ComputePipelineHandle compositePipeline;
    nvrhi::BindingLayoutHandle compositeLayout;
    nvrhi::ComputePipelineHandle blurPipeline;
    nvrhi::BindingLayoutHandle blurLayout;
    nvrhi::ComputePipelineHandle temporalFilterPipeline;
    nvrhi::BindingLayoutHandle temporalFilterLayout;
    nvrhi::ComputePipelineHandle volPipeline;
    nvrhi::BindingLayoutHandle volLayout;

    nvrhi::IBuffer* cb = nullptr;
    nvrhi::SamplerHandle sampler;

    nvrhi::TextureHandle reservoirA[2];
    nvrhi::TextureHandle reservoirB[2];
    nvrhi::TextureHandle reservoirC[2];
    nvrhi::TextureHandle specReservoirA[2];
    nvrhi::TextureHandle specReservoirB[2];
    nvrhi::TextureHandle diReservoir[2];
    nvrhi::TextureHandle directLighting;
    nvrhi::TextureHandle noisyDiffuse;
    nvrhi::TextureHandle noisySpecular;
    nvrhi::TextureHandle blurTemp;
    nvrhi::TextureHandle blurTempSpec;
    nvrhi::TextureHandle histDiffuse;
    nvrhi::TextureHandle histSpecular;
    nvrhi::TextureHandle hitDistance;
    nvrhi::TextureHandle volSceneColor;
    nvrhi::BufferHandle irradianceCache;
    u32 irradianceCacheSize = 0;
    u32 currTemporalIdx = 0;
    u32 texWidth = 0;
    u32 texHeight = 0;
    u32 volTexWidth = 0;
    u32 volTexHeight = 0;
    bool volAllocFailed = false;
    nvrhi::ITexture* waterUnderWorldPos = nullptr;

    bool initialized = false;
    bool enabled = false;
    bool initialLoadFailed = false;
};

struct ReSTIRGIOutput {
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle noisyDiffuse;
    framegraph::VirtualResourceHandle noisySpecular;
    framegraph::VirtualResourceHandle hitDistance;
};

bool IsRTGIActive(const RTAccelStructManager* accelMgr);

ReSTIRGIOutput setupReSTIRGIPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle worldPos,
    framegraph::VirtualResourceHandle prevNormals,
    framegraph::VirtualResourceHandle prevWorldPos,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData,
    bool skipInTreeDenoise = false,
    framegraph::VirtualResourceHandle classifyWorldPos = {},
    bool skipLightingTemporal = false
);

void ShutdownReSTIRGI(ReSTIRGIPassState& state);

framegraph::VirtualResourceHandle setupRTVolumetricPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle worldPos,
    const Fmatrix& invViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height,
    ReSTIRGIPassState& state
);

}
