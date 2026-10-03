#include "xrEngine/stdafx.h"
#include "VulkanGameLighting.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
void copy3(const Fvector& source, std::array<float, 3>& destination)
{
    destination = {source.x, source.y, source.z};
}
}

void VulkanLight::set_type(LT type)
{
    switch (type)
    {
    case DIRECT: state_.type = VulkanLightType::Direct; break;
    case SPOT: state_.type = VulkanLightType::Spot; break;
    case OMNIPART: state_.type = VulkanLightType::OmniPart; break;
    case REFLECTED: state_.type = VulkanLightType::Reflected; break;
    case POINT:
    default: state_.type = VulkanLightType::Point; break;
    }
}

void VulkanLight::set_position(const Fvector& value)
{
    copy3(value, state_.position);
}

void VulkanLight::set_cone(float angle)
{
    state_.cone = std::isfinite(angle) ? std::clamp(angle, 0.f, 2.f) : 0.785398163f;
}

void VulkanLight::set_range(float range)
{
    state_.range = std::isfinite(range) ? std::max(range, 0.f) : 0.f;
}

void VulkanLight::set_rotation(const Fvector& direction, const Fvector& right)
{
    copy3(direction, state_.direction);
    copy3(right, right_);
    const float length = std::sqrt(state_.direction[0] * state_.direction[0] +
        state_.direction[1] * state_.direction[1] + state_.direction[2] * state_.direction[2]);
    if (std::isfinite(length) && length > 1e-6f)
        for (float& value : state_.direction) value /= length;
    else
        state_.direction = {0.f, -1.f, 0.f};
}

void VulkanLight::set_color(const Fcolor& color)
{
    state_.color = {color.r, color.g, color.b};
}

void VulkanLight::set_color(float r, float g, float b)
{
    state_.color = {r, g, b};
}

void VulkanGlow::set_position(const Fvector& value)
{
    copy3(value, position_);
}

void VulkanGlow::set_direction(const Fvector& value)
{
    copy3(value, direction_);
}

void VulkanGlow::set_color(const Fcolor& color)
{
    color_ = {color.r, color.g, color.b, color.a};
}

void VulkanGlow::set_color(float r, float g, float b)
{
    color_ = {r, g, b, 1.f};
}

void VulkanObjectSpecific::update(const float* ambient, const float* hemi, const float* sun,
    const std::vector<VulkanLightSnapshot>& lights)
{
    std::array<float, 3> position{};
    if (parent_)
    {
        const RenderData& data = parent_->GetRenderData();
        Fvector origin = data.xform.c;
        if (data.visual)
        {
            const Fvector& local_center = data.visual->getVisData().sphere.P;
            data.xform.transform_tiny(origin, local_center);
        }
        position = {origin.x, origin.y, origin.z};
    }

    const VulkanObjectLighting lighting = evaluate_object_lighting(position,
        ambient, hemi, sun, mode_, lights);
    luminocity_ = lighting.luminocity;
    hemi_luminocity_ = lighting.hemi_luminocity;
    hemi_cube_ = lighting.hemi_cube;
}
}
