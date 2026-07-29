#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg {

struct UpscaleState
{
    u32 displayWidth = 0;
    u32 displayHeight = 0;
    u32 renderWidth = 0;
    u32 renderHeight = 0;
    float renderScale = 1.0f;
    float jitterX = 0.f;
    float jitterY = 0.f;
    float prevJitterX = 0.f;
    float prevJitterY = 0.f;
    bool upscaleActive = false;
    bool resetHistory = false;
};

void UpdateUpscaleState(UpscaleState& state, u32 displayW, u32 displayH);

inline bool NeedsResolveToDisplay(const UpscaleState& state)
{
    return state.renderWidth != state.displayWidth || state.renderHeight != state.displayHeight;
}

}
