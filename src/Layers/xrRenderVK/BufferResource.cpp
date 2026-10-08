#include "BufferResource.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace xray::render::vulkan
{
struct BufferMemoryBlock
{
    VkDevice device{};
    VkDeviceMemory memory{};
    uint32_t memory_type{};
    VkDeviceSize size{};
    VkDeviceSize next{};
    PFN_vkFreeMemory free_memory{};
    PFN_vkUnmapMemory unmap_memory{};
    void* mapped{};

    ~BufferMemoryBlock()
    {
        if (device && memory && mapped && unmap_memory) unmap_memory(device, memory);
        if (device && memory && free_memory) free_memory(device, memory, nullptr);
    }
};

namespace
{
std::mutex& pool_mutex() { static auto* mutex = new std::mutex; return *mutex; }
std::vector<std::weak_ptr<BufferMemoryBlock>>& pool_blocks()
{
    static auto* blocks = new std::vector<std::weak_ptr<BufferMemoryBlock>>;
    return *blocks;
}

bool allocate_pooled(VkDevice device, uint32_t memory_type, const VkMemoryRequirements& requirements,
    bool host_visible, const BufferResourceDispatch& vk, std::shared_ptr<BufferMemoryBlock>& block,
    VkDeviceSize& offset, std::string& error)
{
    std::lock_guard lock(pool_mutex());
    auto& blocks = pool_blocks();
    const VkDeviceSize alignment = std::max<VkDeviceSize>(requirements.alignment, 1);
    for (auto it = blocks.begin(); it != blocks.end(); )
    {
        auto candidate = it->lock();
        if (!candidate) { it = blocks.erase(it); continue; }
        ++it;
        if (candidate->device != device || candidate->memory_type != memory_type ||
            static_cast<bool>(candidate->mapped) != host_visible ||
            candidate->next > std::numeric_limits<VkDeviceSize>::max() - (alignment - 1)) continue;
        const VkDeviceSize aligned = (candidate->next + alignment - 1) / alignment * alignment;
        if (aligned > candidate->size || requirements.size > candidate->size - aligned) continue;
        offset = aligned;
        candidate->next = aligned + requirements.size;
        block = std::move(candidate);
        return true;
    }
    constexpr VkDeviceSize block_size = 8u * 1024u * 1024u;
    const VkDeviceSize capacity = std::max(block_size, requirements.size);
    VkMemoryAllocateInfo info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    info.allocationSize = capacity;
    info.memoryTypeIndex = memory_type;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkResult result = vk.allocate_memory(device, &info, nullptr, &memory);
    if (result != VK_SUCCESS && capacity > requirements.size)
    {
        info.allocationSize = requirements.size;
        result = vk.allocate_memory(device, &info, nullptr, &memory);
    }
    if (result != VK_SUCCESS)
    {
        error = "vkAllocateMemory failed for pooled buffer block result=" + std::to_string(result) +
            " bytes=" + std::to_string(info.allocationSize);
        return false;
    }
    void* mapped = nullptr;
    if (host_visible && (vk.map_memory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS || !mapped))
    {
        vk.free_memory(device, memory, nullptr);
        error = "vkMapMemory failed for pooled host-visible buffer block";
        return false;
    }
    block = std::make_shared<BufferMemoryBlock>();
    block->device = device;
    block->memory = memory;
    block->memory_type = memory_type;
    block->size = info.allocationSize;
    block->next = requirements.size;
    block->free_memory = vk.free_memory;
    block->unmap_memory = vk.unmap_memory;
    block->mapped = mapped;
    blocks.push_back(block);
    offset = 0;
    return true;
}

uint32_t find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags required,
    const VkPhysicalDeviceMemoryProperties& properties)
{
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index)
        if ((type_bits & (1u << index)) &&
            (properties.memoryTypes[index].propertyFlags & required) == required)
            return index;
    return UINT32_MAX;
}

bool complete(const BufferResourceDispatch& vk)
{
    return vk.create_buffer && vk.destroy_buffer && vk.get_buffer_memory_requirements &&
        vk.allocate_memory && vk.free_memory && vk.bind_buffer_memory && vk.map_memory && vk.unmap_memory;
}
}

BufferResource::~BufferResource()
{
    destroy();
}

BufferResource::BufferResource(BufferResource&& other) noexcept
{
    *this = std::move(other);
}

BufferResource& BufferResource::operator=(BufferResource&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_mapped = other.m_mapped;
        m_pool_block = std::move(other.m_pool_block);
        m_vk = other.m_vk;
        other.m_device = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mapped = nullptr;
        other.m_vk = {};
    }
    return *this;
}

