#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "UpscaleState.h"
#include "IUpscaleBackend.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes {

struct UpscalePassState
{
    nvrhi::TextureHandle displayColor;
    nvrhi::TextureHandle rrDiffuseAlbedo;
    nvrhi::TextureHandle rrSpecularAlbedo;
    nvrhi::TextureHandle rrSpecularHitDist;
    nvrhi::ComputePipelineHandle rrGuidePipeline;
    nvrhi::BindingLayoutHandle rrGuideLayout;
    nvrhi::IBuffer* rrGuideCB = nullptr;
    u32 displayW = 0;
    u32 displayH = 0;
    u32 rrGuideW = 0;
    u32 rrGuideH = 0;
};

struct UpscaleRRGuides
{
    framegraph::VirtualResourceHandle normals;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle depth;
    nvrhi::ITexture* noisyDiffuse = nullptr;
    nvrhi::ITexture* noisySpecular = nullptr;
    nvrhi::ITexture* hitDistance = nullptr;
};

framegraph::VirtualResourceHandle setupUpscaleOrResolvePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle exposure,
    const UpscaleState& upscaleState,
    IUpscaleBackend* backend,
    u32 renderW,
    u32 renderH,
    UpscalePassState& state,
    const UpscaleRRGuides* rrGuides = nullptr);

}
