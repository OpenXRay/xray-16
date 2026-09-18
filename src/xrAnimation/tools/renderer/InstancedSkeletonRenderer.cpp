#include "stdafx.h"
#include "InstancedSkeletonRenderer.h"

#include "VulkanDevice.h"

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/span.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

#define Msg(...) do { printf(__VA_ARGS__); printf("\n"); } while (0)

namespace xray {
namespace animation {
namespace renderer {

namespace {
struct Vertex {
    float position[3];
};

struct CameraUBO {
    float view_proj[16];
};

SkeletonInstanceData MakeIdentityInstance() {
    SkeletonInstanceData data{};
    for (int i = 0; i < 16; ++i) {
        data.transform[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    return data;
}
} // namespace

bool InstancedSkeletonRenderer::Initialize(VulkanDevice* device) {
    device_ = device;
    if (!device_) {
        Msg("! InstancedSkeletonRenderer::Initialize requires a valid device");
        return false;
    }

    if (!CreateDescriptorSetLayout()) return false;
    if (!CreatePipeline()) return false;
    if (!CreateBuffers()) return false;
    if (!CreateDescriptorPool()) return false;

    initialized_ = true;
    return true;
}

void InstancedSkeletonRenderer::Shutdown() {
    if (!device_) {
        vertex_buffer_.Destroy();
        instance_buffer_.Destroy();
        uniform_buffer_.Destroy();
        pipeline_.Destroy();
        descriptor_set_layout_ = VK_NULL_HANDLE;
        descriptor_pool_ = VK_NULL_HANDLE;
        descriptor_set_ = VK_NULL_HANDLE;
        instance_data_.clear();
        initialized_ = false;
        return;
    }

    VkDevice logical_device = device_->GetDevice();

    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(logical_device, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }

    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }

    pipeline_.Destroy();
    uniform_buffer_.Destroy();
    instance_buffer_.Destroy();
    vertex_buffer_.Destroy();

    descriptor_set_ = VK_NULL_HANDLE;
    instance_count_ = 0;
    initialized_ = false;
    device_ = nullptr;
    instance_buffer_dirty_ = true;
    instance_data_.clear();
}

bool InstancedSkeletonRenderer::SetSkeletonLines(ozz::span<const SkeletonLinePoint> points) {
    if (!device_) {
        return false;
    }

    std::vector<Vertex> vertices;
    if (points.empty()) {
        vertices.push_back(Vertex{{0.0f, 0.0f, 0.0f}});
        vertices.push_back(Vertex{{0.0f, 1.0f, 0.0f}});
    } else {
        vertices.reserve(points.size());
        for (const SkeletonLinePoint& p : points) {
            vertices.push_back(Vertex{{p.x, p.y, p.z}});
        }
    }

    const VkDeviceSize vertex_size = sizeof(Vertex) * vertices.size();
    if (vertex_size == 0) {
        return false;
    }

    vertex_buffer_.Destroy();
    vertex_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), vertex_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    vertex_buffer_.Upload(vertices.data(), vertex_size);
    vertex_count_ = static_cast<uint32_t>(vertices.size());
    return vertex_count_ > 0;
}

bool InstancedSkeletonRenderer::SetInstanceTransforms(ozz::span<const ozz::math::Float4x4> transforms) {
    if (!device_) {
        return false;
    }

    if (transforms.empty()) {
        instance_data_.assign(1, MakeIdentityInstance());
        instance_transforms_.assign(1, ozz::math::Float4x4::identity());
    } else {
        instance_data_.resize(transforms.size());
        instance_transforms_.assign(transforms.begin(), transforms.end());
        for (size_t i = 0; i < transforms.size(); ++i) {
            SkeletonInstanceData& dst = instance_data_[i];
            for (int col = 0; col < 4; ++col) {
                ozz::math::StorePtrU(transforms[i].cols[col], dst.transform + col * 4);
            }
        }
    }

    instance_count_ = static_cast<uint32_t>(instance_data_.size());
    const uint32_t required_capacity = std::max(instance_count_, 1u);
    if (instance_buffer_.GetBuffer() == VK_NULL_HANDLE || required_capacity > max_instances_) {
        max_instances_ = required_capacity;
        instance_buffer_.Destroy();
        const VkDeviceSize buffer_size = sizeof(SkeletonInstanceData) * max_instances_;
        instance_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    instance_buffer_dirty_ = true;
    return true;
}

void InstancedSkeletonRenderer::Render(VkCommandBuffer cmd, const ozz::math::Float4x4& view_proj) {
    if (!initialized_ || vertex_count_ == 0 || instance_count_ == 0 || descriptor_set_ == VK_NULL_HANDLE) {
        return;
    }

    UpdateUniforms(view_proj);
    UpdateInstanceBuffer();

    pipeline_.Bind(cmd);

    VkDescriptorSet sets[] = {descriptor_set_};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetLayout(), 0, 1, sets, 0, nullptr);

    VkBuffer buffers[] = {vertex_buffer_.GetBuffer(), instance_buffer_.GetBuffer()};
    VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);

    vkCmdDraw(cmd, vertex_count_, instance_count_, 0, 0);
}

bool InstancedSkeletonRenderer::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding camera_binding = {};
    camera_binding.binding = 0;
    camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camera_binding.descriptorCount = 1;
    camera_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &camera_binding;

