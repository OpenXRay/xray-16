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

void TestSimpleTriangle(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

void TestTwoPassRender(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

void TestResourceAliasing(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer);

}
