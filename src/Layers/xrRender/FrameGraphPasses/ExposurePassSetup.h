#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct ExposurePassState {
    nvrhi::BufferHandle histogramBuffer;
    nvrhi::TextureHandle exposureTexture;
    nvrhi::ComputePipelineHandle histogramPipeline;
    nvrhi::ComputePipelineHandle adaptPipeline;
    nvrhi::BindingLayoutHandle histogramLayout;
    nvrhi::BindingLayoutHandle adaptLayout;
    bool initialized = false;
    bool computeEnabled = false;
    float currentExposure = 1.0f;
    float f_luminance_adapt = 0.5f;
    u32 pipeVersion = 0;
    nvrhi::BufferHandle histReadback[3];
    u32 histWriteSlot = 0;
    u32 histBins[64] = {};
};

struct ExposureConfig {
    float middleGray = 1.0f;
    float amount = 0.7f;
    float lowLum = 0.0001f;
    float adaptation = 1.0f;
};

struct ExposurePassData {
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle exposureTexture;
    framegraph::VirtualResourceHandle histogramBuffer;
    fg::RenderDevice* device;
    ExposureConfig config;
    float deltaTime;
    u32 width;
    u32 height;
    ExposurePassState* passState;
};

struct ExposureOutput {
    framegraph::VirtualResourceHandle exposureTexture;
    framegraph::VirtualResourceHandle histogramBuffer;
};

void InitializeExposureResources(fg::RenderDevice* device, ExposurePassState& state);

ExposureOutput setupExposurePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrSceneColor,
    const ExposureConfig& config,
    float deltaTime,
    u32 width,
    u32 height,
    ExposurePassState& state
);

ExposureConfig GetDefaultExposureConfig();

nvrhi::ITexture* GetExposureTexture(const ExposurePassState& state);
void PollExposureHistogram(ExposurePassState& state, nvrhi::IDevice* device);

} // namespace xray::render::fg::passes
