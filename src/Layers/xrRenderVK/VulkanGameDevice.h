#pragma once

#include "DeferredFrame.h"
#include "DeferredShaderFactory.h"
#include "GameTextureFactory.h"
#include "VulkanUIRender.h"
#include "VulkanUIShader.h"
#include "VulkanWindowDevice.h"
#include "GpuModel.h"
#include "VulkanFontRender.h"
#include "ScreenCopyPass.h"
#include "LocalShadowTargets.h"
#include "WaterTargets.h"

#include <array>
#include <memory>
#include <unordered_map>

namespace xray::render::vulkan
{
class VulkanParticleEffect;
class VulkanParticleGroup;
class VulkanRainRender;
class VulkanThunderboltRender;
// Owns the Vulkan gameplay frame resources. Destruction waits for submitted
// work before releasing descriptors, pipelines, images and the device.
class VulkanGameDevice
{
public:
    bool initialize(SDL_Window* window, VkExtent2D extent, std::string& error);
    void begin_frame();
    void queue_weather(VkDescriptorSet set, const WeatherLighting& lighting,
        const float (&fog_color)[3], float fog_near, float fog_far);
    void queue_lights(const std::vector<VulkanLightSnapshot>& lights) { light_snapshots_ = lights; }
    VkDescriptorSet glow_texture(const std::string& name);
    void release_level_glows();
    void queue_rain(VulkanRainRender& rain);
    void queue_thunderbolt(VulkanThunderboltRender& bolt);
    void discard_rain(const VulkanRainRender* rain);
    void discard_thunderbolt(const VulkanThunderboltRender* bolt);
    void request_screenshot() { screenshot_requested_ = true; }
    bool take_screenshot(std::vector<uint8_t>& pixels, VkExtent2D& extent, VkFormat& format);
    void set_postprocess(const PostProcessConstants& params, std::string first, std::string second);
    bool render(const GpuLevel& level, const float (&mvp)[16],
        const DeferredLight& light, FrameStatus& status, std::string& error,
        bool render_world = true, bool clear_target = false);
    bool recreate_swapchain(VkExtent2D extent, std::string& error, bool recreate_surface = false);
    bool prepare_for_reset(std::string& error);
    bool reload_game_shaders(std::string& error);
    bool wait_idle() { return window_.frame().device_lost() || window_.frame().wait_idle(); }

    void queue_model(GpuModel& model, IKinematics* skeleton, const float (&mvp)[16], bool hud = false,
        float sort_distance = 0.f, const void* instance = nullptr, float lod = 1.f,
        const Fmatrix* world = nullptr);
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
    uint32_t last_ui_draw_calls() const { return last_ui_draw_calls_; }
    uint32_t last_ui_triangles() const { return last_ui_triangles_; }
    std::unique_ptr<VulkanUIShader> create_ui_shader()
    {
        return std::make_unique<VulkanUIShader>(textures_, ui_pass_);
    }
    std::unique_ptr<VulkanFontRender> create_font_render()
    {
        return std::make_unique<VulkanFontRender>(*this);
    }
    GameTextureFactory &textures()
    {
        return textures_;
    }
    DeferredPass &deferred()
    {
        return deferred_;
    }
    GameShaderResources &shader_resources()
    {
        return shader_resources_;
    }
    bool request_shader_pair(const std::string &svs, const std::string &sps, SurfaceMode mode, bool hud, bool skinned, std::string &error)
    {
        return shader_resources_.pipeline(deferred_, svs, sps, mode, hud, skinned, error);
    }
    ScenePass &ui_pass()
    {
        return ui_pass_;
    }
    VulkanWindowDevice &window()
    {
        return window_;
    }
    const BufferUploadDispatch &buffer_upload() const
    {
        return buffer_upload_;
    }
    PFN_vkDeviceWaitIdle device_wait_idle_proc() const { return frame_dispatch_.device_wait_idle; }

private:
    static void record_ui(const FrameRecordingContext& frame, void* user);
    static void record_hud(const FrameRecordingContext& frame, void* user);
    static void record_models(const FrameRecordingContext& frame, void* user);
    static void record_transparent(const FrameRecordingContext& frame, void* user);
    static void record_local_lights(const FrameRecordingContext& frame, void* user);
    static void record_level_visuals(const FrameRecordingContext& frame, void* user);
    static void record_readback(VkCommandBuffer command, VkImage image, VkExtent2D extent, void* user);
    static void record_postprocess(const FrameRecordingContext& frame, void* user);
    bool create_postprocess(std::string& error);
    bool refresh_color_maps(std::string& error);
    bool create_forward_targets(std::string& error);
    void release_forward_targets();
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
        std::array<float, 12> normal_rows{};
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
#if defined(XR_PLATFORM_ANDROID)
    SDL_Window* scaled_android_window_{};
    VkExtent2D requested_android_extent_{};
#endif
    FrameDispatch frame_dispatch_{};
    TextureUploadDispatch texture_dispatch_{};
    BufferUploadDispatch buffer_upload_{};
    GBufferTargets targets_;
    WaterTargets water_targets_;
    SunShadowTargets sun_shadows_;
    LocalShadowTargets local_shadows_;
    std::vector<std::array<VkDescriptorSet, LocalLightCapacity>> local_sets_;
    std::vector<VulkanLightSnapshot> light_snapshots_;
    std::vector<LocalLightUniform> local_uniforms_;
    std::vector<VkRect2D> local_scissors_;
    std::vector<BufferResource> forward_buffers_;
    ForwardLightUniform forward_lighting_{};
    bool local_lights_recorded_{true};
    DeferredPass deferred_;
    GameShaderResources shader_resources_;
    ScenePass ui_pass_;
    GameTextureFactory textures_;
    std::unordered_map<std::string, VkDescriptorSet> glow_textures_;
    VulkanUIRender ui_;
    DeferredFrame frame_;
    std::vector<std::unique_ptr<ScreenCopyPass>> postprocess_passes_;
    PostProcessConstants postprocess_params_{};
    std::string color_map_names_[2];
    std::string active_map_names_[2];
    VkImageView color_map_views_[2]{};
    bool ui_recorded_{true};
    std::string ui_error_;
    std::vector<ModelDraw> model_draws_;
    std::vector<ParticleDraw> particle_draws_;
    std::vector<VulkanRainRender*> rain_draws_;
    std::vector<VulkanThunderboltRender*> thunderbolt_draws_;
    std::vector<LevelDraw> level_draws_;
    // A visual can be submitted through several sector roots. Look up only
    // submissions for the same visual instead of scanning every draw.
    std::unordered_multimap<uint32_t, size_t> level_draw_lookup_;
    const GpuLevel* current_level_{};
    std::array<float, 16> scene_mvp_{};
    bool scene_visibility_{};
    bool level_recorded_{true};
    std::string level_error_;
    bool models_recorded_{true};
    std::string model_error_;
    PFN_vkCreateSampler create_sampler_{};
    PFN_vkDestroySampler destroy_sampler_{};
    bool reset_required_{};
    VkDescriptorSet weather_set_{};
    WeatherLighting weather_lighting_{};
    BufferResource screenshot_buffer_;
    bool screenshot_requested_{}, screenshot_ready_{}, readback_enabled_{};
    uint32_t last_ui_draw_calls_{}, last_ui_triangles_{};
    VkExtent2D screenshot_extent_{};
};
}
