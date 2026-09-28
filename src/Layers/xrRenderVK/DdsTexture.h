#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct DdsTexture
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent{};
    uint32_t mip_levels{};
    uint32_t layers = 1;
    bool cube{};
    std::vector<std::uint8_t> pixels;
    std::vector<VkBufferImageCopy> copies;
};

// Accepts the same DDS bytes returned by the game's virtual filesystem.
bool decode_dds(const void* data, size_t size, bool bc_supported, DdsTexture& result, std::string& error);
}
