#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::fg::passes {

struct ExposurePassState;

struct TonemapPassState {
    nvrhi::TextureHandle fallbackExposureTexture;
    nvrhi::TextureHandle fallbackDepthTexture;
    nvrhi::TextureHandle fallbackBloom;
    nvrhi::TextureHandle bloom0;
    nvrhi::TextureHandle bloom1;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::ComputePipelineHandle bloomExtractPipeline;
    nvrhi::BindingLayoutHandle bloomExtractLayout;
    nvrhi::ComputePipelineHandle bloomBlurPipeline;
    nvrhi::BindingLayoutHandle bloomBlurLayout;
    nvrhi::BufferHandle bloomCB;
    nvrhi::BufferHandle hdrCB;
    nvrhi::GraphicsPipelineHandle encodePipeline;
    nvrhi::BindingLayoutHandle encodeLayout;
    u32 bloomW = 0;
    u32 bloomH = 0;
    bool initialized = false;
    u32 pipeVersion = 0;
    u32 hdr10 = 0;
};

struct TonemapPassData {
    framegraph::VirtualResourceHandle hdrInput;
    framegraph::VirtualResourceHandle exposureInput;
    framegraph::VirtualResourceHandle depthInput;
    framegraph::VirtualResourceHandle worldPosInput;
    framegraph::VirtualResourceHandle ldrOutput;
    bool hasExposure;
    bool hasDepth;
    bool hasWorldPos;
    u32 width;
    u32 height;
    TonemapPassState* passState;
    const ExposurePassState* exposurePassState;
    fg::RenderDevice* device = nullptr;
};

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    TonemapPassState& state,
    const ExposurePassState* exposureState = nullptr,
    framegraph::VirtualResourceHandle depthTexture = {},
    framegraph::VirtualResourceHandle worldPosTexture = {}
);

void InitializeTonemapPass(nvrhi::IDevice* device, TonemapPassState& state);
void ShutdownTonemapPass(TonemapPassState& state);
void RenderHdrDebugUI(const ExposurePassState* exposure);

framegraph::VirtualResourceHandle setupHdr10EncodePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle src,
    framegraph::VirtualResourceHandle dst,
    u32 width,
    u32 height,
    TonemapPassState& state
);

} // namespace xray::render::fg::passes
