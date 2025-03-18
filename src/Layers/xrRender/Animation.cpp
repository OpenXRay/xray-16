#include "stdafx.h"

#include "Animation.h"

namespace xray::render::RENDER_NAMESPACE
{
using namespace animation;

const channal_rule channels::rules[channels::max] = {{lerp, lerp}, {lerp, lerp}, {lerp, add}, {lerp, add}};

channels::channels() { init(); }
void channels::init() { std::fill(factors, factors + max, 1.f); }
void channels::set_factor(u16 channel, float factor)
{
    VERIFY(channel < max);
    factors[channel] = factor;
}
} // namespace xray::render::RENDER_NAMESPACE
