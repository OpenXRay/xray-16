#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class LightingFrameState;
class DisplayCalibration;
}

namespace xray::render::fg::passes
{
class DisplayPassState
{
public:
    nvrhi::ComputePipelineHandle histogramPipeline;
    nvrhi::BindingLayoutHandle histogramLayout;
    nvrhi::ComputePipelineHandle adaptPipeline;
    nvrhi::BindingLayoutHandle adaptLayout;
    nvrhi::GraphicsPipelineHandle outputPipeline;
    nvrhi::BindingLayoutHandle outputLayout;
    nvrhi::Format outputFormat = nvrhi::Format::UNKNOWN;
    nvrhi::BufferHandle histogram;
    nvrhi::BufferHandle exposure;
    bool exposurePipelinesFailed = false;
    bool outputPipelineFailed = false;
    bool exposureInitialized = false;
    bool meteredPreviousFrame = false;
    u32 frameIndex = 0;
};

class ExposureHistogramConstants
{
public:
    u32 sceneWidth = 0;
    u32 sceneHeight = 0;
    u32 pad0 = 0;
    u32 pad1 = 0;
};

class ExposureAdaptConstants
{
public:
    float compensation = 0.0f;
    float minEv = 0.0f;
    float maxEv = 0.0f;
    float rateUp = 0.0f;
    float rateDown = 0.0f;
    float deltaTime = 0.0f;
    u32 autoExposure = 0;
    u32 reset = 0;
};

class DisplayOutputConstants
{
public:
    u32 tonemapper = 0;
    u32 sceneValid = 0;
    u32 frameIndex = 0;
    float gamma = 1.0f;
    float brightness = 1.0f;
    float contrast = 1.0f;
    float pad0 = 0.0f;
    float pad1 = 0.0f;
};

class LightingFailurePassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    LightingFrameState* lighting = nullptr;
};

class InterfaceLayerPassData
{
public:
    framegraph::VirtualResourceHandle layer;
};

class ExposureInitPassData
{
public:
    framegraph::VirtualResourceHandle histogram;
    framegraph::VirtualResourceHandle exposure;
    DisplayPassState* state = nullptr;
};

class ExposureHistogramPassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle histogram;
    fg::RenderDevice* device = nullptr;
    DisplayPassState* state = nullptr;
    u32 width = 0;
    u32 height = 0;
};

class ExposureAdaptPassData
{
public:
    framegraph::VirtualResourceHandle histogram;
    framegraph::VirtualResourceHandle exposure;
    fg::RenderDevice* device = nullptr;
    DisplayPassState* state = nullptr;
    ExposureAdaptConstants constants;
};

class DisplayOutputPassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle exposure;
    framegraph::VirtualResourceHandle interfaceLayer;
    framegraph::VirtualResourceHandle output;
    fg::RenderDevice* device = nullptr;
    DisplayPassState* state = nullptr;
    const LightingFrameState* lighting = nullptr;
    DisplayOutputConstants constants;
    u32 width = 0;
    u32 height = 0;
};

framegraph::VirtualResourceHandle setupLightingFailurePass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneColor,
    LightingFrameState& lighting);

framegraph::VirtualResourceHandle setupInterfaceLayer(
    framegraph::FrameGraph& fg,
    u32 width,
    u32 height);

framegraph::VirtualResourceHandle setupExposurePasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    u32 width,
    u32 height,
    bool meterScene,
    DisplayPassState& state);

framegraph::VirtualResourceHandle setupDisplayOutputPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle exposure,
    framegraph::VirtualResourceHandle interfaceLayer,
    framegraph::VirtualResourceHandle output,
    u32 width,
    u32 height,
    const DisplayCalibration& calibration,
    DisplayPassState& state,
    const LightingFrameState* lighting);
}
