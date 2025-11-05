#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace xray {
namespace animation {
namespace renderer {

class VulkanDevice;

// Vulkan buffer wrapper with VMA integration (STUB - to be implemented)
class VulkanBuffer {
public:
    VulkanBuffer();
    ~VulkanBuffer();

    void Create(VkDevice device, VmaAllocator allocator, VkDeviceSize size,
                VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void Destroy();

    void* Map();
    void Unmap();
    void Upload(const void* data, VkDeviceSize size);

    VkBuffer GetBuffer() const { return buffer_; }
    VkDeviceSize GetSize() const { return size_; }

private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    void* mapped_data_ = nullptr;
    VmaAllocator allocator_ = VK_NULL_HANDLE;
};

} // namespace renderer
} // namespace animation
} // namespace xray
