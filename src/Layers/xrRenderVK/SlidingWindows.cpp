#include "SlidingWindows.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
uint16_t u16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t u32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
        (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
}

bool decode_slide_windows(const uint8_t* data, size_t size,
    const std::vector<uint32_t>& indices, size_t vertex_count,
    std::vector<SlideWindow>& result, std::string& error)
{
    if (!data || size < 28 || indices.size() > UINT32_MAX)
    {
        error = "invalid OGF sliding window header or geometry";
        return false;
    }
    const uint32_t count = u32(data + 16);
    if (!count || count > 65536 || size != 20 + size_t(count) * 8)
    {
        error = "invalid OGF sliding window table size";
        return false;
    }
    std::vector<SlideWindow> windows;
    windows.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        const uint8_t* p = data + 20 + size_t(i) * 8;
        const uint32_t offset = u32(p), triangles = u16(p + 4);
        const uint16_t vertices = u16(p + 6);
        if (!triangles || !vertices || vertices > vertex_count ||
            offset > indices.size() || triangles > (indices.size() - offset) / 3)
        {
            error = "OGF sliding window exceeds its buffers";
            return false;
        }
        const uint32_t index_count = triangles * 3;
        for (size_t j = offset; j < size_t(offset) + index_count; ++j)
            if (indices[j] >= vertices)
            {
                error = "OGF sliding window references an inactive vertex";
                return false;
            }
        windows.push_back({offset, index_count, vertices});
    }
    result = std::move(windows);
    error.clear();
    return true;
}

SlideWindow select_slide_window(const std::vector<SlideWindow>& windows, float lod,
    size_t full_index_count)
{
    if (windows.empty()) return {0, static_cast<uint32_t>(full_index_count), 0};
    const float clamped = std::isfinite(lod) ? std::clamp(lod, 0.f, 1.f) : 1.f;
    const size_t index = static_cast<size_t>((1.f - clamped) * float(windows.size() - 1) + 0.5f);
    return windows[index];
}

float lod_for_distance(float radius, float distance_squared)
{
    if (!std::isfinite(radius) || radius <= 0.f ||
        !std::isfinite(distance_squared) || distance_squared < 0.f) return 1.f;
    // Approximate angular radius. A radius spanning roughly 10% of the view
    // stays at full detail; past 1% it uses the coarsest available window.
    const float projected = radius / std::sqrt(std::max(distance_squared, 0.0001f));
    return std::clamp((projected - 0.01f) / 0.09f, 0.f, 1.f);
}
}
