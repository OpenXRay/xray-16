#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ClusterDrawConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
        class FGDetailManager;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

constexpr u32 kVisIdEntryLimit = 1u << 25;

struct VisibilityPassState {
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle psAlphaTest;
    nvrhi::ShaderHandle psFade;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::ShaderHandle skinnedVS;
    nvrhi::BindingLayoutHandle skinnedLayout;
    nvrhi::GraphicsPipelineHandle skinnedPipeline;
    nvrhi::ShaderHandle bladeVS;
    nvrhi::ShaderHandle bladePS;
    nvrhi::BindingLayoutHandle bladeLayout;
    nvrhi::GraphicsPipelineHandle bladePipeline;
    nvrhi::ShaderHandle pulledPS;
    nvrhi::BindingLayoutHandle pulledLayout;
    nvrhi::GraphicsPipelineHandle pulledPipeline;
    nvrhi::ShaderHandle debugShader;
    nvrhi::BindingLayoutHandle debugLayout;
    nvrhi::ComputePipelineHandle debugPipeline;
    bool initialized = false;
    bool failed = false;
    bool debugFailed = false;
};

struct VisibilityPassOutput {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle visId;
};

bool EnsureVisibilityResources(fg::RenderDevice* device, VisibilityPassState& state);

VisibilityPassOutput setupVisibilityPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle visIdTarget,
    framegraph::VirtualResourceHandle drawArgsBuffer,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const ClusterDrawConfig& config,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    FGDetailManager* detailManager,
    framegraph::VirtualResourceHandle detailArgs,
    u32 grassEntryBase,
    VisibilityPassState* state,
    bool retest = false);

framegraph::VirtualResourceHandle setupVisDebugViewPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle visId,
    framegraph::VirtualResourceHandle motion,
    u32 width,
    u32 height,
    u32 mode,
    VisibilityPassState* state);

}
