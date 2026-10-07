#pragma once

#include "Include/xrRender/ThunderboltDescRender.h"
#include "Include/xrRender/ThunderboltRender.h"
#include "BufferResource.h"
#include "FrameContext.h"
#include "LevelModels.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanThunderboltDescRender final : public IThunderboltDescRender
{
public:
    explicit VulkanThunderboltDescRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanThunderboltDescRender() override { DestroyModel(); }
    void Copy(IThunderboltDescRender& source) override;
    void CreateModel(LPCSTR name) override;
    void DestroyModel() override;
    const std::vector<LevelVertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    VkDescriptorSet material() const { return material_; }
private:
    VulkanGameDevice& device_;
    std::string name_;
    std::vector<LevelVertex> vertices_;
    std::vector<uint32_t> indices_;
    VkDescriptorSet material_{};
};

class VulkanThunderboltRender final : public IThunderboltRender
{
public:
    explicit VulkanThunderboltRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanThunderboltRender() override;
    void Copy(IThunderboltRender&) override {}
    void Render(CEffect_Thunderbolt& owner) override;
    bool record(const FrameRecordingContext& frame, std::string& error);
private:
    struct FrameBuffers { BufferResource vertices, indices; };
    bool ensure(BufferResource& resource, size_t bytes, VkBufferUsageFlags usage,
        std::string& error);
    VulkanGameDevice& device_;
    std::array<FrameBuffers, FrameContext::FramesInFlight> frames_{};
    std::vector<LevelVertex> vertices_;
    std::vector<uint32_t> indices_;
    VkDescriptorSet material_{};
    float mvp_[16]{};
};
}
