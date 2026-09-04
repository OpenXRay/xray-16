#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "LocalShadowPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::fg::passes {

struct TransparentPassConfig {
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* drawArgsBuffer = nullptr;
    u32 objectCount = 0;

    bool IsValid() const {
        return objectCount > 0 && drawArgsBuffer && instanceBuffer && megaVertexBuffer && megaIndexBuffer;
    }
};

struct TransparentPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::SamplerHandle sampler;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    bool initialized = false;
};

struct TransparentPassData {
    framegraph::VirtualResourceHandle localTiles;
    framegraph::VirtualResourceHandle localStatic;
    framegraph::VirtualResourceHandle localDyn;
    LocalShadowOutput localShadow;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    fg::RenderDevice* device;
    TransparentPassConfig config;
    TransparentPassState* passState;
    u32 width, height;
};

void InitializeTransparentResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TransparentPassState& state);

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    const LocalShadowOutput& localShadow,
    u32 width, u32 height,
    TransparentPassState& state
);

} // namespace xray::render::fg::passes
