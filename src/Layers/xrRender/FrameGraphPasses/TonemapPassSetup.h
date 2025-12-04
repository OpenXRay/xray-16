// xrRender/FrameGraphPasses/TonemapPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace nvrhi {
    class IDevice;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Lambda-based tonemap pass setup
// Converts HDR scene color (RGBA16_FLOAT) to LDR output (RGBA8_UNORM) using ACES filmic tonemap
// Now accepts exposure texture from ExposurePass for auto-exposure
// If outputTarget is valid, writes directly to it (e.g., imported backbuffer)
// If outputTarget is invalid, creates internal rt_Final texture
framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,  // 1x1 R32_FLOAT from ExposurePass (can be invalid for fixed exposure)
    framegraph::VirtualResourceHandle outputTarget,     // Optional: imported backbuffer (invalid = create rt_Final)
    u32 width,
    u32 height
);

// Initialize tonemap pass resources (call once at startup)
void InitializeTonemapPass(nvrhi::IDevice* device);

// Shutdown tonemap pass resources
void ShutdownTonemapPass();

} // namespace xray::render::RENDER_NAMESPACE::passes
