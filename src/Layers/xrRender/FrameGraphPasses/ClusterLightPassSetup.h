#pragma once

#include <nvrhi/nvrhi.h>
#include "Layers/xrRender/FrameGraph/FGTypes.h"

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg {
    class ClusteredLightManager;
}

namespace xray::render::fg::passes {

struct ClusterLightPassState {
    nvrhi::ComputePipelineHandle assignPipeline;
    nvrhi::BindingLayoutHandle assignLayout;
    nvrhi::ShaderHandle assignShader;
    bool assignInitialized = false;

    nvrhi::ComputePipelineHandle cullPipeline;
    nvrhi::BindingLayoutHandle cullLayout;
    nvrhi::ShaderHandle cullShader;
    bool cullInitialized = false;
};

void setupClusterLightPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    ClusteredLightManager* lightManager,
    u32 screenWidth,
    u32 screenHeight,
    ClusterLightPassState* state,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const Fmatrix& prevViewProj,
    bool hasPrevViewProj
);

}
