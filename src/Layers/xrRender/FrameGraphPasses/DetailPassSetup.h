#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

struct Fmatrix;

namespace xray::render::RENDER_NAMESPACE {
    class FGDetailManager;
}

namespace xray::render {
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {
    class FrameGraph;
    class ShaderLoader;
    struct DefaultOutputLayout;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct DetailPassData
{
    framegraph::VirtualResourceHandle depthBuffer;
    framegraph::VirtualResourceHandle colorTarget;
    framegraph::VirtualResourceHandle hiZPyramid;
    u32 width;
    u32 height;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
};

framegraph::DefaultOutputLayout setupDetailPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    RENDER_NAMESPACE::FGDetailManager* detailManager,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle hiZPyramid = {},
    u32 hiZWidth = 0,
    u32 hiZHeight = 0,
    u32 hiZMipLevels = 0,
    const Fmatrix* prevViewProj = nullptr,
    xray::profiler::GPUProfiler* gpuProfiler = nullptr
);

}
