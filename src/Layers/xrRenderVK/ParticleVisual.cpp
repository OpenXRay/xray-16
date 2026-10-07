#include "xrEngine/stdafx.h"
#include "ParticleVisual.h"

#include "xrParticles/psystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
constexpr u32 step_ms = 33;
constexpr float step_seconds = .033f;
constexpr u32 enabled = 1u << 2;
constexpr u32 deferred_stop = 1u;

void bounds(vis_data& vis, const Fvector& position)
{
    vis.box.set(position, position);
    vis.box.grow(EPS_L);
    vis.box.getsphere(vis.sphere.P, vis.sphere.R);
}

}

void VulkanParticleEffect::on_birth(void* owner, u32, PAPI::Particle& particle, u32)
{
    auto& effect = *static_cast<VulkanParticleEffect*>(owner);
    const auto& def = *effect.def_;
    if (def.flags & (1u << 12))
        particle.frame = static_cast<u16>(Random.randI(def.frame_count) * 255);
    if ((def.flags & (1u << 11)) && (def.flags & (1u << 13)) && Random.randI(2))
        particle.flags.set(PAPI::Particle::ANIMATE_CCW, TRUE);
    if (effect.events_) effect.events_(true, particle);
}

void VulkanParticleEffect::on_dead(void* owner, u32, PAPI::Particle& particle, u32)
{
    auto& effect = *static_cast<VulkanParticleEffect*>(owner);
    if (effect.events_) effect.events_(false, particle);
}

void VulkanParticleEffect::set_events(std::function<void(bool, PAPI::Particle&)> events)
{
    events_ = std::move(events);
}

VulkanParticleEffect::VulkanParticleEffect(std::shared_ptr<const ParticleCatalog> catalog,
    const ParticleEffectDef& def) : catalog_(std::move(catalog)), def_(&def)
{
    parent_.identity();
    transform_.identity();
    initial_position_.set(0, 0, 0);
    bounds(visibility_, initial_position_);
    auto* manager = PAPI::ParticleManager();
    effect_ = manager->CreateEffect(def.max_particles);
    actions_ = manager->CreateActionList();
    if (effect_ >= 0 && actions_ >= 0)
    {
        IReader reader(const_cast<uint8_t*>(def.actions.data()), def.actions.size());
        manager->LoadActions(actions_, reader);
        manager->SetMaxParticles(effect_, def.max_particles);
        manager->SetCallback(effect_, on_birth, on_dead, this, 0);
    }
}

VulkanParticleEffect::~VulkanParticleEffect()
{
    release_gpu();
    auto* manager = PAPI::ParticleManager();
    if (effect_ >= 0) manager->DestroyEffect(effect_);
    if (actions_ >= 0) manager->DestroyActionList(actions_);
}

bool VulkanParticleEffect::initialize(VkDevice device, const VkPhysicalDeviceMemoryProperties& memory,
    const BufferResourceDispatch& dispatch, GameTextureFactory& textures, DeferredPass& pass,
    std::string& error)
{
    if (effect_ < 0 || actions_ < 0)
    { error = "particle simulator could not allocate effect/actions"; return false; }
    device_ = device;
    memory_ = memory;
    dispatch_ = dispatch;
    textures_ = &textures;
    pass_ = &pass;
    if ((def_->flags & 1u) == 0) return true; // Non-sprite actions still simulate.
    if (!textures.material(def_->texture, pass, material_, error))
    {
        release_gpu();
        return false;
    }
    material_name_ = def_->texture;
    return true;
}

void VulkanParticleEffect::release_gpu()
{
    for (auto& buffers : buffers_)
    {
        buffers.vertices.destroy();
        buffers.indices.destroy();
        buffers.capacity = 0;
    }
    if (material_ && textures_ && pass_)
        textures_->release_material(material_, *pass_);
    material_ = VK_NULL_HANDLE;
    textures_ = nullptr;
    pass_ = nullptr;
    device_ = VK_NULL_HANDLE;
}

void VulkanParticleEffect::UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL transform)
{
    parent_.set(m);
    initial_position_.set(m.c);
    if (transform) transform_.set(m);
    else
    {
        transform_.identity();
        if (actions_ >= 0) PAPI::ParticleManager()->Transform(actions_, m, velocity);
    }
}

void VulkanParticleEffect::Play()
{
    if (effect_ < 0 || actions_ < 0) return;
    elapsed_ = 0.f;
    remainder_ = 0;
    deferred_ = false;
    playing_ = true;
    PAPI::ParticleManager()->PlayEffect(effect_, actions_);
}

