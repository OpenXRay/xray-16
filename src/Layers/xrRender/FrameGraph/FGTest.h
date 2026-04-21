// xrRender/FrameGraph/FGTest.h
#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class RenderContext;
    class RenderDevice;
}

namespace xray::render::resources {
    class FGResourceManager;
}

namespace xray::render::framegraph {

// Simple triangle test - single pass clear + render
void TestSimpleTriangle(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

// Two-pass test - HDR render → Tonemap
void TestTwoPassRender(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

// Resource aliasing test - Multiple transient resources with non-overlapping lifetimes
void TestResourceAliasing(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

} // namespace xray::render::framegraph
