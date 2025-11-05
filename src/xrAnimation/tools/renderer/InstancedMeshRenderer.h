#pragma once

#include "VulkanBuffer.h"
#include "VulkanPipeline.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <ozz/base/maths/simd_math.h>

namespace ozz {
namespace math {
struct Float4x4;
} // namespace math
namespace sample {
struct Mesh;
} // namespace sample
} // namespace ozz

namespace xray {
namespace animation {
namespace renderer {

class VulkanDevice;

struct MeshInstanceData {
    ozz::math::Float4x4 transform;
    uint32_t bone_matrix_offset = 0;
};

// GPU skinned mesh renderer supporting instanced draws.
class InstancedMeshRenderer {
public:
    InstancedMeshRenderer() = default;
    ~InstancedMeshRenderer() = default;

    bool Initialize(VulkanDevice* device);
    void Shutdown();

    bool UploadMesh(const ozz::sample::Mesh& mesh);

    void Render(VkCommandBuffer cmd,
                const ozz::math::Float4x4& view_proj,
                const std::vector<MeshInstanceData>& instances,
                const std::vector<ozz::math::Float4x4>& bone_matrices);

    bool IsInitialized() const { return initialized_; }
    bool HasMesh() const { return mesh_uploaded_; }
    uint32_t BonesPerInstance() const { return bones_per_instance_; }
    void SetUseGpuSkinning(bool enabled) { use_gpu_skinning_ = enabled; }
    bool GetUseGpuSkinning() const { return use_gpu_skinning_; }

private:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];
        float bone_weights[4];
        uint32_t bone_indices[4];
    };

    struct InstanceGpuData {
        float transform[16];
        uint32_t bone_matrix_offset;
        float padding[3]; // Keep stride 16-byte aligned.
    };

    bool CreateDescriptorSetLayout();
    bool CreatePipeline();
    bool CreateDescriptorPool();
    bool CreateUniformBuffer();

    bool EnsureVertexBuffer(size_t vertex_count);
    bool EnsureIndexBuffer(size_t index_count);
    bool EnsureInstanceBuffer(size_t instance_count);
    bool EnsureBoneBuffer(size_t matrix_count);

    void UpdateUniforms(const ozz::math::Float4x4& view_proj);
    bool UpdateInstanceBufferData(const std::vector<MeshInstanceData>& instances);
    bool UpdateBoneBufferData(const std::vector<ozz::math::Float4x4>& bone_matrices);
    void UpdateDescriptorSet();

    VulkanDevice* device_ = nullptr;

    VulkanBuffer vertex_buffer_;
    VulkanBuffer index_buffer_;
    VulkanBuffer instance_buffer_;
    VulkanBuffer bone_matrix_buffer_;      // Model-space matrices (per-frame)
    VulkanBuffer inverse_bind_pose_buffer_; // Inverse bind poses (uploaded once)
    VulkanBuffer joint_remap_buffer_;       // Joint remaps (palette → skeleton, uploaded once)
    VulkanBuffer uniform_buffer_;

    VulkanPipeline pipeline_;

    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

    uint32_t vertex_count_ = 0;
    uint32_t index_count_ = 0;
    uint32_t instance_capacity_ = 0;
    uint32_t instance_count_ = 0;
    uint32_t bone_matrix_capacity_ = 0;
    uint32_t bones_per_instance_ = 0;
    bool initialized_ = false;
    bool mesh_uploaded_ = false;
    bool use_gpu_skinning_ = true;
};

} // namespace renderer
} // namespace animation
} // namespace xray
