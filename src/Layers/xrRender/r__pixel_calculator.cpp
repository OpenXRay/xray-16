#include "stdafx.h"

#include "r__pixel_calculator.h"
#include "Layers/xrRender/FBasicVisual.h"

// TODO: Implement NVRHI-based pixel calculator
// This replaces the legacy D3D11/RCache pixel calculation code

namespace xray::render::fg
{

void r_pixel_calculator::begin()
{
    // TODO: Implement via NVRHI
}

void r_pixel_calculator::end()
{
    // TODO: Implement via NVRHI
}

r_aabb_ssa r_pixel_calculator::calculate(dxRender_Visual* V)
{
    // TODO: Implement via NVRHI
    r_aabb_ssa result = {0};
    return result;
}

void r_pixel_calculator::run()
{
    // TODO: Implement via NVRHI
    Log("----- ssa build not yet implemented for FrameGraph -----");
}

} // namespace xray::render::fg
