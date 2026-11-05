#pragma once

#include "GBufferTargets.h"
#include "GpuLevel.h"
#include "SunShadowTargets.h"
#include "LocalShadowTargets.h"
#include "WaterTargets.h"

namespace xray::render::vulkan
{
using ShadowRecorder = void (*)(const FrameRecordingContext &, const float (&)[16], void *);
// Connects the offscreen G-buffer, level draws, fullscreen lighting, and an
// optional UI recorder to a single FrameContext submission and present.
class DeferredFrame
{
  public:
    void set_distortion_recorder(FrameRecorder recorder, void *user)
    {
        distortion_ = recorder;
        distortion_data_ = user;
    }
    void set_shadow_recorder(ShadowRecorder recorder, void *user)
    {
        shadow_models_ = recorder;
        shadow_models_data_ = user;
    }
    void set_light_grid_recorder(FrameRecorder recorder, void *user)
    {
        light_grid_ = recorder;
        light_grid_data_ = user;
    }
    bool render(FrameContext &frame, GBufferTargets &targets, const GpuLevel &level, const DeferredPass &pass,
                const float (&mvp)[16], const DeferredLight &light, FrameStatus &status, std::string &error,
                FrameRecorder ui = nullptr, void *ui_data = nullptr, FrameRecorder hud = nullptr,
                void *hud_data = nullptr, FrameRecorder models = nullptr, void *models_data = nullptr,
                FrameRecorder transparent = nullptr, void *transparent_data = nullptr,
                FrameRecorder level_visuals = nullptr, void *level_data = nullptr, bool render_world = true,
                bool clear_target = false, VkDescriptorSet weather_set = VK_NULL_HANDLE,
                const WeatherLighting *weather = nullptr, FrameReadbackRecorder readback = nullptr,
                void *readback_data = nullptr, FrameRecorder compositor = nullptr, void *compositor_data = nullptr,
                SunShadowTargets *shadows = nullptr, const SunShadowUniform *shadow_uniform = nullptr,
                LocalShadowTargets *local_shadows = nullptr,
                const std::vector<LocalLightUniform> *local_lights = nullptr, FrameRecorder local_lighting = nullptr,
                void *local_data = nullptr, WaterTargets *water = nullptr,
                const WaterSceneUniform *water_uniform = nullptr, FrameInterpassRecorder final_capture = nullptr,
                void *final_capture_data = nullptr, FrameRecorder final_overlay = nullptr,
                void *final_overlay_data = nullptr);

  private:
    FrameRecorder light_grid_{};
    void *light_grid_data_{};
    ShadowRecorder shadow_models_{};
    void *shadow_models_data_{};
    static void geometry(const FrameRecordingContext &frame, void *user_data);
    static void lighting(const FrameRecordingContext &frame, void *user_data);
    static void overlay(const FrameRecordingContext &frame, void *user_data);
    static void foreground(const FrameRecordingContext &frame, void *user_data);
    static void distortion_overlay(const FrameRecordingContext &frame, void *user_data);
    static void capture_distortion(VkCommandBuffer command, VkImage image, uint32_t index, void *user_data);
    static void capture(VkCommandBuffer command, VkImage image, uint32_t index, void *user_data);
    GBufferTargets *targets_{};
    SunShadowTargets *shadows_{};
    SunShadowUniform shadow_uniform_{};
    LocalShadowTargets *local_shadows_{};
    const std::vector<LocalLightUniform> *local_lights_{};
    FrameRecorder local_lighting_{};
    void *local_data_{};
    WaterTargets *water_{};
    WaterSceneUniform water_uniform_{};
    FrameContext *frame_{};
    const GpuLevel *level_{};
    const DeferredPass *pass_{};
    const float *mvp_{};
    DeferredLight light_{};
    VkDescriptorSet weather_set_{};
    WeatherLighting weather_{};
    FrameRecorder ui_{};
    void *ui_data_{};
    FrameRecorder hud_{};
    void *hud_data_{};
    FrameRecorder models_{};
    void *models_data_{};
    FrameRecorder transparent_{};
    FrameRecorder distortion_{};
    void *distortion_data_{};
    void *transparent_data_{};
    FrameRecorder level_visuals_{};
    void *level_data_{};
    bool render_world_{true};
    bool recorded_{};
    FrameRecorder compositor_{};
    void *compositor_data_{};
};
} // namespace xray::render::vulkan
