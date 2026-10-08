#include "src/Layers/xrRenderVK/BufferResource.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
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

std::array<unsigned char, 32> mapped_bytes{};
std::vector<unsigned char> pooled_mapped_bytes(8u * 1024u * 1024u);
std::vector<int> cleanup_order;
VkResult create_result = VK_SUCCESS;
VkResult allocate_result = VK_SUCCESS;
VkResult bind_result = VK_SUCCESS;
VkResult map_result = VK_SUCCESS;
uint32_t destroyed_buffers = 0;
uint32_t freed_memories = 0;
uint32_t unmapped_memories = 0;
uint32_t allocated_memories = 0;
bool allow_pooled_offsets = false;
bool map_pooled = false;

VkResult VKAPI_CALL create_buffer(VkDevice, const VkBufferCreateInfo* info,
    const VkAllocationCallbacks*, VkBuffer* buffer)
{
    assert(info->size == 16);
    assert(info->usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(info->sharingMode == VK_SHARING_MODE_EXCLUSIVE);
    *buffer = fake_handle<VkBuffer>(1);
    return create_result;
}

void VKAPI_CALL destroy_buffer(VkDevice, VkBuffer, const VkAllocationCallbacks*)
{
    cleanup_order.push_back(1);
    ++destroyed_buffers;
}

void VKAPI_CALL get_requirements(VkDevice, VkBuffer, VkMemoryRequirements* requirements)
{
    requirements->size = 32;
    requirements->alignment = 16;
    requirements->memoryTypeBits = 1;
}

VkResult VKAPI_CALL allocate_memory(VkDevice, const VkMemoryAllocateInfo* info,
    const VkAllocationCallbacks*, VkDeviceMemory* memory)
{
    assert(info->allocationSize == 32 || info->allocationSize == 8u * 1024u * 1024u);
    ++allocated_memories;
    *memory = fake_handle<VkDeviceMemory>(2);
    return allocate_result;
}

void VKAPI_CALL free_memory(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*)
{
    cleanup_order.push_back(2);
    ++freed_memories;
}

VkResult VKAPI_CALL bind_buffer_memory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize offset)
{
    assert(offset % 16 == 0);
    if (!allow_pooled_offsets) assert(offset == 0);
    return bind_result;
}

VkResult VKAPI_CALL map_memory(VkDevice, VkDeviceMemory, VkDeviceSize offset,
    VkDeviceSize size, VkMemoryMapFlags, void** data)
{
    assert(offset == 0 && size == VK_WHOLE_SIZE);
    *data = map_pooled ? pooled_mapped_bytes.data() : mapped_bytes.data();
    return map_result;
}

void VKAPI_CALL unmap_memory(VkDevice, VkDeviceMemory)
{
    cleanup_order.push_back(0);
    ++unmapped_memories;
}

xray::render::vulkan::BufferResourceDispatch make_dispatch()
{
    return {create_buffer, destroy_buffer, get_requirements, allocate_memory, free_memory,
        bind_buffer_memory, map_memory, unmap_memory};
}

VkPhysicalDeviceMemoryProperties host_memory()
{
    VkPhysicalDeviceMemoryProperties properties{};
    properties.memoryTypeCount = 1;
    properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    return properties;
}
}

int main()
{
    using xray::render::vulkan::BufferResource;
    const auto device = fake_handle<VkDevice>(3);
    const auto dispatch = make_dispatch();
    const auto memory = host_memory();
    std::string error;
    BufferResource buffer;
    assert(buffer.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory, dispatch, error));
    assert(error.empty() && buffer.handle() && buffer.size() == 16 && buffer.host_visible());

    const unsigned char source[] = {4, 5, 6, 7};
    assert(buffer.write(3, source, sizeof(source), error));
    assert(!std::memcmp(mapped_bytes.data() + 3, source, sizeof(source)));
    unsigned char readback[sizeof(source)]{};
    assert(buffer.read(3, readback, sizeof(readback), error));
    assert(!std::memcmp(readback, source, sizeof(source)));
    assert(!buffer.read(14, readback, sizeof(readback), error));
    assert(!buffer.write(14, source, sizeof(source), error));
    assert(!error.empty());
    assert(!buffer.write(0, nullptr, sizeof(source), error));

    BufferResource moved(std::move(buffer));
    assert(!buffer.handle() && moved.handle());
    moved.destroy();
    assert(cleanup_order == std::vector<int>({0, 1, 2}));
    assert(unmapped_memories == 1 && destroyed_buffers == 1 && freed_memories == 1);

    auto unavailable_memory = memory;
    unavailable_memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    assert(!buffer.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, unavailable_memory, dispatch, error));
    assert(!error.empty() && destroyed_buffers == 2 && freed_memories == 1);

    map_result = VK_ERROR_MEMORY_MAP_FAILED;
    assert(!buffer.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory, dispatch, error));
    assert(!error.empty() && destroyed_buffers == 3 && freed_memories == 2);
    assert(cleanup_order[4] == 1 && cleanup_order[5] == 2);

    map_result = VK_SUCCESS;
    BufferResource device_local;
    assert(device_local.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, unavailable_memory, dispatch, error));
    assert(!device_local.host_visible());
    assert(!device_local.write(0, source, sizeof(source), error));
    device_local.destroy();
    assert(unmapped_memories == 1 && destroyed_buffers == 4 && freed_memories == 3);

    allow_pooled_offsets = true;
    const auto allocations_before = allocated_memories;
    const auto frees_before = freed_memories;
    std::vector<BufferResource> pooled(100);
    for (auto& resource : pooled)
        assert(resource.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, unavailable_memory, dispatch, error, true));
    assert(allocated_memories == allocations_before + 1);
    pooled.clear();
    assert(freed_memories == frees_before + 1);

    map_pooled = true;
    const auto host_allocations_before = allocated_memories;
    const auto host_frees_before = freed_memories;
    const auto host_unmaps_before = unmapped_memories;
    BufferResource first, second;
    assert(first.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory, dispatch, error, true));
    assert(second.initialize(device, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory, dispatch, error, true));
    assert(allocated_memories == host_allocations_before + 1);
    assert(first.write(0, source, sizeof(source), error));
    const unsigned char other[] = {8, 9, 10, 11};
    assert(second.write(0, other, sizeof(other), error));
    assert(!std::memcmp(pooled_mapped_bytes.data(), source, sizeof(source)));
    assert(!std::memcmp(pooled_mapped_bytes.data() + 32, other, sizeof(other)));
    first.destroy();
    assert(freed_memories == host_frees_before && unmapped_memories == host_unmaps_before);
    second.destroy();
    assert(freed_memories == host_frees_before + 1 && unmapped_memories == host_unmaps_before + 1);
}
