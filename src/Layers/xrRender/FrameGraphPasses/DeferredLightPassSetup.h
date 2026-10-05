#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/SkyEnvironment.h"
#include "LocalShadowPassSetup.h"
#include "ClusterLightPassSetup.h"
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

class DeferredLightPassState
{
public:
    nvrhi::ComputePipelineHandle classifyPipeline;
    nvrhi::BindingLayoutHandle classifyLayout;
    nvrhi::ShaderHandle classifyShader;
    nvrhi::ComputePipelineHandle tilePipelines[kLightTileClasses];
    nvrhi::BindingLayoutHandle tileLayouts[kLightTileClasses];
    nvrhi::ShaderHandle tileShaders[kLightTileClasses];
    bool initialized = false;
    bool failed = false;
    u32 skyVisibilityRecordedFrame = UINT32_MAX;
    u32 skyVisibilityRecordedStatus = 0;

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

class DeferredLightPassData
{
public:
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle sunMask;
    framegraph::VirtualResourceHandle localTiles;
    framegraph::VirtualResourceHandle localStatic;
    framegraph::VirtualResourceHandle localDyn;
    framegraph::VirtualResourceHandle localHud;
    framegraph::VirtualResourceHandle clusterLightData;
    framegraph::VirtualResourceHandle clusterGrid;
    framegraph::VirtualResourceHandle clusterLightIndexList;
    framegraph::VirtualResourceHandle skyIrradiance;
    framegraph::VirtualResourceHandle skySpecular;
    framegraph::VirtualResourceHandle skyDFG;
    framegraph::VirtualResourceHandle skyProbes;
    LocalShadowOutput localShadow;
    RenderDevice* device = nullptr;
    DeferredLightPassState* state = nullptr;
    LightingFrameState* lighting = nullptr;
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
    u32 width = 0;
    u32 height = 0;
};

void ProcessDeferredLightStats(DeferredLightPassState& state, nvrhi::IDevice* device);

framegraph::DefaultOutputLayout setupDeferredLightPass(framegraph::FrameGraph& fg, fg::RenderDevice* device, const framegraph::DefaultOutputLayout& inputs,
    u32 width, u32 height, framegraph::VirtualResourceHandle sunMask, const LocalShadowOutput& localShadow, const ClusterLightOutput& clusterLights,
    const SkyEnvironmentFrame& sky, xray::profiler::GPUProfiler* gpuProfiler, DeferredLightPassState* state, LightingFrameState* lighting);
}
