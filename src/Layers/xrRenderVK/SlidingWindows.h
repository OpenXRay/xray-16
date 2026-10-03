#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct SlideWindow
{
    uint32_t offset{};
    uint32_t index_count{};
    uint16_t vertex_count{};
};

// OGF_SWIDATA stores four reserved words, a count, then FSlideWindow records.
// Validate every window before exposing any of them to vkCmdDrawIndexed.
bool decode_slide_windows(const uint8_t* data, size_t size,
    const std::vector<uint32_t>& indices, size_t vertex_count,
    std::vector<SlideWindow>& result, std::string& error);
SlideWindow select_slide_window(const std::vector<SlideWindow>& windows, float lod,
    size_t full_index_count);
float lod_for_distance(float radius, float distance_squared);
}
