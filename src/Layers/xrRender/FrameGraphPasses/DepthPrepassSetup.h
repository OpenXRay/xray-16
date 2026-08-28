// xrRender/FrameGraphPasses/DepthPrepassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ForwardColorPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct DepthPrepassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle psOpaque;
    nvrhi::GraphicsPipelineHandle clusterPipeline;
    nvrhi::BindingLayoutHandle clusterLayout;
    nvrhi::ShaderHandle clusterVS;
    nvrhi::GraphicsPipelineHandle clusterTerrainPipeline;
    nvrhi::BindingLayoutHandle clusterTerrainLayout;
    nvrhi::ShaderHandle psFade;
    bool initialized = false;
};

struct DepthPrepassData {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle drawArgsBuffer;
    fg::RenderDevice* device;
    MaterialCache* materialCache;
    DepthPrepassState* passState;
    BindlessForwardConfig bindlessConfig;
    u32 width;
    u32 height;
};

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle drawArgsBuffer,
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    DepthPrepassState* state
);

} // namespace xray::render::fg::passes
