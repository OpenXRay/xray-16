#include "xrEngine/stdafx.h"
#include "DeferredFrame.h"

#include <cstring>

namespace xray::render::vulkan
{
void DeferredFrame::geometry(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    if (context.shadows_ && context.render_world_)
    {
        FrameRecordingContext shadow_frame;
        std::string error;
        if (!context.shadows_->begin(frame, context.shadow_uniform_, shadow_frame, error))
        { context.recorded_ = false; return; }
        context.recorded_ &= context.level_->record_sun_shadow(shadow_frame, *context.pass_,
            context.shadow_uniform_.sun_view_projection);
        context.shadows_->end(frame.command_buffer);
    }
    if (context.local_shadows_ && context.render_world_ && context.local_lights_ &&
        !context.local_lights_->empty())
    {
        context.recorded_ &= context.local_shadows_->initialize_layers(frame);
        for (uint32_t i = 0; i < context.local_lights_->size(); ++i)
        {
            const LocalLightUniform& light = (*context.local_lights_)[i];
            std::string error;
            if (!context.local_shadows_->write_light(frame.image_index, i, light, error))
            { context.recorded_ = false; break; }
            if (light.shadow_params[2] < .5f) continue;
            const uint32_t slot = uint32_t(light.shadow_params[0]) / LocalShadowFaces;
            const uint32_t faces = light.color_type[3] >= .5f ? 1 : LocalShadowFaces;
            for (uint32_t face = 0; face < faces; ++face)
            {
                FrameRecordingContext shadow_frame;
                if (!context.local_shadows_->begin_face(frame, slot, face, shadow_frame))
                { context.recorded_ = false; break; }
                context.recorded_ &= context.level_->record_sun_shadow(shadow_frame,
                    *context.pass_, light.shadow_matrices[face]);
                context.local_shadows_->end(frame.command_buffer);
            }
            if (!context.recorded_) break;
        }
    }
    FrameRecordingContext geometry_frame;
    if (!context.targets_->begin(frame, geometry_frame))
    {
        context.recorded_ = false;
        return;
    }
    float transform[16];
    std::memcpy(transform, context.mvp_, sizeof(transform));
    if (context.render_world_)
    {
        if (context.level_visuals_)
            context.level_visuals_(geometry_frame, context.level_data_);
        else
            context.recorded_ &= context.level_->record(geometry_frame, *context.pass_, transform,
                GeometryPhase::OpaqueAndAlphaTest);
        if (context.models_) context.models_(geometry_frame, context.models_data_);
    }
    context.targets_->end(frame.command_buffer);
    context.recorded_ &= context.targets_->copy_depth(frame);
}

void DeferredFrame::lighting(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    context.recorded_ &= context.pass_->record_lighting(frame,
        context.targets_->lighting_set(frame.image_index), context.light_,
        context.weather_set_, context.weather_set_ ? &context.weather_ : nullptr);
    if (context.render_world_ && context.local_lighting_)
        context.local_lighting_(frame, context.local_data_);
    if (!context.water_) overlay(frame, user_data);
}

void DeferredFrame::capture(VkCommandBuffer command, VkImage image,
    uint32_t index, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    std::string error;
    context.recorded_ &= context.water_->write_uniform(index, context.water_uniform_, error) &&
        context.water_->capture(command, image, index);
}

void DeferredFrame::overlay(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    if (context.render_world_)
    {
        if (context.transparent_) context.transparent_(frame, context.transparent_data_);
        else if (!context.level_visuals_)
        {
            float transform[16];
            std::memcpy(transform, context.mvp_, sizeof(transform));
            context.recorded_ &= context.level_->record(frame, *context.pass_, transform,
                GeometryPhase::Transparent);
        }
    }
    // The HUD uses its own depth hierarchy and is never hidden by world geometry.
    context.frame_->clear_depth(frame.command_buffer);
    if (context.hud_) context.hud_(frame, context.hud_data_);
    if (context.ui_) context.ui_(frame, context.ui_data_);
}

bool DeferredFrame::render(FrameContext& frame, GBufferTargets& targets, const GpuLevel& level,
    const DeferredPass& pass, const float (&mvp)[16], const DeferredLight& light,
    FrameStatus& status, std::string& error, FrameRecorder ui, void* ui_data,
    FrameRecorder hud, void* hud_data,
    FrameRecorder models, void* models_data,
    FrameRecorder transparent, void* transparent_data,
    FrameRecorder level_visuals, void* level_data, bool render_world, bool clear_target,
    VkDescriptorSet weather_set, const WeatherLighting* weather,
    FrameReadbackRecorder readback, void* readback_data,
    FrameRecorder compositor, void* compositor_data,
    SunShadowTargets* shadows, const SunShadowUniform* shadow_uniform,
    LocalShadowTargets* local_shadows, const std::vector<LocalLightUniform>* local_lights,
    FrameRecorder local_lighting, void* local_data,
    WaterTargets* water, const WaterSceneUniform* water_uniform)
{
    targets_ = &targets;
    shadows_ = shadows;
    if (shadow_uniform) shadow_uniform_ = *shadow_uniform;
    local_shadows_ = local_shadows;
    local_lights_ = local_lights;
    local_lighting_ = local_lighting;
    local_data_ = local_data;
    water_ = water;
    if (water_uniform) water_uniform_ = *water_uniform;
    frame_ = &frame;
    level_ = &level;
    pass_ = &pass;
    mvp_ = mvp;
    light_ = light;
    weather_set_ = weather_set;
    if (weather) weather_ = *weather;
    ui_ = ui;
    ui_data_ = ui_data;
    hud_ = hud;
    hud_data_ = hud_data;
    models_ = models;
    models_data_ = models_data;
    transparent_ = transparent;
    transparent_data_ = transparent_data;
    level_visuals_ = level_visuals;
    level_data_ = level_data;
    render_world_ = render_world;
    recorded_ = true;
    compositor_ = compositor;
    compositor_data_ = compositor_data;
    const VkClearColorValue clear{{0, 0, 0, 1}};
    if (!frame.render_frame(clear, status, error, lighting, this,
            readback, readback_data, geometry, this, clear_target,
            compositor_, compositor_data_,
            water_ ? capture : nullptr, this, water_ ? overlay : nullptr, this)) return false;
    if (!recorded_)
    {
        error = "Vulkan deferred level command recording failed";
        return false;
    }
    error.clear();
    return true;
}
}
