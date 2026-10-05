#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/SkyEnvironment.h"
#include "LocalShadowPassSetup.h"
#include "ClusterLightPassSetup.h"
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

class TransparentPassConfig
{
public:
    GeometryFrameResources geometry;
    u32 objectCount = 0;
    const xr_vector<TransparentDrawRange>* ranges = nullptr;
    GPUCullingManager* gpuCulling = nullptr;
    const LightingFrameState* lighting = nullptr;
    bool skinned = false;

    bool HasRigid() const;
    bool IsValid() const;
};

struct TransparentPassState {
    xr_map<u64, nvrhi::GraphicsPipelineHandle> pipelines;
    nvrhi::GraphicsPipelineHandle distortPipeline;
    nvrhi::GraphicsPipelineHandle skinnedDistortPipeline;
    xr_map<u32, nvrhi::GraphicsPipelineHandle> wallmarkPipelines;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::BindingLayoutHandle distortLayout;
    nvrhi::BindingLayoutHandle wallmarkLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::InputLayoutHandle skinnedInputLayout;
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
    framegraph::VirtualResourceHandle clusterLightData;
    framegraph::VirtualResourceHandle clusterGrid;
    framegraph::VirtualResourceHandle clusterLightIndexList;
    framegraph::VirtualResourceHandle skyIrradiance;
    framegraph::VirtualResourceHandle skySpecular;
    framegraph::VirtualResourceHandle skyDFG;
    framegraph::VirtualResourceHandle skyProbes;
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

void InitializeTransparentResources(fg::RenderDevice* device, TransparentPassState& state);
void WarmTransparentPipelines(fg::RenderDevice* device, TransparentPassState& state);

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    const LocalShadowOutput& localShadow,
    const ClusterLightOutput& clusterLights,
    const SkyEnvironmentFrame& sky,
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