void VulkanParticleEffect::Stop(BOOL deferred)
{
    if (effect_ < 0 || actions_ < 0) return;
    PAPI::ParticleManager()->StopEffect(effect_, actions_, deferred != FALSE);
    deferred_ = deferred != FALSE;
    if (!deferred_) playing_ = false;
}

u32 VulkanParticleEffect::ParticlesCount()
{
    return effect_ < 0 ? 0 : PAPI::ParticleManager()->GetParticlesCount(effect_);
}

void VulkanParticleEffect::OnFrame(u32 dt)
{
    if (!playing_) { bounds(visibility_, initial_position_); return; }
    remainder_ = std::min<u32>(remainder_ + std::min<u32>(dt, 1000), 1000);
    const u32 steps = std::min<u32>(remainder_ / step_ms, 3);
    remainder_ %= step_ms;
    for (u32 i = 0; i < steps; ++i)
    {
        if (def_->time_limit >= 0.f && !deferred_ && (elapsed_ += step_seconds) > def_->time_limit)
            Stop(TRUE);
        PAPI::ParticleManager()->Update(effect_, actions_, step_seconds);
        if (def_->flags & (1u << 11))
        {
            PAPI::Particle* particles = nullptr;
            u32 count = 0;
            PAPI::ParticleManager()->GetParticles(effect_, particles, count);
            for (u32 index = 0; index < count; ++index)
            {
                float frame = particles[index].frame / 255.f +
                    (particles[index].flags.is(PAPI::Particle::ANIMATE_CCW) ? -1.f : 1.f) *
                    def_->frame_rate * step_seconds;
                frame = std::fmod(frame, float(def_->frame_count));
                if (frame < 0.f) frame += def_->frame_count;
                particles[index].frame = static_cast<u16>(frame * 255.f);
            }
        }
    }
    PAPI::Particle* particles = nullptr;
    u32 count = 0;
    PAPI::ParticleManager()->GetParticles(effect_, particles, count);
    if (deferred_ && count == 0) { deferred_ = false; playing_ = false; }
    if (!count || !particles) { bounds(visibility_, initial_position_); return; }
    visibility_.box.invalidate();
    float radius = 0.f;
    for (u32 i = 0; i < count; ++i)
    {
        Fvector position;
        transform_.transform_tiny(position, particles[i].pos);
        visibility_.box.modify(position);
        radius = std::max(radius, std::max(particles[i].size.x, particles[i].size.y));
    }
    visibility_.box.grow(radius);
    visibility_.box.getsphere(visibility_.sphere.P, visibility_.sphere.R);
}

