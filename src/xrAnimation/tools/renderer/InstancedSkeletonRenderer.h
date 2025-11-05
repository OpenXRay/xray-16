#pragma once

#include "VulkanBuffer.h"
#include "VulkanPipeline.h"

#include <ozz/base/span.h>

#include <vector>
#include <vulkan/vulkan.h>

namespace ozz {
namespace math {
struct Float3;
struct Float4x4;
} // namespace math
} // namespace ozz

namespace xray {
namespace animation {
namespace renderer {

class VulkanDevice;

struct SkeletonLinePoint {
    float x;
    float y;
    float z;
};

struct SkeletonInstanceData {
    float transform[16];
};

// Renders line-based skeletons using instancing.
class InstancedSkeletonRenderer {
public:
    InstancedSkeletonRenderer() = default;
    ~InstancedSkeletonRenderer() = default;

    bool Initialize(VulkanDevice* device);
    void Shutdown();

    bool SetSkeletonLines(ozz::span<const SkeletonLinePoint> points);
    bool SetInstanceTransforms(ozz::span<const ozz::math::Float4x4> transforms);

    void Render(VkCommandBuffer cmd, const ozz::math::Float4x4& view_proj);
    bool IsInitialized() const { return initialized_; }
    const std::vector<ozz::math::Float4x4>& GetInstanceTransforms() const { return instance_transforms_; }

private:
    bool CreateDescriptorSetLayout();
    bool CreatePipeline();
    bool CreateBuffers();
    bool CreateDescriptorPool();

    void UpdateUniforms(const ozz::math::Float4x4& view_proj);
    void UpdateInstanceBuffer();

    VulkanDevice* device_ = nullptr;

    VulkanBuffer vertex_buffer_;
    VulkanBuffer instance_buffer_;
    VulkanBuffer uniform_buffer_;

    VulkanPipeline pipeline_;

    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

    std::vector<SkeletonInstanceData> instance_data_;
    std::vector<ozz::math::Float4x4> instance_transforms_;

    uint32_t vertex_count_ = 0;
    uint32_t instance_count_ = 0;
    uint32_t max_instances_ = 1;
    bool initialized_ = false;
    bool instance_buffer_dirty_ = true;
};

} // namespace renderer
} // namespace animation
} // namespace xray
