#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace xray {
namespace animation {
namespace renderer {

class VulkanDevice;

// Configuration for creating a graphics pipeline
struct PipelineConfig {
    // Shader stages
    std::string vertex_shader_path;
    std::string fragment_shader_path;

    // Vertex input
    std::vector<VkVertexInputBindingDescription> vertex_bindings;
    std::vector<VkVertexInputAttributeDescription> vertex_attributes;

    // Rasterization
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
    float line_width = 1.0f;

    // Depth/stencil
    bool depth_test_enable = true;
    bool depth_write_enable = true;
    VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

    // Blending
    bool blend_enable = false;

    // Dynamic rendering formats
    VkFormat color_format = VK_FORMAT_B8G8R8A8_SRGB;

    // Descriptor set layouts
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

    // Push descriptor support
    bool use_push_descriptors = false;

    // Push constants
    std::vector<VkPushConstantRange> push_constant_ranges;

    // Specialization constants
    std::vector<VkSpecializationMapEntry> specialization_entries;
    std::vector<uint8_t> specialization_data;
};

// Vulkan graphics pipeline wrapper
class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    bool Create(VkDevice device, const PipelineConfig& config);
    void Destroy();

    void Bind(VkCommandBuffer cmd_buffer);

    VkPipeline GetPipeline() const { return pipeline_; }
    VkPipelineLayout GetLayout() const { return pipeline_layout_; }

private:
    // Helper functions
    VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);
    std::vector<char> ReadShaderFile(const std::string& filename);

    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

} // namespace renderer
} // namespace animation
} // namespace xray
