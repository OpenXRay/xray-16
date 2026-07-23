// xrRender/FrameGraphPasses/TonemapPassSetup.h
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
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    bool initialized = false;
};

struct TonemapPassData {
    framegraph::VirtualResourceHandle hdrInput;
    framegraph::VirtualResourceHandle exposureInput;
    framegraph::VirtualResourceHandle ldrOutput;
    bool hasExposure;
    u32 width;
    u32 height;
    TonemapPassState* passState;
    const ExposurePassState* exposurePassState;
};

// Lambda-based tonemap pass setup
// Converts HDR → LDR via classic CoP modified Reinhard (white=1.7) + MiddleGray exposure
// If outputTarget is valid, writes directly to it (e.g., imported backbuffer)
// If outputTarget is invalid, creates internal rt_Final texture
framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    TonemapPassState& state,
    const ExposurePassState* exposureState = nullptr
);

void InitializeTonemapPass(nvrhi::IDevice* device, TonemapPassState& state);
void ShutdownTonemapPass(TonemapPassState& state);

} // namespace xray::render::fg::passes