bool VulkanParticleEffect::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], const Fvector& right, const Fvector& up, bool hud, std::string& error)
{
    if (!material_ || !device_) return true;
    PAPI::Particle* particles = nullptr;
    u32 count = 0;
    PAPI::ParticleManager()->GetParticles(effect_, particles, count);
    if (!count) return true;
    if (!particles || frame.frame_index >= buffers_.size() || count > def_->max_particles)
    { error = "invalid particle frame/count"; return false; }
    auto& buffers = buffers_[frame.frame_index];
    if (count > buffers.capacity)
    {
        // FrameContext waited for this frame slot's fence before invoking the recorder.
        const size_t capacity = std::min<size_t>(def_->max_particles,
            std::max<size_t>(count, buffers.capacity ? buffers.capacity * 2 : 16));
        BufferResource vertices, indices;
        if (!vertices.initialize(device_, capacity * 4 * sizeof(LevelVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory_, dispatch_, error) ||
            !indices.initialize(device_, capacity * 6 * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory_, dispatch_, error)) return false;
        buffers.vertices = std::move(vertices);
        buffers.indices = std::move(indices);
        buffers.capacity = capacity;
    }
    std::vector<LevelVertex> vertices(count * 4);
    std::vector<uint32_t> indices(count * 6);
    for (u32 i = 0; i < count; ++i)
    {
        const auto& p = particles[i];
        Fvector center;
        transform_.transform_tiny(center, p.pos);
        const float sx = p.size.x * .5f, sy = p.size.y * .5f;
        const u32 sprite_frame = p.frame / 255u % def_->frame_count;
        const float u0 = (sprite_frame % def_->frame_columns) * def_->frame_size[0];
        const float v0 = (sprite_frame / def_->frame_columns) * def_->frame_size[1];
        const float du = def_->frame_size[0], dv = def_->frame_size[1];
        const float sine = std::sin(p.rot.x), cosine = std::cos(p.rot.x);
        for (u32 corner = 0; corner < 4; ++corner)
        {
            const float x = (corner == 0 || corner == 3) ? -sx : sx;
            const float y = corner < 2 ? -sy : sy;
            auto& v = vertices[i * 4 + corner];
            const float rotated_x = cosine * x - sine * y;
            const float rotated_y = sine * x + cosine * y;
            v.position[0] = center.x + right.x * rotated_x + up.x * rotated_y;
            v.position[1] = center.y + right.y * rotated_x + up.y * rotated_y;
            v.position[2] = center.z + right.z * rotated_x + up.z * rotated_y;
            v.normal[0] = 0.f; v.normal[1] = 1.f; v.normal[2] = 0.f;
            v.uv[0] = u0 + (corner == 0 || corner == 3 ? 0.f : du);
            v.uv[1] = v0 + (corner < 2 ? 0.f : dv);
        }
        for (u32 j = 0; j < 6; ++j)
            indices[i * 6 + j] = i * 4 + (j == 0 || j == 3 ? 0 :
                j == 1 ? 1 : j == 2 || j == 4 ? 2 : 3);
    }
    if (!buffers.vertices.write(0, vertices.data(), vertices.size() * sizeof(LevelVertex), error) ||
        !buffers.indices.write(0, indices.data(), indices.size() * sizeof(uint32_t), error)) return false;
    return hud ? pass.record_hud(frame, buffers.vertices.handle(), buffers.indices.handle(), count * 6, mvp, material_) :
        pass.record_transparent(frame, buffers.vertices.handle(), buffers.indices.handle(), count * 6, mvp, material_);
}

VulkanParticleGroup::VulkanParticleGroup(std::shared_ptr<const ParticleCatalog> catalog,
    const ParticleGroupDef& def) : catalog_(std::move(catalog)), def_(&def)
{
    initial_position_.set(0, 0, 0);
    parent_.identity();
    bounds(visibility_, initial_position_);
    for (const auto& item : def.effects)
        effects_.push_back(std::make_unique<VulkanParticleEffect>(catalog_, *catalog_->effect(item.effect)));
    for (size_t i = 0; i < effects_.size(); ++i)
        effects_[i]->set_events([this, i](bool birth, PAPI::Particle& particle)
        { on_particle(i, birth, particle); });
}

VulkanParticleGroup::~VulkanParticleGroup()
{
    for (auto& effect : effects_) effect->set_events({});
}

bool VulkanParticleGroup::initialize(VkDevice device, const VkPhysicalDeviceMemoryProperties& memory,
    const BufferResourceDispatch& dispatch, GameTextureFactory& textures, DeferredPass& pass,
    std::string& error)
{
    device_ = device;
    memory_ = memory;
    dispatch_ = dispatch;
    textures_ = &textures;
    pass_ = &pass;
    for (auto& effect : effects_)
        if (!effect->initialize(device, memory, dispatch, textures, pass, error))
        {
            OnDeviceDestroy();
            return false;
        }
    return true;
}

void VulkanParticleGroup::OnDeviceDestroy()
{
    // Triggered children may have been created after initialize(). Release
    // them before the catalog/pass leases are detached; a later initialize()
    // must start with only the definition's persistent effects.
    for (auto& item : triggered_) item.effect->set_events({});
    triggered_.clear();
    pending_.clear();
    for (auto& effect : effects_) effect->OnDeviceDestroy();
    textures_ = nullptr;
    pass_ = nullptr;
    device_ = VK_NULL_HANDLE;
}

void VulkanParticleGroup::UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL transform)
{
    initial_position_.set(m.c);
    parent_.set(m);
    transform_parent_ = transform != FALSE;
    for (auto& effect : effects_) effect->UpdateParent(m, velocity, transform);
}

void VulkanParticleGroup::Play()
{
    time_ = 0.f;
    deferred_ = false;
    playing_ = true;
    for (auto& effect : effects_) effect->Stop(FALSE);
    for (auto& item : triggered_) item.effect->Stop(FALSE);
    pending_.clear();
}

void VulkanParticleGroup::Stop(BOOL deferred)
{
    deferred_ = deferred != FALSE;
    if (!deferred_) playing_ = false;
    for (auto& effect : effects_) effect->Stop(deferred);
    for (auto& item : triggered_) item.effect->Stop(deferred);
}

