#pragma once

#include "GBufferTargets.h"
#include "GpuLevel.h"

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
        FrameRecorder ui = nullptr, void* ui_data = nullptr);

private:
    static void geometry(const FrameRecordingContext& frame, void* user_data);
    static void lighting(const FrameRecordingContext& frame, void* user_data);
    GBufferTargets* targets_{};
    const GpuLevel* level_{};
    const DeferredPass* pass_{};
    const float* mvp_{};
    DeferredLight light_{};
    FrameRecorder ui_{};
    void* ui_data_{};
    bool recorded_{};
};
}
