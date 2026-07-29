#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg {
class RenderDevice;
class FLOD;
}

namespace xray::render::fg::passes
{

struct LodImpostorInstance
{
    FLOD* lod = nullptr;
    Fmatrix world;
};

struct LodPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::BufferHandle dynamicVB;
    nvrhi::BufferHandle quadIB;
    u32 dynamicVBCapacity = 0;
    bool initialized = false;
};

framegraph::DefaultOutputLayout setupLodPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const xr_vector<LodImpostorInstance>* impostors,
    u32 width,
    u32 height,
    LodPassState& state);

} // namespace
