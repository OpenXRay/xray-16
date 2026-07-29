// xrRender/FrameGraphPasses/ExposurePassSetup.h
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

// Classic CoP auto-exposure (MiddleGray / bloom_luminance_3):
//   scale = middlegray.x / (Lw * middlegray.y + middlegray.z)
//   out   = lerp(prev, scale, adaptBlend); clamp [1/128, 20]
// Wired to r2_tonemap* console vars.

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
    float adaptBlend = 0.5f; // classic f_luminance_adapt init
};

struct ExposureConfig {
    float middleGray = 1.0f;
    float amount = 0.7f;       // 0 when r2_tonemap off
    float lowLum = 0.0001f;
    float adaptation = 1.0f;   // r2_tonemap_adaptation
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

// Builds config from r2_tonemap* / R2FLAG_TONEMAP (classic binder).
ExposureConfig GetDefaultExposureConfig();

nvrhi::ITexture* GetExposureTexture(const ExposurePassState& state);

} // namespace xray::render::fg::passes
