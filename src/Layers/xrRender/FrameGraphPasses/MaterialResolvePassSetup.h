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
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle color,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle material,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const ClusterDrawConfig& config,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    nvrhi::IBuffer* splatBuffer,
    const Fmatrix& prevView,
    const Fmatrix& prevProj,
    bool motionValid,
    u32 entryLimit,
    u32 width,
    u32 height,
    MaterialResolvePassState* state);

}
