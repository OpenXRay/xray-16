#include "stdafx.h"
#include "VulkanCommandBuffer.h"

namespace xray {
namespace animation {
namespace renderer {

VulkanCommandBuffer::VulkanCommandBuffer() {}
VulkanCommandBuffer::~VulkanCommandBuffer() {}

void VulkanCommandBuffer::Create(VkDevice device, VkCommandPool pool) {
    // STUB: To be implemented
}

void VulkanCommandBuffer::Destroy(VkDevice device, VkCommandPool pool) {
    // STUB: To be implemented
}

void VulkanCommandBuffer::Begin() {
    // STUB: To be implemented
}

void VulkanCommandBuffer::End() {
    // STUB: To be implemented
}

void VulkanCommandBuffer::Submit(VkQueue queue, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
    // STUB: To be implemented
}

} // namespace renderer
} // namespace animation
} // namespace xray
