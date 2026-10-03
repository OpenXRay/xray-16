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

void release_upload(VkDevice device, VkCommandPool pool, const TextureUploadDispatch& vk,
    PendingTextureUpload& upload)
{
    if (upload.fence)
        vk.destroy_fence(device, upload.fence, nullptr);
    if (upload.command)
        vk.free_command_buffers(device, pool, 1, &upload.command);
    if (upload.staging)
        vk.destroy_buffer(device, upload.staging, nullptr);
    if (upload.staging_memory)
        vk.free_memory(device, upload.staging_memory, nullptr);
    upload = {};
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
    const DdsTexture& source, UploadedTexture& result,
    std::vector<PendingTextureUpload>& pending_uploads, ImageStateTracker& image_states,
    std::string& error)
{
    collect_completed_uploads(device, pool, vk, pending_uploads);
    result = {};
    if (source.format == VK_FORMAT_UNDEFINED || source.pixels.empty() || source.copies.empty() || !source.mip_levels ||
        (source.layers != 1 && source.layers != 6) || (source.cube != (source.layers == 6)))
    {
        error = "texture data is empty";
        return false;
    }

    PendingTextureUpload upload;
    auto cleanup = [&]
    {
        release_upload(device, pool, vk, upload);
    };
    auto fail = [&](const char* why)
    {
        error = why;
        cleanup();
        if (result.image)
            image_states.forget_image(result.image);
        destroy_texture(device, vk, result);
        return false;
    };

    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = source.pixels.size();
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vk.create_buffer(device, &buffer_info, nullptr, &upload.staging) != VK_SUCCESS)
        return fail("Vulkan staging buffer creation failed");
    VkMemoryRequirements requirements{};
    vk.get_buffer_memory_requirements(device, upload.staging, &requirements);
    const uint32_t staging_type = memory_index(memory_types, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (staging_type == VK_MAX_MEMORY_TYPES)
        return fail("Vulkan host-visible coherent memory is unavailable");
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = staging_type;
    if (vk.allocate_memory(device, &allocate, nullptr, &upload.staging_memory) != VK_SUCCESS ||
        vk.bind_buffer_memory(device, upload.staging, upload.staging_memory, 0) != VK_SUCCESS)
        return fail("Vulkan staging memory allocation failed");
    void* mapped = nullptr;
    if (vk.map_memory(device, upload.staging_memory, 0, source.pixels.size(), 0, &mapped) != VK_SUCCESS)
        return fail("Vulkan staging memory mapping failed");
    std::memcpy(mapped, source.pixels.data(), source.pixels.size());
    vk.unmap_memory(device, upload.staging_memory);

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = source.format;
    image_info.extent = source.extent;
    image_info.mipLevels = source.mip_levels;
    image_info.arrayLayers = source.layers;
    image_info.flags = source.cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vk.create_image(device, &image_info, nullptr, &result.image) != VK_SUCCESS)
        return fail("Vulkan texture image creation failed");
    VkImageSubresourceRange image_range{};
    image_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_range.levelCount = source.mip_levels;
    image_range.layerCount = source.layers;
    if (!image_states.register_image(result.image, image_range, ImageUse::Undefined, error))
    {
        cleanup();
        destroy_texture(device, vk, result);
        return false;
    }
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
    if (vk.allocate_command_buffers(device, &command_info, &upload.command) != VK_SUCCESS)
        return fail("Vulkan upload command allocation failed");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vk.begin_command_buffer(upload.command, &begin) != VK_SUCCESS)
        return fail("Vulkan upload command recording failed");
    const ImageStateDispatch image_state_dispatch{vk.cmd_pipeline_barrier};
    if (!image_states.transition(upload.command, result.image, ImageUse::TransferDestination,
            image_state_dispatch, error))
    {
        cleanup();
        image_states.forget_image(result.image);
        destroy_texture(device, vk, result);
        return false;
    }
    vk.cmd_copy_buffer_to_image(upload.command, upload.staging, result.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(source.copies.size()), source.copies.data());
    if (!image_states.transition(upload.command, result.image, ImageUse::Sampled,
            image_state_dispatch, error))
    {
        cleanup();
        image_states.forget_image(result.image);
        destroy_texture(device, vk, result);
        return false;
    }
    if (vk.end_command_buffer(upload.command) != VK_SUCCESS)
        return fail("Vulkan upload command finalization failed");

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = result.image;
    view_info.viewType = source.cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = source.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = image_info.mipLevels;
    view_info.subresourceRange.layerCount = source.layers;
    if (vk.create_image_view(device, &view_info, nullptr, &result.view) != VK_SUCCESS)
        return fail("Vulkan texture view creation failed");

    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vk.create_fence(device, &fence_info, nullptr, &upload.fence) != VK_SUCCESS)
        return fail("Vulkan upload fence creation failed");
    try
    {
        pending_uploads.reserve(pending_uploads.size() + 1);
    }
    catch (...)
    {
        return fail("Vulkan pending upload queue allocation failed");
    }
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &upload.command;
    if (vk.queue_submit(queue, 1, &submit, upload.fence) != VK_SUCCESS)
        return fail("Vulkan texture upload submission failed");
    pending_uploads.push_back(upload);
    upload = {};
    error.clear();
    return true;
}

void collect_completed_uploads(VkDevice device, VkCommandPool pool, const TextureUploadDispatch& vk,
    std::vector<PendingTextureUpload>& pending_uploads)
{
    auto it = pending_uploads.begin();
    while (it != pending_uploads.end())
    {
        if (vk.get_fence_status(device, it->fence) == VK_SUCCESS)
        {
            release_upload(device, pool, vk, *it);
            it = pending_uploads.erase(it);
        }
        else
            ++it;
    }
}

bool wait_for_uploads(VkDevice device, VkCommandPool pool, const TextureUploadDispatch& vk,
    std::vector<PendingTextureUpload>& pending_uploads)
{
    for (const auto& upload : pending_uploads)
        if (vk.wait_for_fences(device, 1, &upload.fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
            return false;
    for (auto& upload : pending_uploads)
        release_upload(device, pool, vk, upload);
    pending_uploads.clear();
    return true;
}
}
