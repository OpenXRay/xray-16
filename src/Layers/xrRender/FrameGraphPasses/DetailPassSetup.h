#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
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
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct DetailPassState {
    bool detailDataUploaded = false;
    float lastBladeWidth = 0.0f;
};

struct DetailPassData {
    framegraph::VirtualResourceHandle inputColor;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle outputColor;
    framegraph::VirtualResourceHandle outputNormal;
    framegraph::VirtualResourceHandle hiZPyramid;
    ng::RenderDevice* device;
    RENDER_NAMESPACE::FGDetailManager* detailManager;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
    Fmatrix prevViewProj;
    bool hasPrevViewProj;
    xray::profiler::GPUProfiler* gpuProfiler;
    DetailPassState* detailState;
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
    xray::profiler::GPUProfiler* gpuProfiler = nullptr,
    DetailPassState* detailState = nullptr
);

}
