#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

class CEnvironment;

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct SunPassState {
    nvrhi::TextureHandle placeholderTexture;
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::InputLayoutHandle inputLayout;
    bool initialized = false;
};

struct SunPassData {
    framegraph::VirtualResourceHandle colorOutput;
    fg::RenderDevice* device;
    CEnvironment* environment;
    u32 width;
    u32 height;
    SunPassState* passState;
};

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SunPassState& state
);

void InitializeSunPass(fg::RenderDevice* device, SunPassState& state);
void ShutdownSunPass(SunPassState& state);

}
