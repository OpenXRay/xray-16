#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE {
    class ClusteredLightManager;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct ClusterLightPassState {
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::ShaderHandle computeShader;
    bool initialized = false;
};

void setupClusterLightPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    ClusteredLightManager* lightManager,
    u32 screenWidth,
    u32 screenHeight,
    ClusterLightPassState* state
);

}
