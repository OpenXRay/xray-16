#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct HiZBuildPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    bool initialized = false;
    bool computeEnabled = false;
    u32 currentWidth = 0;
    u32 currentHeight = 0;
    u32 mipLevels = 0;
};

struct HiZBuildData {
    framegraph::VirtualResourceHandle depthInput;
    framegraph::VirtualResourceHandle hizPyramid;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
    u32 mipLevels;
    HiZBuildPassState* passState;
};

struct HiZPyramidOutput {
    framegraph::VirtualResourceHandle pyramid;
    u32 mipLevels;
    u32 width;
    u32 height;
};

void InitializeHiZResources(fg::RenderDevice* device, u32 width, u32 height, HiZBuildPassState& state);

HiZPyramidOutput setupHiZBuildPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    u32 width,
    u32 height,
    HiZBuildPassState& state
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
