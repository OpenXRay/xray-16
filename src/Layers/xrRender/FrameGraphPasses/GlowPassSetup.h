#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes {

struct GlowBillboard
{
    Fvector pos{};
    float radius = 1.f;
    Fcolor color{ 1.f, 1.f, 1.f, 1.f };
    shared_str texture;
};

struct GlowPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::BufferHandle vb;
    nvrhi::BufferHandle ib;
    nvrhi::TextureHandle placeholderTex;
    u32 vbCapacityVerts = 0;
    bool initialized = false;
};

void GlowRegistry_Register(void* glow);
void GlowRegistry_Unregister(void* glow);
using GlowCollectFn = bool (*)(void* glow, GlowBillboard& out);
void GlowRegistry_SetCollect(GlowCollectFn fn);
void GlowRegistry_Collect(xr_vector<GlowBillboard>& out);

framegraph::VirtualResourceHandle setupGlowBillboardPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    GlowPassState& state);

}
