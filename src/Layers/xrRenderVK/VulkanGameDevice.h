#pragma once

#include "DeferredFrame.h"
#include "DeferredShaderFactory.h"
#include "GameTextureFactory.h"
#include "VulkanUIRender.h"
#include "VulkanUIShader.h"
#include "VulkanWindowDevice.h"
#include "GpuModel.h"
#include "VulkanFontRender.h"

#include <array>
#include <memory>

namespace xray::render::vulkan
{
class VulkanParticleEffect;
class VulkanParticleGroup;
// Owns the Vulkan gameplay frame resources. Destruction waits for submitted
// work before releasing descriptors, pipelines, images and the device.
class VulkanGameDevice
{
public:
    bool initialize(SDL_Window* window, VkExtent2D extent, std::string& error);
    void begin_frame();
    void queue_weather(VkDescriptorSet set, const WeatherLighting& lighting);
    bool render(const GpuLevel& level, const float (&mvp)[16],
        const DeferredLight& light, FrameStatus& status, std::string& error,
        bool render_world = true, bool clear_target = false);
    bool recreate_swapchain(VkExtent2D extent, std::string& error, bool recreate_surface = false);
    bool prepare_for_reset(std::string& error);
    bool wait_idle() { return window_.frame().wait_idle();
    }

    void queue_model(GpuModel& model, IKinematics* skeleton, const float (&mvp)[16], bool hud = false,
        float sort_distance = 0.f, const void* instance = nullptr, float lod = 1.f);
    void queue_level_visual(uint32_t index, const float (&mvp)[16],
        bool hud = false, float sort_distance = 0.f, const void* instance = nullptr, float lod = 1.f);
    void queue_particle(IRenderVisual* visual, const float (&mvp)[16],
        const Fvector& right, const Fvector& up, bool hud, float distance);
    void discard_scene_draws();
    void discard_model_draws(const void* instance);
    void use_scene_visibility(bool enabled) { scene_visibility_ = enabled; }
    void destroy();
    bool reset_required() const { return reset_required_; }
    bool device_lost() const { return window_.frame().device_lost(); }
    bool surface_lost() const { return window_.frame().surface_lost(); }
    void clear_reset_required() { reset_required_ = false; }
    ~VulkanGameDevice() { destroy(); }

    VulkanUIRender& ui() { return ui_; }
    std::unique_ptr<VulkanUIShader> create_ui_shader()
    {
        return std::make_unique<VulkanUIShader>(textures_, ui_pass_);
    }
    std::unique_ptr<VulkanFontRender> create_font_render()
    {
        return std::make_unique<VulkanFontRender>(*this);
    }
    GameTextureFactory& textures() { return textures_; }
    DeferredPass& deferred() { return deferred_; }
    ScenePass& ui_pass() { return ui_pass_; }
    VulkanWindowDevice& window() { return window_; }
    const BufferUploadDispatch& buffer_upload() const { return buffer_upload_; }
    PFN_vkDeviceWaitIdle device_wait_idle_proc() const { return frame_dispatch_.device_wait_idle; }

private:
    static void record_ui(const FrameRecordingContext& frame, void* user);
    static void record_hud(const FrameRecordingContext& frame, void* user);
    static void record_models(const FrameRecordingContext& frame, void* user);
    static void record_transparent(const FrameRecordingContext& frame, void* user);
    static void record_level_visuals(const FrameRecordingContext& frame, void* user);
    struct LevelDraw
    {
        uint32_t index{};
        std::array<float, 16> mvp{};
        float sort_distance{};
        float lod{1.f};
        bool hud{};
        const void* instance{};
    };
    struct ModelDraw
    {
        GpuModel* model{};
        const void* instance{};
        IKinematics* skeleton{};
        std::array<float, 16> mvp{};
        float sort_distance{};
        float lod{1.f};
        bool hud{};
    };
    struct ParticleDraw
    {
        IRenderVisual* visual{};
        std::array<float, 16> mvp{};
        Fvector right{}, up{};
        float sort_distance{};
        bool hud{};
    };
    VulkanWindowDevice window_;
    FrameDispatch frame_dispatch_{};
    TextureUploadDispatch texture_dispatch_{};
    BufferUploadDispatch buffer_upload_{};
    GBufferTargets targets_;
    DeferredPass deferred_;
    ScenePass ui_pass_;
    GameTextureFactory textures_;
    VulkanUIRender ui_;
    DeferredFrame frame_;
    bool ui_recorded_{true};
    std::string ui_error_;
    std::vector<ModelDraw> model_draws_;
    std::vector<ParticleDraw> particle_draws_;
    std::vector<LevelDraw> level_draws_;
    const GpuLevel* current_level_{};
    bool scene_visibility_{};
    bool level_recorded_{true};
    bool models_recorded_{true};
    std::string model_error_;
    PFN_vkCreateSampler create_sampler_{};
    PFN_vkDestroySampler destroy_sampler_{};
    bool reset_required_{};
    VkDescriptorSet weather_set_{};
    WeatherLighting weather_lighting_{};
};
}
