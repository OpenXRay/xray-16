#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace xray::render::vulkan
{
inline uint16_t fog_half(float value)
{
    if (!std::isfinite(value) || value <= 0.f) return 0;
    value = std::min(value, 65504.f);
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    int exponent = int((bits >> 23) & 255) - 127 + 15;
    if (exponent <= 0) return 0;
    if (exponent >= 31) return 0x7bff;
    uint32_t mantissa = ((bits & 0x7fffffu) + 0x1000u) >> 13;
    if (mantissa == 0x400u) { mantissa = 0; ++exponent; }
    return exponent >= 31 ? 0x7bff : uint16_t((uint32_t(exponent) << 10) | mantissa);
}

inline float pack_fog_pair(float first, float second)
{
    const uint32_t bits = uint32_t(fog_half(first)) | uint32_t(fog_half(second)) << 16;
    float packed;
    std::memcpy(&packed, &bits, 4);
    return packed;
}

inline void set_weather_fog(float (&ray_base)[4], float (&ray_dx)[4], float (&ray_dy)[4],
    const float (&color)[3], float near_distance, float far_distance, float projection_far)
{
    const float start = std::max(0.f, std::isfinite(near_distance) ? near_distance : 0.f);
    const float end = std::max(start + .01f, std::isfinite(far_distance) ? far_distance : start + 1.f);
    ray_base[3] = pack_fog_pair(color[0], color[1]);
    ray_dx[3] = pack_fog_pair(color[2], start);
    ray_dy[3] = pack_fog_pair(end, projection_far);
}
}
