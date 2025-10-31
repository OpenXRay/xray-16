// xrRender/FrameGraph/FGTest.h
#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderContext;
}

namespace xray::render::framegraph {

// Simple triangle test - single pass clear + render
void TestSimpleTriangle(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

// Two-pass test - HDR render → Tonemap
void TestTwoPassRender(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

} // namespace xray::render::framegraph
