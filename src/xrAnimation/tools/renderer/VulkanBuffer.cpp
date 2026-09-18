#include "stdafx.h"
#include "VulkanBuffer.h"
#include <cstring>
#include <cstdio>

#define Msg(...) printf(__VA_ARGS__), printf("\n")

namespace xray {
namespace animation {
namespace renderer {

VulkanBuffer::VulkanBuffer() {}

VulkanBuffer::~VulkanBuffer() {
    Destroy();
}

void VulkanBuffer::Create(VkDevice device, VmaAllocator allocator, VkDeviceSize size,
                          VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
    // Destroy existing buffer if any
    Destroy();

    allocator_ = allocator;
    size_ = size;

    // Create buffer info
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // VMA allocation info
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = memory_usage;

    // Create buffer using VMA
    VkResult result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
                                      &buffer_, &allocation_, nullptr);

    if (result != VK_SUCCESS) {
        Msg("! Failed to create VulkanBuffer (size: %llu, error: %d)",
            (unsigned long long)size, result);
        buffer_ = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
    }
}

void VulkanBuffer::Destroy() {
    if (buffer_ != VK_NULL_HANDLE && allocator_ != VK_NULL_HANDLE) {
        // Unmap if still mapped
        if (mapped_data_) {
            Unmap();
        }

        vmaDestroyBuffer(allocator_, buffer_, allocation_);
    }

    buffer_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
    allocator_ = VK_NULL_HANDLE;
    size_ = 0;
}

void* VulkanBuffer::Map() {
    if (mapped_data_) {
        return mapped_data_;  // Already mapped
    }

    if (buffer_ == VK_NULL_HANDLE || allocator_ == VK_NULL_HANDLE) {
        Msg("! Cannot map uninitialized buffer");
        return nullptr;
    }

    VkResult result = vmaMapMemory(allocator_, allocation_, &mapped_data_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to map buffer memory (error: %d)", result);
        mapped_data_ = nullptr;
    }

    return mapped_data_;
}

void VulkanBuffer::Unmap() {
    if (mapped_data_ && allocator_ != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE) {
        vmaUnmapMemory(allocator_, allocation_);
        mapped_data_ = nullptr;
    }
}

void VulkanBuffer::Upload(const void* data, VkDeviceSize upload_size) {
    if (!data) {
        Msg("! Cannot upload null data to buffer");
        return;
    }

    if (upload_size > size_) {
        Msg("! Upload size (%llu) exceeds buffer size (%llu)",
            (unsigned long long)upload_size, (unsigned long long)size_);
        return;
    }

    void* mapped = Map();
    if (mapped) {
        memcpy(mapped, data, static_cast<size_t>(upload_size));
        Unmap();
    }
}

} // namespace renderer
} // namespace animation
} // namespace xray
