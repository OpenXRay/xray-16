#pragma once

#include "Include/xrRender/StatGraphRender.h"
#include "xrEngine/StatGraph.h"
#include "ScenePass.h"

// The graph owns its single white UI texture. GPU geometry is accumulated by
// VulkanUIRender and submitted in the same ordered UI pass as fonts and menus.
namespace xray::render::vulkan
{
class VulkanGameDevice;

class VulkanStatGraphRender final : public IStatGraphRender
{
public:
    explicit VulkanStatGraphRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanStatGraphRender() override { OnDeviceDestroy(); }

    void Copy(IStatGraphRender& source) override;
    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void OnRender(CStatGraph& owner) override;

private:
    void line(float x0, float y0, float x1, float y1, u32 color);
    void rectangle(float x0, float y0, float x1, float y1, u32 color);
    VulkanGameDevice& device_;
    VkDescriptorSet white_ = VK_NULL_HANDLE;
};
}
