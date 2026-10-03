#pragma once

#include "Include/xrRender/RainRender.h"
#include "BufferResource.h"
#include "FrameContext.h"
#include "LevelModels.h"

#include <array>
#include <vector>

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanRainRender final : public IRainRender
{
public:
    explicit VulkanRainRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanRainRender() override;
    void Copy(IRainRender& source) override;
    void Render(CEffect_Rain& owner) override;
    const Fsphere& GetDropBounds() const override { return drop_bounds_; }
    bool record(const FrameRecordingContext& frame, std::string& error);

private:
    struct FrameBuffers { BufferResource vertices, indices; };
    void quad(const Fvector& a, const Fvector& b, const Fvector& right, float width);
    bool ensure(BufferResource& resource, size_t bytes, VkBufferUsageFlags usage,
        std::string& error);
    VulkanGameDevice& device_;
    Fsphere drop_bounds_{{0.f, 0.f, 0.f}, .6f};
    std::array<FrameBuffers, FrameContext::FramesInFlight> frames_{};
    std::vector<LevelVertex> vertices_;
    std::vector<uint32_t> indices_;
    VkDescriptorSet material_{};
    float mvp_[16]{};
};
}
