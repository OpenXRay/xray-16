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

class CEnvironment;

namespace xray::render::RENDER_NAMESPACE::passes {

struct SkyPassState {
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::TextureHandle placeholderCubemap;
    bool initialized = false;
};

struct SkyPassData {
    framegraph::VirtualResourceHandle colorOutput;
    framegraph::VirtualResourceHandle depthOutput;
    ng::RenderDevice* device;
    CEnvironment* environment;
    SkyPassState* passState;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SkyPassState& state
);

void InitializeSkyGeometry(ng::RenderDevice* device, SkyPassState& state);
void ShutdownSkyGeometry(SkyPassState& state);

}
