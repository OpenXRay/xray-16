#pragma once

#include <cstdint>

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
}
