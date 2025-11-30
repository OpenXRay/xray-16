#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct HiZPyramidOutput {
    framegraph::VirtualResourceHandle pyramid;
    u32 mipLevels;
    u32 width;
    u32 height;
};

HiZPyramidOutput setupHiZBuildPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    u32 width,
    u32 height
);

inline u32 CalculateHiZMipLevels(u32 width, u32 height) {
    u32 mipLevels = 1;
    u32 w = width, h = height;
    while (w > 1 || h > 1) {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        mipLevels++;
    }
    return mipLevels;
}

}
