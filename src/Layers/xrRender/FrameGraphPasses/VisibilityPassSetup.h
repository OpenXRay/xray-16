#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ClusterDrawConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
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

constexpr u32 kVisIdEntryLimit = (1u << 25) - 1u;
constexpr u32 kVisIdDetailSlotBits = 22u;
constexpr u32 kVisIdDetailKindSpan = 1u << kVisIdDetailSlotBits;

constexpr bool VisIdRangeFits(u32 base, u32 count)
{
    return base <= kVisIdEntryLimit && count <= kVisIdEntryLimit - base;
}

struct VisibilityPassState {
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle psAlphaTest;
    nvrhi::ShaderHandle psFade;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::ShaderHandle meshShader;
    nvrhi::ShaderHandle meshPsAlphaTest;
    nvrhi::ShaderHandle meshPsFade;
    nvrhi::BindingLayoutHandle meshLayout;
    nvrhi::BindingLayoutHandle meshTerrainLayout;
    nvrhi::MeshletPipelineHandle meshPipeline;
    nvrhi::MeshletPipelineHandle meshTerrainPipeline;
    bool meshFailed = false;
    int meshMode = -1;
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
    nvrhi::ShaderHandle swShader;
    nvrhi::BindingLayoutHandle swLayout;
    nvrhi::ComputePipelineHandle swPipeline;
    nvrhi::ShaderHandle swGrassShader;
    nvrhi::BindingLayoutHandle swGrassLayout;
    nvrhi::ComputePipelineHandle swGrassPipeline;
    bool swGrassFailed = false;
    nvrhi::ShaderHandle swResolvePS;
    nvrhi::BindingLayoutHandle swResolveLayout;
    nvrhi::GraphicsPipelineHandle swResolvePipeline;
    nvrhi::BufferHandle swVisBuffer;
    u32 swWidth = 0;
    u32 swHeight = 0;
    bool swFailed = false;
    bool initialized = false;
    bool failed = false;
    bool debugFailed = false;
};

struct VisibilityPassOutput {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle visId;
};

bool EnsureVisibilityResources(fg::RenderDevice* device, VisibilityPassState& state);

bool EnsureSwRasterResources(fg::RenderDevice* device, VisibilityPassState& state, u32 width, u32 height);

framegraph::VirtualResourceHandle setupSwRasterPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const ClusterDrawConfig& config,
    u32 width,
    u32 height,
    VisibilityPassState* state,
    framegraph::VirtualResourceHandle prevVis);

framegraph::VirtualResourceHandle setupSwGrassPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle detailArgs,
    framegraph::VirtualResourceHandle prevVis,
    FGDetailManager* detailManager,
    u32 grassEntryBase,
    u32 width,
    u32 height,
    VisibilityPassState* state);

VisibilityPassOutput setupVisibilityPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle visIdTarget,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    framegraph::VirtualResourceHandle swVis,
    const ClusterDrawConfig& config,
    GPUCullingManager* gpuCulling,
    FGDetailManager* detailManager,
    framegraph::VirtualResourceHandle detailArgs,
    u32 grassEntryBase,
    VisibilityPassState* state,
    bool retest = false,
    bool swGrass = false);

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
