#include "ScenePass.h"

#include <algorithm>

namespace xray::render::vulkan
{
namespace
{
bool complete(const ScenePassDispatch& vk)
{
    return vk.create_pipeline_layout && vk.destroy_pipeline_layout && vk.create_graphics_pipelines &&
        vk.destroy_pipeline && vk.cmd_bind_pipeline && vk.cmd_set_viewport && vk.cmd_set_scissor &&
        vk.cmd_bind_vertex_buffers && vk.cmd_bind_index_buffer && vk.cmd_push_constants && vk.cmd_draw_indexed &&
        vk.create_descriptor_set_layout && vk.destroy_descriptor_set_layout && vk.create_descriptor_pool &&
        vk.destroy_descriptor_pool && vk.allocate_descriptor_sets && vk.free_descriptor_sets &&
        vk.update_descriptor_sets &&
        vk.cmd_bind_descriptor_sets;
}

bool make_pipeline(VkDevice device, VkRenderPass render_pass, VkPipelineLayout layout,
    VkShaderModule vertex, VkShaderModule fragment, bool ui, bool use_depth, const ScenePassDispatch& vk,
    VkPipeline& result)
{
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";

    const VkVertexInputBindingDescription binding{0,
        static_cast<uint32_t>(ui ? sizeof(UiVertex) : sizeof(SceneVertex)), VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription scene_attributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SceneVertex, color)}
    };
    const VkVertexInputAttributeDescription ui_attributes[] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UiVertex, position)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UiVertex, uv)},
        {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(UiVertex, color)}
    };
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 3;
    vertex_input.pVertexAttributeDescriptions = ui ? ui_attributes : scene_attributes;
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
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = use_depth && !ui ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = use_depth && !ui ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = ui ? VK_TRUE : VK_FALSE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamics{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamics.dynamicStateCount = 2;
    dynamics.pDynamicStates = states;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = use_depth ? &depth : nullptr;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamics;
    info.layout = layout;
    info.renderPass = render_pass;
    return vk.create_graphics_pipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &result) == VK_SUCCESS;
}
}

bool load_scene_pass_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
    ScenePassDispatch& dispatch, std::string& error)
{
    dispatch = {};
    if (!device || !get_proc)
    {
        error = "Vulkan scene pass requires a device";
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
    XRAY_LOAD(cmd_bind_vertex_buffers, "vkCmdBindVertexBuffers");
    XRAY_LOAD(cmd_bind_index_buffer, "vkCmdBindIndexBuffer");
    XRAY_LOAD(cmd_push_constants, "vkCmdPushConstants");
    XRAY_LOAD(cmd_draw_indexed, "vkCmdDrawIndexed");
    XRAY_LOAD(cmd_draw, "vkCmdDraw");
    XRAY_LOAD(create_descriptor_set_layout, "vkCreateDescriptorSetLayout");
    XRAY_LOAD(destroy_descriptor_set_layout, "vkDestroyDescriptorSetLayout");
    XRAY_LOAD(create_descriptor_pool, "vkCreateDescriptorPool");
    XRAY_LOAD(destroy_descriptor_pool, "vkDestroyDescriptorPool");
    XRAY_LOAD(allocate_descriptor_sets, "vkAllocateDescriptorSets");
    XRAY_LOAD(free_descriptor_sets, "vkFreeDescriptorSets");
    XRAY_LOAD(update_descriptor_sets, "vkUpdateDescriptorSets");
    XRAY_LOAD(cmd_bind_descriptor_sets, "vkCmdBindDescriptorSets");
#undef XRAY_LOAD
    if (!complete(dispatch))
    {
        dispatch = {};
        error = "required Vulkan scene pass procedures are unavailable";
        return false;
    }
    error.clear();
    return true;
}

bool ScenePass::initialize(VkDevice device, VkRenderPass render_pass,
    VkShaderModule scene_vertex, VkShaderModule scene_fragment,
    VkShaderModule ui_vertex, VkShaderModule ui_fragment,
    const ScenePassDispatch& dispatch, std::string& error, bool use_depth)
{
    destroy();
    if (!device || !render_pass || !scene_vertex || !scene_fragment || !ui_vertex || !ui_fragment ||
        !complete(dispatch))
    {
        error = "Vulkan scene pass requires shaders, render pass and procedures";
        return false;
    }
    m_device = device;
    m_render_pass = render_pass;
    m_vk = dispatch;
    const VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_info.bindingCount = 1;
    descriptor_info.pBindings = &binding;
    if (m_vk.create_descriptor_set_layout(device, &descriptor_info, nullptr,
            &m_ui_descriptor_layout) != VK_SUCCESS)
    {
        error = "could not create Vulkan UI texture layout";
        destroy();
        return false;
    }
    const VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 128;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    if (m_vk.create_descriptor_pool(device, &pool_info, nullptr, &m_ui_descriptor_pool) != VK_SUCCESS)
    {
        error = "could not create Vulkan UI texture pool";
        destroy();
        return false;
    }
    const VkPushConstantRange scene_range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(SceneConstants)};
    const VkPushConstantRange ui_range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(float) * 3};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &scene_range;
    if (m_vk.create_pipeline_layout(device, &layout, nullptr, &m_scene_layout) != VK_SUCCESS)
    {
        error = "could not create Vulkan scene pipeline layout";
        destroy();
        return false;
    }
    layout.pPushConstantRanges = &ui_range;
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &m_ui_descriptor_layout;
    if (m_vk.create_pipeline_layout(device, &layout, nullptr, &m_ui_layout) != VK_SUCCESS ||
        !make_pipeline(device, render_pass, m_scene_layout, scene_vertex, scene_fragment,
            false, use_depth, m_vk, m_scene_pipeline) ||
        !make_pipeline(device, render_pass, m_ui_layout, ui_vertex, ui_fragment,
            true, use_depth, m_vk, m_ui_pipeline))
    {
        error = "could not create Vulkan geometry, lighting or UI pipeline";
        destroy();
        return false;
    }
    error.clear();
    return true;
}

