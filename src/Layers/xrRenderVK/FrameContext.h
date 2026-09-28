#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct FrameDispatch
{
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR get_surface_capabilities{};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats{};
    PFN_vkCreateSwapchainKHR create_swapchain{};
    PFN_vkDestroySwapchainKHR destroy_swapchain{};
    PFN_vkGetSwapchainImagesKHR get_swapchain_images{};
    PFN_vkAcquireNextImageKHR acquire_next_image{};
    PFN_vkQueuePresentKHR queue_present{};
    PFN_vkCreateImageView create_image_view{};
    PFN_vkDestroyImageView destroy_image_view{};
    PFN_vkCreateRenderPass create_render_pass{};
    PFN_vkDestroyRenderPass destroy_render_pass{};
    PFN_vkCreateFramebuffer create_framebuffer{};
    PFN_vkDestroyFramebuffer destroy_framebuffer{};
    PFN_vkCreateCommandPool create_command_pool{};
    PFN_vkDestroyCommandPool destroy_command_pool{};
    PFN_vkAllocateCommandBuffers allocate_command_buffers{};
    PFN_vkResetCommandBuffer reset_command_buffer{};
    PFN_vkBeginCommandBuffer begin_command_buffer{};
    PFN_vkCmdBeginRenderPass cmd_begin_render_pass{};
    PFN_vkCmdEndRenderPass cmd_end_render_pass{};
    PFN_vkEndCommandBuffer end_command_buffer{};
    PFN_vkCreateSemaphore create_semaphore{};
    PFN_vkDestroySemaphore destroy_semaphore{};
    PFN_vkCreateFence create_fence{};
    PFN_vkDestroyFence destroy_fence{};
    PFN_vkWaitForFences wait_for_fences{};
    PFN_vkResetFences reset_fences{};
    PFN_vkQueueSubmit queue_submit{};
    PFN_vkDeviceWaitIdle device_wait_idle{};
};

enum class FrameStatus
{
    Presented,
    RecreateRequired
};

bool load_frame_dispatch(VkInstance instance, PFN_vkGetInstanceProcAddr get_instance_proc,
    VkDevice device, PFN_vkGetDeviceProcAddr get_device_proc, FrameDispatch& dispatch, std::string& error);

class FrameContext
{
public:
    static constexpr uint32_t FramesInFlight = 2;

    FrameContext() = default;
    ~FrameContext();
    FrameContext(const FrameContext&) = delete;
    FrameContext& operator=(const FrameContext&) = delete;

    bool initialize(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
        VkQueue queue, uint32_t queue_family, VkExtent2D requested_extent,
        const FrameDispatch& dispatch, std::string& error);
    bool render_frame(const VkClearColorValue& clear, FrameStatus& status, std::string& error);
    void destroy();

    VkExtent2D extent() const { return m_extent; }
    VkFormat format() const { return m_format; }

private:
    bool create_swapchain(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        VkExtent2D requested_extent, std::string& error);
    bool create_render_targets(std::string& error);
    bool create_commands(std::string& error);
    bool create_sync(std::string& error);

    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_queue_family = UINT32_MAX;
    FrameDispatch m_vk{};
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    VkExtent2D m_extent{};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkCommandBuffer> m_commands;
    std::vector<VkSemaphore> m_render_finished;
    std::array<VkSemaphore, FramesInFlight> m_image_available{};
    std::array<VkFence, FramesInFlight> m_frame_fences{};
    std::vector<VkFence> m_image_fences;
    uint32_t m_current_frame{};
};
}
