#include "VulkanGameLightingMath.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
float safe_nonnegative(float value)
{
    return std::isfinite(value) ? std::max(value, 0.f) : 0.f;
}

float luminance(const float* color)
{
    return 0.2126f * safe_nonnegative(color[0]) +
        0.7152f * safe_nonnegative(color[1]) +
        0.0722f * safe_nonnegative(color[2]);
}

bool finite3(const std::array<float, 3>& value)
{
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
}
}

VulkanObjectLighting evaluate_object_lighting(const std::array<float, 3>& position,
    const float* ambient, const float* hemi, const float* sun, uint32_t mode,
    const std::vector<VulkanLightSnapshot>& lights)
{
    const float ambient_light = mode & VulkanTraceHemi ? luminance(hemi) : 0.f;
    const float sun_light = mode & VulkanTraceSun ? 0.2f * luminance(sun) : 0.f;
    const float environment_light = (mode & VulkanTraceHemi ? luminance(ambient) : 0.f) +
        ambient_light + sun_light;
    float local_light = 0.f;
    constexpr std::array<std::array<float, 3>, 6> face_directions{{
        {{1.f, 0.f, 0.f}}, {{-1.f, 0.f, 0.f}}, {{0.f, 1.f, 0.f}},
        {{0.f, -1.f, 0.f}}, {{0.f, 0.f, 1.f}}, {{0.f, 0.f, -1.f}}
    }};
    VulkanObjectLighting result;
    result.hemi_cube.fill(ambient_light);
    if ((mode & VulkanTraceLights) && finite3(position))
    {
        for (const VulkanLightSnapshot& light : lights)
        {
            if (!light.active || light.type == VulkanLightType::Direct ||
                !std::isfinite(light.range) || light.range <= 1e-5f || !finite3(light.position))
                continue;
            float to_light[3]{light.position[0] - position[0],
                light.position[1] - position[1], light.position[2] - position[2]};
            const float distance = std::sqrt(to_light[0] * to_light[0] +
                to_light[1] * to_light[1] + to_light[2] * to_light[2]);
            if (!std::isfinite(distance) || distance >= light.range)
                continue;
            const float inverse_distance = distance > 1e-6f ? 1.f / distance : 0.f;
            for (float& component : to_light) component *= inverse_distance;
            float attenuation = 1.f - distance / light.range;
            attenuation *= attenuation;
            if (light.type == VulkanLightType::Spot)
            {
                if (!finite3(light.direction))
                    continue;
                const float direction_dot = -(light.direction[0] * to_light[0] +
                    light.direction[1] * to_light[1] + light.direction[2] * to_light[2]);
                const float cutoff = std::cos(std::clamp(light.cone, 0.f, 2.f) * 0.5f);
                if (!std::isfinite(direction_dot) || direction_dot <= cutoff)
                    continue;
                attenuation *= std::clamp((direction_dot - cutoff) /
                    std::max(1.f - cutoff, 1e-4f), 0.f, 1.f);
            }
            const float energy = luminance(light.color.data()) * attenuation;
            local_light += energy;
            for (size_t face = 0; face < face_directions.size(); ++face)
            {
                const auto& normal = face_directions[face];
                const float facing = std::max(0.f, normal[0] * to_light[0] +
                    normal[1] * to_light[1] + normal[2] * to_light[2]);
                result.hemi_cube[face] += energy * facing;
            }
        }
    }

    result.hemi_luminocity = std::clamp(ambient_light + local_light, 0.f, 1.f);
    result.luminocity = std::clamp(environment_light + local_light, 0.f, 1.f);
    for (float& value : result.hemi_cube)
        value = std::clamp(value, 0.f, 1.f);
    return result;
}
}
