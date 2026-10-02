#include "DeferredPass.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace xray::render::vulkan
{
namespace
{
bool make_pipeline(VkDevice device, VkRenderPass pass, VkPipelineLayout layout,
    VkShaderModule vertex, VkShaderModule fragment, bool geometry_input, bool gbuffer,
    bool transparent, bool hud,
    const ScenePassDispatch& vk, VkPipeline& pipeline)
{
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";
    const VkVertexInputBindingDescription binding{0, sizeof(LevelVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription attributes[]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LevelVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LevelVertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(LevelVertex, uv)}
    };
    VkPipelineVertexInputStateCreateInfo inputs{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    inputs.vertexBindingDescriptionCount = geometry_input ? 1 : 0;
    inputs.pVertexBindingDescriptions = geometry_input ? &binding : nullptr;
    inputs.vertexAttributeDescriptionCount = geometry_input ? 3 : 0;
    inputs.pVertexAttributeDescriptions = geometry_input ? attributes : nullptr;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    // The present pass has its own cleared depth attachment. HUD geometry
    // draws after world lighting, so depth testing here orders HUD surfaces
    // against one another without occluding the HUD with world geometry.
    depth.depthTestEnable = (gbuffer || hud) ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = ((gbuffer && !transparent) || hud) ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState attachments[2]{};
    for (auto& attachment : attachments)
    {
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment.blendEnable = transparent || hud ? VK_TRUE : VK_FALSE;
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = gbuffer ? 2 : 1;
    blend.pAttachments = attachments;
    const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = states;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &inputs;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multi;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = layout;
    info.renderPass = pass;
    return vk.create_graphics_pipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) == VK_SUCCESS;
}

void viewport_scissor(const FrameRecordingContext& frame, const ScenePassDispatch& vk)
{
    const VkViewport viewport{0, 0, float(frame.extent.width), float(frame.extent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, frame.extent};
    vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
    vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
}
}

DeferredLight make_environment_deferred_light(const DeferredEnvironment& environment)
{
    const auto safe_nonnegative = [](float value)
    {
        return std::isfinite(value) ? std::max(value, 0.f) : 0.f;
    };
    float direction[3]{environment.sun_direction[0], environment.sun_direction[1], environment.sun_direction[2]};
    const float magnitude = std::sqrt(direction[0] * direction[0] +
        direction[1] * direction[1] + direction[2] * direction[2]);
    if (std::isfinite(magnitude) && magnitude > 1e-6f)
        for (float& value : direction) value /= magnitude;
    else
    {
        direction[0] = 0.f;
        direction[1] = -1.f;
        direction[2] = 0.f;
    }

    const auto luminance = [&](const float (&color)[3])
    {
        return 0.2126f * safe_nonnegative(color[0]) +
            0.7152f * safe_nonnegative(color[1]) +
            0.0722f * safe_nonnegative(color[2]);
    };
    const float ambient = luminance(environment.ambient_color) +
        0.25f * luminance(environment.hemi_color);
    return {{direction[0], direction[1], direction[2], ambient},
        {safe_nonnegative(environment.sun_color[0]),
            safe_nonnegative(environment.sun_color[1]),
            safe_nonnegative(environment.sun_color[2]), 0.f}};
}

bool create_gbuffer_render_pass(VkDevice device, VkFormat albedo_format,
    VkFormat normal_format, VkFormat depth_format, const FrameDispatch& vk,
    VkRenderPass& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    if (!device || !vk.create_render_pass || albedo_format == VK_FORMAT_UNDEFINED ||
        normal_format == VK_FORMAT_UNDEFINED || depth_format == VK_FORMAT_UNDEFINED)
    {
        error = "G-buffer requires color and depth formats and vkCreateRenderPass";
        return false;
    }
    VkAttachmentDescription attachments[3]{};
    for (unsigned i = 0; i < 3; ++i)
    {
        attachments[i].format = i == 0 ? albedo_format : i == 1 ? normal_format : depth_format;
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // Keep depth alongside albedo and normals so later deferred passes can
        // reconstruct positions and test depth without rerendering geometry.
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout = i < 2 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    const VkAttachmentReference color[]{{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    const VkAttachmentReference depth{2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 2;
    subpass.pColorAttachments = color;
    subpass.pDepthStencilAttachment = &depth;
    const VkSubpassDependency dependencies[]{
        {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT},
        {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT}
    };
    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = 3;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 2;
    info.pDependencies = dependencies;
    if (vk.create_render_pass(device, &info, nullptr, &result) != VK_SUCCESS)
    {
        error = "could not create Vulkan G-buffer render pass";
        return false;
    }
    error.clear();
    return true;
}

bool DeferredPass::initialize(VkDevice device, VkRenderPass geometry_pass, VkRenderPass light_pass,
    VkShaderModule geometry_vertex, VkShaderModule geometry_fragment,
    VkShaderModule alpha_test_fragment,
    VkShaderModule light_vertex, VkShaderModule light_fragment,
    const ScenePassDispatch& dispatch, std::string& error)
{
    destroy();
    if (!device || !geometry_pass || !light_pass || !geometry_vertex || !geometry_fragment ||
        !alpha_test_fragment ||
        !light_vertex || !light_fragment || !dispatch.create_pipeline_layout ||
        !dispatch.destroy_pipeline_layout || !dispatch.create_graphics_pipelines ||
        !dispatch.destroy_pipeline || !dispatch.create_descriptor_set_layout ||
        !dispatch.destroy_descriptor_set_layout || !dispatch.create_descriptor_pool ||
        !dispatch.destroy_descriptor_pool || !dispatch.allocate_descriptor_sets ||
        !dispatch.update_descriptor_sets || !dispatch.cmd_bind_pipeline ||
        !dispatch.cmd_set_viewport || !dispatch.cmd_set_scissor ||
        !dispatch.cmd_bind_vertex_buffers || !dispatch.cmd_bind_index_buffer ||
        !dispatch.cmd_bind_descriptor_sets || !dispatch.cmd_push_constants ||
        !dispatch.cmd_draw_indexed || !dispatch.cmd_draw)
    {
        error = "deferred pass requires Vulkan shaders and procedures";
        return false;
    }
    device_ = device;
    geometry_pass_ = geometry_pass;
    light_pass_ = light_pass;
    vk_ = dispatch;
    const VkDescriptorSetLayoutBinding bindings[]{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo descriptor{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor.bindingCount = 1;
    descriptor.pBindings = bindings;
    if (vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &material_layout_) != VK_SUCCESS)
        goto failed;
    descriptor.bindingCount = 2;
    if (vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &gbuffer_layout_) != VK_SUCCESS)
        goto failed;
    {
        const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool.maxSets = 256;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &size;
        if (vk_.create_descriptor_pool(device, &pool, nullptr, &pool_) != VK_SUCCESS) goto failed;
    }
    {
        const VkPushConstantRange camera{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16};
        const VkPushConstantRange light{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DeferredLight)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount = layout.pushConstantRangeCount = 1;
        layout.pSetLayouts = &material_layout_;
        layout.pPushConstantRanges = &camera;
        if (vk_.create_pipeline_layout(device, &layout, nullptr, &geometry_layout_) != VK_SUCCESS)
            goto failed;
        layout.pSetLayouts = &gbuffer_layout_;
        layout.pPushConstantRanges = &light;
        if (vk_.create_pipeline_layout(device, &layout, nullptr, &light_layout_) != VK_SUCCESS)
            goto failed;
    }
    if (!make_pipeline(device, geometry_pass, geometry_layout_, geometry_vertex,
            geometry_fragment, true, true, false, false, vk_, geometry_) ||
        !make_pipeline(device, geometry_pass, geometry_layout_, geometry_vertex,
            alpha_test_fragment, true, true, false, false, vk_, alpha_test_) ||
        !make_pipeline(device, geometry_pass, geometry_layout_, geometry_vertex,
            geometry_fragment, true, true, true, false, vk_, transparent_) ||
        !make_pipeline(device, light_pass, geometry_layout_, geometry_vertex,
            geometry_fragment, true, false, false, true, vk_, hud_) ||
        !make_pipeline(device, light_pass, light_layout_, light_vertex,
            light_fragment, false, false, false, false, vk_, lighting_)) goto failed;
    error.clear();
    return true;
failed:
    error = "could not initialize Vulkan deferred pipeline";
    destroy();
    return false;
}

bool DeferredPass::allocate(VkDescriptorSetLayout layout, VkImageView first, VkImageView second,
    VkSampler sampler, VkDescriptorSet& set, std::string& error)
{
    set = VK_NULL_HANDLE;
    if (!pool_ || !first || !sampler || (layout == gbuffer_layout_ && !second))
    {
        error = "missing deferred texture or descriptor pool";
        return false;
    }
    VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorPool = pool_;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;
    if (vk_.allocate_descriptor_sets(device_, &info, &set) != VK_SUCCESS)
    {
        error = "deferred texture descriptor pool is exhausted";
        return false;
    }
    const VkDescriptorImageInfo images[]{
        {sampler, first, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {sampler, second, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };
    VkWriteDescriptorSet writes[2]{};
    for (uint32_t i = 0; i < (second ? 2u : 1u); ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &images[i];
    }
    vk_.update_descriptor_sets(device_, second ? 2 : 1, writes, 0, nullptr);
    error.clear();
    return true;
}

bool DeferredPass::material(VkImageView albedo, VkSampler sampler, VkDescriptorSet& set, std::string& error)
{
    return allocate(material_layout_, albedo, VK_NULL_HANDLE, sampler, set, error);
}

bool DeferredPass::gbuffer(VkImageView albedo, VkImageView normal, VkSampler sampler,
    VkDescriptorSet& set, std::string& error)
{
    return allocate(gbuffer_layout_, albedo, normal, sampler, set, error);
}

void DeferredPass::release_gbuffer(VkDescriptorSet& set)
{
    if (device_ && pool_ && set && vk_.free_descriptor_sets)
        vk_.free_descriptor_sets(device_, pool_, 1, &set);
    set = VK_NULL_HANDLE;
}

bool DeferredPass::record_geometry(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    SurfaceMode mode, uint32_t first_index) const
{
    const VkPipeline pipeline = mode == SurfaceMode::AlphaTest ? alpha_test_ :
        mode == SurfaceMode::Transparent ? transparent_ : geometry_;
    if (!pipeline || frame.render_pass != geometry_pass_ || !frame.command_buffer ||
        !frame.extent.width || !frame.extent.height || !vertices || !indices ||
        !index_count || !material_set) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        geometry_layout_, 0, 1, &material_set, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    return true;
}

bool DeferredPass::record_hud(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    uint32_t first_index) const
{
    if (!hud_ || frame.render_pass != light_pass_ || !frame.command_buffer ||
        !frame.extent.width || !frame.extent.height || !vertices || !indices ||
        !index_count || !material_set) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hud_);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        geometry_layout_, 0, 1, &material_set, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    return true;
}

bool DeferredPass::record_lighting(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
    const DeferredLight& light) const
{
    if (!lighting_ || frame.render_pass != light_pass_ || !frame.command_buffer ||
        !frame.extent.width || !frame.extent.height || !gbuffer_set || !vk_.cmd_draw)
        return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_);
    viewport_scissor(frame, vk_);
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        light_layout_, 0, 1, &gbuffer_set, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, light_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(light), &light);
    vk_.cmd_draw(frame.command_buffer, 3, 1, 0, 0);
    return true;
}

void DeferredPass::destroy()
{
    if (device_ && vk_.destroy_pipeline)
    {
        if (geometry_) vk_.destroy_pipeline(device_, geometry_, nullptr);
        if (alpha_test_) vk_.destroy_pipeline(device_, alpha_test_, nullptr);
        if (transparent_) vk_.destroy_pipeline(device_, transparent_, nullptr);
        if (hud_) vk_.destroy_pipeline(device_, hud_, nullptr);
        if (lighting_) vk_.destroy_pipeline(device_, lighting_, nullptr);
    }
    if (device_ && vk_.destroy_pipeline_layout)
    {
        if (geometry_layout_) vk_.destroy_pipeline_layout(device_, geometry_layout_, nullptr);
        if (light_layout_) vk_.destroy_pipeline_layout(device_, light_layout_, nullptr);
    }
    if (device_ && vk_.destroy_descriptor_pool && pool_)
        vk_.destroy_descriptor_pool(device_, pool_, nullptr);
    if (device_ && vk_.destroy_descriptor_set_layout)
    {
        if (material_layout_) vk_.destroy_descriptor_set_layout(device_, material_layout_, nullptr);
        if (gbuffer_layout_) vk_.destroy_descriptor_set_layout(device_, gbuffer_layout_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    geometry_pass_ = light_pass_ = VK_NULL_HANDLE;
    geometry_ = alpha_test_ = transparent_ = hud_ = lighting_ = VK_NULL_HANDLE;
    geometry_layout_ = light_layout_ = VK_NULL_HANDLE;
    material_layout_ = gbuffer_layout_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    vk_ = {};
}
}
