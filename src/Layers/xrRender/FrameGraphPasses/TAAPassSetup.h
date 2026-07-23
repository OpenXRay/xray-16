#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes
{

struct TAAPassState
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::TextureHandle history[2];
    u32 historyW = 0;
    u32 historyH = 0;
    u32 historyIndex = 0;
    bool hasHistory = false;
    bool initialized = false;
};

struct alignas(16) TAAParamsCB
{
    float screenSizeX, screenSizeY;
    float blendAlpha;
    float sharpness;
    float jitterX, jitterY;     // current frame Halton offset in pixels [-0.5, +0.5]
    float prevJitterX, prevJitterY; // previous frame (for history reprojection)
};

// Pixel-space Halton jitter (filled by ApplyTAAJitter)
extern float g_taa_jitter_px;
extern float g_taa_jitter_py;
extern float g_taa_jitter_prev_px;
extern float g_taa_jitter_prev_py;

// View-proj without subpixel jitter — use for motion vectors / prev-frame storage
extern Fmatrix g_taa_unjittered_full_transform;
extern Fmatrix g_taa_unjittered_inv_full_transform;

void ApplyTAAJitter();

framegraph::VirtualResourceHandle setupTAAPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle motionVectors, // may be invalid
    u32 width,
    u32 height,
    bool hasPrevFrame,
    TAAPassState& state);

void ShutdownTAAPass(TAAPassState& state);

} // namespace xray::render::fg::passes
