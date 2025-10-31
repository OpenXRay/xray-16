#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderContext;
}

namespace xray::render::framegraph {

void TestSimpleTriangle(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

void TestTwoPassRender(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer);

}
