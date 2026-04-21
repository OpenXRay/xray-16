#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
class RenderDevice;
}

namespace xray::render::framegraph {
class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE {
class RTAccelStructManager;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct PathTracerConfig {
    u32 maxBounces = 8;
    u32 sampleIndex = 0;
};

struct PathTracerOutput {
    framegraph::VirtualResourceHandle composited;
};

PathTracerOutput setupPathTracerPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    const PathTracerConfig& config,
    const Fmatrix& invViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height
);

void ShutdownPathTracer();

}
