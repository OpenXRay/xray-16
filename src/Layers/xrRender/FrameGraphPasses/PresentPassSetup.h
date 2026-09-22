#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class RenderDevice;
    class LightingFrameState;
}

namespace xray::render::fg::passes {

struct PresentPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    bool initialized = false;
};

class PresentPassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    framegraph::VirtualResourceHandle output;
    u32 width = 0;
    u32 height = 0;
    PresentPassState* passState = nullptr;
    const LightingFrameState* lighting = nullptr;
};

class LightingFailurePassData
{
public:
    framegraph::VirtualResourceHandle sceneColor;
    LightingFrameState* lighting = nullptr;
};

framegraph::VirtualResourceHandle setupLightingFailurePass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneColor,
    LightingFrameState& lighting
);

framegraph::VirtualResourceHandle setupPresentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    PresentPassState& state,
    LightingFrameState* lighting = nullptr
);

}
