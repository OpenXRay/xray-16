// xrRender/FrameGraphPasses/TonemapPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Lambda-based tonemap pass setup
// Converts HDR scene color (RGBA16_FLOAT) to LDR output (RGBA8_UNORM) using ACES filmic tonemap
// Now accepts exposure texture from ExposurePass for auto-exposure
framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,  // 1x1 R32_FLOAT from ExposurePass (can be invalid for fixed exposure)
    u32 width,
    u32 height
);

} // namespace xray::render::RENDER_NAMESPACE::passes
