#pragma once

#include "BufferResource.h"
#include "FrameContext.h"

namespace xray::render::vulkan
{
struct alignas(16) SunShadowUniform
{
    float inverse_view_projection[16]{};
    float sun_view_projection[16]{};
    float options[4]{0.0015f, 0.f, 0.f, 0.f};
};

// Each acquired swapchain image owns a depth map and its uniform buffer;
// neither is overwritten while that image may still be in flight.
class SunShadowTargets
{
public:
    ~SunShadowTargets() { destroy(); }
    bool initialize(VkDevice device, VkFormat depth_format, uint32_t image_count,
        const VkPhysicalDeviceMemoryProperties& memory, const FrameDispatch& dispatch,
        const BufferResourceDispatch& buffers, PFN_vkCreateSampler create_sampler,
        PFN_vkDestroySampler destroy_sampler, std::string& error);
    bool begin(const FrameRecordingContext& frame, const SunShadowUniform& constants,
        FrameRecordingContext& shadow_frame, std::string& error);
    void end(VkCommandBuffer command) const;
    VkRenderPass render_pass() const { return pass_; }
    VkImageView view(uint32_t index) const { return index < targets_.size() ? targets_[index].view : VK_NULL_HANDLE; }
    VkBuffer uniform(uint32_t index) const { return index < targets_.size() ? targets_[index].constants.handle() : VK_NULL_HANDLE; }
    VkSampler sampler() const { return sampler_; }
    void destroy();
    static constexpr uint32_t Size = 1024;

private:
    struct Target
    {
        VkImage image{};
        VkImageView view{};
        VkDeviceMemory memory{};
        VkFramebuffer framebuffer{};
        BufferResource constants;
    };
    VkDevice device_{};
    VkRenderPass pass_{};
    VkSampler sampler_{};
    FrameDispatch vk_{};
    PFN_vkDestroySampler destroy_sampler_{};
    std::vector<Target> targets_;
};
}
