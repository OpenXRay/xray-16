#include "src/Layers/xrRenderVK/VulkanHardware.h"

#include <cassert>
#include <string>

using namespace xray::render::vulkan;

namespace
{
bool sampled_depth = true;
bool color_attachment = true;

void VKAPI_PTR formats(VkPhysicalDevice, VkFormat format, VkFormatProperties* output)
{
    *output = {};
    if (format == VK_FORMAT_R8G8B8A8_UNORM)
        output->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
            (color_attachment ? VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT : 0);
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM)
        output->optimalTilingFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
            (sampled_depth ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT : 0);
}
}

int main()
{
    VkPhysicalDeviceProperties properties{};
    properties.limits.maxPushConstantsSize = 128;
    properties.limits.maxColorAttachments = 2;
    properties.limits.maxImageDimension2D = 4096;
    properties.limits.maxPerStageDescriptorSamplers = 8;
    properties.limits.maxPerStageDescriptorSampledImages = 8;
    std::string error;
    assert(supports_game_formats(VK_NULL_HANDLE, formats, properties, error) && error.empty());
    sampled_depth = false;
    assert(!supports_game_formats(VK_NULL_HANDLE, formats, properties, error) && !error.empty());
    sampled_depth = true;
    color_attachment = false;
    assert(!supports_game_formats(VK_NULL_HANDLE, formats, properties, error));
    color_attachment = true;
    properties.limits.maxPushConstantsSize = 64;
    assert(!supports_game_formats(VK_NULL_HANDLE, formats, properties, error));
}
