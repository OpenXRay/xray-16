#include "stdafx.h"
#include "UpscaleState.h"
#include <cmath>
#include <algorithm>

extern ENGINE_API float ps_r_render_scale;
extern ENGINE_API int ps_r_upscale;
extern ENGINE_API int ps_r_dlss;
extern ENGINE_API int ps_r_dlss_quality;

namespace xray::render::fg {
namespace {

float DlssQualityScale(int quality)
{
    switch (quality)
    {
    case 0: return 0.33f;
    case 1: return 0.50f;
    case 2: return 0.58f;
    case 3: return 0.67f;
    case 5: return 1.00f;
    default: return 0.67f;
    }
}

}

void UpdateUpscaleState(UpscaleState& state, u32 displayW, u32 displayH, bool backendAvailable)
{
    if (backendAvailable && ps_r_dlss != 0 && ps_r_upscale == 0)
        ps_r_upscale = 2;
    if (ps_r_upscale == 1)
        ps_r_upscale = 0;
    if (ps_r_upscale == 3)
        ps_r_upscale = 2;

    state.displayWidth = std::max(1u, displayW);
    state.displayHeight = std::max(1u, displayH);

    float scale = ps_r_render_scale;
    if (scale <= 0.f)
        scale = 1.0f;
    scale = std::clamp(scale, 0.25f, 1.0f);

    if (ps_r_upscale == 0 || !backendAvailable)
        scale = 1.0f;
    else if (ps_r_upscale == 2 && ps_r_dlss_quality == 5)
        scale = 1.0f;
    else if (ps_r_upscale == 2 && scale >= 0.999f)
        scale = DlssQualityScale(ps_r_dlss_quality);

    state.renderScale = scale;
    state.renderWidth = std::max(1u, (u32)std::lround(float(state.displayWidth) * scale));
    state.renderHeight = std::max(1u, (u32)std::lround(float(state.displayHeight) * scale));
    state.upscaleActive = backendAvailable && ps_r_upscale == 2 &&
        (state.renderWidth != state.displayWidth ||
         state.renderHeight != state.displayHeight ||
         ps_r_dlss_quality == 5);
}

}
