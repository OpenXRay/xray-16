#include "xrEngine/stdafx.h"
#include "VulkanEnvironmentRender.h"
#include "EngineParticleSource.h"
#include "VulkanGameDevice.h"
#include "xrEngine/Environment.h"

namespace xray::render::vulkan
{
shared_str const& VulkanEnvironmentRender::Library::particles_group_id(particles_systems::PS::CPGDef const&) const
{
    static const shared_str empty;
    R_ASSERT2(false, "Vulkan particle definitions use particles_group_ids");
    return empty;
}

void VulkanEnvironmentRender::Library::particles_group_ids(xr_vector<shared_str>& ids) const
{
    ids.insert(ids.end(), ids_.begin(), ids_.end());
}

void VulkanEnvironmentRender::Library::load()
{
    clear();
    string_path path;
    FS.update_path(path, _game_data_, "particles.xr");
    IReader* source = FS.r_open(path);
    if (!source)
    {
        Msg("! [renderer-vulkan] particle library not found: %s", path);
        return;
    }
    const auto bytes = particle_library_bytes(*source);
    FS.r_close(source);
    std::string error;
    if (!parse_particle_catalog({bytes.data(), bytes.size()}, catalog_, error))
    {
        Msg("! [renderer-vulkan] environment particle library: %s", error.c_str());
        return;
    }
    for (const auto& group : catalog_.groups) ids_.emplace_back(group.name.c_str());
}

void VulkanEnvironmentRender::Library::clear()
{
    catalog_ = {};
    ids_.clear();
}

void VulkanEnvironmentRender::Copy(IEnvironmentRender& source)
{
    const auto& other = static_cast<VulkanEnvironmentRender&>(source);
    sky_a_ = other.sky_a_;
    sky_b_ = other.sky_b_;
    environment_a_ = other.environment_a_;
    environment_b_ = other.environment_b_;
    clouds_a_ = other.clouds_a_;
    clouds_b_ = other.clouds_b_;
    blend_ = other.blend_;
}

void VulkanEnvironmentRender::OnDeviceCreate() { particles_.load(); }
void VulkanEnvironmentRender::OnDeviceDestroy() { Clear(); particles_.clear(); }
void VulkanEnvironmentRender::Clear()
{
    sky_a_ = sky_b_ = environment_a_ = environment_b_ = clouds_a_ = clouds_b_ = VK_NULL_HANDLE;
    blend_ = 0.f;
}

void VulkanEnvironmentRender::lerp(CEnvDescriptorMixer& current,
    IEnvDescriptorRender* a, IEnvDescriptorRender* b)
{
    const auto* first = static_cast<const VulkanEnvDescriptorRender*>(a);
    const auto* second = static_cast<const VulkanEnvDescriptorRender*>(b);
    sky_a_ = first ? first->sky() : VK_NULL_HANDLE;
    sky_b_ = second ? second->sky() : VK_NULL_HANDLE;
    environment_a_ = first ? first->sky_environment() : VK_NULL_HANDLE;
    environment_b_ = second ? second->sky_environment() : VK_NULL_HANDLE;
    clouds_a_ = first ? first->clouds() : VK_NULL_HANDLE;
    clouds_b_ = second ? second->clouds() : VK_NULL_HANDLE;
    blend_ = current.weight;
}

void VulkanEnvironmentRender::RenderSky(CEnvironment&) {}
void VulkanEnvironmentRender::RenderClouds(CEnvironment&) {}
}
