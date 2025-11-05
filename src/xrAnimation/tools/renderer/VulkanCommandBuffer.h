#pragma once

#include <vulkan/vulkan.h>

namespace xray {
namespace animation {
namespace renderer {

// Vulkan command buffer wrapper (STUB - to be implemented)
class VulkanCommandBuffer {
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();

    void Create(VkDevice device, VkCommandPool pool);
    void Destroy(VkDevice device, VkCommandPool pool);

    void Begin();
    void End();
    void Submit(VkQueue queue, VkSemaphore wait, VkSemaphore signal, VkFence fence);

    VkCommandBuffer GetHandle() const { return command_buffer_; }

private:
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
};

} // namespace renderer
} // namespace animation
} // namespace xray