bool ScenePass::valid_frame(const FrameRecordingContext& frame) const
{
    return m_scene_pipeline && m_ui_pipeline && frame.command_buffer && frame.render_pass == m_render_pass &&
        frame.extent.width && frame.extent.height;
}

bool ScenePass::record_geometry(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
    VkIndexType index_type, uint32_t index_count, const SceneConstants& constants,
    VkDeviceSize vertex_offset, VkDeviceSize index_offset) const
{
    if (!valid_frame(frame) || !vertices || !indices || !index_count ||
        (index_type != VK_INDEX_TYPE_UINT16 && index_type != VK_INDEX_TYPE_UINT32))
        return false;
    const VkViewport viewport{0, 0, static_cast<float>(frame.extent.width),
        static_cast<float>(frame.extent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, frame.extent};
    m_vk.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_scene_pipeline);
    m_vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
    m_vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
    m_vk.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &vertex_offset);
    m_vk.cmd_bind_index_buffer(frame.command_buffer, indices, index_offset, index_type);
    m_vk.cmd_push_constants(frame.command_buffer, m_scene_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
    m_vk.cmd_draw_indexed(frame.command_buffer, index_count, 1, 0, 0, 0);
    return true;
}

bool ScenePass::record_ui(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
    VkIndexType index_type, uint32_t index_count, VkDescriptorSet texture_set,
    const VkRect2D* requested_scissor,
    VkDeviceSize vertex_offset, VkDeviceSize index_offset, float alpha_ref) const
{
    if (!valid_frame(frame) || !vertices || !indices || !texture_set || !index_count ||
        (index_type != VK_INDEX_TYPE_UINT16 && index_type != VK_INDEX_TYPE_UINT32))
        return false;
    VkRect2D scissor{{0, 0}, frame.extent};
    if (requested_scissor)
    {
        const int64_t left = std::clamp<int64_t>(requested_scissor->offset.x, 0, frame.extent.width);
        const int64_t top = std::clamp<int64_t>(requested_scissor->offset.y, 0, frame.extent.height);
        const int64_t right = std::clamp<int64_t>(
            static_cast<int64_t>(requested_scissor->offset.x) + requested_scissor->extent.width,
            left, frame.extent.width);
        const int64_t bottom = std::clamp<int64_t>(
            static_cast<int64_t>(requested_scissor->offset.y) + requested_scissor->extent.height,
            top, frame.extent.height);
        scissor = {{static_cast<int32_t>(left), static_cast<int32_t>(top)},
            {static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top)}};
        if (!scissor.extent.width || !scissor.extent.height)
            return true;
    }
    const VkViewport viewport{0, 0, static_cast<float>(frame.extent.width),
        static_cast<float>(frame.extent.height), 0, 1};
    const float constants[]{static_cast<float>(frame.extent.width),
        static_cast<float>(frame.extent.height), std::clamp(alpha_ref, 0.0f, 1.0f)};
    m_vk.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ui_pipeline);
    m_vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
    m_vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
    m_vk.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &vertex_offset);
    m_vk.cmd_bind_index_buffer(frame.command_buffer, indices, index_offset, index_type);
    m_vk.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_ui_layout, 0, 1, &texture_set, 0, nullptr);
    m_vk.cmd_push_constants(frame.command_buffer, m_ui_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(constants), constants);
    m_vk.cmd_draw_indexed(frame.command_buffer, index_count, 1, 0, 0, 0);
    return true;
}

