#pragma once

#include "FrameContext.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace xray::render::vulkan
{
// The first game-shaped draw format. Resource ownership and synchronization
// remain with the caller; do not modify buffers used by in-flight frames.
struct SceneVertex
{
    float position[3];
    float normal[3];
    float color[4];
};

struct UiVertex
{
    float position[2]; // framebuffer pixels, top-left origin
    float uv[2];       // reserved for the textured UI pipeline
    uint32_t color;    // normalized RGBA8
};

struct SceneConstants
{
    float model_view_projection[16]; // column-major
    float light_direction_ambient[4]; // direction xyz, ambient intensity w
    float light_color[4]; // RGB intensity, alpha ignored
};
static_assert(sizeof(SceneConstants) == 96);

struct ScenePassDispatch
{
    PFN_vkCreatePipelineLayout create_pipeline_layout{};
    PFN_vkDestroyPipelineLayout destroy_pipeline_layout{};
    PFN_vkCreateGraphicsPipelines create_graphics_pipelines{};
    PFN_vkDestroyPipeline destroy_pipeline{};
    PFN_vkCmdBindPipeline cmd_bind_pipeline{};
    PFN_vkCmdSetViewport cmd_set_viewport{};
    PFN_vkCmdSetScissor cmd_set_scissor{};
    PFN_vkCmdBindVertexBuffers cmd_bind_vertex_buffers{};
    PFN_vkCmdBindIndexBuffer cmd_bind_index_buffer{};
    PFN_vkCmdPushConstants cmd_push_constants{};
    PFN_vkCmdDrawIndexed cmd_draw_indexed{};
    PFN_vkCreateDescriptorSetLayout create_descriptor_set_layout{};
    PFN_vkDestroyDescriptorSetLayout destroy_descriptor_set_layout{};
    PFN_vkCreateDescriptorPool create_descriptor_pool{};
    PFN_vkDestroyDescriptorPool destroy_descriptor_pool{};
    PFN_vkAllocateDescriptorSets allocate_descriptor_sets{};
    PFN_vkFreeDescriptorSets free_descriptor_sets{};
    PFN_vkUpdateDescriptorSets update_descriptor_sets{};
    PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets{};
    PFN_vkCmdDraw cmd_draw{};
};

bool load_scene_pass_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
    ScenePassDispatch& dispatch, std::string& error);

// A diagnostic forward geometry pass followed by alpha-blended textured UI.
// The caller supplies SPIR-V modules, vertex/index buffers and optional depth.
class ScenePass
{
public:
    ScenePass() = default;
    ~ScenePass() { destroy(); }
    ScenePass(const ScenePass&) = delete;
    ScenePass& operator=(const ScenePass&) = delete;

    bool initialize(VkDevice device, VkRenderPass render_pass,
        VkShaderModule scene_vertex, VkShaderModule scene_fragment,
        VkShaderModule ui_vertex, VkShaderModule ui_fragment,
        const ScenePassDispatch& dispatch, std::string& error, bool use_depth = false);
    bool record_geometry(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        VkIndexType index_type, uint32_t index_count, const SceneConstants& constants,
        VkDeviceSize vertex_offset = 0, VkDeviceSize index_offset = 0) const;
    bool record_ui(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        VkIndexType index_type, uint32_t index_count, VkDescriptorSet texture_set,
        const VkRect2D* scissor = nullptr,
        VkDeviceSize vertex_offset = 0, VkDeviceSize index_offset = 0,
        float alpha_ref = 0.0f, VkExtent2D logical_extent = {}, int blend_mode = 1) const;
    bool create_ui_texture_set(VkImageView view, VkSampler sampler, VkDescriptorSet& result,
        std::string& error);
    void update_ui_texture_set(VkDescriptorSet set, VkImageView view, VkSampler sampler);
    void release_ui_texture_set(VkDescriptorSet& set);
    void rebind_render_pass(VkRenderPass compatible_render_pass, VkRenderPass overlay_pass = VK_NULL_HANDLE)
    { m_render_pass = compatible_render_pass; m_overlay_pass = overlay_pass; }
    void destroy();

private:
    bool valid_frame(const FrameRecordingContext& frame) const;
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkRenderPass m_overlay_pass = VK_NULL_HANDLE;
    VkPipelineLayout m_scene_layout = VK_NULL_HANDLE;
    VkPipelineLayout m_ui_layout = VK_NULL_HANDLE;
    VkPipeline m_scene_pipeline = VK_NULL_HANDLE;
    VkPipeline m_ui_pipelines[11]{};
    VkDescriptorSetLayout m_ui_descriptor_layout = VK_NULL_HANDLE;
    VkDescriptorPool m_ui_descriptor_pool = VK_NULL_HANDLE;
    ScenePassDispatch m_vk{};
};
}
