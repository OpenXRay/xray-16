#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

class CEnvironment;

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct SunPassState {
    nvrhi::TextureHandle placeholderTexture;
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    bool initialized = false;
};

struct SunPassData {
    framegraph::VirtualResourceHandle colorOutput;
    ng::RenderDevice* device;
    CEnvironment* environment;
    u32 width;
    u32 height;
    SunPassState* passState;
};

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SunPassState& state
);

void InitializeSunPass(ng::RenderDevice* device, SunPassState& state);
void ShutdownSunPass(SunPassState& state);

}
