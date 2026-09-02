#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ForwardColorPassSetup.h"
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
};

bool EnsureMaterialResolveResources(fg::RenderDevice* device, MaterialResolvePassState& state);

MaterialResolveOutput setupMaterialResolvePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle visId,
    framegraph::VirtualResourceHandle color,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    nvrhi::IBuffer* splatBuffer,
    u32 width,
    u32 height,
    MaterialResolvePassState* state);

}
