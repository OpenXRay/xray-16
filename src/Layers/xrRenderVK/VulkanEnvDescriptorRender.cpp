#include "xrEngine/stdafx.h"
#include "VulkanEnvDescriptorRender.h"
#include "xrEngine/Environment.h"

namespace xray::render::vulkan
{
void VulkanEnvDescriptorRender::acquire(const std::string& name, VkImageView& view)
{
    if (name.empty()) return;
    std::string error;
    if (!textures_.environment(name, view, error))
        Msg("! [renderer-vulkan] weather texture '%s': %s", name.c_str(), error.c_str());
}

void VulkanEnvDescriptorRender::OnDeviceCreate(CEnvDescriptor& owner)
{
    OnDeviceDestroy();
    sky_name_ = owner.sky_texture_name.c_str();
    sky_environment_name_ = owner.sky_texture_env_name.c_str();
    clouds_name_ = owner.clouds_texture_name.c_str();
    acquire(sky_name_, sky_);
    acquire(sky_environment_name_, sky_environment_);
    acquire(clouds_name_, clouds_);
    static unsigned descriptors = 0;
    if (++descriptors <= 8 || descriptors % 32 == 0)
        Msg("[renderer-vulkan] weather.assets descriptor=%u sky='%s' cube=%d clouds='%s' cloud-view=%d",
            descriptors, sky_name_.c_str(), sky_ != VK_NULL_HANDLE,
            clouds_name_.c_str(), clouds_ != VK_NULL_HANDLE);
}

void VulkanEnvDescriptorRender::Copy(IEnvDescriptorRender& source)
{
    if (&source == this) return;
    auto& other = static_cast<VulkanEnvDescriptorRender&>(source);
    OnDeviceDestroy();
    sky_name_ = other.sky_name_;
    sky_environment_name_ = other.sky_environment_name_;
    clouds_name_ = other.clouds_name_;
    acquire(sky_name_, sky_);
    acquire(sky_environment_name_, sky_environment_);
    acquire(clouds_name_, clouds_);
}

void VulkanEnvDescriptorRender::OnDeviceDestroy()
{
    textures_.release_environment(sky_);
    textures_.release_environment(sky_environment_);
    textures_.release_environment(clouds_);
    sky_ = sky_environment_ = clouds_ = VK_NULL_HANDLE;
    sky_name_.clear();
    sky_environment_name_.clear();
    clouds_name_.clear();
}
}
