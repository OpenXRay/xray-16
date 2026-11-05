#include "xrEngine/stdafx.h"
#include "VulkanGameDevice.h"
#include "VulkanSettings.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace xray::render::vulkan
{
namespace
{
// The light shader draws a screen triangle. Restrict it to the projection of
// its range sphere so distant lamps do not shade millions of unrelated pixels.
// A cube encloses the sphere; if it crosses the near plane, use the whole
// viewport rather than risk clipping the visible part of the light.
VkRect2D light_scissor(const Fmatrix &view_projection, const VulkanLightSnapshot &light, VkExtent2D extent)
{
    const VkRect2D full{{0, 0}, extent};
    if (!extent.width || !extent.height)
        return {};
    Fvector4 clips[8];
    uint32_t index = 0;
    for (int x : {-1, 1})
        for (int y : {-1, 1})
            for (int z : {-1, 1})
            {
                Fvector4 world;
                world.set(light.position[0] + x * light.range, light.position[1] + y * light.range,
                          light.position[2] + z * light.range, 1.f);
                view_projection.transform(clips[index++], world);
            }
    const auto outside = [&](auto plane) {
        return std::all_of(std::begin(clips), std::end(clips), [&](const Fvector4 &clip) { return plane(clip); });
    };
    if (outside([](const Fvector4 &c) { return c.x < -c.w; }) || outside([](const Fvector4 &c) { return c.x > c.w; }) ||
        outside([](const Fvector4 &c) { return c.y < -c.w; }) || outside([](const Fvector4 &c) { return c.y > c.w; }) ||
        outside([](const Fvector4 &c) { return c.z < 0.f; }) || outside([](const Fvector4 &c) { return c.z > c.w; }))
        return {};
    float min_x = 1.f, min_y = 1.f, max_x = 0.f, max_y = 0.f;
    for (const auto &clip : clips)
    {
        if (!std::isfinite(clip.w) || clip.w <= 1e-5f)
            return full;
        const float nx = clip.x / clip.w, ny = clip.y / clip.w;
        if (!std::isfinite(nx) || !std::isfinite(ny))
            return full;
        min_x = std::min(min_x, .5f * (nx + 1.f));
        max_x = std::max(max_x, .5f * (nx + 1.f));
        min_y = std::min(min_y, .5f * (1.f - ny));
        max_y = std::max(max_y, .5f * (1.f - ny));
    }
    const auto left = uint32_t(std::floor(std::clamp(min_x, 0.f, 1.f) * extent.width));
    const auto top = uint32_t(std::floor(std::clamp(min_y, 0.f, 1.f) * extent.height));
    const auto right = uint32_t(std::ceil(std::clamp(max_x, 0.f, 1.f) * extent.width));
    const auto bottom = uint32_t(std::ceil(std::clamp(max_y, 0.f, 1.f) * extent.height));
    return {{int32_t(left), int32_t(top)}, {right - left, bottom - top}};
}
} // namespace
void VulkanGameDevice::render_lights(const float (&mvp)[16], bool render_world, const DeferredLight &light,
                                     const SunShadowUniform &sun_uniform)
{
    local_uniforms_.clear();
    local_scissors_.clear();
    local_projection_views_.clear();
    if (render_world)
    {
        Fmatrix camera, inverse;
        std::memcpy(&camera, mvp, sizeof(camera));
        inverse.invert_44(camera);
        uint32_t shadow_slot = 0, point_slots = 0, spot_slots = 0;
        light_candidates_.clear();
        const VkExtent2D extent = window_.frame().extent();
        for (const auto &snapshot : light_snapshots_)
        {
            if (!snapshot.active || !std::isfinite(snapshot.range) || snapshot.range <= .1f ||
                (snapshot.type != VulkanLightType::Point && snapshot.type != VulkanLightType::Spot &&
                 snapshot.type != VulkanLightType::OmniPart) ||
                !std::all_of(snapshot.position.begin(), snapshot.position.end(),
                             [](float value) { return std::isfinite(value); }))
                continue;
            const VkRect2D scissor = light_scissor(camera, snapshot, extent);
            if (!scissor.extent.width || !scissor.extent.height)
                continue;
            const float dx = snapshot.position[0] - Device.vCameraPosition.x;
            const float dy = snapshot.position[1] - Device.vCameraPosition.y;
            const float dz = snapshot.position[2] - Device.vCameraPosition.z;
            light_candidates_.push_back({&snapshot, dx * dx + dy * dy + dz * dz, scissor});
        }
        std::stable_sort(light_candidates_.begin(), light_candidates_.end(),
                         [](const LightCandidate &a, const LightCandidate &b) { return a.distance < b.distance; });
        for (const auto &candidate : light_candidates_)
        {
            if (local_uniforms_.size() >= LocalLightCapacity)
                break;
            const auto &light_snapshot = *candidate.light;
            const VkRect2D scissor = candidate.scissor;
            if (!scissor.extent.width || !scissor.extent.height)
                continue;
            LocalLightUniform local{};
            local.debug_mode[0] = sun_uniform.options[1];
            std::memcpy(local.inverse_view_projection, &inverse, sizeof(inverse));
            const auto &position = light_snapshot.position;
            local.position_range[0] = position[0];
            local.position_range[1] = position[1];
            local.position_range[2] = position[2];
            local.position_range[3] = std::clamp(light_snapshot.range, .2f, 1000.f);
            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                local.direction_cone[axis] = light_snapshot.direction[axis];
                local.color_type[axis] =
                    std::isfinite(light_snapshot.color[axis]) ? std::max(0.f, light_snapshot.color[axis]) : 0.f;
            }
            const bool spot = light_snapshot.type == VulkanLightType::Spot;
            local.color_type[3] = spot ? 1.f : 0.f;
            const float cone = std::clamp(light_snapshot.cone, .15f, 3.f);
            local.direction_cone[3] = std::cos(cone * .5f);
            const uint64_t pixels = uint64_t(scissor.extent.width) * scissor.extent.height;
            const uint64_t screen_pixels = uint64_t(extent.width) * extent.height;
            const bool cast = light_snapshot.shadow && shadow_slot < LocalShadowSlots &&
                              (spot ? spot_slots < uint32_t(std::max(spot_shadow_budget, 0))
                                    : point_slots < uint32_t(std::max(point_shadow_budget, 0))) &&
                              pixels * 20 >= screen_pixels;
            local.shadow_params[0] = float(shadow_slot * LocalShadowFaces);
            local.shadow_params[1] = LocalShadowReceiverOffset;
            local.shadow_params[2] = cast ? 1.f : 0.f;
            const bool cookie = spot && !light_snapshot.texture.empty();
            local.shadow_params[3] = cookie ? 1.f : 0.f;
            for (unsigned axis = 0; axis < 3; ++axis)
                local.spot_right[axis] = light_snapshot.right[axis];
            local.spot_right[3] = std::tan(cone * .5f);
            local_projection_views_.push_back(light_projection(cookie ? light_snapshot.texture : std::string{}));
            if (cast)
            {
                const float directions[6][3]{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
                const float ups[6][3]{{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
                const uint32_t faces = spot ? 1 : LocalShadowFaces;
                for (uint32_t face = 0; face < faces; ++face)
                {
                    Fvector origin, direction, up;
                    origin.set(position[0], position[1], position[2]);
                    direction.set(directions[face][0], directions[face][1], directions[face][2]);
                    up.set(ups[face][0], ups[face][1], ups[face][2]);
                    if (spot)
                    {
                        direction
                            .set(light_snapshot.direction[0], light_snapshot.direction[1], light_snapshot.direction[2])
                            .normalize_safe();
                        up.set(0.f, std::abs(direction.y) > .95f ? 0.f : 1.f, std::abs(direction.y) > .95f ? 1.f : 0.f);
                    }
                    Fmatrix view, projection, vp;
                    view.build_camera_dir(origin, direction, up);
                    projection.build_projection(spot ? cone : 1.57079632679f, 1.f, .1f, local.position_range[3]);
                    vp.mul(projection, view);
                    std::memcpy(local.shadow_matrices[face], &vp, sizeof(vp));
                }
                ++shadow_slot;
                if (spot)
                    ++spot_slots;
                else
                    ++point_slots;
            }
            local_uniforms_.push_back(local);
            local_scissors_.push_back(scissor);
        }
    }
    std::copy_n(light.direction_ambient, 4, forward_lighting_.sun_direction_ambient);
    std::copy_n(sun_uniform.ambient_color, 4, forward_lighting_.ambient_color);
    std::copy_n(sun_uniform.hemi_color, 4, forward_lighting_.hemi_color);
    std::copy_n(light.color, 3, forward_lighting_.sun_color_count);
    const uint32_t forward_count = std::min<size_t>(local_uniforms_.size(), ForwardLightCapacity);
    forward_lighting_.sun_color_count[3] = static_cast<float>(forward_count);
    const VkExtent2D forward_extent = window_.frame().extent();
    forward_lighting_.viewport[0] = forward_extent.width ? 1.f / forward_extent.width : 0.f;
    forward_lighting_.viewport[1] = forward_extent.height ? 1.f / forward_extent.height : 0.f;
    forward_lighting_.viewport[2] = std::clamp(weather_lighting_.light.color[3], 0.f, 1.f);
    forward_lighting_.hemi_color[3] = render_world && weather_set_ ? 1.f : 0.f;
    for (uint32_t i = 0; i < forward_count; ++i)
    {
        std::copy_n(local_uniforms_[i].position_range, 4, forward_lighting_.local[i].position_range);
        std::copy_n(local_uniforms_[i].direction_cone, 4, forward_lighting_.local[i].direction_cone);
        std::copy_n(local_uniforms_[i].color_type, 4, forward_lighting_.local[i].color_type);
    }
}
} // namespace xray::render::vulkan
