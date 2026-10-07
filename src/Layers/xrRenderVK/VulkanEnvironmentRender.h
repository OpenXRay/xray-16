#pragma once

#include "Include/xrRender/EnvironmentRender.h"
#include "Include/xrRender/particles_systems_library_interface.hpp"
#include "ParticleCatalog.h"
#include "DeferredPass.h"
#include "VulkanEnvDescriptorRender.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanEnvironmentRender final : public IEnvironmentRender
{
public:
    explicit VulkanEnvironmentRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanEnvironmentRender() override { Clear(); }
    void Copy(IEnvironmentRender& source) override;
    void RenderSky(CEnvironment& env) override;
    void RenderClouds(CEnvironment& env) override;
    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void Clear() override;
    void lerp(CEnvDescriptorMixer& current, IEnvDescriptorRender* a,
        IEnvDescriptorRender* b) override;
    particles_systems::library_interface const& particles_systems_library() override
    { return particles_; }

private:
    class Library final : public particles_systems::library_interface
    {
    public:
        particles_systems::PS::CPGDef const* const* particles_group_begin() const override { return nullptr; }
        particles_systems::PS::CPGDef const* const* particles_group_end() const override { return nullptr; }
        void particles_group_next(particles_systems::PS::CPGDef const* const*&) const override {}
        shared_str const& particles_group_id(particles_systems::PS::CPGDef const&) const override;
        void particles_group_ids(xr_vector<shared_str>& ids) const override;
        void load();
        void clear();
    private:
        ParticleCatalog catalog_;
        xr_vector<shared_str> ids_;
    } particles_;
    VulkanGameDevice& device_;
    VkImageView sky_a_{}, sky_b_{}, environment_a_{}, environment_b_{}, clouds_a_{}, clouds_b_{};
    VkDescriptorSet weather_set_{};
    WeatherLighting lighting_{};
    float blend_{};
    bool weather_error_reported_{};
};
}
