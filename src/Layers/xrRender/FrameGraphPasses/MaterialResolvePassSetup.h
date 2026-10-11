#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ClusterDrawConfig.h"
#include "ViewFrame.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct MaterialResolvePassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::ShaderHandle shader;
    bool initialized = false;
    bool failed = false;
};

struct MaterialResolveOutput {
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle material;
    framegraph::VirtualResourceHandle motionVectors;
    framegraph::VirtualResourceHandle visDepth;
};

bool EnsureMaterialResolveResources(fg::RenderDevice* device, MaterialResolvePassState& state);

MaterialResolveOutput setupMaterialResolvePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle visId,
    const framegraph::DefaultOutputLayout& gbuffer,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const ClusterDrawConfig& config,
    GPUCullingManager* gpuCulling,
    nvrhi::IBuffer* splatBuffer,
    const ViewFrame& view,
    u32 entryLimit,
    MaterialResolvePassState* state);

}
