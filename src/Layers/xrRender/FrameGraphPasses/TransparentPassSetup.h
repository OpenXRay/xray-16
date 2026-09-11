#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "LocalShadowPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::fg::passes {

struct TransparentPassConfig {
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* drawArgsBuffer = nullptr;
    u32 objectCount = 0;
    const xr_vector<TransparentDrawRange>* ranges = nullptr;
    GPUCullingManager* gpuCulling = nullptr;
    bool skinned = false;

    bool HasRigid() const {
        return objectCount > 0 && ranges && drawArgsBuffer && instanceBuffer && megaVertexBuffer && megaIndexBuffer;
    }
    bool IsValid() const {
        return HasRigid() || (skinned && gpuCulling);
    }
};

struct TransparentPassState {
    xr_map<u32, nvrhi::GraphicsPipelineHandle> pipelines;
    nvrhi::GraphicsPipelineHandle distortPipeline;
    xr_map<u32, nvrhi::GraphicsPipelineHandle> wallmarkPipelines;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle distortLayout;
    nvrhi::BindingLayoutHandle wallmarkLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle distortPS;
    nvrhi::ShaderHandle wallmarkPS;
    nvrhi::FramebufferInfoEx fbInfo;
    nvrhi::FramebufferInfoEx wallmarkFbInfo;
    bool initialized = false;
};

struct TransparentPassData {
    framegraph::VirtualResourceHandle localTiles;
    framegraph::VirtualResourceHandle localStatic;
    framegraph::VirtualResourceHandle localDyn;
    framegraph::VirtualResourceHandle sunMask;
    LocalShadowOutput localShadow;
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle distortion;
    framegraph::VirtualResourceHandle skinnedOrder;
    fg::RenderDevice* device;
    TransparentPassConfig config;
    TransparentPassState* passState;
    u32 width, height;
};

void InitializeTransparentResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TransparentPassState& state);

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    const LocalShadowOutput& localShadow,
    framegraph::VirtualResourceHandle sunMask,
    framegraph::VirtualResourceHandle skinnedOrder,
    u32 width, u32 height,
    TransparentPassState& state
);

framegraph::DefaultOutputLayout setupStaticWallmarkPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state
);

} // namespace xray::render::fg::passes
