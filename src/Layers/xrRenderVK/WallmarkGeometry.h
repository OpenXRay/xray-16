#pragma once

#include <array>
#include <vector>

namespace xray::render::vulkan
{
struct WallmarkVertex
{
    std::array<float, 3> position{};
    float u{}, v{};
};

// Clip projected geometry to the decal's UV square before it reaches the GPU.
// Interpolation preserves positions on the source face at each clipped edge.
std::vector<WallmarkVertex> clip_wallmark_triangle(const std::array<WallmarkVertex, 3>& triangle);
}
