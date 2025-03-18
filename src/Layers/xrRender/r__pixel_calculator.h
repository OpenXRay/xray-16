#pragma once

namespace xray::render::RENDER_NAMESPACE
{
struct r_aabb_ssa
{
    u8 ssa[6];
};

class r_pixel_calculator
{
    ref_rt rt;
    ref_rt zb;

public:
    void begin();
    r_aabb_ssa calculate(dxRender_Visual* V);
    void end();

    void run();
};
} // namespace xray::render::RENDER_NAMESPACE
