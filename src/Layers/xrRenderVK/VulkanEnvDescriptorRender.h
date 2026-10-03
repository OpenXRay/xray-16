#pragma once

#include "Include/xrRender/EnvironmentRender.h"
#include "GameTextureFactory.h"

#include <string>

namespace xray::render::vulkan
{
class VulkanEnvDescriptorRender final : public IEnvDescriptorRender
{
public:
    explicit VulkanEnvDescriptorRender(GameTextureFactory& textures) : textures_(textures) {}
    ~VulkanEnvDescriptorRender() override { OnDeviceDestroy(); }
    void Copy(IEnvDescriptorRender& source) override;
    void OnDeviceCreate(CEnvDescriptor& owner) override;
    void OnDeviceDestroy() override;

    VkImageView sky() const { return sky_; }
    VkImageView sky_environment() const { return sky_environment_; }
    VkImageView clouds() const { return clouds_; }

private:
    void acquire(const std::string& name, VkImageView& view);
    GameTextureFactory& textures_;
    std::string sky_name_, sky_environment_name_, clouds_name_;
    VkImageView sky_{}, sky_environment_{}, clouds_{};
};
}
