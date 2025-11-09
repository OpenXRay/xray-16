// xrRender/FrameGraph/FGTest.h
#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderContext;
    class RenderDevice;
}

namespace xray::render::resources {
    class FGResourceManager;
}

namespace xray::render::framegraph {

// Simple triangle test - single pass clear + render
void TestSimpleTriangle(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

// Two-pass test - HDR render → Tonemap
void TestTwoPassRender(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

// Resource aliasing test - Multiple transient resources with non-overlapping lifetimes
void TestResourceAliasing(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

} // namespace xray::render::framegraph
