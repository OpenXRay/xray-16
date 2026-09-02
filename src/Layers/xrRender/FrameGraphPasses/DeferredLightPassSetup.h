#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "SunShadowPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DeferredLightPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle shader;
    bool initialized = false;
    bool failed = false;
};

framegraph::DefaultOutputLayout setupDeferredLightPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    SunShadowMaps sunShadowMaps,
    xray::profiler::GPUProfiler* gpuProfiler,
    DeferredLightPassState* state);

}
