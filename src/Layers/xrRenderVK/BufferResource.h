#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <string>

namespace xray::render::vulkan
{
struct BufferResourceDispatch
{
    PFN_vkCreateBuffer create_buffer{};
    PFN_vkDestroyBuffer destroy_buffer{};
    PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements{};
    PFN_vkAllocateMemory allocate_memory{};
    PFN_vkFreeMemory free_memory{};
    PFN_vkBindBufferMemory bind_buffer_memory{};
    PFN_vkMapMemory map_memory{};
    PFN_vkUnmapMemory unmap_memory{};
};

// A small mapped-buffer owner for host-visible coherent memory and an
// allocation owner for device-local buffers. Callers synchronize GPU use
// before modifying bytes that may still be read by an in-flight submission.
class BufferResource
{
public:
    BufferResource() = default;
    ~BufferResource();
    BufferResource(const BufferResource&) = delete;
    BufferResource& operator=(const BufferResource&) = delete;
    BufferResource(BufferResource&& other) noexcept;
    BufferResource& operator=(BufferResource&& other) noexcept;

    bool initialize(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags required_memory_properties,
        const VkPhysicalDeviceMemoryProperties& memory_properties,
        const BufferResourceDispatch& dispatch, std::string& error);
    bool write(VkDeviceSize offset, const void* data, size_t size, std::string& error);
    // Caller waits for the GPU write before reading coherent host memory.
    bool read(VkDeviceSize offset, void* data, size_t size, std::string& error) const;
    void destroy();

    VkBuffer handle() const { return m_buffer; }
    VkDeviceSize size() const { return m_size; }
    bool host_visible() const { return m_mapped != nullptr; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mapped = nullptr;
    BufferResourceDispatch m_vk{};
};
}
