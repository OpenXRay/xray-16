#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::fg::passes
{
struct ViewFrame
{
    Fmatrix view = Fidentity;
    Fmatrix project = Fidentity;
    Fmatrix viewProj = Fidentity;
    Fmatrix invViewProj = Fidentity;
    Fmatrix prevView = Fidentity;
    Fmatrix prevProject = Fidentity;
    Fmatrix prevViewProj = Fidentity;
    Fvector cameraPos = {};
    float prevDetailTime = 0.0f;
    u32 frameIndex = 0;
    u32 width = 0;
    u32 height = 0;
    bool hasPrevFrameData = false;
};
}
