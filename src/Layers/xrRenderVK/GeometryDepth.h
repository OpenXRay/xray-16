#pragma once
#include <vulkan/vulkan.h>
namespace xray::render::vulkan
{
inline VkCompareOp geometry_depth_compare(bool shadow)
{
    return shadow ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_LESS_OR_EQUAL;
}
}
