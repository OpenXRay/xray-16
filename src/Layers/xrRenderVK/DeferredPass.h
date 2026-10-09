#pragma once

#include "FrameContext.h"
#include "LevelModels.h"
#include "ScenePass.h"
#include "ForwardLighting.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace xray::render::vulkan
{
struct DeferredLight
{
    float direction_ambient[4];
    float color[4];
    float grade[4]{1.f, 1.f, 1.f, 0.f}; // gamma, brightness, contrast, hemisphere intensity
};

struct WeatherLighting
{
    float ray_base[4]{}, ray_dx[4]{}, ray_dy[4]{};
    DeferredLight light{};
    float sky_color[4]{};
    float clouds_color[4]{};
};
static_assert(sizeof(WeatherLighting) == 128);

struct DeferredEnvironment
{
    float sun_direction[3]{0.f, -1.f, 0.f};
    float sun_color[3]{1.f, 1.f, 1.f};
    float ambient_color[3]{};
    float hemi_color[3]{};
};

enum class GeometryPhase
{
    OpaqueAndAlphaTest,
    Transparent,
    Hud
};

DeferredLight make_environment_deferred_light(const DeferredEnvironment& environment);

// Framebuffer attachment order: albedo, normal, depth. Color images must
// support color attachment and sampling; depth must support attachment and
// sampling because the render pass preserves it in a read-only layout.
bool create_gbuffer_render_pass(VkDevice device, VkFormat albedo_format,
    VkFormat normal_format, VkFormat depth_format, const FrameDispatch& vk,
    VkRenderPass& result, std::string& error);

// Geometry pass expects two color attachments (RGBA albedo, encoded normal)
// and depth. Transparent geometry blends into the lit swapchain color, using
// the opaque scene depth; HUD follows with a fresh depth buffer.
// Attachments, render-pass transitions and per-frame synchronization belong to
// the caller.
class DeferredPass
{
public:
    ~DeferredPass() { destroy(); }
    DeferredPass(const DeferredPass&) = delete;
    DeferredPass& operator=(const DeferredPass&) = delete;
    DeferredPass() = default;

    bool initialize(VkDevice device, VkRenderPass geometry_pass, VkRenderPass light_pass,
        VkShaderModule geometry_vertex, VkShaderModule geometry_fragment,
        VkShaderModule alpha_test_fragment, VkShaderModule transparent_fragment,
        VkShaderModule light_vertex, VkShaderModule light_fragment,
        VkShaderModule weather_fragment,
        const ScenePassDispatch& dispatch, std::string& error,
        VkRenderPass shadow_pass = VK_NULL_HANDLE,
        VkShaderModule shadow_vertex = VK_NULL_HANDLE,
        VkShaderModule shadow_opaque = VK_NULL_HANDLE,
        VkShaderModule shadow_cutout = VK_NULL_HANDLE,
        VkRenderPass local_shadow_pass = VK_NULL_HANDLE,
        VkShaderModule local_light_fragment = VK_NULL_HANDLE,
        VkShaderModule water_fragment = VK_NULL_HANDLE);
    bool material(VkImageView albedo, VkSampler sampler, VkDescriptorSet& set, std::string& error);
    bool lightmapped_material(VkImageView albedo, VkImageView lightmap,
        VkSampler sampler, VkDescriptorSet& set, std::string& error);
    // A named SVS/SPS pair from the game's precompiled Vulkan shader set.
    // Unknown names and incompatible pass state fail before a draw is recorded.
    bool create_game_pipeline(const std::string &vertex_name, const std::string &fragment_name, VkShaderModule vertex, VkShaderModule fragment,
                              SurfaceMode mode, bool hud, std::string &error, bool skinned = false);
    bool has_game_pipeline(const std::string &vertex_name, const std::string &fragment_name) const;
    using GamePipelineRequest = std::function<bool(const std::string &, const std::string &, SurfaceMode, bool, bool, std::string &)>;
    void set_game_pipeline_request(GamePipelineRequest request)
    {
        game_pipeline_request_ = std::move(request);
    }
    bool request_game_pipeline(const std::string &vertex_name, const std::string &fragment_name, SurfaceMode mode, bool hud, bool skinned, std::string &error)
    {
        if (game_pipeline_request_)
            return game_pipeline_request_(vertex_name, fragment_name, mode, hud, skinned, error);
        return require_game_pipeline(vertex_name, fragment_name, mode, hud, skinned, error);
    }
    bool require_game_pipeline(const std::string &vertex_name, const std::string &fragment_name, SurfaceMode mode, bool hud, bool skinned,
                               std::string &error) const;
    void begin_game_pipeline_reload();
    void commit_game_pipeline_reload();
    void abort_game_pipeline_reload();
    bool pose_descriptor(VkBuffer pose, VkDeviceSize bytes, VkDescriptorSet& result, std::string& error);
    void release_pose_descriptor(VkDescriptorSet& set);
    bool record_skinned(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        VkDescriptorSet pose_set, SurfaceMode mode, bool hud, uint32_t first_index,
        const char* vertex_name, const char* fragment_name,
        float alpha_ref = 128.f / 255.f, const float* normal_rows = nullptr) const;
    void update_material(VkDescriptorSet set, VkImageView view, VkSampler sampler);
    void update_lightmapped_material(VkDescriptorSet set, VkImageView albedo,
        VkImageView lightmap, VkSampler sampler);
    bool gbuffer(VkImageView albedo, VkImageView normal, VkImageView depth, VkSampler sampler,
        VkDescriptorSet& set, std::string& error);
    void bind_sun_shadow(VkDescriptorSet set, VkImageView view, VkSampler sampler,
        VkBuffer uniform);
    bool local_light_set(VkImageView shadow_array, VkSampler sampler, VkBuffer uniform,
        VkDeviceSize offset, VkDeviceSize range, VkDescriptorSet& set, std::string& error);
    bool water_set(uint32_t image_index, VkImageView refraction, VkImageView reflection,
        VkImageView depth, VkSampler sampler, VkSampler depth_sampler, VkBuffer scene_uniform, std::string& error);
    void release_water_sets();
    bool weather_set(VkImageView sky_a, VkImageView sky_b,
        VkImageView clouds_a, VkImageView clouds_b, VkSampler sampler,
        VkDescriptorSet& set, std::string& error);
    void release_gbuffer(VkDescriptorSet& set);
    bool forward_set(uint32_t image, VkBuffer buffer, std::string& error);
    void release_forward_sets();
    void rebind_compatible_render_passes(VkRenderPass geometry_pass, VkRenderPass light_pass,
        VkRenderPass shadow_pass = VK_NULL_HANDLE, VkRenderPass local_shadow_pass = VK_NULL_HANDLE,
        VkRenderPass overlay_pass = VK_NULL_HANDLE)
    {
        geometry_pass_ = geometry_pass;
        light_pass_ = light_pass;
        overlay_pass_ = overlay_pass;
        if (shadow_pass) shadow_pass_ = shadow_pass;
        if (local_shadow_pass) local_shadow_pass_ = local_shadow_pass;
    }
    bool record_geometry(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        SurfaceMode mode = SurfaceMode::Opaque, uint32_t first_index = 0,
        const char* vertex_name = nullptr, const char* fragment_name = nullptr,
        float alpha_ref = 128.f / 255.f, const float* normal_rows = nullptr) const;
    bool record_sun_shadow(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material,
        bool alpha_test, uint32_t first_index = 0,
        float alpha_ref = 128.f / 255.f) const;
    bool record_hud(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        uint32_t first_index = 0, const char* vertex_name = nullptr,
        const char* fragment_name = nullptr, float alpha_ref = 0.f,
        const float* normal_rows = nullptr) const;
    bool record_transparent(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        uint32_t first_index = 0, const char* vertex_name = nullptr,
        const char* fragment_name = nullptr, const char** failure = nullptr,
        float alpha_ref = 0.f, const float* normal_rows = nullptr) const;
    bool record_lighting(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
        const DeferredLight& light, VkDescriptorSet weather_set = VK_NULL_HANDLE,
        const WeatherLighting* weather = nullptr) const;
    bool record_local_light(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
        VkDescriptorSet local_set, const VkRect2D& scissor) const;
    bool record_water(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material,
        uint32_t first_index, float time, float opacity = .72f) const;
    void destroy();
    void reset_draw_statistics() { draw_calls_ = triangles_ = 0; }
    uint32_t draw_calls() const { return draw_calls_; }
    uint32_t triangles() const { return triangles_; }

private:
    struct GamePipeline { VkPipeline handle{}; SurfaceMode mode{}; bool hud{}, skinned{}; };
    const GamePipeline* game_pipeline(const char* vertex_name, const char* fragment_name) const;
    bool allocate(VkDescriptorSetLayout layout, VkImageView first, VkImageView second,
        VkSampler sampler, VkDescriptorSet& set, std::string& error);
    VkDevice device_{};
    VkRenderPass geometry_pass_{}, light_pass_{}, shadow_pass_{}, local_shadow_pass_{}, overlay_pass_{};
    bool lighting_compatible(const FrameRecordingContext& frame) const
    { return frame.render_pass == light_pass_ || (overlay_pass_ && frame.render_pass == overlay_pass_); }
    VkPipeline geometry_{}, alpha_test_{}, transparent_{}, hud_{}, lighting_{}, weather_pipeline_{}, shadow_opaque_{}, shadow_cutout_{}, local_light_pipeline_{}, water_pipeline_{};
    std::unordered_map<std::string, GamePipeline> game_pipelines_;
    std::unordered_map<std::string, GamePipeline> pending_game_pipelines_;
    bool reloading_game_pipelines_{};
    VkPipelineLayout geometry_layout_{}, skinned_layout_{}, light_layout_{}, weather_layout_{}, local_light_layout_{}, water_layout_{};
    VkDescriptorSetLayout material_layout_{}, pose_layout_{}, gbuffer_layout_{}, weather_set_layout_{}, local_set_layout_{}, water_set_layout_{}, forward_layout_{};
    std::vector<VkDescriptorSet> water_sets_;
    std::vector<VkDescriptorSet> forward_sets_;
    VkDescriptorPool pool_{};
    ScenePassDispatch vk_{};
    GamePipelineRequest game_pipeline_request_;
    mutable uint32_t draw_calls_{}, triangles_{};
};
} // namespace xray::render::vulkan
