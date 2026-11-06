#pragma once

#include <cstdint>
#include "RenderPolicy.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace xray::render::vulkan
{
// Values exposed through the game's renderer console. The Vulkan renderer
// reads these every frame, so the UI and saved user config share one state.
#if defined(XR_PLATFORM_ANDROID)
inline constexpr unsigned DefaultVulkanQuality = 1;
#else
inline constexpr unsigned DefaultVulkanQuality = 2;
#endif
inline constexpr int PresetAnisotropy[]{2, 4, 8, 16, 16};
inline uint32_t quality_preset = DefaultVulkanQuality;
inline int detail_radius = RenderQualities[DefaultVulkanQuality].detail_distance;
inline int detail_update_cells = RenderQualities[DefaultVulkanQuality].detail_updates;
inline int texture_anisotropy = PresetAnisotropy[DefaultVulkanQuality];
inline float detail_prepare_ms = 2.f;
inline int use_light_grid = 1;
inline int point_shadow_budget = RenderQualities[DefaultVulkanQuality].point_shadows;
inline int spot_shadow_budget = RenderQualities[DefaultVulkanQuality].spot_shadows;
inline int ros_ray_budget = 128;
inline float hemi_sky_scale = .08f;
inline float hemi_light_scale = .2f, hemi_light_flow = .1f, hemi_smooth = 1.f;
inline float ambient_scale = 1.f;
inline float hemisphere_scale = 1.f;
inline float sun_scale = 1.f;
inline int sun_enabled = 1;
inline int tonemap_enabled = 1;
inline float tonemap_middlegray = 1.f;
inline float tonemap_amount = .7f;

inline void set_vulkan_quality(unsigned preset)
{
    quality_preset = preset;
    const auto& quality = render_quality(preset);
    detail_radius = quality.detail_distance;
    detail_update_cells = quality.detail_updates;
    point_shadow_budget = quality.point_shadows;
    spot_shadow_budget = quality.spot_shadows;
    texture_anisotropy = PresetAnisotropy[preset];
}

// Interpret explicit command-line overrides as defaults. The saved user
// config and subsequent changes in the game own these settings.
inline void apply_vulkan_launch_settings(const char *parameters)
{
    if (!parameters)
        return;
    std::istringstream input(parameters);
    std::string token, value;
    while (input >> token)
    {
        if (token == "-android-mobile-preset" && input >> value)
        {
            const char *names[]{"Minimum", "Low", "Default", "High", "Extreme"};
            for (uint32_t i = 0; i < 5; ++i)
                if (value == names[i])
                {
                    set_vulkan_quality(i);
                    break;
                }
        }
        else if ((token == "-vk-detail-distance" || token == "-vk-anisotropy") && input >> value)
        {
            char *end = nullptr;
            const long distance = std::strtol(value.c_str(), &end, 10);
            if (end != value.c_str() && !*end)
            {
                if (token == "-vk-detail-distance")
                    detail_radius = int(std::clamp(distance, 0L, 80L));
                else
                    texture_anisotropy = int(std::clamp(distance, 1L, 16L));
            }
        }
        else if (token == "-vk-ambient-scale" || token == "-vk-hemi-scale" || token == "-vk-sun-scale")
        {
            if (!(input >> value))
                break;
            char *end = nullptr;
            const float scale = std::strtof(value.c_str(), &end);
            if (end != value.c_str() && !*end && std::isfinite(scale))
                (token == "-vk-ambient-scale" ? ambient_scale
                 : token == "-vk-hemi-scale"  ? hemisphere_scale
                                              : sun_scale) = std::clamp(scale, 0.f, 3.f);
        }
        else if (token == "-vk-no-tonemap")
            tonemap_enabled = 0;
    }
}
} // namespace xray::render::vulkan
