#include "BufferUpload.h"

#include <limits>
#include <utility>

namespace xray::render::vulkan
{
namespace
{
bool complete(const BufferUploadDispatch& vk)
{
    return vk.buffer.create_buffer && vk.buffer.destroy_buffer && vk.buffer.get_buffer_memory_requirements &&
        vk.buffer.allocate_memory && vk.buffer.free_memory && vk.buffer.bind_buffer_memory &&
        vk.buffer.map_memory && vk.buffer.unmap_memory && vk.allocate_command_buffers &&
        vk.free_command_buffers && vk.begin_command_buffer && vk.end_command_buffer && vk.cmd_copy_buffer &&
        vk.cmd_pipeline_barrier && vk.queue_submit && vk.create_fence && vk.destroy_fence &&
        vk.get_fence_status && vk.wait_for_fences;
}

void release_upload(VkDevice device, VkCommandPool pool, const BufferUploadDispatch& vk,
    PendingBufferUpload& upload)
{
    if (upload.fence)
        vk.destroy_fence(device, upload.fence, nullptr);
    if (upload.command_buffer)
        vk.free_command_buffers(device, pool, 1, &upload.command_buffer);
    upload.staging.destroy();
    upload = {};
}
}

bool upload_buffer(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_properties, const BufferUploadDispatch& vk,
    const void* data, size_t size, VkBufferUsageFlags usage, BufferResource& result,
    std::vector<PendingBufferUpload>& pending_uploads, std::string& error)
{
    if (!device || !queue || !pool || !data || !size || !usage || result.handle() || !complete(vk))
    {
        error = "Vulkan buffer upload requires empty output, data, usage, queue and complete procedures";
        return false;
    }
    if (size > std::numeric_limits<size_t>::max() - 3)
    {
        error = "Vulkan buffer upload size overflows four-byte copy alignment";
        return false;
    }

    collect_completed_buffer_uploads(device, pool, vk, pending_uploads);
    const size_t padded_size = (size + 3) & ~size_t(3);
    const VkDeviceSize buffer_size = static_cast<VkDeviceSize>(padded_size);
    PendingBufferUpload upload;
    BufferResource destination;
    auto cleanup = [&]
    {
        release_upload(device, pool, vk, upload);
    };
    auto fail = [&](const char* why)
    {
        error = why;
        cleanup();
        destination.destroy();
        return false;
    };

    if (!upload.staging.initialize(device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_properties, vk.buffer, error))
    {
        cleanup();
        return false;
    }
    if (!upload.staging.write(0, data, size, error))
    {
        cleanup();
        return false;
    }
    if (padded_size > size)
    {
        const unsigned char padding[3]{};
        if (!upload.staging.write(size, padding, padded_size - size, error))
        {
            cleanup();
            return false;
        }
    }
    if (!destination.initialize(device, buffer_size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_properties, vk.buffer, error))
    {
        cleanup();
        return false;
    }

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    if (vk.allocate_command_buffers(device, &allocate_info, &upload.command_buffer) != VK_SUCCESS)
        return fail("vkAllocateCommandBuffers failed for buffer upload");

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vk.begin_command_buffer(upload.command_buffer, &begin_info) != VK_SUCCESS)
        return fail("vkBeginCommandBuffer failed for buffer upload");

    const VkBufferCopy copy{0, 0, buffer_size};
    vk.cmd_copy_buffer(upload.command_buffer, upload.staging.handle(), destination.handle(), 1, &copy);
    VkBufferMemoryBarrier destination_barrier{};
    destination_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    destination_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    destination_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    destination_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    destination_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    destination_barrier.buffer = destination.handle();
    destination_barrier.offset = 0;
    destination_barrier.size = buffer_size;
    vk.cmd_pipeline_barrier(upload.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &destination_barrier, 0, nullptr);
    if (vk.end_command_buffer(upload.command_buffer) != VK_SUCCESS)
        return fail("vkEndCommandBuffer failed for buffer upload");

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vk.create_fence(device, &fence_info, nullptr, &upload.fence) != VK_SUCCESS)
        return fail("vkCreateFence failed for buffer upload");

    try
    {
        pending_uploads.reserve(pending_uploads.size() + 1);
    }
    catch (...)
    {
        return fail("pending Vulkan buffer upload queue allocation failed");
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &upload.command_buffer;
    if (vk.queue_submit(queue, 1, &submit, upload.fence) != VK_SUCCESS)
        return fail("vkQueueSubmit failed for buffer upload");

    pending_uploads.push_back(std::move(upload));
    result = std::move(destination);
    error.clear();
    return true;
}

void collect_completed_buffer_uploads(VkDevice device, VkCommandPool pool,
    const BufferUploadDispatch& vk, std::vector<PendingBufferUpload>& pending_uploads)
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

bool wait_for_buffer_uploads(VkDevice device, VkCommandPool pool,
    const BufferUploadDispatch& vk, std::vector<PendingBufferUpload>& pending_uploads)
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
