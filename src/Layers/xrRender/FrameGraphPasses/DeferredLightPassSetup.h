#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "LocalShadowPassSetup.h"
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

constexpr u32 kLightTileSize = 8;
constexpr u32 kLightTileClasses = 4;

struct DeferredLightPassState {
    nvrhi::ComputePipelineHandle classifyPipeline;
    nvrhi::BindingLayoutHandle classifyLayout;
    nvrhi::ShaderHandle classifyShader;
    nvrhi::ComputePipelineHandle tilePipelines[kLightTileClasses];
    nvrhi::BindingLayoutHandle tileLayouts[kLightTileClasses];
    nvrhi::ShaderHandle tileShaders[kLightTileClasses];
    bool initialized = false;
    bool failed = false;

    nvrhi::BufferHandle tileListBuffer;
    nvrhi::BufferHandle tileArgsBuffer;
    u32 tilesX = 0;
    u32 tilesY = 0;
    u32 maxTiles = 0;

    static constexpr u32 kReadbackSlots = 6;
    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;
    u32 readbackFrame = 0;
    u32 tileCounts[kLightTileClasses] = {};
};

void ProcessDeferredLightStats(DeferredLightPassState& state, nvrhi::IDevice* device);

framegraph::DefaultOutputLayout setupDeferredLightPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle sunMask,
    const LocalShadowOutput& localShadow,
    xray::profiler::GPUProfiler* gpuProfiler,
    DeferredLightPassState* state);

}
