#include "ScreenCopyPass.h"

namespace xray::render::vulkan
{
namespace
{
bool complete(const ScreenCopyDispatch& vk)
{
    return vk.create_descriptor_set_layout && vk.destroy_descriptor_set_layout && vk.create_descriptor_pool &&
        vk.destroy_descriptor_pool && vk.allocate_descriptor_sets && vk.update_descriptor_sets &&
        vk.create_pipeline_layout && vk.destroy_pipeline_layout && vk.create_graphics_pipelines &&
        vk.destroy_pipeline && vk.cmd_bind_pipeline && vk.cmd_bind_descriptor_sets && vk.cmd_set_viewport &&
        vk.cmd_set_scissor && vk.cmd_draw;
}

template <typename T>
T load_screen_copy_device_proc(VkDevice device, PFN_vkGetDeviceProcAddr get_proc, const char* name)
{
    return reinterpret_cast<T>(get_proc(device, name));
}
}

bool load_screen_copy_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_device_proc,
    ScreenCopyDispatch& dispatch, std::string& error)
{
    dispatch = {};
    if (!device || !get_device_proc)
    {
        error = "Vulkan screen-copy dispatch requires a valid device";
        return false;
    }

#define XRAY_LOAD_DEVICE(member, name) \
    dispatch.member = load_screen_copy_device_proc<decltype(dispatch.member)>(device, get_device_proc, name)
    XRAY_LOAD_DEVICE(create_descriptor_set_layout, "vkCreateDescriptorSetLayout");
    XRAY_LOAD_DEVICE(destroy_descriptor_set_layout, "vkDestroyDescriptorSetLayout");
    XRAY_LOAD_DEVICE(create_descriptor_pool, "vkCreateDescriptorPool");
    XRAY_LOAD_DEVICE(destroy_descriptor_pool, "vkDestroyDescriptorPool");
    XRAY_LOAD_DEVICE(allocate_descriptor_sets, "vkAllocateDescriptorSets");
    XRAY_LOAD_DEVICE(update_descriptor_sets, "vkUpdateDescriptorSets");
    XRAY_LOAD_DEVICE(create_pipeline_layout, "vkCreatePipelineLayout");
    XRAY_LOAD_DEVICE(destroy_pipeline_layout, "vkDestroyPipelineLayout");
    XRAY_LOAD_DEVICE(create_graphics_pipelines, "vkCreateGraphicsPipelines");
    XRAY_LOAD_DEVICE(destroy_pipeline, "vkDestroyPipeline");
    XRAY_LOAD_DEVICE(cmd_bind_pipeline, "vkCmdBindPipeline");
    XRAY_LOAD_DEVICE(cmd_bind_descriptor_sets, "vkCmdBindDescriptorSets");
    XRAY_LOAD_DEVICE(cmd_set_viewport, "vkCmdSetViewport");
    XRAY_LOAD_DEVICE(cmd_set_scissor, "vkCmdSetScissor");
    XRAY_LOAD_DEVICE(cmd_draw, "vkCmdDraw");
    XRAY_LOAD_DEVICE(cmd_push_constants, "vkCmdPushConstants");
#undef XRAY_LOAD_DEVICE

    if (!complete(dispatch))
    {
        dispatch = {};
        error = "required Vulkan screen-copy procedures are unavailable";
        return false;
    }
    error.clear();
    return true;
}

ScreenCopyPass::~ScreenCopyPass()
{
    destroy();
}

