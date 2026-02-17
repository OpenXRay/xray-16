// DetailPassSetup.h - Framegraph pass for detail objects (grass, etc.)
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
// Lambda-based detail pass setup with GPU culling support
// Renders detail objects (grass, vegetation) using:
// - GPU compute culling (frustum + Hi-Z occlusion)
// - Single unified draw call via DrawIndexedInstancedIndirect
// - Interactive grass system (wind + entity interactions)
// - Virtual texturing for interaction atlas
// Renders AFTER forward color pass (details on top of world geometry)
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
    const Fmatrix* prevViewProj = nullptr,  // Previous frame's viewProj for temporal Hi-Z
    xray::profiler::GPUProfiler* gpuProfiler = nullptr,
    DetailPassState* detailState = nullptr
);

} // namespace xray::render::RENDER_NAMESPACE::passes
