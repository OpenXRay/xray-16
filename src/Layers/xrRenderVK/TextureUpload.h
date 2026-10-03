#pragma once

#include "DdsTexture.h"
#include "ImageStateTracker.h"

#include <vector>

namespace xray::render::vulkan
{
struct TextureUploadDispatch
{
    PFN_vkCreateBuffer create_buffer{};
    PFN_vkDestroyBuffer destroy_buffer{};
    PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements{};
    PFN_vkCreateImage create_image{};
    PFN_vkDestroyImage destroy_image{};
    PFN_vkGetImageMemoryRequirements get_image_memory_requirements{};
    PFN_vkAllocateMemory allocate_memory{};
    PFN_vkFreeMemory free_memory{};
    PFN_vkBindBufferMemory bind_buffer_memory{};
    PFN_vkBindImageMemory bind_image_memory{};
    PFN_vkMapMemory map_memory{};
    PFN_vkUnmapMemory unmap_memory{};
    PFN_vkCreateImageView create_image_view{};
    PFN_vkDestroyImageView destroy_image_view{};
    PFN_vkAllocateCommandBuffers allocate_command_buffers{};
    PFN_vkFreeCommandBuffers free_command_buffers{};
    PFN_vkBeginCommandBuffer begin_command_buffer{};
    PFN_vkEndCommandBuffer end_command_buffer{};
    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier{};
    PFN_vkCmdCopyBufferToImage cmd_copy_buffer_to_image{};
    PFN_vkQueueSubmit queue_submit{};
    PFN_vkCreateFence create_fence{};
    PFN_vkDestroyFence destroy_fence{};
    PFN_vkGetFenceStatus get_fence_status{};
    PFN_vkWaitForFences wait_for_fences{};
};

struct UploadedTexture
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

// Staging resources and command buffers stay alive until the GPU signals the
// submission fence. Keeping these in a queue avoids vkQueueWaitIdle per asset.
struct PendingTextureUpload
{
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};

bool upload_texture(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_types, const TextureUploadDispatch& vk,
    const DdsTexture& source, UploadedTexture& result,
    std::vector<PendingTextureUpload>& pending_uploads, ImageStateTracker& image_states,
    std::string& error);
void collect_completed_uploads(VkDevice device, VkCommandPool pool, const TextureUploadDispatch& vk,
    std::vector<PendingTextureUpload>& pending_uploads);
bool wait_for_uploads(VkDevice device, VkCommandPool pool, const TextureUploadDispatch& vk,
    std::vector<PendingTextureUpload>& pending_uploads);
void destroy_texture(VkDevice device, const TextureUploadDispatch& vk, UploadedTexture& texture);
}
