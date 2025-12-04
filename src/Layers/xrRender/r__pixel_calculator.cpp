#include "stdafx.h"

#include "r__pixel_calculator.h"
#include "Layers/xrRender/FBasicVisual.h"

namespace xray::render::RENDER_NAMESPACE
{

void r_pixel_calculator::begin()
{
}

void r_pixel_calculator::end()
{
}

r_aabb_ssa r_pixel_calculator::calculate(dxRender_Visual* V)
{
    r_aabb_ssa result = {0};
    return result;
}

void r_pixel_calculator::run()
{
    Log("----- ssa build not yet implemented for FrameGraph -----");
}

} // namespace xray::render::RENDER_NAMESPACE
