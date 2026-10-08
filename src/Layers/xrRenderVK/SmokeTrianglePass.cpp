#include "SmokeTrianglePass.h"

namespace xray::render::vulkan
{
namespace
{
bool complete(const SmokeTriangleDispatch& vk)
{
    return vk.create_pipeline_layout && vk.destroy_pipeline_layout && vk.create_graphics_pipelines &&
        vk.destroy_pipeline && vk.cmd_bind_pipeline && vk.cmd_set_viewport && vk.cmd_set_scissor && vk.cmd_draw;
}
}

bool load_smoke_triangle_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
    SmokeTriangleDispatch& dispatch, std::string& error)
{
    dispatch = {};
    if (!device || !get_proc)
    {
        error = "Vulkan smoke pipeline requires a device";
        return false;
    }
#define XRAY_LOAD(member, name) dispatch.member = reinterpret_cast<decltype(dispatch.member)>(get_proc(device, name))
    XRAY_LOAD(create_pipeline_layout, "vkCreatePipelineLayout");
    XRAY_LOAD(destroy_pipeline_layout, "vkDestroyPipelineLayout");
    XRAY_LOAD(create_graphics_pipelines, "vkCreateGraphicsPipelines");
    XRAY_LOAD(destroy_pipeline, "vkDestroyPipeline");
    XRAY_LOAD(cmd_bind_pipeline, "vkCmdBindPipeline");
    XRAY_LOAD(cmd_set_viewport, "vkCmdSetViewport");
    XRAY_LOAD(cmd_set_scissor, "vkCmdSetScissor");
    XRAY_LOAD(cmd_draw, "vkCmdDraw");
#undef XRAY_LOAD
    if (!complete(dispatch))
    {
        dispatch = {};
        error = "required Vulkan smoke pipeline procedures are unavailable";
        return false;
    }
    error.clear();
    return true;
}

bool SmokeTrianglePass::initialize(VkDevice device, VkRenderPass render_pass, VkShaderModule vertex,
    VkShaderModule fragment, const SmokeTriangleDispatch& dispatch, std::string& error)
{
    destroy();
    if (!device || !render_pass || !vertex || !fragment || !complete(dispatch))
    {
        error = "Vulkan smoke pipeline requires shaders and a render pass";
        return false;
    }
    m_device = device;
    m_render_pass = render_pass;
    m_vk = dispatch;

    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (m_vk.create_pipeline_layout(device, &layout, nullptr, &m_layout) != VK_SUCCESS)
    {
        error = "vkCreatePipelineLayout failed for Vulkan smoke";
        destroy();
        return false;
    }
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    const VkDynamicState dynamics[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamics;

    VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline.stageCount = 2;
    pipeline.pStages = stages;
    pipeline.pVertexInputState = &vertex_input;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewport;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &multisample;
    pipeline.pColorBlendState = &blend;
    pipeline.pDynamicState = &dynamic;
    pipeline.layout = m_layout;
    pipeline.renderPass = render_pass;
    if (m_vk.create_graphics_pipelines(device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        error = "vkCreateGraphicsPipelines failed for Vulkan smoke";
        destroy();
        return false;
    }
    error.clear();
    return true;
}

void SmokeTrianglePass::record(const FrameRecordingContext& frame) const
{
    if (!m_pipeline || frame.render_pass != m_render_pass || !frame.command_buffer ||
        !frame.extent.width || !frame.extent.height)
        return;
    const VkViewport viewport{0, 0, static_cast<float>(frame.extent.width),
        static_cast<float>(frame.extent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, frame.extent};
    m_vk.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    m_vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
    m_vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
    m_vk.cmd_draw(frame.command_buffer, 3, 1, 0, 0);
}

void SmokeTrianglePass::destroy()
{
    if (m_device && m_pipeline && m_vk.destroy_pipeline)
        m_vk.destroy_pipeline(m_device, m_pipeline, nullptr);
    if (m_device && m_layout && m_vk.destroy_pipeline_layout)
        m_vk.destroy_pipeline_layout(m_device, m_layout, nullptr);
    m_device = VK_NULL_HANDLE;
    m_render_pass = VK_NULL_HANDLE;
    m_layout = VK_NULL_HANDLE;
    m_pipeline = VK_NULL_HANDLE;
    m_vk = {};
}
}
