#include "stdafx.h"
#include "VulkanPipeline.h"
#include <fstream>
#include <cstdio>
#include <cstring>

#define Msg(...) printf(__VA_ARGS__), printf("\n")

namespace xray {
namespace animation {
namespace renderer {

VulkanPipeline::VulkanPipeline() {}

VulkanPipeline::~VulkanPipeline() {
    Destroy();
}

bool VulkanPipeline::Create(VkDevice device, const PipelineConfig& config) {
    device_ = device;

    // Read shader files
    auto vert_code = ReadShaderFile(config.vertex_shader_path);
    auto frag_code = ReadShaderFile(config.fragment_shader_path);

    if (vert_code.empty() || frag_code.empty()) {
        Msg("! Failed to read shader files");
        return false;
    }

    // Create shader modules
    VkShaderModule vert_module = CreateShaderModule(device, vert_code);
    VkShaderModule frag_module = CreateShaderModule(device, frag_code);

    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        Msg("! Failed to create shader modules");
        if (vert_module) vkDestroyShaderModule(device, vert_module, nullptr);
        if (frag_module) vkDestroyShaderModule(device, frag_module, nullptr);
        return false;
    }

    // Setup specialization info if provided
    VkSpecializationInfo spec_info = {};
    if (!config.specialization_entries.empty() && !config.specialization_data.empty()) {
        spec_info.mapEntryCount = static_cast<uint32_t>(config.specialization_entries.size());
        spec_info.pMapEntries = config.specialization_entries.data();
        spec_info.dataSize = config.specialization_data.size();
        spec_info.pData = config.specialization_data.data();
    }

    // Shader stage creation
    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";
    if (spec_info.mapEntryCount > 0) {
        frag_stage_info.pSpecializationInfo = &spec_info;
    }

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    // Vertex input state
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(config.vertex_bindings.size());
    vertex_input_info.pVertexBindingDescriptions = config.vertex_bindings.empty() ? nullptr : config.vertex_bindings.data();
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertex_attributes.size());
    vertex_input_info.pVertexAttributeDescriptions = config.vertex_attributes.empty() ? nullptr : config.vertex_attributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = config.topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic, will be set in command buffer)
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = config.polygon_mode;
    rasterizer.lineWidth = config.line_width;
    rasterizer.cullMode = config.cull_mode;
    rasterizer.frontFace = config.front_face;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth/stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = config.depth_test_enable ? VK_TRUE : VK_FALSE;
    depth_stencil.depthWriteEnable = config.depth_write_enable ? VK_TRUE : VK_FALSE;
    depth_stencil.depthCompareOp = config.depth_compare_op;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = config.blend_enable ? VK_TRUE : VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    // Dynamic state (viewport and scissor)
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    // Pipeline layout (with push constants if provided)
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(config.descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts = config.descriptor_set_layouts.empty() ? nullptr : config.descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(config.push_constant_ranges.size());
    pipeline_layout_info.pPushConstantRanges = config.push_constant_ranges.empty() ? nullptr : config.push_constant_ranges.data();

    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create pipeline layout");
        vkDestroyShaderModule(device, vert_module, nullptr);
        vkDestroyShaderModule(device, frag_module, nullptr);
        return false;
    }

    // Dynamic rendering info (Vulkan 1.3+)
    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &config.color_format;
    rendering_info.depthAttachmentFormat = config.depth_format;

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = &rendering_info;  // Chain dynamic rendering info
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = VK_NULL_HANDLE;  // No render pass needed with dynamic rendering
    pipeline_info.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);

    // Clean up shader modules (no longer needed after pipeline creation)
    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);

    if (result != VK_SUCCESS) {
        Msg("! Failed to create graphics pipeline (error: %d)", result);
        return false;
    }

    Msg("* Graphics pipeline created successfully");
    return true;
}

void VulkanPipeline::Destroy() {
    const VkDevice device = device_;
    if (device != VK_NULL_HANDLE) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline_, nullptr);
        }
        if (pipeline_layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
        }
    }

    pipeline_ = VK_NULL_HANDLE;
    pipeline_layout_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
}

void VulkanPipeline::Bind(VkCommandBuffer cmd_buffer) {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    }
}

VkShaderModule VulkanPipeline::CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create shader module (error: %d)", result);
        return VK_NULL_HANDLE;
    }

    return shader_module;
}

std::vector<char> VulkanPipeline::ReadShaderFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        Msg("! Failed to open shader file: %s", filename.c_str());
        return {};
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    Msg("* Loaded shader: %s (%zu bytes)", filename.c_str(), file_size);
    return buffer;
}

} // namespace renderer
} // namespace animation
} // namespace xray
