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
    PFN_vkGetPhysicalDeviceFormatProperties get_format_properties{};
    PFN_vkGetPhysicalDeviceMemoryProperties get_memory_properties{};
    PFN_vkCreateImage create_image{};
    PFN_vkDestroyImage destroy_image{};
    PFN_vkGetImageMemoryRequirements get_image_memory_requirements{};
    PFN_vkAllocateMemory allocate_memory{};
    PFN_vkFreeMemory free_memory{};
    PFN_vkBindImageMemory bind_image_memory{};
    PFN_vkCreateRenderPass create_render_pass{};
    PFN_vkDestroyRenderPass destroy_render_pass{};
    PFN_vkCreateFramebuffer create_framebuffer{};
    PFN_vkDestroyFramebuffer destroy_framebuffer{};
    PFN_vkCreateCommandPool create_command_pool{};
    PFN_vkDestroyCommandPool destroy_command_pool{};
    PFN_vkAllocateCommandBuffers allocate_command_buffers{};
    PFN_vkFreeCommandBuffers free_command_buffers{};
    PFN_vkResetCommandBuffer reset_command_buffer{};
    PFN_vkBeginCommandBuffer begin_command_buffer{};
    PFN_vkCmdBeginRenderPass cmd_begin_render_pass{};
    PFN_vkCmdEndRenderPass cmd_end_render_pass{};
    PFN_vkCmdClearAttachments cmd_clear_attachments{};
    PFN_vkCmdCopyImageToBuffer cmd_copy_image_to_buffer{};
    PFN_vkCmdCopyImage cmd_copy_image{};
    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier{};
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

bool mark_surface_lost(VkResult result, bool& surface_lost, FrameStatus& status);

struct FrameRecordingContext
{
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkExtent2D extent{};
    uint32_t image_index = 0;
    uint32_t frame_index = 0;
};

using FrameRecorder = void (*)(const FrameRecordingContext& frame, void* user_data);
using FrameReadbackRecorder = void (*)(VkCommandBuffer command, VkImage image, VkExtent2D extent, void* user_data);
// Records offscreen work before the swapchain render pass. render_pass and
// framebuffer are null; the callback owns any pass it begins and ends.
using FramePrepassRecorder = void (*)(const FrameRecordingContext& frame, void* user_data);
using FrameInterpassRecorder = void (*)(VkCommandBuffer command, VkImage image,
    uint32_t image_index, void* user_data);

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
        const FrameDispatch& dispatch, std::string& error, bool allow_readback = false,
        bool use_depth = false, bool preserve_prepass_depth = false, bool postprocess = false);
    // Borrow depth views produced by the offscreen pass. Caller must restore
    // the owned views (empty vector) before destroying the borrowed images.
    bool attach_scene_depth(const std::vector<VkImageView>& views, std::string& error);
    bool enable_interpass(std::string& error);
    void clear_depth(VkCommandBuffer command) const;
    bool recreate(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        VkExtent2D requested_extent, std::string& error);
    bool release_swapchain();
    bool wait_idle();
    bool render_frame(const VkClearColorValue& clear, FrameStatus& status, std::string& error,
        FrameRecorder recorder = nullptr, void* user_data = nullptr,
        FrameReadbackRecorder readback = nullptr, void* readback_data = nullptr,
        FramePrepassRecorder prepass = nullptr, void* prepass_data = nullptr,
        bool clear_target = false, FrameRecorder compositor = nullptr,
        void* compositor_data = nullptr,
        FrameInterpassRecorder interpass = nullptr, void* interpass_data = nullptr,
        FrameRecorder overlay = nullptr, void* overlay_data = nullptr);
    void destroy();

    VkExtent2D extent() const { return m_extent; }
    VkFormat format() const { return m_format; }
    VkRenderPass render_pass() const { return m_render_pass; }
    VkRenderPass composite_render_pass() const { return m_composite_pass; }
    VkImageView postprocess_view(size_t index) const { return m_post_views.at(index); }
    VkFormat depth_format() const { return m_depth_format; }
    VkCommandPool command_pool() const { return m_command_pool; }
    size_t image_count() const { return m_images.size(); }
    bool device_lost() const { return m_device_lost; }
    bool surface_lost() const { return m_surface_lost; }

private:
    bool create_swapchain(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        VkExtent2D requested_extent, std::string& error);
    bool create_render_targets(std::string& error);
    bool create_postprocess_targets(std::string& error);
    bool create_commands(std::string& error);
    bool create_sync(std::string& error);
    void destroy_swapchain_resources();

    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_queue_family = UINT32_MAX;
    FrameDispatch m_vk{};
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkRenderPass m_overlay_pass = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    VkExtent2D m_extent{};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkFormat m_depth_format = VK_FORMAT_UNDEFINED;
    VkPhysicalDeviceMemoryProperties m_memory_properties{};
    bool m_allow_readback = false;
    bool m_preserve_prepass_depth = false;
    bool m_postprocess = false;
    bool m_scene_depth_attached = false;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    std::vector<VkImage> m_depth_images;
    std::vector<VkDeviceMemory> m_depth_memories;
    std::vector<VkImageView> m_depth_views;
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass m_composite_pass = VK_NULL_HANDLE;
    std::vector<VkImage> m_post_images;
    std::vector<VkDeviceMemory> m_post_memories;
    std::vector<VkImageView> m_post_views;
    std::vector<VkFramebuffer> m_composite_framebuffers;
    std::vector<VkCommandBuffer> m_commands;
    std::vector<VkSemaphore> m_render_finished;
    std::array<VkSemaphore, FramesInFlight> m_image_available{};
    std::array<VkFence, FramesInFlight> m_frame_fences{};
    std::vector<VkFence> m_image_fences;
    uint32_t m_current_frame{};
    bool m_device_lost{};
    bool m_surface_lost{};
};
}
