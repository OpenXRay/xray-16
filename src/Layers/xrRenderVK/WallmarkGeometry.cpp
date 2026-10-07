#include "WallmarkGeometry.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
WallmarkVertex interpolate(const WallmarkVertex& a, const WallmarkVertex& b, float t)
{
    WallmarkVertex result;
    for (size_t i = 0; i != 3; ++i)
        result.position[i] = a.position[i] + (b.position[i] - a.position[i]) * t;
    result.u = a.u + (b.u - a.u) * t;
    result.v = a.v + (b.v - a.v) * t;
    return result;
}

void clip(std::vector<WallmarkVertex>& polygon, bool u, float boundary, bool upper)
{
    if (polygon.empty()) return;
    std::vector<WallmarkVertex> output;
    output.reserve(polygon.size() + 1);
    auto previous = polygon.back();
    for (const auto& vertex : polygon)
    {
        const float a = u ? previous.u : previous.v;
        const float b = u ? vertex.u : vertex.v;
        const bool was_inside = upper ? a <= boundary : a >= boundary;
        const bool is_inside = upper ? b <= boundary : b >= boundary;
        if (was_inside != is_inside)
        {
            const float t = (boundary - a) / (b - a);
            output.push_back(interpolate(previous, vertex, std::clamp(t, 0.f, 1.f)));
        }
        if (is_inside) output.push_back(vertex);
        previous = vertex;
    }
    polygon.swap(output);
}
}

std::vector<WallmarkVertex> clip_wallmark_triangle(const std::array<WallmarkVertex, 3>& triangle)
{
    for (const auto& vertex : triangle)
        if (!std::isfinite(vertex.u) || !std::isfinite(vertex.v) ||
            !std::isfinite(vertex.position[0]) || !std::isfinite(vertex.position[1]) ||
            !std::isfinite(vertex.position[2])) return {};
    std::vector<WallmarkVertex> polygon(triangle.begin(), triangle.end());
    clip(polygon, true, 0.f, false);
    clip(polygon, true, 1.f, true);
    clip(polygon, false, 0.f, false);
    clip(polygon, false, 1.f, true);
    if (polygon.size() < 3) return {};
    std::vector<WallmarkVertex> triangles;
    triangles.reserve((polygon.size() - 2) * 3);
    for (size_t i = 2; i < polygon.size(); ++i)
    {
        triangles.push_back(polygon[0]);
        triangles.push_back(polygon[i - 1]);
        triangles.push_back(polygon[i]);
    }
    return triangles;
}
}
