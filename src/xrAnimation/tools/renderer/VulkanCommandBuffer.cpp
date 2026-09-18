#include "stdafx.h"
#include "VulkanCommandBuffer.h"

namespace xray {
namespace animation {
namespace renderer {

VulkanCommandBuffer::VulkanCommandBuffer() {}
VulkanCommandBuffer::~VulkanCommandBuffer() {}

void VulkanCommandBuffer::Create(VkDevice device, VkCommandPool pool) {
}

void VulkanCommandBuffer::Destroy(VkDevice device, VkCommandPool pool) {
}

void VulkanCommandBuffer::Begin() {
}

void VulkanCommandBuffer::End() {
}

void VulkanCommandBuffer::Submit(VkQueue queue, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
}

}
}
}