bool ScenePass::create_ui_texture_set(VkImageView view, VkSampler sampler, VkDescriptorSet& result,
    std::string& error)
{
    result = VK_NULL_HANDLE;
    if (!m_device || !m_ui_descriptor_pool || !view || !sampler)
    {
        error = "Vulkan UI texture requires a pass, image view and sampler";
        return false;
    }
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = m_ui_descriptor_pool;
    allocate.descriptorSetCount = 1;
    allocate.pSetLayouts = &m_ui_descriptor_layout;
    if (m_vk.allocate_descriptor_sets(m_device, &allocate, &result) != VK_SUCCESS)
    {
        error = "Vulkan UI texture descriptor pool is exhausted";
        return false;
    }
    const VkDescriptorImageInfo image{sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = result;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    m_vk.update_descriptor_sets(m_device, 1, &write, 0, nullptr);
    error.clear();
    return true;
}

void ScenePass::update_ui_texture_set(VkDescriptorSet set, VkImageView view, VkSampler sampler)
{
    const VkDescriptorImageInfo image{sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    m_vk.update_descriptor_sets(m_device, 1, &write, 0, nullptr);
}

void ScenePass::release_ui_texture_set(VkDescriptorSet& set)
{
    if (m_device && m_ui_descriptor_pool && set && m_vk.free_descriptor_sets)
        m_vk.free_descriptor_sets(m_device, m_ui_descriptor_pool, 1, &set);
    set = VK_NULL_HANDLE;
}

void ScenePass::destroy()
{
    if (m_device && m_vk.destroy_pipeline)
    {
        if (m_ui_pipeline) m_vk.destroy_pipeline(m_device, m_ui_pipeline, nullptr);
        if (m_scene_pipeline) m_vk.destroy_pipeline(m_device, m_scene_pipeline, nullptr);
    }
    if (m_device && m_vk.destroy_pipeline_layout)
    {
        if (m_ui_layout) m_vk.destroy_pipeline_layout(m_device, m_ui_layout, nullptr);
        if (m_scene_layout) m_vk.destroy_pipeline_layout(m_device, m_scene_layout, nullptr);
    }
    if (m_device && m_ui_descriptor_pool && m_vk.destroy_descriptor_pool)
        m_vk.destroy_descriptor_pool(m_device, m_ui_descriptor_pool, nullptr);
    if (m_device && m_ui_descriptor_layout && m_vk.destroy_descriptor_set_layout)
        m_vk.destroy_descriptor_set_layout(m_device, m_ui_descriptor_layout, nullptr);
    m_device = VK_NULL_HANDLE;
    m_render_pass = VK_NULL_HANDLE;
    m_scene_layout = m_ui_layout = VK_NULL_HANDLE;
    m_scene_pipeline = m_ui_pipeline = VK_NULL_HANDLE;
    m_ui_descriptor_pool = VK_NULL_HANDLE;
    m_ui_descriptor_layout = VK_NULL_HANDLE;
    m_vk = {};
}
}
