#include "DeferredPass.h"
#include "ModelGeometry.h"
#if defined(XR_PLATFORM_ANDROID)
#include "xrEngine/x_ray.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
bool make_pipeline(VkDevice device, VkRenderPass pass, VkPipelineLayout layout,
    VkShaderModule vertex, VkShaderModule fragment, bool geometry_input, bool gbuffer,
    bool transparent, bool hud, bool skinned,
    const ScenePassDispatch& vk, VkPipeline& pipeline, std::string& error,
    const char* label, bool shadow = false, bool additive = false,
    bool alpha_additive = false, bool multiply = false, bool multiply_2x = false,
    bool particle_set = false, bool water_depth_write = false, bool wallmark = false)
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
        static_cast<uint32_t>(skinned ? sizeof(ModelVertex) : sizeof(LevelVertex)), VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription attributes[]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LevelVertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(LevelVertex, uv)},
        {3, 0, VK_FORMAT_R16G16B16A16_UINT, offsetof(ModelVertex, bones)},
        {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ModelVertex, weights)},
        {5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(LevelVertex, lightmap_uv)},
        {6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(LevelVertex, baked)}
    };
    const VkVertexInputAttributeDescription static_attributes[]{attributes[0], attributes[1], attributes[2], attributes[5], attributes[6]};
    VkPipelineVertexInputStateCreateInfo inputs{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    inputs.vertexBindingDescriptionCount = geometry_input ? 1 : 0;
    inputs.pVertexBindingDescriptions = geometry_input ? &binding : nullptr;
    inputs.vertexAttributeDescriptionCount = geometry_input ? 5 : 0;
    inputs.pVertexAttributeDescriptions = geometry_input ? (skinned ? attributes : static_attributes) : nullptr;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    // Depth precision on shallow terrain slopes otherwise produces long,
    // repeating self-shadow bands. Apply bias while writing shadow depth,
    // in addition to the small receiver bias used by the lighting shader.
    raster.depthBiasEnable = shadow || wallmark ? VK_TRUE : VK_FALSE;
    raster.depthBiasConstantFactor = shadow ? 1.25f : wallmark ? -1.f : 0.f;
    raster.depthBiasSlopeFactor = shadow ? 1.5f : wallmark ? -1.f : 0.f;
    VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    // The present pass has its own cleared depth attachment. HUD geometry
    // draws after world lighting, so depth testing here orders HUD surfaces
    // against one another without occluding the HUD with world geometry.
    depth.depthTestEnable = (gbuffer || hud || transparent || shadow) &&
        (!additive && !alpha_additive || transparent || hud) ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = ((gbuffer && !transparent) || hud || shadow || particle_set ||
        water_depth_write) ? VK_TRUE : VK_FALSE;
    // Decal polygons lie on top of the level surface. Strict LESS rejects
    // coplanar pixels and makes papers/wallmarks blink as the view moves.
    depth.depthCompareOp = wallmark ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS;
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
        if (additive)
        {
            attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        }
        if (alpha_additive)
        {
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        }
        if (multiply || multiply_2x)
        {
            attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            attachment.dstColorBlendFactor = multiply_2x ? VK_BLEND_FACTOR_SRC_COLOR : VK_BLEND_FACTOR_ZERO;
        }
        if (particle_set) attachment.blendEnable = VK_FALSE;
    }
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = shadow ? 0 : gbuffer ? 2 : 1;
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
#if defined(XR_PLATFORM_ANDROID)
    char context[160];
    std::snprintf(context, sizeof(context), "vulkan pipeline: %s", label ? label : "unknown");
    android_set_load_context(context);
#endif
    const VkResult result = vk.create_graphics_pipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
#if defined(XR_PLATFORM_ANDROID)
    android_set_load_context("vulkan pipeline: created");
