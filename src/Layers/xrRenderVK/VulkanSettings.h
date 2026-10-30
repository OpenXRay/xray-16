#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace xray::render::vulkan
{
// Values exposed through the game's renderer console. The Vulkan renderer
// reads these every frame, so the UI and saved user config share one state.
inline uint32_t quality_preset = 2;
inline int detail_radius = 40;
inline float ambient_scale = 1.f;
inline float hemisphere_scale = 1.f;
inline float sun_scale = 1.f;
inline int sun_enabled = 1;
inline int tonemap_enabled = 1;
inline float tonemap_middlegray = 1.f;
inline float tonemap_amount = .7f;

// Apply explicit launcher choices after user.ltx has loaded. Do this once per
// device: later in-game console changes remain effective for the session.
inline void apply_vulkan_launch_settings(const char* parameters)
{
    if (!parameters) return;
    std::istringstream input(parameters);
    std::string token, value;
    while (input >> token)
    {
        if (token == "-android-mobile-preset" && input >> value)
        {
            const char* names[]{"Minimum","Low","Default","High","Extreme"};
            for (uint32_t i=0;i<5;++i) if (value==names[i])
            { quality_preset=i; detail_radius=int(i)*20; break; }
        }
        else if (token == "-vk-detail-distance" && input >> value)
        {
            char* end=nullptr;const long distance=std::strtol(value.c_str(),&end,10);
            if (end!=value.c_str() && !*end) detail_radius=int(std::clamp(distance,0L,80L));
        }
        else if (token == "-vk-ambient-scale" || token == "-vk-hemi-scale" || token == "-vk-sun-scale")
        {
            if (!(input >> value)) break;
            char* end=nullptr;const float scale=std::strtof(value.c_str(),&end);
            if (end!=value.c_str() && !*end && std::isfinite(scale))
                (token=="-vk-ambient-scale" ? ambient_scale : token=="-vk-hemi-scale" ? hemisphere_scale : sun_scale)=std::clamp(scale,0.f,3.f);
        }
        else if (token == "-vk-no-tonemap") tonemap_enabled=0;
    }
}
}
