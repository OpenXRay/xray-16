#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
}

namespace xray::render::fg::passes
{

struct RainShadowPassState
{
    xray::render::fg::TextureHandle rainSMHandle;
    nvrhi::ITexture* rainSM = nullptr;
    u32 resolution = 0;
    Fmatrix clipVP;
    Fmatrix sampleVP;
    bool initialized = false;
    bool enabled = false;
};

struct RainShadowOutputs
{
    framegraph::VirtualResourceHandle rainSM;
    nvrhi::ITexture* rainSMTex = nullptr;
    Fmatrix sampleVP;
    bool valid = false;
};

void InitializeRainShadowPass(fg::RenderDevice* device, RainShadowPassState& state);
void ShutdownRainShadowPass(fg::RenderDevice* device, RainShadowPassState& state);

RainShadowOutputs setupRainShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    RainShadowPassState& state);

} // namespace xray::render::fg::passes
