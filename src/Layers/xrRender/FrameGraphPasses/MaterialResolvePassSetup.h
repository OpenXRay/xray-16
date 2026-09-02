#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "ForwardColorPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
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
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    MaterialResolvePassState* state);

}
