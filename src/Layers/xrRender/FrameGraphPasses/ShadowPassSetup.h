#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class FGDetailManager;
}

namespace xray::render::fg::passes
{

struct alignas(16) ShadowCascadeCB
{
    Fmatrix lightVP;
};
static_assert(sizeof(ShadowCascadeCB) % 16 == 0);

struct alignas(16) GrassShadowCB
{
    float grassBladeHeight;
    u32 buildDetailsIndex;
    float windAngleDeg;
    float windSpeed;
    float time;
    float windDisplacement;
    float pad0, pad1;
};
static_assert(sizeof(GrassShadowCB) % 16 == 0);

struct GrassShadowPassState
{
    nvrhi::GraphicsPipelineHandle grassPipeline;
    nvrhi::BindingLayoutHandle grassLayout;
    nvrhi::GraphicsPipelineHandle billboardGrassPipeline;
    nvrhi::BindingLayoutHandle billboardGrassLayout;
    nvrhi::InputLayoutHandle grassInputLayout;
    nvrhi::ShaderHandle grassVs;
    nvrhi::ShaderHandle grassPs;
    nvrhi::ShaderHandle billboardGrassVs;
    nvrhi::ShaderHandle billboardGrassPs;
    nvrhi::BufferHandle cascadeCB;
    nvrhi::BufferHandle grassCB;
    xray::render::fg::TextureHandle shadowHandle;
    nvrhi::ITexture* shadowMap = nullptr;
    Fmatrix clipVP;
    Fmatrix sampleVP;
    u32 resolution = 0;
    bool initialized = false;
    bool enabled = false;
};

struct GrassShadowOutputs
{
    framegraph::VirtualResourceHandle shadowMap;
    nvrhi::ITexture* shadowTex = nullptr;
    Fmatrix sampleVP;
    bool valid = false;
};

void InitializeGrassShadowPass(fg::RenderDevice* device, GrassShadowPassState& state);
void ShutdownGrassShadowPass(fg::RenderDevice* device, GrassShadowPassState& state);

GrassShadowOutputs setupGrassShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    const Fvector& sunDirection,
    GrassShadowPassState& state,
    framegraph::VirtualResourceHandle orderAfter = {},
    framegraph::VirtualResourceHandle cullArgs = {});

}
