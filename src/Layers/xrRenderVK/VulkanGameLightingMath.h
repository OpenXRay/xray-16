#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace xray::render::vulkan
{
enum class VulkanLightType : uint32_t
{
    Direct,
    Point,
    Spot,
    OmniPart,
    Reflected
};

struct VulkanLightSnapshot
{
    VulkanLightType type{VulkanLightType::Point};
    bool active{};
    bool shadow{};
    std::array<float, 3> position{};
    std::array<float, 3> direction{0.f, -1.f, 0.f};
    std::array<float, 3> color{1.f, 1.f, 1.f};
    float range{1.f};
    float cone{0.785398163f};
};

struct VulkanObjectLighting
{
    float luminocity{};
    float hemi_luminocity{};
    std::array<float, 6> hemi_cube{};
};

enum VulkanObjectLightingMode : uint32_t
{
    VulkanTraceLights = 1u << 0,
    VulkanTraceSun = 1u << 1,
    VulkanTraceHemi = 1u << 2,
    VulkanTraceAll = VulkanTraceLights | VulkanTraceSun | VulkanTraceHemi
};

VulkanObjectLighting evaluate_object_lighting(const std::array<float, 3>& position,
    const float* ambient, const float* hemi, const float* sun, uint32_t mode,
    const std::vector<VulkanLightSnapshot>& lights);
}