bool BufferResource::initialize(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags required_memory_properties,
    const VkPhysicalDeviceMemoryProperties& memory_properties,
    const BufferResourceDispatch& dispatch, std::string& error, bool pooled)
{
    destroy();
    if (!device || !size || !usage || !complete(dispatch))
    {
        error = "Vulkan buffer resource requires a device, size, usage and complete procedures";
        return false;
    }

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = VK_NULL_HANDLE;
    if (dispatch.create_buffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    {
        error = "vkCreateBuffer failed";
        return false;
    }

    VkMemoryRequirements requirements{};
    dispatch.get_buffer_memory_requirements(device, buffer, &requirements);
    const VkMemoryPropertyFlags memory_flags = required_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        ? required_memory_properties | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : required_memory_properties;
    const uint32_t memory_type = find_memory_type(requirements.memoryTypeBits,
        memory_flags, memory_properties);
    if (memory_type == UINT32_MAX)
    {
        dispatch.destroy_buffer(device, buffer, nullptr);
        error = "no Vulkan memory type satisfies the buffer requirements";
        return false;
    }

    std::shared_ptr<BufferMemoryBlock> pool_block;
    VkDeviceSize binding_offset = 0;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (pooled)
    {
        if (!allocate_pooled(device, memory_type, requirements,
                (required_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0,
                dispatch, pool_block, binding_offset, error))
        {
            dispatch.destroy_buffer(device, buffer, nullptr);
            return false;
        }
        memory = pool_block->memory;
    }
    else
    {
        VkMemoryAllocateInfo allocate_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate_info.allocationSize = requirements.size;
        allocate_info.memoryTypeIndex = memory_type;
        if (dispatch.allocate_memory(device, &allocate_info, nullptr, &memory) != VK_SUCCESS)
        {
            dispatch.destroy_buffer(device, buffer, nullptr);
            error = "vkAllocateMemory failed for buffer resource";
            return false;
        }
    }

    if (dispatch.bind_buffer_memory(device, buffer, memory, binding_offset) != VK_SUCCESS)
    {
        dispatch.destroy_buffer(device, buffer, nullptr);
        if (pool_block) pool_block.reset();
        else dispatch.free_memory(device, memory, nullptr);
        error = "vkBindBufferMemory failed for buffer resource";
        return false;
    }

    void* mapped = nullptr;
    if (pool_block && pool_block->mapped)
        mapped = static_cast<unsigned char*>(pool_block->mapped) + binding_offset;
    else if (required_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        if (dispatch.map_memory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS || !mapped)
        {
            dispatch.destroy_buffer(device, buffer, nullptr);
            if (pool_block) pool_block.reset();
            else dispatch.free_memory(device, memory, nullptr);
            error = "vkMapMemory failed for host-visible buffer resource";
            return false;
        }
    }

    m_device = device;
    m_buffer = buffer;
    m_memory = memory;
    m_size = size;
    m_mapped = mapped;
    m_pool_block = std::move(pool_block);
    m_vk = dispatch;
    error.clear();
    return true;
}

bool BufferResource::write(VkDeviceSize offset, const void* data, size_t size, std::string& error)
{
    if (!m_buffer || !m_mapped || !data || !size || offset > m_size || size > m_size - offset ||
        offset > std::numeric_limits<size_t>::max())
    {
        error = "buffer write requires mapped storage and a non-empty in-range data span";
        return false;
    }
    std::memcpy(static_cast<unsigned char*>(m_mapped) + offset, data, size);
    error.clear();
    return true;
}

bool BufferResource::read(VkDeviceSize offset, void* data, size_t size, std::string& error) const
{
    if (!m_buffer || !m_mapped || !data || !size || offset > m_size || size > m_size - offset ||
        offset > std::numeric_limits<size_t>::max())
    {
        error = "buffer read requires mapped storage and a non-empty in-range data span";
        return false;
    }
    std::memcpy(data, static_cast<const unsigned char*>(m_mapped) + offset, size);
    error.clear();
    return true;
}

void BufferResource::destroy()
{
    if (m_device && m_memory && m_mapped && !m_pool_block && m_vk.unmap_memory)
        m_vk.unmap_memory(m_device, m_memory);
    if (m_device && m_buffer && m_vk.destroy_buffer)
        m_vk.destroy_buffer(m_device, m_buffer, nullptr);
    if (m_pool_block) m_pool_block.reset();
    else if (m_device && m_memory && m_vk.free_memory)
        m_vk.free_memory(m_device, m_memory, nullptr);
    m_device = VK_NULL_HANDLE;
    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_size = 0;
    m_mapped = nullptr;
    m_vk = {};
}
}
