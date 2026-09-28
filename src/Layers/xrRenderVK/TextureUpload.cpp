#include "TextureUpload.h"

#include <cstring>

namespace xray::render::vulkan
{
namespace
{
uint32_t memory_index(const VkPhysicalDeviceMemoryProperties& types, uint32_t allowed,
    VkMemoryPropertyFlags required)
{
    for (uint32_t index = 0; index < types.memoryTypeCount; ++index)
        if ((allowed & (1u << index)) && (types.memoryTypes[index].propertyFlags & required) == required)
            return index;
    return VK_MAX_MEMORY_TYPES;
}
}

void destroy_texture(VkDevice device, const TextureUploadDispatch& vk, UploadedTexture& texture)
{
    if (texture.view)
        vk.destroy_image_view(device, texture.view, nullptr);
    if (texture.image)
        vk.destroy_image(device, texture.image, nullptr);
    if (texture.memory)
        vk.free_memory(device, texture.memory, nullptr);
    texture = {};
}

bool upload_texture(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_types, const TextureUploadDispatch& vk,
    const DdsTexture& source, UploadedTexture& result, std::string& error)
{
    result = {};
    if (source.format == VK_FORMAT_UNDEFINED || source.pixels.empty() || source.copies.empty())
    {
        error = "texture data is empty";
        return false;
    }

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    auto cleanup = [&]
    {
        if (command)
            vk.free_command_buffers(device, pool, 1, &command);
        if (staging)
            vk.destroy_buffer(device, staging, nullptr);
        if (staging_memory)
            vk.free_memory(device, staging_memory, nullptr);
    };
    auto fail = [&](const char* why)
    {
        error = why;
        cleanup();
        destroy_texture(device, vk, result);
        return false;
    };

    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = source.pixels.size();
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vk.create_buffer(device, &buffer_info, nullptr, &staging) != VK_SUCCESS)
        return fail("Vulkan staging buffer creation failed");
    VkMemoryRequirements requirements{};
    vk.get_buffer_memory_requirements(device, staging, &requirements);
    const uint32_t staging_type = memory_index(memory_types, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (staging_type == VK_MAX_MEMORY_TYPES)
        return fail("Vulkan host-visible coherent memory is unavailable");
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = staging_type;
    if (vk.allocate_memory(device, &allocate, nullptr, &staging_memory) != VK_SUCCESS ||
        vk.bind_buffer_memory(device, staging, staging_memory, 0) != VK_SUCCESS)
        return fail("Vulkan staging memory allocation failed");
    void* mapped = nullptr;
    if (vk.map_memory(device, staging_memory, 0, source.pixels.size(), 0, &mapped) != VK_SUCCESS)
        return fail("Vulkan staging memory mapping failed");
    std::memcpy(mapped, source.pixels.data(), source.pixels.size());
    vk.unmap_memory(device, staging_memory);

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = source.format;
    image_info.extent = source.extent;
    image_info.mipLevels = static_cast<uint32_t>(source.copies.size());
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vk.create_image(device, &image_info, nullptr, &result.image) != VK_SUCCESS)
        return fail("Vulkan texture image creation failed");
    vk.get_image_memory_requirements(device, result.image, &requirements);
    uint32_t image_type = memory_index(memory_types, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (image_type == VK_MAX_MEMORY_TYPES)
        image_type = memory_index(memory_types, requirements.memoryTypeBits, 0);
    if (image_type == VK_MAX_MEMORY_TYPES)
        return fail("Vulkan texture memory type is unavailable");
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = image_type;
    if (vk.allocate_memory(device, &allocate, nullptr, &result.memory) != VK_SUCCESS ||
        vk.bind_image_memory(device, result.image, result.memory, 0) != VK_SUCCESS)
        return fail("Vulkan texture image memory allocation failed");

    VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;
    if (vk.allocate_command_buffers(device, &command_info, &command) != VK_SUCCESS)
        return fail("Vulkan upload command allocation failed");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vk.begin_command_buffer(command, &begin) != VK_SUCCESS)
        return fail("Vulkan upload command recording failed");
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.image = result.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = image_info.mipLevels;
    barrier.subresourceRange.layerCount = 1;
    vk.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    vk.cmd_copy_buffer_to_image(command, staging, result.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(source.copies.size()), source.copies.data());
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vk.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    if (vk.end_command_buffer(command) != VK_SUCCESS)
        return fail("Vulkan upload command finalization failed");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    if (vk.queue_submit(queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
        return fail("Vulkan texture upload submission failed");
    if (vk.queue_wait_idle(queue) != VK_SUCCESS)
    {
        // The queue may still access staging resources; keep them alive until device teardown.
        error = "Vulkan texture upload queue synchronization failed";
        return false;
    }
    cleanup();

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = result.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = source.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = image_info.mipLevels;
    view_info.subresourceRange.layerCount = 1;
    if (vk.create_image_view(device, &view_info, nullptr, &result.view) != VK_SUCCESS)
        return fail("Vulkan texture view creation failed");
    error.clear();
    return true;
}
}
