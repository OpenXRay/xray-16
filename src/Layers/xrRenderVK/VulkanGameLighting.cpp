#include "xrEngine/stdafx.h"
#include "VulkanGameLighting.h"
#include "LevelLights.h"
#include "VulkanSettings.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/device.h"
#include "xrCDB/xr_area.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"

#include <algorithm>
#include <cmath>
#include <atomic>

namespace xray::render::vulkan
{
namespace
{
void copy3(const Fvector &source, std::array<float, 3> &destination)
{
    destination = {source.x, source.y, source.z};
}
} // namespace

VulkanLight::VulkanLight()
{
    static std::atomic<uint64_t> next{1};
    state_.id = next.fetch_add(1);
}

void VulkanLight::set_type(LT type)
{
    switch (type)
    {
    case DIRECT:
        state_.type = VulkanLightType::Direct;
        break;
    case SPOT:
        state_.type = VulkanLightType::Spot;
        break;
    case OMNIPART:
        state_.type = VulkanLightType::OmniPart;
        break;
    case REFLECTED:
        state_.type = VulkanLightType::Reflected;
        break;
    case POINT:
    default:
        state_.type = VulkanLightType::Point;
        break;
    }
}

void VulkanLight::set_position(const Fvector &value)
{
    copy3(value, state_.position);
}

void VulkanLight::set_cone(float angle)
{
    state_.cone = std::isfinite(angle) ? std::clamp(angle, 0.f, 3.14159265f) : 0.785398163f;
}

void VulkanLight::set_range(float range)
{
    state_.range = std::isfinite(range) ? std::max(range, 0.f) : 0.f;
}

void VulkanLight::set_rotation(const Fvector &direction, const Fvector &right)
{
    copy3(direction, state_.direction);
    copy3(right, right_);
    state_.right = right_;
    const float length =
        std::sqrt(state_.direction[0] * state_.direction[0] + state_.direction[1] * state_.direction[1] +
                  state_.direction[2] * state_.direction[2]);
    if (std::isfinite(length) && length > 1e-6f)
        for (float &value : state_.direction)
            value /= length;
    else
        state_.direction = {0.f, -1.f, 0.f};
}

void VulkanLight::set_color(const Fcolor &color)
{
    state_.color = {color.r, color.g, color.b};
}

void VulkanLight::set_color(float r, float g, float b)
{
    state_.color = {r, g, b};
}

void VulkanGlow::set_position(const Fvector &value)
{
    copy3(value, position_);
}

void VulkanGlow::set_direction(const Fvector &value)
{
    copy3(value, direction_);
}

void VulkanGlow::set_color(const Fcolor &color)
{
    color_ = {color.r, color.g, color.b, color.a};
}

void VulkanGlow::set_color(float r, float g, float b)
{
    color_ = {r, g, b, 1.f};
}

void VulkanObjectSpecific::update(const float *ambient, const float *hemi, const float *sun,
                                  const VulkanHemiLightGrid &lights, const VulkanHemiLightGrid *baked,
                                  unsigned *ray_budget)
{
    std::array<float, 3> position{};
    if (parent_)
    {
        const RenderData &data = parent_->GetRenderData();
        Fvector origin = data.xform.c;
        if (data.visual)
        {
            const Fvector &local_center = data.visual->getVisData().sphere.P;
            data.xform.transform_tiny(origin, local_center);
            origin.y += .3f * data.visual->getVisData().sphere.R;
        }
        position = {origin.x, origin.y, origin.z};
    }

    const float dx = position[0] - last_position_[0];
    const float dy = position[1] - last_position_[1];
    const float dz = position[2] - last_position_[2];
    const uint32_t frame = Device.dwFrame;
    if (lighting_initialized_ && int32_t(frame - next_update_) < 0 && dx*dx + dy*dy + dz*dz < .15f*.15f)
        return;
    const uint32_t elapsed = std::clamp(frame - last_update_, 1u, 16u);
    last_update_ = frame;
    last_position_ = position;
    next_update_ = frame + 4 + uint32_t((reinterpret_cast<uintptr_t>(this) >> 4) % 5);
    lighting_sources_.clear();
    lights.append(position, lighting_sources_);
    if (baked)
        baked->append(position, lighting_sources_);
    visibility_.position(position);
    unsigned work = ray_budget ? std::min(*ray_budget, 3u) : 0;
    const unsigned before = work;
    const auto trace = [](const auto &from, const auto &to) {
        if (!g_pGameLevel)
            return false;
        Fvector origin, direction;
        origin.set(from[0], from[1], from[2]);
        direction.set(to[0] - from[0], to[1] - from[1], to[2] - from[2]);
        const float distance = direction.magnitude();
        if (distance < EPS_L)
            return false;
        direction.div(distance);
        collide::rq_result hit;
        return g_pGameLevel->ObjectSpace.RayPick(origin, direction, std::max(distance - .05f, 0.f), collide::rqtStatic,
                                                 hit, nullptr);
    };
    if (mode_ & VulkanTraceHemi)
        visibility_.sky(work, trace);
    for (auto &light : lighting_sources_)
    {
        float distance = 0;
        for (unsigned i = 0; i < 3; ++i)
            distance += (light.position[i] - position[i]) * (light.position[i] - position[i]);
        if (distance < light.range * light.range)
            light.visibility = visibility_.light(light.id, light.position, Device.dwFrame, work, trace);
    }
    if (ray_budget)
        *ray_budget -= before - work;
    const auto sky_faces = visibility_.faces(hemi_sky_scale);
    const VulkanObjectLighting lighting =
        evaluate_object_lighting(position, ambient, hemi, sun, mode_, lighting_sources_,
                                 ray_budget ? &sky_faces : nullptr, hemi_light_scale, hemi_light_flow);
    const float blend = lighting_initialized_ ? std::clamp(Device.fTimeDelta * elapsed * hemi_smooth, 0.f, 1.f) : 1.f;
    luminocity_ = lighting.luminocity;
    hemi_luminocity_ += blend * (lighting.hemi_luminocity - hemi_luminocity_);
    for (unsigned i = 0; i < 6; ++i)
        hemi_cube_[i] += blend * (lighting.hemi_cube[i] - hemi_cube_[i]);
    lighting_initialized_ = true;
}
} // namespace xray::render::vulkan