void VulkanParticleGroup::OnFrame(u32 dt)
{
    if (!playing_) { bounds(visibility_, initial_position_); return; }
    const float next = time_ + dt / 1000.f;
    for (size_t i = 0; i < effects_.size(); ++i)
    {
        auto& effect = *effects_[i];
        const auto& item = def_->effects[i];
        if ((item.flags & enabled) && !deferred_)
        {
            if (!effect.IsPlaying() && time_ <= item.begin && next >= item.begin)
            {
                effect.Play();
                if ((item.flags & (1u << 1)) && !item.on_play.empty())
                    pending_.push_back({item.on_play, initial_position_, {0.f, 0.f, 0.f}});
            }
            if (effect.IsPlaying() && time_ <= item.end && next >= item.end)
                effect.Stop(item.flags & deferred_stop);
        }
        effect.OnFrame(dt);
    }
    for (size_t index = 0; index < pending_.size(); ++index)
        spawn(pending_[index].name, pending_[index].position, pending_[index].velocity);
    pending_.clear();
    for (auto& child : triggered_) if (child.effect->IsPlaying()) child.effect->OnFrame(dt);
    time_ = next;
    if (!deferred_ && def_->time_limit > 0.f && time_ > def_->time_limit) Stop(TRUE);
    if (deferred_ && ParticlesCount() == 0) { playing_ = false; deferred_ = false; }
    visibility_.box.invalidate();
    for (const auto& effect : effects_)
    {
        visibility_.box.modify(effect->getVisData().box.vMin);
        visibility_.box.modify(effect->getVisData().box.vMax);
    }
    for (const auto& child : triggered_)
    {
        visibility_.box.modify(child.effect->getVisData().box.vMin);
        visibility_.box.modify(child.effect->getVisData().box.vMax);
    }
    if (effects_.empty() && triggered_.empty()) bounds(visibility_, initial_position_);
    else visibility_.box.getsphere(visibility_.sphere.P, visibility_.sphere.R);
}

u32 VulkanParticleGroup::ParticlesCount()
{
    u32 count = 0;
    for (auto& effect : effects_) count += effect->ParticlesCount();
    for (auto& child : triggered_) count += child.effect->ParticlesCount();
    return count;
}

void VulkanParticleGroup::SetHudMode(BOOL value)
{
    hud_ = value != FALSE;
    for (auto& effect : effects_) effect->SetHudMode(value);
    for (auto& child : triggered_) child.effect->SetHudMode(value);
}

bool VulkanParticleGroup::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], const Fvector& right, const Fvector& up, bool hud, std::string& error)
{
    for (auto& effect : effects_)
        if (!effect->record(frame, pass, mvp, right, up, hud, error)) return false;
    for (auto& child : triggered_)
        if (!child.effect->record(frame, pass, mvp, right, up, hud, error)) return false;
    return true;
}

void VulkanParticleGroup::on_particle(size_t index, bool birth, PAPI::Particle& particle)
{
    const auto& item = def_->effects[index];
    const auto& name = birth ? item.on_birth : item.on_dead;
    if (!(item.flags & (birth ? (1u << 5) : (1u << 6))) || name.empty()) return;
    Fvector position;
    if (transform_parent_) parent_.transform_tiny(position, particle.pos);
    else position.set(particle.pos);
    Fvector velocity;
    velocity.sub(particle.pos, particle.posB);
    velocity.div(step_seconds);
    pending_.push_back({name, position, velocity});
}

void VulkanParticleGroup::spawn(const std::string& name, const Fvector& position,
    const Fvector& velocity)
{
    const auto* def = catalog_->effect(name);
    if (!def) return;
    VulkanParticleEffect* effect = nullptr;
    for (auto& child : triggered_)
        if (child.name == name && !child.effect->IsPlaying() && !child.effect->ParticlesCount())
        { effect = child.effect.get(); break; }
    if (!effect)
    {
        auto child = std::make_unique<VulkanParticleEffect>(catalog_, *def);
        if (device_)
        {
            std::string error;
            if (!child->initialize(device_, memory_, dispatch_, *textures_, *pass_, error))
            {
                Msg("! [renderer-vulkan] particle child '%s': %s", name.c_str(), error.c_str());
                return;
            }
        }
        effect = child.get();
        triggered_.push_back({name, std::move(child)});
    }
    Fmatrix parent;
    parent.identity();
    parent.c.set(position);
    effect->SetHudMode(hud_);
    effect->UpdateParent(parent, velocity, FALSE);
    effect->Play();
}
} // namespace xray::render::vulkan
