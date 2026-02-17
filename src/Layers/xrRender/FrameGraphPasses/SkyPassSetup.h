// xrRender/FrameGraphPasses/SkyPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

// Forward declarations
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

// ═══════════════════════════════════════════════════════
//  SKY PASS (Sky dome + Sun disc rendering)
// ═══════════════════════════════════════════════════════
//
// Renders the sky dome using two cubemap textures (blended)
// and the sun disc/lens flare effects.
//
// PIPELINE:
// 1. Render sky half-box geometry with cubemap sampling
// 2. Blend between two sky states (CurrentEnv.weight)
// 3. Apply HDR scaling from exposure/tonemap
// 4. Render sun disc (lens flare source)
//
// OUTPUT:
// - Renders directly to scene color buffer (behind geometry)
// - Uses reverse-Z (z = w) for infinite far plane

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

} // namespace xray::render::RENDER_NAMESPACE::passes
