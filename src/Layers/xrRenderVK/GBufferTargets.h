#pragma once

#include "DeferredPass.h"

namespace xray::render::vulkan
{
// Offscreen target triplets are indexed by the acquired swapchain image, so
// their reuse follows FrameContext's image fences. Destroy after GPU idle and
// before destroying the device, command pool or DeferredPass descriptor pool.
class GBufferTargets
{
public:
    GBufferTargets() = default;
    ~GBufferTargets() { destroy(); }
    GBufferTargets(const GBufferTargets&) = delete;
    GBufferTargets& operator=(const GBufferTargets&) = delete;

    bool initialize(VkPhysicalDevice physical_device, VkDevice device, VkExtent2D extent, uint32_t image_count,
        VkFormat depth_format, const VkPhysicalDeviceMemoryProperties& memory,
        const FrameDispatch& dispatch, PFN_vkCreateSampler create_sampler,
        PFN_vkDestroySampler destroy_sampler, std::string& error);
    bool bind_lighting(DeferredPass& pass, std::string& error);
    bool begin(const FrameRecordingContext& frame, FrameRecordingContext& geometry_frame) const;
    void end(VkCommandBuffer command) const;
    VkDescriptorSet lighting_set(uint32_t image_index) const;
    VkRenderPass render_pass() const { return pass_; }
    void destroy();

private:
    struct Attachment
    {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
    };
    struct Target
    {
        Attachment albedo, normal, depth;
        VkFramebuffer framebuffer{};
        VkDescriptorSet lighting{};
    };
    bool create_attachment(VkFormat format, VkImageUsageFlags usage,
        VkImageAspectFlags aspect, Attachment& attachment, std::string& error);

    VkDevice device_{};
    VkExtent2D extent_{};
    VkRenderPass pass_{};
    VkSampler sampler_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    FrameDispatch vk_{};
    PFN_vkDestroySampler destroy_sampler_{};
    std::vector<Target> targets_;
};
}
