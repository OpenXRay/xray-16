#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct ExposureConfig {
    float minLogLuminance = -10.0f;
    float maxLogLuminance = 4.0f;

    float lowPercentile = 0.5f;
    float highPercentile = 0.98f;

    float adaptSpeedUp = 3.0f;
    float adaptSpeedDown = 1.0f;

    float minExposure = 0.001f;
    float maxExposure = 64.0f;

    float exposureCompensation = 0.0f;
    float calibrationConstant = 12.5f;
};

struct ExposureOutput {
    framegraph::VirtualResourceHandle exposureTexture;
    framegraph::VirtualResourceHandle histogramBuffer;
};

ExposureOutput setupExposurePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrSceneColor,
    const ExposureConfig& config,
    float deltaTime,
    u32 width,
    u32 height
);

ExposureConfig GetDefaultExposureConfig();

nvrhi::ITexture* GetExposureTexture();

}
