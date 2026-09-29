#pragma once

#include "FrameContext.h"
#include "LevelModels.h"
#include "ScenePass.h"

namespace xray::render::vulkan
{
struct DeferredLight
{
    float direction_ambient[4];
    float color[4];
};

// Framebuffer attachment order: albedo, normal, depth. Both color images must
// have VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT.
bool create_gbuffer_render_pass(VkDevice device, VkFormat albedo_format,
    VkFormat normal_format, VkFormat depth_format, const FrameDispatch& vk,
    VkRenderPass& result, std::string& error);

// Geometry pass expects two color attachments (RGBA albedo, encoded normal)
// and depth. The light pass expects one swapchain color attachment. Attachments,
// render-pass transitions and per-frame synchronization belong to the caller.
class DeferredPass
{
public:
    ~DeferredPass() { destroy(); }
    DeferredPass(const DeferredPass&) = delete;
    DeferredPass& operator=(const DeferredPass&) = delete;
    DeferredPass() = default;

    bool initialize(VkDevice device, VkRenderPass geometry_pass, VkRenderPass light_pass,
        VkShaderModule geometry_vertex, VkShaderModule geometry_fragment,
        VkShaderModule light_vertex, VkShaderModule light_fragment,
        const ScenePassDispatch& dispatch, std::string& error);
    bool material(VkImageView albedo, VkSampler sampler, VkDescriptorSet& set, std::string& error);
    bool gbuffer(VkImageView albedo, VkImageView normal, VkSampler sampler,
        VkDescriptorSet& set, std::string& error);
    bool record_geometry(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set) const;
    bool record_lighting(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
        const DeferredLight& light) const;
    void destroy();

private:
    bool allocate(VkDescriptorSetLayout layout, VkImageView first, VkImageView second,
        VkSampler sampler, VkDescriptorSet& set, std::string& error);
    VkDevice device_{};
    VkRenderPass geometry_pass_{}, light_pass_{};
    VkPipeline geometry_{}, lighting_{};
    VkPipelineLayout geometry_layout_{}, light_layout_{};
    VkDescriptorSetLayout material_layout_{}, gbuffer_layout_{};
    VkDescriptorPool pool_{};
    ScenePassDispatch vk_{};
};
}
