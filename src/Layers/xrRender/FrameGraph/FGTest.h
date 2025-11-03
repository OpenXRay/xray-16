#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderContext;
    class RenderDevice;
}

namespace xray::render::resources {
    class ModernResourceManager;
}

namespace xray::render::framegraph {

void TestSimpleTriangle(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

void TestTwoPassRender(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

void TestResourceAliasing(ng::RenderDevice* renderDevice, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

}