    VkResult result = vkCreateDescriptorSetLayout(device_->GetDevice(), &layout_info, nullptr, &descriptor_set_layout_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create skeleton descriptor set layout (error: %d)", result);
        descriptor_set_layout_ = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

bool InstancedSkeletonRenderer::CreatePipeline() {
    PipelineConfig config;

#ifdef OZZ_SHADER_BINARY_DIR
    const std::filesystem::path shader_root{OZZ_SHADER_BINARY_DIR};
    config.vertex_shader_path = (shader_root / "skeleton_instanced.vert.spv").string();
    config.fragment_shader_path = (shader_root / "skeleton_instanced.frag.spv").string();
#else
    config.vertex_shader_path = "src/xrAnimation/tools/shaders/skeleton_instanced.vert.spv";
    config.fragment_shader_path = "src/xrAnimation/tools/shaders/skeleton_instanced.frag.spv";
#endif

    VkVertexInputBindingDescription vertex_binding = {};
    vertex_binding.binding = 0;
    vertex_binding.stride = sizeof(Vertex);
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription instance_binding = {};
    instance_binding.binding = 1;
    instance_binding.stride = sizeof(SkeletonInstanceData);
    instance_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    config.vertex_bindings = {vertex_binding, instance_binding};

    VkVertexInputAttributeDescription position_attr = {};
    position_attr.binding = 0;
    position_attr.location = 0;
    position_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    position_attr.offset = 0;

    VkVertexInputAttributeDescription instance_attr0 = {};
    instance_attr0.binding = 1;
    instance_attr0.location = 1;
    instance_attr0.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    instance_attr0.offset = 0;

    VkVertexInputAttributeDescription instance_attr1 = instance_attr0;
    instance_attr1.location = 2;
    instance_attr1.offset = sizeof(float) * 4;

    VkVertexInputAttributeDescription instance_attr2 = instance_attr0;
    instance_attr2.location = 3;
    instance_attr2.offset = sizeof(float) * 8;

    VkVertexInputAttributeDescription instance_attr3 = instance_attr0;
    instance_attr3.location = 4;
    instance_attr3.offset = sizeof(float) * 12;

    config.vertex_attributes = {position_attr, instance_attr0, instance_attr1, instance_attr2, instance_attr3};

    config.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    config.cull_mode = VK_CULL_MODE_NONE;
    config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.line_width = 1.0f;
    config.depth_test_enable = true;
    config.depth_write_enable = true;
    config.blend_enable = false;
    config.descriptor_set_layouts = {descriptor_set_layout_};

    if (!pipeline_.Create(device_->GetDevice(), config)) {
        return false;
    }

    return true;
}

bool InstancedSkeletonRenderer::CreateBuffers() {
    const std::array<SkeletonLinePoint, 6> default_points = {
        SkeletonLinePoint{0.0f, 0.0f, 0.0f}, SkeletonLinePoint{0.0f, 0.5f, 0.0f},
        SkeletonLinePoint{0.0f, 0.5f, 0.0f}, SkeletonLinePoint{0.0f, 1.0f, 0.0f},
        SkeletonLinePoint{0.0f, 1.0f, 0.0f}, SkeletonLinePoint{0.0f, 1.5f, 0.0f},
    };

    if (!SetSkeletonLines(ozz::make_span(default_points))) {
        return false;
    }

    max_instances_ = 1;
    const VkDeviceSize instance_size = sizeof(SkeletonInstanceData) * max_instances_;
    instance_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), instance_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    instance_data_.clear();
    instance_data_.push_back(MakeIdentityInstance());
    instance_count_ = 1;
    instance_buffer_dirty_ = true;

    const VkDeviceSize uniform_size = sizeof(CameraUBO);
    uniform_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), uniform_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    CameraUBO identity{};
    for (int i = 0; i < 16; ++i) {
        identity.view_proj[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    uniform_buffer_.Upload(&identity, sizeof(CameraUBO));

    return true;
}

bool InstancedSkeletonRenderer::CreateDescriptorPool() {
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;

    VkResult result = vkCreateDescriptorPool(device_->GetDevice(), &pool_info, nullptr, &descriptor_pool_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to create skeleton descriptor pool (error: %d)", result);
        descriptor_pool_ = VK_NULL_HANDLE;
        return false;
    }

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout_;

    result = vkAllocateDescriptorSets(device_->GetDevice(), &alloc_info, &descriptor_set_);
    if (result != VK_SUCCESS) {
        Msg("! Failed to allocate skeleton descriptor set (error: %d)", result);
        descriptor_set_ = VK_NULL_HANDLE;
        return false;
    }

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = uniform_buffer_.GetBuffer();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(CameraUBO);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set_;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device_->GetDevice(), 1, &write, 0, nullptr);

    return true;
}

void InstancedSkeletonRenderer::UpdateUniforms(const ozz::math::Float4x4& view_proj) {
    CameraUBO ubo{};
    for (int i = 0; i < 4; ++i) {
        ozz::math::StorePtrU(view_proj.cols[i], ubo.view_proj + i * 4);
    }
    uniform_buffer_.Upload(&ubo, sizeof(CameraUBO));
}

void InstancedSkeletonRenderer::UpdateInstanceBuffer() {
    if (!instance_buffer_dirty_ || instance_count_ == 0) {
        return;
    }

    const VkDeviceSize size = sizeof(SkeletonInstanceData) * instance_count_;
    instance_buffer_.Upload(instance_data_.data(), size);
    instance_buffer_dirty_ = false;
}

} // namespace renderer
} // namespace animation
} // namespace xray