bool ScreenCopyPass::initialize(VkDevice device, VkRenderPass render_pass, VkImageView source_view,
    VkSampler sampler, VkShaderModule vertex_shader, VkShaderModule fragment_shader,
    const ScreenCopyDispatch& dispatch, std::string& error)
{
    destroy();
    if (!device || !render_pass || !source_view || !sampler || !vertex_shader || !fragment_shader ||
        !complete(dispatch))
    {
        error = "Vulkan screen-copy pass requires shaders, a source image, a render pass and procedures";
        return false;
    }

    m_device = device;
    m_render_pass = render_pass;
    m_vk = dispatch;
    m_source_view = source_view;

    const VkDescriptorSetLayoutBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{};
    descriptor_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_info.bindingCount = static_cast<uint32_t>(sizeof(bindings) / sizeof(bindings[0]));
    descriptor_layout_info.pBindings = bindings;
    if (m_vk.create_descriptor_set_layout(m_device, &descriptor_layout_info, nullptr,
            &m_descriptor_layout) != VK_SUCCESS)
    {
        error = "vkCreateDescriptorSetLayout failed for screen-copy pass";
        destroy();
        return false;
    }

    const VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1}
    };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = static_cast<uint32_t>(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
    pool_info.pPoolSizes = pool_sizes;
    if (m_vk.create_descriptor_pool(m_device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
    {
        error = "vkCreateDescriptorPool failed for screen-copy pass";
        destroy();
        return false;
    }

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = m_descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &m_descriptor_layout;
    if (m_vk.allocate_descriptor_sets(m_device, &set_info, &m_descriptor_set) != VK_SUCCESS)
    {
        error = "vkAllocateDescriptorSets failed for screen-copy pass";
        destroy();
        return false;
    }

    VkDescriptorImageInfo image_info{};
    image_info.imageView = source_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkDescriptorImageInfo sampler_info{};
    sampler_info.sampler = sampler;
    VkWriteDescriptorSet writes[4]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_descriptor_set;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[0].pImageInfo = &image_info;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptor_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[1].pImageInfo = &sampler_info;
    for (uint32_t i = 2; i != 4; ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_descriptor_set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[i].pImageInfo = &image_info;
    }
    m_vk.update_descriptor_sets(m_device, static_cast<uint32_t>(sizeof(writes) / sizeof(writes[0])), writes,
        0, nullptr);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &m_descriptor_layout;
    const VkPushConstantRange constants{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PostProcessConstants)};
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &constants;
    if (m_vk.create_pipeline_layout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
    {
        error = "vkCreatePipelineLayout failed for screen-copy pass";
        destroy();
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex_shader;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment_shader;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_attachment{};
    color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &color_attachment;

    const VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamic_states) / sizeof(dynamic_states[0]));
    dynamic_state.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = static_cast<uint32_t>(sizeof(stages) / sizeof(stages[0]));
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    if (m_vk.create_graphics_pipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
            &m_pipeline) != VK_SUCCESS)
    {
        error = "vkCreateGraphicsPipelines failed for screen-copy pass";
        destroy();
        return false;
    }

    error.clear();
    return true;
}

void ScreenCopyPass::set_color_maps(VkImageView first, VkImageView second)
{
    if (!m_descriptor_set || !m_vk.update_descriptor_sets) return;
    VkDescriptorImageInfo images[2]{};
    images[0].imageView = first ? first : m_source_view;
    images[1].imageView = second ? second : m_source_view;
    images[0].imageLayout = images[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet writes[2]{};
    for (uint32_t i = 0; i < 2; ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_descriptor_set;
        writes[i].dstBinding = i + 2;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[i].pImageInfo = &images[i];
    }
    m_vk.update_descriptor_sets(m_device, 2, writes, 0, nullptr);
}

void ScreenCopyPass::record(const FrameRecordingContext& frame) const
{
    if (!frame.command_buffer || frame.render_pass != m_render_pass || !frame.extent.width ||
        !frame.extent.height || !m_pipeline || !m_pipeline_layout || !m_descriptor_set)
        return;

    const VkViewport viewport{0.0f, 0.0f, static_cast<float>(frame.extent.width),
        static_cast<float>(frame.extent.height), 0.0f, 1.0f};
    const VkRect2D scissor{{0, 0}, frame.extent};
    m_vk.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    m_vk.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
        0, 1, &m_descriptor_set, 0, nullptr);
    if (m_vk.cmd_push_constants)
        m_vk.cmd_push_constants(frame.command_buffer, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(m_constants), &m_constants);
    m_vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
    m_vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
    m_vk.cmd_draw(frame.command_buffer, 3, 1, 0, 0);
}

void ScreenCopyPass::record_callback(const FrameRecordingContext& frame, void* user_data)
{
    if (user_data)
        static_cast<ScreenCopyPass*>(user_data)->record(frame);
}

void ScreenCopyPass::destroy()
{
    if (m_device && m_pipeline && m_vk.destroy_pipeline)
        m_vk.destroy_pipeline(m_device, m_pipeline, nullptr);
    if (m_device && m_pipeline_layout && m_vk.destroy_pipeline_layout)
        m_vk.destroy_pipeline_layout(m_device, m_pipeline_layout, nullptr);
    if (m_device && m_descriptor_pool && m_vk.destroy_descriptor_pool)
        m_vk.destroy_descriptor_pool(m_device, m_descriptor_pool, nullptr);
    if (m_device && m_descriptor_layout && m_vk.destroy_descriptor_set_layout)
        m_vk.destroy_descriptor_set_layout(m_device, m_descriptor_layout, nullptr);
    m_device = VK_NULL_HANDLE;
    m_render_pass = VK_NULL_HANDLE;
    m_vk = {};
    m_pipeline = VK_NULL_HANDLE;
    m_pipeline_layout = VK_NULL_HANDLE;
    m_descriptor_pool = VK_NULL_HANDLE;
    m_descriptor_set = VK_NULL_HANDLE;
    m_descriptor_layout = VK_NULL_HANDLE;
    m_source_view = VK_NULL_HANDLE;
}
}
