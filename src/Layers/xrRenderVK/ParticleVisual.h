#pragma once

#include "ParticleCatalog.h"
#include "BufferResource.h"
#include "DeferredPass.h"
#include "GameTextureFactory.h"
#include "Include/xrRender/ParticleCustom.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"

#include <array>
#include <functional>
#include <memory>

namespace PAPI { struct Particle; }

namespace xray::render::vulkan
{
class VulkanParticleEffect final : public IRenderVisual, public IParticleCustom
{
public:
    VulkanParticleEffect(std::shared_ptr<const ParticleCatalog> catalog, const ParticleEffectDef& def);
    ~VulkanParticleEffect() override;
    VulkanParticleEffect(const VulkanParticleEffect&) = delete;
    bool initialize(VkDevice device, const VkPhysicalDeviceMemoryProperties& memory,
        const BufferResourceDispatch& dispatch, GameTextureFactory& textures, DeferredPass& pass,
        std::string& error);
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass, const float (&mvp)[16],
        const Fvector& right, const Fvector& up, bool hud, std::string& error);
    void release_gpu(); // The owner waits for all submitted frames first.
    void set_events(std::function<void(bool, PAPI::Particle&)> events);
    u32 getType() const override { return 8; }
    vis_data& getVisData() override { return visibility_; }
    IParticleCustom* dcast_ParticleCustom() override { return this; }
    void OnDeviceCreate() override {}
    void OnDeviceDestroy() override { release_gpu(); }
    void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL transform) override;
    void OnFrame(u32 dt) override;
    void Play() override;
    void Stop(BOOL deferred = TRUE) override;
    BOOL IsPlaying() override { return playing_; }
    u32 ParticlesCount() override;
    float GetTimeLimit() override { return def_->time_limit; }
    const shared_str Name() override { return def_->name.c_str(); }
    void SetHudMode(BOOL value) override { hud_ = value; }
    BOOL GetHudMode() override { return hud_; }
#ifdef DEBUG
    shared_str getDebugName() override { return def_->name.c_str(); }
#endif

private:
    static void on_birth(void* owner, u32, PAPI::Particle& particle, u32);
    static void on_dead(void* owner, u32, PAPI::Particle& particle, u32);
    std::shared_ptr<const ParticleCatalog> catalog_;
    const ParticleEffectDef* def_{};
    vis_data visibility_;
    Fmatrix parent_;
    Fvector initial_position_;
    Fmatrix transform_;
    bool playing_{}, deferred_{}, hud_{};
    float elapsed_{};
    u32 remainder_{};
    int effect_{-1}, actions_{-1};
    VkDevice device_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    BufferResourceDispatch dispatch_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    VkDescriptorSet material_{};
    std::string material_name_;
    int blend_mode_{-1};
    int alpha_ref_{128};
    std::function<void(bool, PAPI::Particle&)> events_;
    struct Buffers
    {
        BufferResource vertices, indices;
        size_t capacity{};
    };
    std::array<Buffers, FrameContext::FramesInFlight> buffers_;
};

class VulkanParticleGroup final : public IRenderVisual, public IParticleCustom
{
public:
    VulkanParticleGroup(std::shared_ptr<const ParticleCatalog> catalog, const ParticleGroupDef& def);
    ~VulkanParticleGroup() override;
    bool initialize(VkDevice device, const VkPhysicalDeviceMemoryProperties& memory,
        const BufferResourceDispatch& dispatch, GameTextureFactory& textures, DeferredPass& pass,
        std::string& error);
    const auto& effects() const { return effects_; }
    size_t triggered_count() const { return triggered_.size(); }
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass, const float (&mvp)[16],
        const Fvector& right, const Fvector& up, bool hud, std::string& error);
    u32 getType() const override { return 9; }
    vis_data& getVisData() override { return visibility_; }
    IRenderVisual* getSubModel(u8 index) override
    { return index < effects_.size() ? effects_[index].get() : nullptr; }
    IParticleCustom* dcast_ParticleCustom() override { return this; }
    void OnDeviceCreate() override {}
    void OnDeviceDestroy() override;
    void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL transform) override;
    void OnFrame(u32 dt) override;
    void Play() override;
    void Stop(BOOL deferred = TRUE) override;
    BOOL IsPlaying() override { return playing_; }
    u32 ParticlesCount() override;
    float GetTimeLimit() override { return def_->time_limit; }
    const shared_str Name() override { return def_->name.c_str(); }
    void SetHudMode(BOOL value) override;
    BOOL GetHudMode() override { return hud_; }
#ifdef DEBUG
    shared_str getDebugName() override { return def_->name.c_str(); }
#endif

private:
    void spawn(const std::string& name, const Fvector& position, const Fvector& velocity);
    void on_particle(size_t index, bool birth, PAPI::Particle& particle);
    std::shared_ptr<const ParticleCatalog> catalog_;
    const ParticleGroupDef* def_{};
    std::vector<std::unique_ptr<VulkanParticleEffect>> effects_;
    struct Triggered
    {
        std::string name;
        std::unique_ptr<VulkanParticleEffect> effect;
    };
    struct Pending
    {
        std::string name;
        Fvector position, velocity;
    };
    std::vector<Triggered> triggered_;
    std::vector<Pending> pending_;
    Fmatrix parent_;
    bool transform_parent_{};
    VkDevice device_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    BufferResourceDispatch dispatch_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    vis_data visibility_;
    Fvector initial_position_;
    float time_{};
    bool playing_{}, deferred_{}, hud_{};
};
} // namespace xray::render::vulkan
