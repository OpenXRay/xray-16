#include "src/Layers/xrRenderVK/WallmarkGeometry.h"

#include <cassert>
#include <cmath>

using namespace xray::render::vulkan;

int main()
{
    const std::array<WallmarkVertex, 3> triangle{{
        {{{-1.f, 0.f, 0.f}}, -1.f, .5f},
        {{{2.f, 0.f, 0.f}}, 2.f, .5f},
        {{{.5f, 1.f, 0.f}}, .5f, 1.5f}
    }};
    const auto clipped = clip_wallmark_triangle(triangle);
    assert(clipped.size() >= 3 && clipped.size() % 3 == 0);
    for (const auto& vertex : clipped)
    {
        assert(vertex.u >= 0.f && vertex.u <= 1.f);
        assert(vertex.v >= 0.f && vertex.v <= 1.f);
        assert(std::abs(vertex.position[0] - vertex.u) < .0001f);
    }
    auto invalid = triangle;
    invalid[0].u = NAN;
    assert(clip_wallmark_triangle(invalid).empty());
}
