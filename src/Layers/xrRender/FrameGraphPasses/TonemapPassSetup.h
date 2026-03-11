#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

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

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
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

}
