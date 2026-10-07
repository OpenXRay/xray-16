#pragma once

#include "BufferResource.h"
#include "FrameContext.h"
#include "VulkanGameLightingMath.h"

#include <array>
#include <vector>

namespace xray::render::vulkan
{
constexpr uint32_t LocalShadowSlots = 4;
constexpr uint32_t LocalShadowFaces = 6;
constexpr uint32_t LocalShadowSize = 512;
constexpr uint32_t LocalLightCapacity = 64;

struct alignas(16) LocalLightUniform
{
    float inverse_view_projection[16]{};
    float shadow_matrices[LocalShadowFaces][16]{};
    float position_range[4]{};
    float direction_cone[4]{};
    float color_type[4]{};
    float shadow_params[4]{}; // base array layer, bias, enabled, unused
};
static_assert(sizeof(LocalLightUniform) % 16 == 0);

// A bounded, frame-image-owned array avoids reusing a depth layer or uniform
// while that swapchain image is in flight. Slots are reassigned after its fence.
class LocalShadowTargets
{
public:
    ~LocalShadowTargets() { destroy(); }
    bool initialize(VkDevice device, VkFormat format, uint32_t image_count,
        const VkPhysicalDeviceMemoryProperties& memory, const FrameDispatch& dispatch,
        const BufferResourceDispatch& buffers, PFN_vkCreateSampler create_sampler,
        PFN_vkDestroySampler destroy_sampler, std::string& error,
        VkDeviceSize uniform_alignment = 256);
    bool write_light(uint32_t image, uint32_t index, const LocalLightUniform& value,
        std::string& error);
    bool initialize_layers(const FrameRecordingContext& frame);
    bool begin_face(const FrameRecordingContext& frame, uint32_t slot, uint32_t face,
        FrameRecordingContext& shadow_frame) const;
    void end(VkCommandBuffer command) const;
    VkImageView view(uint32_t image) const;
    VkBuffer uniform(uint32_t image) const;
    VkSampler sampler() const { return sampler_; }
    VkRenderPass render_pass() const { return pass_; }
    VkDeviceSize uniform_stride() const { return uniform_stride_; }
    void destroy();

private:
    struct Target
    {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView array_view{};
        std::array<VkImageView, LocalShadowSlots * LocalShadowFaces> layer_views{};
        std::array<VkFramebuffer, LocalShadowSlots * LocalShadowFaces> framebuffers{};
        BufferResource constants;
        bool initialized{};
    };
    VkDevice device_{};
    VkRenderPass pass_{};
    VkSampler sampler_{};
    PFN_vkDestroySampler destroy_sampler_{};
    FrameDispatch vk_{};
    VkDeviceSize uniform_stride_{sizeof(LocalLightUniform)};
    std::vector<Target> targets_;
};
}
