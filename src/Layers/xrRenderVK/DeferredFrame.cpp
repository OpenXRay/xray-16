#include "xrEngine/stdafx.h"
#include "DeferredFrame.h"

#include <cstring>

namespace xray::render::vulkan
{
void DeferredFrame::geometry(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
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
}

void DeferredFrame::lighting(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    context.recorded_ &= context.pass_->record_lighting(frame,
        context.targets_->lighting_set(frame.image_index), context.light_,
        context.weather_set_, context.weather_set_ ? &context.weather_ : nullptr);
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
    FrameReadbackRecorder readback, void* readback_data)
{
    targets_ = &targets;
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
    const VkClearColorValue clear{{0, 0, 0, 1}};
    if (!frame.render_frame(clear, status, error, lighting, this,
            readback, readback_data, geometry, this, clear_target)) return false;
    if (!recorded_)
    {
        error = "Vulkan deferred level command recording failed";
        return false;
    }
    error.clear();
    return true;
}
}
