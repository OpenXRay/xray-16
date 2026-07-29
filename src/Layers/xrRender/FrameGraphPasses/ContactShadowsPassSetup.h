#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct ContactShadowsPassState
{
    nvrhi::GraphicsPipelineHandle marchPipeline;
    nvrhi::BindingLayoutHandle marchLayout;
    nvrhi::TextureHandle history[2];
    u32 historyW = 0;
    u32 historyH = 0;
    u32 historyIndex = 0;
    bool hasHistory = false;
    bool initialized = false;
};

// Returns the history texture to bind as g_ContactHistory @ t28 (may be null).
nvrhi::ITexture* GetContactShadowHistory(ContactShadowsPassState& state);

framegraph::VirtualResourceHandle setupContactShadowsPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle worldPos,
    framegraph::VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    ContactShadowsPassState& state);

} // namespace
