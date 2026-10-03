#pragma once

#include "GBufferTargets.h"
#include "GpuLevel.h"
#include "SunShadowTargets.h"
#include "LocalShadowTargets.h"
#include "WaterTargets.h"

namespace xray::render::vulkan
{
// Connects the offscreen G-buffer, level draws, fullscreen lighting, and an
// optional UI recorder to a single FrameContext submission and present.
class DeferredFrame
{
public:
    bool render(FrameContext& frame, GBufferTargets& targets, const GpuLevel& level,
        const DeferredPass& pass, const float (&mvp)[16], const DeferredLight& light,
        FrameStatus& status, std::string& error,
        FrameRecorder ui = nullptr, void* ui_data = nullptr,
        FrameRecorder hud = nullptr, void* hud_data = nullptr,
        FrameRecorder models = nullptr, void* models_data = nullptr,
        FrameRecorder transparent = nullptr, void* transparent_data = nullptr,
        FrameRecorder level_visuals = nullptr, void* level_data = nullptr,
        bool render_world = true, bool clear_target = false,
        VkDescriptorSet weather_set = VK_NULL_HANDLE,
        const WeatherLighting* weather = nullptr,
        FrameReadbackRecorder readback = nullptr, void* readback_data = nullptr,
        FrameRecorder compositor = nullptr, void* compositor_data = nullptr,
        SunShadowTargets* shadows = nullptr, const SunShadowUniform* shadow_uniform = nullptr,
        LocalShadowTargets* local_shadows = nullptr,
        const std::vector<LocalLightUniform>* local_lights = nullptr,
        FrameRecorder local_lighting = nullptr, void* local_data = nullptr,
        WaterTargets* water = nullptr, const WaterSceneUniform* water_uniform = nullptr);

private:
    static void geometry(const FrameRecordingContext& frame, void* user_data);
    static void lighting(const FrameRecordingContext& frame, void* user_data);
    static void overlay(const FrameRecordingContext& frame, void* user_data);
    static void capture(VkCommandBuffer command, VkImage image, uint32_t index, void* user_data);
    GBufferTargets* targets_{};
    SunShadowTargets* shadows_{};
    SunShadowUniform shadow_uniform_{};
    LocalShadowTargets* local_shadows_{};
    const std::vector<LocalLightUniform>* local_lights_{};
    FrameRecorder local_lighting_{};
    void* local_data_{};
    WaterTargets* water_{};
    WaterSceneUniform water_uniform_{};
    FrameContext* frame_{};
    const GpuLevel* level_{};
    const DeferredPass* pass_{};
    const float* mvp_{};
    DeferredLight light_{};
    VkDescriptorSet weather_set_{};
    WeatherLighting weather_{};
    FrameRecorder ui_{};
    void* ui_data_{};
    FrameRecorder hud_{};
    void* hud_data_{};
    FrameRecorder models_{};
    void* models_data_{};
    FrameRecorder transparent_{};
    void* transparent_data_{};
    FrameRecorder level_visuals_{};
    void* level_data_{};
    bool render_world_{true};
    bool recorded_{};
    FrameRecorder compositor_{};
    void* compositor_data_{};
};
}
