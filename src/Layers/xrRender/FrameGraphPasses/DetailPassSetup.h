#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>
#include "SunShadowPassSetup.h"

struct Fmatrix;

namespace xray::render::fg {
    class FGDetailManager;
}

namespace xray::render {
    namespace fg {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DetailPassState {
    bool detailDataUploaded = false;
    float lastBladeWidth = 0.0f;
};

struct DetailPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle outputNormal;
    framegraph::VirtualResourceHandle baseColor;
    fg::RenderDevice* device;
    fg::FGDetailManager* detailManager;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    xray::profiler::GPUProfiler* gpuProfiler;
};
framegraph::DefaultOutputLayout setupDetailPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    const framegraph::DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    xray::profiler::GPUProfiler* gpuProfiler = nullptr
);

}
