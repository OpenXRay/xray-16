#include "src/Layers/xrRenderVK/BufferUpload.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace
{
template <typename T>
T fake_handle(uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>)
        return reinterpret_cast<T>(value);
    else
        return static_cast<T>(value);
}

std::array<unsigned char, 32> staging_bytes{};
VkBuffer latest_staging = VK_NULL_HANDLE;
VkBuffer latest_destination = VK_NULL_HANDLE;
uint32_t next_buffer = 0;
uint32_t next_memory = 100;
uint32_t next_command = 200;
uint32_t next_fence = 300;
uint32_t copies = 0;
uint32_t barriers = 0;
uint32_t submissions = 0;
uint32_t freed_commands = 0;
uint32_t destroyed_fences = 0;
uint32_t destroyed_buffers = 0;
uint32_t freed_memories = 0;
uint32_t unmapped_memories = 0;
VkResult submit_result = VK_SUCCESS;
VkResult fence_status = VK_SUCCESS;

VkResult VKAPI_CALL create_buffer(VkDevice, const VkBufferCreateInfo* info,
    const VkAllocationCallbacks*, VkBuffer* buffer)
{
    assert(info->size == 8);
    const uintptr_t value = ++next_buffer;
    *buffer = fake_handle<VkBuffer>(value);
    if (info->usage == VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        latest_staging = *buffer;
    else
    {
        assert((info->usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0);
        assert((info->usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) != 0);
        latest_destination = *buffer;
    }
    return VK_SUCCESS;
}

void VKAPI_CALL destroy_buffer(VkDevice, VkBuffer, const VkAllocationCallbacks*)
{
    ++destroyed_buffers;
}

void VKAPI_CALL get_buffer_requirements(VkDevice, VkBuffer buffer, VkMemoryRequirements* requirements)
{
    requirements->size = 16;
    requirements->alignment = 4;
    requirements->memoryTypeBits = buffer == latest_staging ? 1u : 2u;
}

VkResult VKAPI_CALL allocate_memory(VkDevice, const VkMemoryAllocateInfo* info,
    const VkAllocationCallbacks*, VkDeviceMemory* memory)
{
    assert(info->allocationSize == 16);
    assert(info->memoryTypeIndex < 2);
    *memory = fake_handle<VkDeviceMemory>(++next_memory);
    return VK_SUCCESS;
}

void VKAPI_CALL free_memory(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*)
{
    ++freed_memories;
}

VkResult VKAPI_CALL bind_buffer_memory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize offset)
{
    assert(offset == 0);
    return VK_SUCCESS;
}

VkResult VKAPI_CALL map_memory(VkDevice, VkDeviceMemory, VkDeviceSize offset,
    VkDeviceSize size, VkMemoryMapFlags, void** data)
{
    assert(offset == 0 && size == VK_WHOLE_SIZE);
    *data = staging_bytes.data();
    return VK_SUCCESS;
}

void VKAPI_CALL unmap_memory(VkDevice, VkDeviceMemory)
{
    ++unmapped_memories;
}

VkResult VKAPI_CALL allocate_command_buffers(VkDevice, const VkCommandBufferAllocateInfo* info,
    VkCommandBuffer* command)
{
    assert(info->commandBufferCount == 1 && info->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    *command = fake_handle<VkCommandBuffer>(++next_command);
    return VK_SUCCESS;
}

void VKAPI_CALL free_command_buffers(VkDevice, VkCommandPool, uint32_t count, const VkCommandBuffer*)
{
    assert(count == 1);
    ++freed_commands;
}

VkResult VKAPI_CALL begin_command_buffer(VkCommandBuffer, const VkCommandBufferBeginInfo* info)
{
    assert(info->flags == VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return VK_SUCCESS;
}

VkResult VKAPI_CALL end_command_buffer(VkCommandBuffer)
{
    return VK_SUCCESS;
}

void VKAPI_CALL copy_buffer(VkCommandBuffer, VkBuffer source, VkBuffer destination,
    uint32_t count, const VkBufferCopy* region)
{
    assert(source == latest_staging && destination == latest_destination);
    assert(count == 1 && region->srcOffset == 0 && region->dstOffset == 0 && region->size == 8);
    ++copies;
}

void VKAPI_CALL pipeline_barrier(VkCommandBuffer, VkPipelineStageFlags source_stage,
    VkPipelineStageFlags destination_stage, VkDependencyFlags, uint32_t memory_count,
    const VkMemoryBarrier*, uint32_t buffer_count, const VkBufferMemoryBarrier* buffer_barriers,
    uint32_t image_count, const VkImageMemoryBarrier*)
{
    assert(source_stage == VK_PIPELINE_STAGE_TRANSFER_BIT);
    assert(destination_stage == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    assert(memory_count == 0 && buffer_count == 1 && image_count == 0);
    assert(buffer_barriers[0].srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT);
    assert(buffer_barriers[0].dstAccessMask == (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT));
    assert(buffer_barriers[0].srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
    assert(buffer_barriers[0].dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
    assert(buffer_barriers[0].buffer == latest_destination);
    assert(buffer_barriers[0].offset == 0 && buffer_barriers[0].size == 8);
    ++barriers;
}

VkResult VKAPI_CALL queue_submit(VkQueue, uint32_t count, const VkSubmitInfo* info, VkFence fence)
{
    assert(count == 1 && info->commandBufferCount == 1 && fence);
    ++submissions;
    return submit_result;
}

VkResult VKAPI_CALL create_fence(VkDevice, const VkFenceCreateInfo*,
    const VkAllocationCallbacks*, VkFence* fence)
{
    *fence = fake_handle<VkFence>(++next_fence);
    return VK_SUCCESS;
}

void VKAPI_CALL destroy_fence(VkDevice, VkFence, const VkAllocationCallbacks*)
{
    ++destroyed_fences;
}

VkResult VKAPI_CALL get_fence_status(VkDevice, VkFence)
{
    return fence_status;
}

VkResult VKAPI_CALL wait_for_fences(VkDevice, uint32_t count, const VkFence*, VkBool32 wait_all,
    uint64_t timeout)
{
    assert(count == 1 && wait_all == VK_TRUE && timeout == UINT64_MAX);
    return fence_status;
}

xray::render::vulkan::BufferUploadDispatch make_dispatch()
{
    xray::render::vulkan::BufferResourceDispatch buffers{
        create_buffer, destroy_buffer, get_buffer_requirements, allocate_memory, free_memory,
        bind_buffer_memory, map_memory, unmap_memory};
    return {buffers, allocate_command_buffers, free_command_buffers, begin_command_buffer,
        end_command_buffer, copy_buffer, pipeline_barrier, queue_submit, create_fence, destroy_fence,
        get_fence_status, wait_for_fences};
}

VkPhysicalDeviceMemoryProperties memory_properties()
{
    VkPhysicalDeviceMemoryProperties properties{};
    properties.memoryTypeCount = 2;
    properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    return properties;
}
}

int main()
{
    using namespace xray::render::vulkan;
    const auto device = fake_handle<VkDevice>(1);
    const auto queue = fake_handle<VkQueue>(2);
    const auto pool = fake_handle<VkCommandPool>(3);
    const auto dispatch = make_dispatch();
    const auto memory = memory_properties();
    const unsigned char source[] = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<PendingBufferUpload> pending;
    BufferResource result;
    std::string error;

    assert(upload_buffer(device, queue, pool, memory, dispatch, source, sizeof(source),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, result, pending, error));
    assert(error.empty() && result.handle() == latest_destination && result.size() == sizeof(source));
    assert(!result.host_visible() && pending.size() == 1 && pending[0].staging.host_visible());
    assert(!std::memcmp(staging_bytes.data(), source, sizeof(source)));
    assert(copies == 1 && barriers == 1 && submissions == 1);

    fence_status = VK_NOT_READY;
    collect_completed_buffer_uploads(device, pool, dispatch, pending);
    assert(pending.size() == 1 && destroyed_buffers == 0);
    fence_status = VK_SUCCESS;
    collect_completed_buffer_uploads(device, pool, dispatch, pending);
    assert(pending.empty());
    assert(unmapped_memories == 1 && destroyed_buffers == 1 && freed_memories == 1);
    assert(freed_commands == 1 && destroyed_fences == 1);
    result.destroy();
    assert(destroyed_buffers == 2 && freed_memories == 2);

    BufferResource waited_result;
    assert(upload_buffer(device, queue, pool, memory, dispatch, source, sizeof(source),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, waited_result, pending, error));
    fence_status = VK_SUCCESS;
    assert(wait_for_buffer_uploads(device, pool, dispatch, pending));
    assert(pending.empty() && waited_result.handle());
    waited_result.destroy();

    BufferResource padded_result;
    assert(upload_buffer(device, queue, pool, memory, dispatch, source, sizeof(source) - 1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, padded_result, pending, error));
    assert(padded_result.size() == sizeof(source) && staging_bytes[7] == 0);
    fence_status = VK_SUCCESS;
    collect_completed_buffer_uploads(device, pool, dispatch, pending);
    padded_result.destroy();

    BufferResource lost_result;
    assert(upload_buffer(device, queue, pool, memory, dispatch, source, sizeof(source),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, lost_result, pending, error));
    const auto retired_before_loss = destroyed_fences;
    fence_status = VK_ERROR_DEVICE_LOST;
    assert(!wait_for_buffer_uploads(device, pool, dispatch, pending));
    assert(pending.empty() && destroyed_fences == retired_before_loss + 1);
    lost_result.destroy();

    BufferResource failed_result;
    submit_result = VK_ERROR_DEVICE_LOST;
    assert(!upload_buffer(device, queue, pool, memory, dispatch, source, sizeof(source),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, failed_result, pending, error));
    assert(!failed_result.handle() && pending.empty() && !error.empty());
}
