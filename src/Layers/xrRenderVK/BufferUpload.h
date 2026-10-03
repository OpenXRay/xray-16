#pragma once

#include "BufferResource.h"

#include <vector>

namespace xray::render::vulkan
{
struct BufferUploadDispatch
{
    BufferResourceDispatch buffer{};
    PFN_vkAllocateCommandBuffers allocate_command_buffers{};
    PFN_vkFreeCommandBuffers free_command_buffers{};
    PFN_vkBeginCommandBuffer begin_command_buffer{};
    PFN_vkEndCommandBuffer end_command_buffer{};
    PFN_vkCmdCopyBuffer cmd_copy_buffer{};
    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier{};
    PFN_vkQueueSubmit queue_submit{};
    PFN_vkCreateFence create_fence{};
    PFN_vkDestroyFence destroy_fence{};
    PFN_vkGetFenceStatus get_fence_status{};
    PFN_vkWaitForFences wait_for_fences{};
};

struct PendingBufferUpload
{
    BufferResource staging;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};

// Creates a device-local destination and submits an asynchronous staging copy.
// Submit consumers on the same queue after this upload, or provide external
// synchronization if another queue will use the destination. Keep the buffer
// alive while it is in use, and retire pending uploads before destroying the
// device or command pool. The allocated capacity is rounded up to Vulkan's
// four-byte buffer-copy alignment; any tail padding is zeroed.
bool upload_buffer(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_properties, const BufferUploadDispatch& dispatch,
    const void* data, size_t size, VkBufferUsageFlags usage, BufferResource& result,
    std::vector<PendingBufferUpload>& pending_uploads, std::string& error);
void collect_completed_buffer_uploads(VkDevice device, VkCommandPool pool,
    const BufferUploadDispatch& dispatch, std::vector<PendingBufferUpload>& pending_uploads);
bool wait_for_buffer_uploads(VkDevice device, VkCommandPool pool,
    const BufferUploadDispatch& dispatch, std::vector<PendingBufferUpload>& pending_uploads);
}
