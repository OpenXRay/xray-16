#pragma once

#include "Include/xrRender/LensFlareRender.h"
#include <vulkan/vulkan.h>
#include <string>

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanFlareRender final : public IFlareRender
{
public:
    explicit VulkanFlareRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanFlareRender() override { DestroyShader(); }
    void Copy(IFlareRender& source) override;
    void CreateShader(LPCSTR shader, LPCSTR texture) override;
    void DestroyShader() override;
    VkDescriptorSet descriptor() const { return texture_set_; }
private:
    VulkanGameDevice& device_;
    std::string texture_name_;
    VkDescriptorSet texture_set_{};
};

class VulkanLensFlareRender final : public ILensFlareRender
{
public:
    explicit VulkanLensFlareRender(VulkanGameDevice& device) : device_(device) {}
    void Copy(ILensFlareRender&) override {}
    void Render(CLensFlare& owner, BOOL sun, BOOL flares, BOOL gradient) override;
    void OnDeviceCreate() override {}
    void OnDeviceDestroy() override {}
private:
    void draw(const Fvector& center, const Fvector& axis_x, const Fvector& axis_y,
        float radius, float opacity, const Fcolor& color, VkDescriptorSet texture);
    VulkanGameDevice& device_;
};
}
