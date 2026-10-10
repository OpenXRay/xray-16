#include "src/Layers/xrRenderVK/VulkanGameLightingMath.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace xray::render::vulkan;

int main()
{
    const float ambient[]{0.1f, 0.1f, 0.1f};
    const float hemi[]{0.2f, 0.2f, 0.2f};
    const float sun[]{0.5f, 0.5f, 0.5f};
    const std::array<float, 3> origin{0.f, 0.f, 0.f};

    VulkanLightSnapshot point;
    point.active = true;
    point.position = {0.f, 0.f, 5.f};
    point.color = {0.5f, 0.5f, 0.5f};
    point.range = 10.f;
    const std::vector<VulkanLightSnapshot> point_lights{point};
    const VulkanObjectLighting lit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, point_lights);
    // The original local-light attenuation is 1 - (distance / range)^2.
    assert(lit.luminocity > 0.77f && lit.luminocity < 0.78f);
    assert(lit.hemi_luminocity > 0.57f && lit.hemi_luminocity < 0.58f);
    assert(lit.hemi_cube[4] > lit.hemi_cube[5]);

    point.active = false;
    const VulkanObjectLighting unlit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(unlit.luminocity < lit.luminocity);
    assert(unlit.hemi_cube[4] == unlit.hemi_cube[5]);

    point.active = true;
    point.type = VulkanLightType::Spot;
    point.direction = {0.f, 0.f, -1.f};
    const VulkanObjectLighting spot_lit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    point.direction = {0.f, 0.f, 1.f};
    const VulkanObjectLighting spot_dark = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(spot_lit.luminocity > spot_dark.luminocity);

    point.type = VulkanLightType::Direct;
    const VulkanObjectLighting direct_excluded = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(direct_excluded.luminocity == unlit.luminocity);

    point.type = VulkanLightType::Point;
    point.color.fill(std::numeric_limits<float>::quiet_NaN());
    const VulkanObjectLighting sanitized = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceLights, {point});
    assert(std::isfinite(sanitized.luminocity));
    assert(sanitized.luminocity == 0.f);
}
