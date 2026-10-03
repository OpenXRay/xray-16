#include "src/Layers/xrRenderVK/FogParameters.h"

#include <cassert>
#include <cstring>

using namespace xray::render::vulkan;

static float half_to_float(uint16_t half)
{
    const uint32_t exponent = (half >> 10) & 31;
    const uint32_t bits = exponent ? ((exponent - 15 + 127) << 23) | (uint32_t(half & 1023) << 13) : 0;
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}
static void unpack(float packed, float& a, float& b)
{
    uint32_t bits;
    std::memcpy(&bits, &packed, 4);
    a = half_to_float(uint16_t(bits));
    b = half_to_float(uint16_t(bits >> 16));
}

int main()
{
    float base[4]{}, dx[4]{}, dy[4]{};
    const float first[]{.1f, .4f, .8f};
    set_weather_fog(base, dx, dy, first, 20.f, 180.f, 400.f);
    float r, g, b, near, far, projection;
    unpack(base[3], r, g); unpack(dx[3], b, near); unpack(dy[3], far, projection);
    assert(r > .09f && g > .39f && b > .79f);
    assert(near == 20.f && far == 180.f && projection == 400.f);
    const float second[]{.7f, .2f, .1f};
    set_weather_fog(base, dx, dy, second, 90.f, 350.f, 500.f);
    unpack(base[3], r, g); unpack(dx[3], b, near); unpack(dy[3], far, projection);
    assert(r > .69f && near == 90.f && far == 350.f && projection == 500.f);
}
