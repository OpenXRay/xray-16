#include "xrEngine/stdafx.h"
#include "VulkanEnvironmentRender.h"
#include "EngineParticleSource.h"
#include "VulkanGameDevice.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "FogParameters.h"

#include <cmath>
#include <cstring>

// The Vulkan backend has no legacy particle renderer. The game-facing
// library interface nevertheless requires CPGDef storage and stable names.
namespace xray::render::RENDER_NAMESPACE::PS
{
CPGDef::CPGDef() : m_fTimeLimit(0.f) { m_Flags.zero(); }
CPGDef::~CPGDef()
{
    for (auto* effect : m_Effects) xr_delete(effect);
}
}

namespace xray::render::vulkan
{
VulkanEnvironmentRender::Library::~Library() = default;

particles_systems::PS::CPGDef const* const*
VulkanEnvironmentRender::Library::particles_group_begin() const
{
    return pointers_.empty() ? nullptr : pointers_.data();
}

particles_systems::PS::CPGDef const* const*
VulkanEnvironmentRender::Library::particles_group_end() const
{
    return pointers_.empty() ? nullptr : pointers_.data() + pointers_.size();
}

void VulkanEnvironmentRender::Library::particles_group_next(
    particles_systems::PS::CPGDef const* const*& iterator) const
{
    // An iterator retained across a catalog reload is no longer part of this
    // array. Relational pointer comparisons against it are undefined; accept
    // only an address that is an element of the current catalog.
    for (size_t index = 0; index < pointers_.size(); ++index)
        if (iterator == pointers_.data() + index)
        {
            iterator = pointers_.data() + index + 1;
            return;
        }
}

shared_str const& VulkanEnvironmentRender::Library::particles_group_id(
    particles_systems::PS::CPGDef const& group) const
{
    return group.m_Name;
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
    if (!error.empty())
        Msg("! [renderer-vulkan] environment particle library: %s", error.c_str());
    assign(std::move(catalog_));
}

void VulkanEnvironmentRender::Library::assign(ParticleCatalog catalog)
{
    clear();
    catalog_ = std::move(catalog);
    definitions_.reserve(catalog_.groups.size());
    pointers_.reserve(catalog_.groups.size());
    for (const auto& group : catalog_.groups)
    {
        auto def = std::make_unique<particles_systems::PS::CPGDef>();
        def->m_Name = group.name.c_str();
        def->m_Flags.assign(group.flags);
        def->m_fTimeLimit = group.time_limit;
        for (const auto& item : group.effects)
        {
            auto* effect = xr_new<particles_systems::PS::CPGDef::SEffect>();
            effect->m_Flags.assign(item.flags);
            effect->m_EffectName = item.effect.c_str();
            effect->m_OnPlayChildName = item.on_play.c_str();
            effect->m_OnBirthChildName = item.on_birth.c_str();
            effect->m_OnDeadChildName = item.on_dead.c_str();
            effect->m_Time0 = item.begin;
            effect->m_Time1 = item.end;
            def->m_Effects.push_back(effect);
        }
        ids_.push_back(def->m_Name);
        pointers_.push_back(def.get());
        definitions_.push_back(std::move(def));
    }
}

void VulkanEnvironmentRender::Library::clear()
{
    catalog_ = {};
    pointers_.clear();
    definitions_.clear();
    ids_.clear();
}

void VulkanEnvironmentRender::Copy(IEnvironmentRender& source)
{
    if (&source == this) return;
    const auto& other = static_cast<VulkanEnvironmentRender&>(source);
    Clear();
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
    if (weather_set_)
    {
        R_ASSERT2(device_.wait_idle(), "weather descriptor release requires idle GPU frames");
        device_.deferred().release_gbuffer(weather_set_);
    }
    sky_a_ = sky_b_ = environment_a_ = environment_b_ = clouds_a_ = clouds_b_ = VK_NULL_HANDLE;
    blend_ = 0.f;
    weather_error_reported_ = false;
}

void VulkanEnvironmentRender::lerp(CEnvDescriptorMixer& current,
    IEnvDescriptorRender* a, IEnvDescriptorRender* b)
{
    const auto* first = static_cast<const VulkanEnvDescriptorRender*>(a);
    const auto* second = static_cast<const VulkanEnvDescriptorRender*>(b);
    if (weather_set_ && (!first || !second || sky_a_ != first->sky() ||
        sky_b_ != second->sky() || clouds_a_ != first->clouds() ||
        clouds_b_ != second->clouds()))
    {
        R_ASSERT2(device_.wait_idle(), "weather texture change requires idle GPU frames");
        device_.deferred().release_gbuffer(weather_set_);
    }
    if (sky_a_ != (first ? first->sky() : VK_NULL_HANDLE) ||
        sky_b_ != (second ? second->sky() : VK_NULL_HANDLE) ||
        clouds_a_ != (first ? first->clouds() : VK_NULL_HANDLE) ||
        clouds_b_ != (second ? second->clouds() : VK_NULL_HANDLE))
        weather_error_reported_ = false;
    sky_a_ = first ? first->sky() : VK_NULL_HANDLE;
    sky_b_ = second ? second->sky() : VK_NULL_HANDLE;
    environment_a_ = first ? first->sky_environment() : VK_NULL_HANDLE;
    environment_b_ = second ? second->sky_environment() : VK_NULL_HANDLE;
    clouds_a_ = first ? first->clouds() : VK_NULL_HANDLE;
    clouds_b_ = second ? second->clouds() : VK_NULL_HANDLE;
    blend_ = current.weight;
}

void VulkanEnvironmentRender::RenderSky(CEnvironment& env)
{
    if (!weather_set_)
    {
        std::string error;
        if (!device_.deferred().weather_set(sky_a_, sky_b_, clouds_a_, clouds_b_,
                device_.textures().sampler(), weather_set_, error))
        {
            // Some weather presets omit textures. Retain the ordinary deferred
            // lighting path and report the asset error once per preset.
            if (!weather_error_reported_)
                Msg("! [renderer-vulkan] weather: %s sky-a=%d sky-b=%d clouds-a=%d clouds-b=%d",
                    error.c_str(), sky_a_ != VK_NULL_HANDLE, sky_b_ != VK_NULL_HANDLE,
                    clouds_a_ != VK_NULL_HANDLE, clouds_b_ != VK_NULL_HANDLE);
            weather_error_reported_ = true;
            return;
        }
    }
    Fmatrix inverse;
    // The full view-projection contains perspective; Fmatrix::invert only
    // handles affine 4x3 transforms and corrupts reconstructed sky rays.
    inverse.invert_44(Device.mFullTransform);
    const auto ray = [&inverse](float x, float y)
    {
        Fvector4 near_clip, far_clip, near_world, far_world;
        near_clip.set(x, y, 0.f, 1.f);
        far_clip.set(x, y, 1.f, 1.f);
        inverse.transform(near_world, near_clip);
        inverse.transform(far_world, far_clip);
        Fvector result;
        result.set(far_world.x / far_world.w - near_world.x / near_world.w,
            far_world.y / far_world.w - near_world.y / near_world.w,
            far_world.z / far_world.w - near_world.z / near_world.w);
        return result;
    };
    // Framebuffer UV starts at the upper edge; the engine's clip-space Y points up.
    const Fvector base = ray(-1.f, 1.f);
    const Fvector dx = ray(1.f, 1.f);
    const Fvector dy = ray(-1.f, -1.f);
    lighting_.ray_base[0] = base.x; lighting_.ray_base[1] = base.y; lighting_.ray_base[2] = base.z;
    lighting_.ray_dx[0] = dx.x - base.x; lighting_.ray_dx[1] = dx.y - base.y; lighting_.ray_dx[2] = dx.z - base.z;
    lighting_.ray_dy[0] = dy.x - base.x; lighting_.ray_dy[1] = dy.y - base.y; lighting_.ray_dy[2] = dy.z - base.z;
    const auto& current = env.CurrentEnv;
    const float fog_color[]{current.fog_color.x, current.fog_color.y, current.fog_color.z};
    set_weather_fog(lighting_.ray_base, lighting_.ray_dx, lighting_.ray_dy,
        fog_color, current.fog_near, current.fog_far, current.far_plane);
    lighting_.light.color[3] = blend_;
    lighting_.sky_color[0] = current.sky_color.x;
    lighting_.sky_color[1] = current.sky_color.y;
    lighting_.sky_color[2] = current.sky_color.z;
    lighting_.sky_color[3] = current.sky_rotation;
    lighting_.clouds_color[0] = current.clouds_color.x;
    lighting_.clouds_color[1] = current.clouds_color.y;
    lighting_.clouds_color[2] = current.clouds_color.z;
    lighting_.clouds_color[3] = current.clouds_color.w;
    // The legacy cloud vertex shader scrolls two planar XZ projections over
    // the hemisphere. Pack rotation and elapsed seconds into the unused
    // weather grade component; the push constant is already 128 bytes.
    const float cloud_time = std::fmod(std::max(0.f, Device.fTimeGlobal), 256.f);
    const float cloud_rotation = std::fmod(current.clouds_rotation, PI_MUL_2) +
        (current.clouds_rotation < 0.f ? PI_MUL_2 : 0.f);
    lighting_.light.grade[3] = pack_fog_pair(cloud_time, cloud_rotation);
    if (Device.dwFrame % 300 == 0)
        Msg("[renderer-vulkan] weather.frame frame=%u sky=(%.3f,%.3f,%.3f) cloud-alpha=%.3f cloud-planar=1 cloud-angle=%.3f ambient=%.3f sky-a=%d sky-b=%d",
            Device.dwFrame, lighting_.sky_color[0], lighting_.sky_color[1],
            lighting_.sky_color[2], current.clouds_color.w, cloud_rotation,
            lighting_.light.direction_ambient[3], sky_a_ != VK_NULL_HANDLE,
            sky_b_ != VK_NULL_HANDLE);
    device_.queue_weather(weather_set_, lighting_, fog_color, current.fog_near, current.fog_far);
}

void VulkanEnvironmentRender::RenderClouds(CEnvironment& env)
{
    if (!weather_set_) return;
    lighting_.clouds_color[3] = env.CurrentEnv.clouds_color.w;
    const auto& current = env.CurrentEnv;
    const float fog_color[]{current.fog_color.x, current.fog_color.y, current.fog_color.z};
    device_.queue_weather(weather_set_, lighting_, fog_color, current.fog_near, current.fog_far);
}
}
