#pragma once

#include "FrameContext.h"

#include <vulkan/vulkan.h>

#include <string>

namespace xray::render::vulkan
{
struct ScreenCopyDispatch
{
    PFN_vkCreateDescriptorSetLayout create_descriptor_set_layout{};
    PFN_vkDestroyDescriptorSetLayout destroy_descriptor_set_layout{};
    PFN_vkCreateDescriptorPool create_descriptor_pool{};
    PFN_vkDestroyDescriptorPool destroy_descriptor_pool{};
    PFN_vkAllocateDescriptorSets allocate_descriptor_sets{};
    PFN_vkUpdateDescriptorSets update_descriptor_sets{};
    PFN_vkCreatePipelineLayout create_pipeline_layout{};
    PFN_vkDestroyPipelineLayout destroy_pipeline_layout{};
    PFN_vkCreateGraphicsPipelines create_graphics_pipelines{};
    PFN_vkDestroyPipeline destroy_pipeline{};
    PFN_vkCmdBindPipeline cmd_bind_pipeline{};
    PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets{};
    PFN_vkCmdSetViewport cmd_set_viewport{};
    PFN_vkCmdSetScissor cmd_set_scissor{};
    PFN_vkCmdDraw cmd_draw{};
    PFN_vkCmdPushConstants cmd_push_constants{};
};

struct PostProcessConstants
{
    float grade[4]{1.f, 1.f, 1.f, 0.f};
    float tint[4]{1.f, 1.f, 1.f, 0.f};
    float add[4]{};
    float gray_weights[4]{.333f, .333f, .333f, 0.f};
    float effect[4]{}; // blur, horizontal duality, vertical duality, grayscale
    float noise[4]{}; // intensity, grain, fps, elapsed time
    float color_map[4]{}; // influence, interpolation
};
static_assert(sizeof(PostProcessConstants) <= 128);

bool load_screen_copy_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_device_proc,
    ScreenCopyDispatch& dispatch, std::string& error);

// Shader set 0 must use binding 0 as a sampled image and binding 1 as a
// sampler. Keep both resources and the pass alive until submitted frames finish.
class ScreenCopyPass
{
public:
    ScreenCopyPass() = default;
    ~ScreenCopyPass();
    ScreenCopyPass(const ScreenCopyPass&) = delete;
    ScreenCopyPass& operator=(const ScreenCopyPass&) = delete;

    bool initialize(VkDevice device, VkRenderPass render_pass, VkImageView source_view,
        VkSampler sampler, VkShaderModule vertex_shader, VkShaderModule fragment_shader,
        const ScreenCopyDispatch& dispatch, std::string& error);
    void record(const FrameRecordingContext& frame) const;
    void set_constants(const PostProcessConstants& constants) { m_constants = constants; }
    void set_color_maps(VkImageView first, VkImageView second);
    static void record_callback(const FrameRecordingContext& frame, void* user_data);
    void destroy();

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    ScreenCopyDispatch m_vk{};
    VkDescriptorSetLayout m_descriptor_layout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptor_set = VK_NULL_HANDLE;
    VkImageView m_source_view = VK_NULL_HANDLE;
    PostProcessConstants m_constants{};
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
}