#endif
    if (result != VK_SUCCESS)
    {
        error = std::string("vkCreateGraphicsPipelines(") + label + ") failed: VkResult=" +
            std::to_string(static_cast<int>(result));
        return false;
    }
    return true;
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
    // The static hemisphere channel already records visibility. Keep its
    // weather intensity separate from ambient so indoor lightmaps can supply
    // the actual indirect contribution instead of multiplying a small ambient.
    const float ambient = luminance(environment.ambient_color);
    return {{direction[0], direction[1], direction[2], ambient},
        {safe_nonnegative(environment.sun_color[0]),
            safe_nonnegative(environment.sun_color[1]),
            safe_nonnegative(environment.sun_color[2]), 0.f},
        {1.f, 1.f, 1.f, luminance(environment.hemi_color)}};
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
    VkShaderModule alpha_test_fragment, VkShaderModule transparent_fragment,
    VkShaderModule light_vertex, VkShaderModule light_fragment,
    VkShaderModule weather_fragment,
    const ScenePassDispatch& dispatch, std::string& error,
    VkRenderPass shadow_pass, VkShaderModule shadow_vertex,
    VkShaderModule shadow_opaque, VkShaderModule shadow_cutout,
    VkRenderPass local_shadow_pass, VkShaderModule local_light_fragment,
    VkShaderModule water_fragment)
{
    destroy();
    if (!device || !geometry_pass || !light_pass || !geometry_vertex || !geometry_fragment ||
        !alpha_test_fragment || !transparent_fragment ||
        !light_vertex || !light_fragment || !weather_fragment || !dispatch.create_pipeline_layout ||
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
    error.clear();
    device_ = device;
    geometry_pass_ = geometry_pass;
    light_pass_ = light_pass;
    shadow_pass_ = shadow_pass;
    local_shadow_pass_ = local_shadow_pass;
    vk_ = dispatch;
#define VK_STEP(label, expression) do { \
    const VkResult step_result = (expression); \
    if (step_result != VK_SUCCESS) { \
        error = std::string(label) + " failed: VkResult=" + std::to_string(static_cast<int>(step_result)); \
        goto failed; \
    } \
} while (false)
    const VkDescriptorSetLayoutBinding bindings[]{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    const VkDescriptorSetLayoutBinding gbuffer_bindings[]{bindings[0], bindings[1], bindings[2], bindings[3],
        {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
    const VkDescriptorSetLayoutBinding pose_binding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    const VkDescriptorSetLayoutBinding forward_binding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutBinding local_bindings[]{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
    const VkDescriptorSetLayoutBinding water_bindings[]{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
    VkDescriptorSetLayoutCreateInfo descriptor{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor.bindingCount = 2;
    descriptor.pBindings = bindings;
    VK_STEP("vkCreateDescriptorSetLayout(material)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &material_layout_));
    descriptor.bindingCount = 1;
    descriptor.pBindings = &pose_binding;
    VK_STEP("vkCreateDescriptorSetLayout(pose)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &pose_layout_));
    descriptor.pBindings = gbuffer_bindings;
    descriptor.bindingCount = 5;
    VK_STEP("vkCreateDescriptorSetLayout(gbuffer)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &gbuffer_layout_));
    descriptor.pBindings = bindings;
    descriptor.bindingCount = 4;
    VK_STEP("vkCreateDescriptorSetLayout(weather)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &weather_set_layout_));
    descriptor.pBindings = local_bindings;
    descriptor.bindingCount = 2;
    VK_STEP("vkCreateDescriptorSetLayout(local light)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &local_set_layout_));
    descriptor.pBindings = water_bindings;
    descriptor.bindingCount = 4;
    VK_STEP("vkCreateDescriptorSetLayout(water)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &water_set_layout_));
    descriptor.pBindings = &forward_binding;
    descriptor.bindingCount = 1;
    VK_STEP("vkCreateDescriptorSetLayout(forward)", vk_.create_descriptor_set_layout(device, &descriptor, nullptr, &forward_layout_));
    {
        const VkDescriptorPoolSize sizes[]{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32768},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4096},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096}
        };
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool.maxSets = 8192;
        pool.poolSizeCount = 3;
        pool.pPoolSizes = sizes;
        VK_STEP("vkCreateDescriptorPool(deferred)", vk_.create_descriptor_pool(device, &pool, nullptr, &pool_));
    }
    {
        // Cutout fragment shaders read a per-material alpha threshold after
        // the camera matrix. Vulkan guarantees at least 128 push-constant bytes.
        const VkPushConstantRange camera{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(float) * 32};
        const VkPushConstantRange light{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DeferredLight)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.pushConstantRangeCount = 1;
        const VkDescriptorSetLayout forward_sets[]{material_layout_, forward_layout_};
        layout.setLayoutCount = 2;
        layout.pSetLayouts = forward_sets;
        layout.pPushConstantRanges = &camera;
        VK_STEP("vkCreatePipelineLayout(geometry)", vk_.create_pipeline_layout(device, &layout, nullptr, &geometry_layout_));
        const VkDescriptorSetLayout skinned_sets[]{material_layout_, pose_layout_, forward_layout_};
        layout.setLayoutCount = 3;
        layout.pSetLayouts = skinned_sets;
        VK_STEP("vkCreatePipelineLayout(skinned)", vk_.create_pipeline_layout(device, &layout, nullptr, &skinned_layout_));
        layout.setLayoutCount = 1;
        layout.pSetLayouts = &gbuffer_layout_;
        layout.pPushConstantRanges = &light;
        VK_STEP("vkCreatePipelineLayout(light)", vk_.create_pipeline_layout(device, &layout, nullptr, &light_layout_));
        const VkDescriptorSetLayout weather_sets[]{gbuffer_layout_, weather_set_layout_};
        const VkPushConstantRange weather_range{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(WeatherLighting)};
        layout.setLayoutCount = 2;
        layout.pSetLayouts = weather_sets;
        layout.pPushConstantRanges = &weather_range;
        VK_STEP("vkCreatePipelineLayout(weather)", vk_.create_pipeline_layout(device, &layout, nullptr, &weather_layout_));
        const VkDescriptorSetLayout local_sets[]{gbuffer_layout_, local_set_layout_};
        layout.pSetLayouts = local_sets;
        layout.pushConstantRangeCount = 0;
        layout.pPushConstantRanges = nullptr;
        VK_STEP("vkCreatePipelineLayout(local light)", vk_.create_pipeline_layout(device, &layout, nullptr, &local_light_layout_));
        const VkDescriptorSetLayout water_sets[]{material_layout_, water_set_layout_};
        const VkPushConstantRange water_ranges[]{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16},
            {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 16, sizeof(float) * 4}};
        layout.pSetLayouts = water_sets;
        layout.pushConstantRangeCount = 2;
        layout.pPushConstantRanges = water_ranges;
        VK_STEP("vkCreatePipelineLayout(water)", vk_.create_pipeline_layout(device, &layout, nullptr, &water_layout_));
    }
    if (!make_pipeline(device, geometry_pass, geometry_layout_, geometry_vertex,
            geometry_fragment, true, true, false, false, false, vk_, geometry_, error, "geometry") ||
        !make_pipeline(device, geometry_pass, geometry_layout_, geometry_vertex,
            alpha_test_fragment, true, true, false, false, false, vk_, alpha_test_, error, "alpha test") ||
        !make_pipeline(device, light_pass, geometry_layout_, geometry_vertex,
            transparent_fragment, true, false, true, false, false, vk_, transparent_, error, "transparent") ||
        !make_pipeline(device, light_pass, geometry_layout_, geometry_vertex,
            transparent_fragment, true, false, false, true, false, vk_, hud_, error, "HUD") ||
        !make_pipeline(device, light_pass, light_layout_, light_vertex,
            light_fragment, false, false, false, false, false, vk_, lighting_, error, "lighting") ||
        !make_pipeline(device, light_pass, weather_layout_, light_vertex,
            weather_fragment, false, false, false, false, false, vk_, weather_pipeline_, error, "weather")) goto failed;
    if (shadow_pass && (!shadow_vertex || !shadow_opaque || !shadow_cutout ||
        !make_pipeline(device, shadow_pass, geometry_layout_, shadow_vertex,
            shadow_opaque, true, false, false, false, false, vk_, shadow_opaque_, error, "shadow opaque", true) ||
        !make_pipeline(device, shadow_pass, geometry_layout_, shadow_vertex,
            shadow_cutout, true, false, false, false, false, vk_, shadow_cutout_, error, "shadow cutout", true))) goto failed;
    if (local_shadow_pass && (!shadow_pass || !local_light_fragment ||
        !make_pipeline(device, light_pass, local_light_layout_, light_vertex,
            local_light_fragment, false, false, false, false, false, vk_, local_light_pipeline_,
            error, "local light", false, true))) goto failed;
    if (water_fragment && !make_pipeline(device, light_pass, water_layout_, geometry_vertex,
        water_fragment, true, false, true, false, false, vk_, water_pipeline_, error, "water",
        false, false, false, false, false, false, true)) goto failed;
    error.clear();
    return true;
failed:
    if (error.empty()) error = "missing shader or render pass for Vulkan deferred pipeline";
    destroy();
    return false;
#undef VK_STEP
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
    const VkImageLayout image_layout = layout == material_layout_ ?
        VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkDescriptorImageInfo images[]{
        {sampler, first, image_layout},
        {sampler, second, image_layout}
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

bool DeferredPass::lightmapped_material(VkImageView albedo, VkImageView lightmap,
    VkSampler sampler, VkDescriptorSet& set, std::string& error)
{
    if (!albedo || !lightmap)
    {
        error = "lightmapped material needs both textures";
        return false;
    }
    return allocate(material_layout_, albedo, lightmap, sampler, set, error);
}

const DeferredPass::GamePipeline* DeferredPass::game_pipeline(const char* vertex_name, const char* fragment_name) const
{
    if (!vertex_name || !fragment_name || !*vertex_name || !*fragment_name) return nullptr;
    std::string key(vertex_name);
    key.push_back('\0');
    key += fragment_name;
    const auto found = game_pipelines_.find(key);
    return found == game_pipelines_.end() ? nullptr : &found->second;
}

bool DeferredPass::has_game_pipeline(const std::string& vertex_name, const std::string& fragment_name) const
{
    return game_pipeline(vertex_name.c_str(), fragment_name.c_str()) != nullptr;
}

bool DeferredPass::require_game_pipeline(const std::string& vertex_name, const std::string& fragment_name,
    SurfaceMode mode, bool hud, bool skinned, std::string& error) const
{
    const auto* pair = game_pipeline(vertex_name.c_str(), fragment_name.c_str());
    if (!pair)
        error = "unknown Vulkan SVS/SPS pair: " + vertex_name + " / " + fragment_name;
    else if (pair->mode != mode || pair->hud != hud || pair->skinned != skinned)
        error = "incompatible Vulkan SVS/SPS pass parameters: " + vertex_name + " / " + fragment_name;
    else
    {
        error.clear();
        return true;
    }
    return false;
}

void DeferredPass::abort_game_pipeline_reload()
{
    if (device_ && vk_.destroy_pipeline)
        for (const auto& [name, pipeline] : pending_game_pipelines_)
            if (pipeline.handle) vk_.destroy_pipeline(device_, pipeline.handle, nullptr);
    pending_game_pipelines_.clear();
    reloading_game_pipelines_ = false;
}

void DeferredPass::begin_game_pipeline_reload()
{
    abort_game_pipeline_reload();
    reloading_game_pipelines_ = true;
}

void DeferredPass::commit_game_pipeline_reload()
{
    if (!reloading_game_pipelines_) return;
    if (device_ && vk_.destroy_pipeline)
        for (const auto& [name, pipeline] : game_pipelines_)
            if (pipeline.handle) vk_.destroy_pipeline(device_, pipeline.handle, nullptr);
    game_pipelines_ = std::move(pending_game_pipelines_);
    pending_game_pipelines_.clear();
    reloading_game_pipelines_ = false;
}

bool DeferredPass::create_game_pipeline(const std::string& vertex_name, const std::string& fragment_name,
    VkShaderModule vertex, VkShaderModule fragment, SurfaceMode mode, bool hud, std::string& error,
    bool skinned)
{
    const auto has_suffix = [](const std::string& name, const char* suffix)
    {
        const size_t length = std::strlen(suffix);
        return name.size() > length && name.compare(name.size() - length, length, suffix) == 0;
    };
    if (!has_suffix(vertex_name, ".vs") || !has_suffix(fragment_name, ".ps"))
    {
        error = "invalid Vulkan SVS/SPS shader stages: " + vertex_name + " / " + fragment_name;
        return false;
    }
    if (!device_ || vertex_name.empty() || fragment_name.empty() || !vertex || !fragment ||
        (hud && mode == SurfaceMode::AlphaTest))
    {
        error = "invalid Vulkan game shader pair or pass state: " + vertex_name + " / " + fragment_name;
        return false;
    }
    std::string key = vertex_name;
    key.push_back('\0');
    key += fragment_name;
    auto& registry = reloading_game_pipelines_ ? pending_game_pipelines_ : game_pipelines_;
    const auto existing = registry.find(key);
    if (existing != registry.end())
    {
        if (existing->second.mode != mode || existing->second.hud != hud || existing->second.skinned != skinned)
        {
            error = "Vulkan game shader pair requested with incompatible pass state: " + vertex_name + " / " + fragment_name;
            return false;
        }
        error.clear();
        return true;
    }
    VkPipeline pipeline{};
    const bool transparent = mode == SurfaceMode::Transparent;
    const bool particle_additive = fragment_name.find("particle_") != std::string::npos &&
        fragment_name.find("_additive.ps") != std::string::npos;
    const bool particle_alpha_add = fragment_name.find("particle_") != std::string::npos &&
        fragment_name.find("_alpha_add.ps") != std::string::npos;
    const bool glow_alpha_add = fragment_name == "vk\\glow_alpha_add.ps";
    const bool particle_multiply = fragment_name.find("particle_") != std::string::npos &&
        fragment_name.find("_multiply.ps") != std::string::npos;
    const bool particle_multiply_2x = fragment_name.find("particle_") != std::string::npos &&
        fragment_name.find("_multiply_2x.ps") != std::string::npos;
    const bool wallmark_multiply_2x = fragment_name == "vk\\wallmark_multiply_2x.ps";
    const bool particle_set = fragment_name.find("particle_") != std::string::npos &&
        fragment_name.find("_set.ps") != std::string::npos;
    if (!make_pipeline(device_, transparent || hud ? light_pass_ : geometry_pass_,
            skinned ? skinned_layout_ : geometry_layout_, vertex, fragment, true,
            !transparent && !hud, transparent, hud, skinned, vk_, pipeline, error,
            "game shader pair", false, particle_additive, particle_alpha_add || glow_alpha_add,
            particle_multiply, particle_multiply_2x || wallmark_multiply_2x, particle_set,
            false, wallmark_multiply_2x))
    {
        error += ": " + vertex_name + " / " + fragment_name;
        return false;
    }
    registry.emplace(std::move(key), GamePipeline{pipeline, mode, hud, skinned});
    error.clear();
    return true;
}

bool DeferredPass::pose_descriptor(VkBuffer pose, VkDeviceSize bytes, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    if (!device_ || !pose_layout_ || !pool_ || !pose || !bytes)
    {
        error = "invalid Vulkan skeletal pose buffer";
        return false;
    }
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = pool_;
    allocate.descriptorSetCount = 1;
    allocate.pSetLayouts = &pose_layout_;
    if (vk_.allocate_descriptor_sets(device_, &allocate, &result) != VK_SUCCESS)
    {
        error = "Vulkan skeletal pose descriptor pool is exhausted";
        return false;
    }
    const VkDescriptorBufferInfo buffer{pose, 0, bytes};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = result;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer;
    vk_.update_descriptor_sets(device_, 1, &write, 0, nullptr);
    error.clear();
    return true;
}

void DeferredPass::release_pose_descriptor(VkDescriptorSet& set)
{
    if (device_ && pool_ && set && vk_.free_descriptor_sets)
        vk_.free_descriptor_sets(device_, pool_, 1, &set);
    set = VK_NULL_HANDLE;
}

bool DeferredPass::record_skinned(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
    uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    VkDescriptorSet pose_set, SurfaceMode mode, bool hud, uint32_t first_index,
    const char* vertex_name, const char* fragment_name, float alpha_ref,
    const float* normal_rows) const
{
    const auto* pair = game_pipeline(vertex_name, fragment_name);
    if (!pair || !pair->skinned || pair->mode != mode || pair->hud != hud ||
        (hud || mode == SurfaceMode::Transparent ? !lighting_compatible(frame) :
            frame.render_pass != geometry_pass_) ||
        !frame.command_buffer || !frame.extent.width || !frame.extent.height ||
        !vertices || !indices || !index_count || !material_set || !pose_set) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pair->handle);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    const VkDescriptorSet sets[]{material_set, pose_set};
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        skinned_layout_, 0, 2, sets, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, skinned_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    if (normal_rows)
        vk_.cmd_push_constants(frame.command_buffer, skinned_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            80, sizeof(float) * 12, normal_rows);
    if (mode == SurfaceMode::AlphaTest)
        vk_.cmd_push_constants(frame.command_buffer, skinned_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(mvp), sizeof(alpha_ref), &alpha_ref);
    if (mode == SurfaceMode::Transparent && !hud)
    {
        if (frame.image_index >= forward_sets_.size() || !forward_sets_[frame.image_index]) return false;
        vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            skinned_layout_, 2, 1, &forward_sets_[frame.image_index], 0, nullptr);
    }
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

void DeferredPass::update_material(VkDescriptorSet set, VkImageView view, VkSampler sampler)
{
    const VkDescriptorImageInfo image{sampler, view, VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vk_.update_descriptor_sets(device_, 1, &write, 0, nullptr);
}

void DeferredPass::update_lightmapped_material(VkDescriptorSet set, VkImageView albedo,
    VkImageView lightmap, VkSampler sampler)
{
    if (!device_ || !set || !albedo || !lightmap) return;
    const VkDescriptorImageInfo images[]{
        {sampler, albedo, VK_IMAGE_LAYOUT_GENERAL},
        {sampler, lightmap, VK_IMAGE_LAYOUT_GENERAL}
    };
    VkWriteDescriptorSet writes[2]{};
    for (uint32_t i = 0; i < 2; ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vk_.update_descriptor_sets(device_, 2, writes, 0, nullptr);
}

bool DeferredPass::gbuffer(VkImageView albedo, VkImageView normal, VkImageView depth, VkSampler sampler,
    VkDescriptorSet& set, std::string& error)
{
    if (!depth || !allocate(gbuffer_layout_, albedo, normal, sampler, set, error)) return false;
    const VkDescriptorImageInfo image{sampler, depth, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.dstBinding = 2;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &image;
    vk_.update_descriptor_sets(device_, 1, &write, 0, nullptr);
    return true;
}

void DeferredPass::bind_sun_shadow(VkDescriptorSet set, VkImageView view, VkSampler sampler,
    VkBuffer uniform)
{
    if (!set || !view || !sampler || !uniform) return;
    const VkDescriptorImageInfo image{sampler, view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    const VkDescriptorBufferInfo buffer{uniform, 0, sizeof(float) * 36};
    VkWriteDescriptorSet writes[2]{};
    for (auto& write : writes)
    {
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.descriptorCount = 1;
    }
    writes[0].dstBinding = 3;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &image;
    writes[1].dstBinding = 4;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].pBufferInfo = &buffer;
    vk_.update_descriptor_sets(device_, 2, writes, 0, nullptr);
}

bool DeferredPass::local_light_set(VkImageView shadow_array, VkSampler sampler,
    VkBuffer uniform, VkDeviceSize offset, VkDeviceSize range,
    VkDescriptorSet& set, std::string& error)
{
    set = VK_NULL_HANDLE;
    if (!pool_ || !shadow_array || !sampler || !uniform || !range)
    { error = "local light needs shadow array and uniform buffer"; return false; }
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool = pool_;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &local_set_layout_;
    if (vk_.allocate_descriptor_sets(device_, &allocation, &set) != VK_SUCCESS)
    { error = "local light descriptor pool exhausted"; return false; }
    const VkDescriptorImageInfo image{sampler, shadow_array,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    const VkDescriptorBufferInfo buffer{uniform, offset, range};
    VkWriteDescriptorSet writes[2]{};
    for (auto& write : writes)
    {
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.descriptorCount = 1;
    }
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &image;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].pBufferInfo = &buffer;
    vk_.update_descriptor_sets(device_, 2, writes, 0, nullptr);
    error.clear();
    return true;
}

bool DeferredPass::water_set(uint32_t image_index, VkImageView refraction,
    VkImageView reflection, VkImageView depth, VkSampler sampler, VkSampler depth_sampler,
    VkBuffer scene_uniform, std::string& error)
{
    if (!pool_ || !refraction || !reflection || !depth || !sampler || !depth_sampler ||
        !scene_uniform || image_index >= 16)
    { error = "water needs reflection, refraction and opaque depth"; return false; }
    if (water_sets_.size() <= image_index) water_sets_.resize(image_index + 1);
    if (water_sets_[image_index])
        release_gbuffer(water_sets_[image_index]);
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool = pool_;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &water_set_layout_;
    VkDescriptorSet& set = water_sets_[image_index];
    if (vk_.allocate_descriptor_sets(device_, &allocation, &set) != VK_SUCCESS)
    { error = "water descriptor pool exhausted"; return false; }
    const VkImageView views[]{refraction, reflection, depth};
    VkDescriptorImageInfo images[3]{};
    VkWriteDescriptorSet writes[4]{};
    for (uint32_t i = 0; i < 3; ++i)
    {
        images[i] = {i == 2 ? depth_sampler : sampler, views[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    const VkDescriptorBufferInfo buffer{scene_uniform, 0, sizeof(float) * 36};
    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = set;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[3].pBufferInfo = &buffer;
    vk_.update_descriptor_sets(device_, 4, writes, 0, nullptr);
    error.clear();
    return true;
}

void DeferredPass::release_water_sets()
{
    for (auto& set : water_sets_) release_gbuffer(set);
    water_sets_.clear();
}

bool DeferredPass::forward_set(uint32_t image, VkBuffer buffer, std::string& error)
{
    if (!pool_ || !forward_layout_ || !buffer || image >= 16)
    { error = "invalid forward light uniform"; return false; }
    if (forward_sets_.size() <= image) forward_sets_.resize(image + 1);
    release_gbuffer(forward_sets_[image]);
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool = pool_;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &forward_layout_;
    VkDescriptorSet& set = forward_sets_[image];
    if (vk_.allocate_descriptor_sets(device_, &allocation, &set) != VK_SUCCESS)
    { error = "forward light descriptor pool exhausted"; return false; }
    const VkDescriptorBufferInfo info{buffer, 0, sizeof(ForwardLightUniform)};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &info;
    vk_.update_descriptor_sets(device_, 1, &write, 0, nullptr);
    error.clear();
    return true;
}

void DeferredPass::release_forward_sets()
{
    for (auto& set : forward_sets_) release_gbuffer(set);
    forward_sets_.clear();
}

bool DeferredPass::weather_set(VkImageView sky_a, VkImageView sky_b,
    VkImageView clouds_a, VkImageView clouds_b, VkSampler sampler,
    VkDescriptorSet& set, std::string& error)
{
    set = VK_NULL_HANDLE;
    if (!pool_ || !sampler || !sky_a || !sky_b || !clouds_a || !clouds_b)
    {
        error = "weather requires both sky cubemaps and cloud textures";
        return false;
    }
    VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorPool = pool_;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &weather_set_layout_;
    if (vk_.allocate_descriptor_sets(device_, &info, &set) != VK_SUCCESS)
    {
        error = "weather descriptor pool is exhausted";
        return false;
    }
    const VkImageView views[]{sky_a, sky_b, clouds_a, clouds_b};
    VkDescriptorImageInfo images[4]{};
    VkWriteDescriptorSet writes[4]{};
    for (uint32_t i = 0; i < 4; ++i)
    {
        images[i] = {sampler, views[i], VK_IMAGE_LAYOUT_GENERAL};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &images[i];
    }
    vk_.update_descriptor_sets(device_, 4, writes, 0, nullptr);
    return true;
}

void DeferredPass::release_gbuffer(VkDescriptorSet& set)
{
    if (device_ && pool_ && set && vk_.free_descriptor_sets)
        vk_.free_descriptor_sets(device_, pool_, 1, &set);
    set = VK_NULL_HANDLE;
}

bool DeferredPass::record_geometry(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    SurfaceMode mode, uint32_t first_index, const char* vertex_name, const char* fragment_name,
    float alpha_ref, const float* normal_rows) const
{
    if (mode == SurfaceMode::Transparent) return false;
    VkPipeline pipeline = mode == SurfaceMode::AlphaTest ? alpha_test_ : geometry_;
    if (vertex_name || fragment_name)
    {
        const auto* pair = game_pipeline(vertex_name, fragment_name);
        if (!pair || pair->mode != mode || pair->hud || pair->skinned) return false;
        pipeline = pair->handle;
    }
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
    if (normal_rows)
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            80, sizeof(float) * 12, normal_rows);
    if (mode == SurfaceMode::AlphaTest)
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(mvp), sizeof(alpha_ref), &alpha_ref);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

bool DeferredPass::record_sun_shadow(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material,
    bool alpha_test, uint32_t first_index, float alpha_ref) const
{
    const VkPipeline pipeline = alpha_test ? shadow_cutout_ : shadow_opaque_;
    if (!pipeline || !frame.command_buffer ||
        (frame.render_pass != shadow_pass_ && frame.render_pass != local_shadow_pass_) ||
        !vertices || !indices || !material || !index_count) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        geometry_layout_, 0, 1, &material, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    if (alpha_test)
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(mvp), sizeof(alpha_ref), &alpha_ref);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

bool DeferredPass::record_transparent(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    uint32_t first_index, const char* vertex_name, const char* fragment_name,
    const char** failure, float alpha_ref, const float* normal_rows) const
{
    const auto reject = [failure](const char* reason)
    {
        if (failure) *failure = reason;
        return false;
    };
    VkPipeline pipeline = transparent_;
    if (vertex_name || fragment_name)
    {
        const auto* pair = game_pipeline(vertex_name, fragment_name);
        if (!pair || pair->mode != SurfaceMode::Transparent || pair->hud || pair->skinned)
            return reject("incompatible shader pair");
        pipeline = pair->handle;
    }
    if (!pipeline) return reject("missing pipeline");
    if (!lighting_compatible(frame)) return reject("incompatible render pass");
    if (!frame.command_buffer) return reject("missing command buffer");
    if (!frame.extent.width || !frame.extent.height) return reject("empty frame extent");
    if (!vertices) return reject("missing vertex buffer");
    if (!indices) return reject("missing index buffer");
    if (!index_count) return reject("empty index window");
    if (!material_set) return reject("missing material descriptor");
    // A rejected draw must not leave half-recorded commands in the frame.
    // Check the forward set before binding the pipeline or issuing writes.
    if ((vertex_name || fragment_name) &&
        (frame.image_index >= forward_sets_.size() || !forward_sets_[frame.image_index]))
        return reject("missing forward lighting descriptor");
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        geometry_layout_, 0, 1, &material_set, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    if (normal_rows)
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            80, sizeof(float) * 12, normal_rows);
    if (fragment_name && std::strstr(fragment_name, "particle_") &&
        std::strstr(fragment_name, "_set.ps"))
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(mvp), sizeof(alpha_ref), &alpha_ref);
    if (vertex_name || fragment_name)
    {
        vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            geometry_layout_, 1, 1, &forward_sets_[frame.image_index], 0, nullptr);
    }
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

bool DeferredPass::record_hud(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
    uint32_t first_index, const char* vertex_name, const char* fragment_name, float alpha_ref,
    const float* normal_rows) const
{
    VkPipeline pipeline = hud_;
    if (vertex_name || fragment_name)
    {
        const auto* pair = game_pipeline(vertex_name, fragment_name);
        if (!pair || !pair->hud || pair->skinned) return false;
        pipeline = pair->handle;
    }
    if (!pipeline || !lighting_compatible(frame) || !frame.command_buffer ||
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
    if (normal_rows)
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            80, sizeof(float) * 12, normal_rows);
    if (fragment_name && std::strstr(fragment_name, "particle_") &&
        std::strstr(fragment_name, "_set.ps"))
        vk_.cmd_push_constants(frame.command_buffer, geometry_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(mvp), sizeof(alpha_ref), &alpha_ref);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

bool DeferredPass::record_lighting(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
    const DeferredLight& light, VkDescriptorSet weather_set,
    const WeatherLighting* weather) const
{
    if (!lighting_ || frame.render_pass != light_pass_ || !frame.command_buffer ||
        !frame.extent.width || !frame.extent.height || !gbuffer_set || !vk_.cmd_draw)
        return false;
    const bool has_weather = weather_set && weather && weather_pipeline_;
    const VkPipelineLayout layout = has_weather ? weather_layout_ : light_layout_;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        has_weather ? weather_pipeline_ : lighting_);
    viewport_scissor(frame, vk_);
    const VkDescriptorSet sets[]{gbuffer_set, weather_set};
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout, 0, has_weather ? 2 : 1, sets, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
        0, has_weather ? sizeof(WeatherLighting) : sizeof(light),
        has_weather ? static_cast<const void*>(weather) : static_cast<const void*>(&light));
    vk_.cmd_draw(frame.command_buffer, 3, 1, 0, 0);
    ++draw_calls_; ++triangles_;
    return true;
}

bool DeferredPass::record_local_light(const FrameRecordingContext& frame,
    VkDescriptorSet gbuffer_set, VkDescriptorSet local_set, const VkRect2D& scissor) const
{
    if (!local_light_pipeline_ || frame.render_pass != light_pass_ ||
        !frame.command_buffer || !gbuffer_set || !local_set) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, local_light_pipeline_);
    viewport_scissor(frame, vk_);
    vk_.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
    const VkDescriptorSet sets[]{gbuffer_set, local_set};
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        local_light_layout_, 0, 2, sets, 0, nullptr);
    vk_.cmd_draw(frame.command_buffer, 3, 1, 0, 0);
    ++draw_calls_; ++triangles_;
    return true;
}

bool DeferredPass::record_water(const FrameRecordingContext& frame, VkBuffer vertices,
    VkBuffer indices, uint32_t index_count, const float (&mvp)[16],
    VkDescriptorSet material, uint32_t first_index, float time, float opacity) const
{
    if (!water_pipeline_ || !lighting_compatible(frame) || !frame.command_buffer ||
        frame.image_index >= water_sets_.size() || !water_sets_[frame.image_index] ||
        !vertices || !indices || !index_count || !material ||
        !frame.extent.width || !frame.extent.height) return false;
    vk_.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, water_pipeline_);
    viewport_scissor(frame, vk_);
    const VkDeviceSize offset = 0;
    vk_.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
    vk_.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    const VkDescriptorSet sets[]{material, water_sets_[frame.image_index]};
    vk_.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        water_layout_, 0, 2, sets, 0, nullptr);
    vk_.cmd_push_constants(frame.command_buffer, water_layout_, VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mvp), mvp);
    const float params[]{time, 1.f / frame.extent.width, 1.f / frame.extent.height, opacity};
    vk_.cmd_push_constants(frame.command_buffer, water_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(mvp), sizeof(params), params);
    vk_.cmd_draw_indexed(frame.command_buffer, index_count, 1, first_index, 0, 0);
    ++draw_calls_; triangles_ += index_count / 3;
    return true;
}

void DeferredPass::destroy()
{
    game_pipeline_request_ = {};
    release_forward_sets();
    release_water_sets();
    abort_game_pipeline_reload();
    if (device_ && vk_.destroy_pipeline)
    {
        for (const auto& [name, pipeline] : game_pipelines_)
            if (pipeline.handle) vk_.destroy_pipeline(device_, pipeline.handle, nullptr);
        if (geometry_) vk_.destroy_pipeline(device_, geometry_, nullptr);
        if (alpha_test_) vk_.destroy_pipeline(device_, alpha_test_, nullptr);
        if (transparent_) vk_.destroy_pipeline(device_, transparent_, nullptr);
        if (hud_) vk_.destroy_pipeline(device_, hud_, nullptr);
        if (lighting_) vk_.destroy_pipeline(device_, lighting_, nullptr);
        if (weather_pipeline_) vk_.destroy_pipeline(device_, weather_pipeline_, nullptr);
        if (shadow_opaque_) vk_.destroy_pipeline(device_, shadow_opaque_, nullptr);
        if (shadow_cutout_) vk_.destroy_pipeline(device_, shadow_cutout_, nullptr);
        if (local_light_pipeline_) vk_.destroy_pipeline(device_, local_light_pipeline_, nullptr);
        if (water_pipeline_) vk_.destroy_pipeline(device_, water_pipeline_, nullptr);
    }
    game_pipelines_.clear();
    if (device_ && vk_.destroy_pipeline_layout)
    {
        if (geometry_layout_) vk_.destroy_pipeline_layout(device_, geometry_layout_, nullptr);
        if (skinned_layout_) vk_.destroy_pipeline_layout(device_, skinned_layout_, nullptr);
        if (light_layout_) vk_.destroy_pipeline_layout(device_, light_layout_, nullptr);
        if (weather_layout_) vk_.destroy_pipeline_layout(device_, weather_layout_, nullptr);
        if (local_light_layout_) vk_.destroy_pipeline_layout(device_, local_light_layout_, nullptr);
        if (water_layout_) vk_.destroy_pipeline_layout(device_, water_layout_, nullptr);
    }
    if (device_ && vk_.destroy_descriptor_pool && pool_)
        vk_.destroy_descriptor_pool(device_, pool_, nullptr);
    if (device_ && vk_.destroy_descriptor_set_layout)
    {
        if (material_layout_) vk_.destroy_descriptor_set_layout(device_, material_layout_, nullptr);
        if (pose_layout_) vk_.destroy_descriptor_set_layout(device_, pose_layout_, nullptr);
        if (gbuffer_layout_) vk_.destroy_descriptor_set_layout(device_, gbuffer_layout_, nullptr);
        if (weather_set_layout_) vk_.destroy_descriptor_set_layout(device_, weather_set_layout_, nullptr);
        if (local_set_layout_) vk_.destroy_descriptor_set_layout(device_, local_set_layout_, nullptr);
        if (water_set_layout_) vk_.destroy_descriptor_set_layout(device_, water_set_layout_, nullptr);
        if (forward_layout_) vk_.destroy_descriptor_set_layout(device_, forward_layout_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    geometry_pass_ = light_pass_ = shadow_pass_ = local_shadow_pass_ = overlay_pass_ = VK_NULL_HANDLE;
    geometry_ = alpha_test_ = transparent_ = hud_ = lighting_ = weather_pipeline_ = VK_NULL_HANDLE;
    shadow_opaque_ = shadow_cutout_ = VK_NULL_HANDLE;
    local_light_pipeline_ = VK_NULL_HANDLE;
    water_pipeline_ = VK_NULL_HANDLE;
    geometry_layout_ = skinned_layout_ = light_layout_ = weather_layout_ = VK_NULL_HANDLE;
    local_light_layout_ = VK_NULL_HANDLE;
    water_layout_ = VK_NULL_HANDLE;
    material_layout_ = pose_layout_ = gbuffer_layout_ = weather_set_layout_ = VK_NULL_HANDLE;
    local_set_layout_ = VK_NULL_HANDLE;
    water_set_layout_ = VK_NULL_HANDLE;
    forward_layout_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    vk_ = {};
}
}
