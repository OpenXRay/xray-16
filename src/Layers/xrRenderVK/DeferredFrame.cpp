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
            context.recorded_ &= context.level_->record(geometry_frame, *context.pass_, transform);
        if (context.models_) context.models_(geometry_frame, context.models_data_);
    }
    context.targets_->end(frame.command_buffer);
}

void DeferredFrame::lighting(const FrameRecordingContext& frame, void* user_data)
{
    auto& context = *static_cast<DeferredFrame*>(user_data);
    context.recorded_ &= context.pass_->record_lighting(frame,
        context.targets_->lighting_set(frame.image_index), context.light_);
    if (context.ui_) context.ui_(frame, context.ui_data_);
}

bool DeferredFrame::render(FrameContext& frame, GBufferTargets& targets, const GpuLevel& level,
    const DeferredPass& pass, const float (&mvp)[16], const DeferredLight& light,
    FrameStatus& status, std::string& error, FrameRecorder ui, void* ui_data,
    FrameRecorder models, void* models_data,
    FrameRecorder level_visuals, void* level_data, bool render_world, bool clear_target)
{
    targets_ = &targets;
    level_ = &level;
    pass_ = &pass;
    mvp_ = mvp;
    light_ = light;
    ui_ = ui;
    ui_data_ = ui_data;
    models_ = models;
    models_data_ = models_data;
    level_visuals_ = level_visuals;
    level_data_ = level_data;
    render_world_ = render_world;
    recorded_ = true;
    const VkClearColorValue clear{{0, 0, 0, 1}};
    if (!frame.render_frame(clear, status, error, lighting, this,
            nullptr, nullptr, geometry, this, clear_target)) return false;
    if (!recorded_)
    {
        error = "Vulkan deferred level command recording failed";
        return false;
    }
    error.clear();
    return true;
}
}
