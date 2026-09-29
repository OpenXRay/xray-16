#pragma once

#include "FrameContext.h"

namespace xray::render::vulkan
{
struct SmokeTriangleDispatch
{
    PFN_vkCreatePipelineLayout create_pipeline_layout{};
    PFN_vkDestroyPipelineLayout destroy_pipeline_layout{};
    PFN_vkCreateGraphicsPipelines create_graphics_pipelines{};
    PFN_vkDestroyPipeline destroy_pipeline{};
    PFN_vkCmdBindPipeline cmd_bind_pipeline{};
    PFN_vkCmdSetViewport cmd_set_viewport{};
    PFN_vkCmdSetScissor cmd_set_scissor{};
    PFN_vkCmdDraw cmd_draw{};
};

bool load_smoke_triangle_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
    SmokeTriangleDispatch& dispatch, std::string& error);

// Diagnostic draw with vertex-index positions and no game resources.
class SmokeTrianglePass
{
public:
    ~SmokeTrianglePass() { destroy(); }
    SmokeTrianglePass(const SmokeTrianglePass&) = delete;
    SmokeTrianglePass& operator=(const SmokeTrianglePass&) = delete;
    SmokeTrianglePass() = default;

    bool initialize(VkDevice device, VkRenderPass render_pass, VkShaderModule vertex,
        VkShaderModule fragment, const SmokeTriangleDispatch& dispatch, std::string& error);
    void record(const FrameRecordingContext& frame) const;
    void destroy();

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    SmokeTriangleDispatch m_vk{};
};
}
