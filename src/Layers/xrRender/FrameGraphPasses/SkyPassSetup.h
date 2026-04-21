#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

class CEnvironment;

namespace xray::render::RENDER_NAMESPACE::passes {

struct SkyPassState {
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::TextureHandle placeholderCubemap;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::InputLayoutHandle inputLayout;
    bool initialized = false;
};

struct SkyPassData {
    framegraph::VirtualResourceHandle colorOutput;
    framegraph::VirtualResourceHandle depthOutput;
    fg::RenderDevice* device;
    CEnvironment* environment;
    SkyPassState* passState;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SkyPassState& state
);

void InitializeSkyGeometry(fg::RenderDevice* device, SkyPassState& state);
void ShutdownSkyGeometry(SkyPassState& state);

}
