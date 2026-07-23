#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class GPUCullingManager;
}

namespace xray::render::fg::passes
{

struct LocalShadowPassState
{
    xray::render::fg::TextureHandle atlasHandle;
    nvrhi::ITexture* atlas = nullptr;
    // Low slope-bias pipelines (CSM's 2.75 slope eats thin furniture/NPC contact shadows).
    nvrhi::GraphicsPipelineHandle opaquePipeline;
    nvrhi::GraphicsPipelineHandle foliagePipeline;
    u32 resolution = 0;
    u32 sliceCount = 0;
    bool initialized = false;
    bool enabled = false;
};

struct LocalShadowOutputs
{
    framegraph::VirtualResourceHandle atlas;
    nvrhi::ITexture* atlasTex = nullptr;
    bool valid = false;
};

void InitializeLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state);
void ShutdownLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state);

LocalShadowOutputs setupLocalShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    LocalShadowPassState& state,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches = nullptr,
    fg::GPUCullingManager* gpuCulling = nullptr);

} // namespace xray::render::fg::passes
