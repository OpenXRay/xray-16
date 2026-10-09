#pragma once

#include <cstdint>

namespace xray::render::vulkan
{
constexpr uint32_t ForwardLightCapacity = 16;
struct alignas(16) ForwardLightUniform
{
    float inverse_view_projection[16]{};
    float sun_direction_ambient[4]{};
    float sun_color_count[4]{};
    float viewport[4]{};
    float fog_color_start[4]{};
    float fog_end_camera[4]{};
    struct alignas(16) Local
    {
        float position_range[4]{};
        float direction_cone[4]{};
        float color_type[4]{};
    } local[ForwardLightCapacity]{};
};
static_assert(sizeof(ForwardLightUniform) == 64 + 80 + 48 * ForwardLightCapacity);
}
