#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "DetailPassSetup.h"

struct Fmatrix;

namespace xray::render::RENDER_NAMESPACE {
    class FGDetailManager;
}

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

void setupDetailCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RENDER_NAMESPACE::FGDetailManager* detailManager,
    framegraph::VirtualResourceHandle hiZPyramid,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    const Fmatrix* prevViewProj,
    xray::profiler::GPUProfiler* gpuProfiler,
    DetailPassState* detailState
);

} // namespace xray::render::RENDER_NAMESPACE::passes
