#include "stdafx.h"
#include "Profiler.h"

namespace xray::profiler
{

static bool g_enabled = true;

void Initialize()
{
    // CPUProfiler is lazily initialized via Instance()
    Msg("* [Profiler] Initialized");
}

void Shutdown()
{
    // Note: CPUProfiler singleton cleanup happens at program exit
    Msg("* [Profiler] Shutdown");
}

void FrameStart()
{
    if (!g_enabled)
        return;

    CPUProfiler::Instance().FrameStart();
}

void FrameEnd()
{
    if (!g_enabled)
        return;

    CPUProfiler::Instance().FrameEnd();
}

CPUProfiler& GetCPUProfiler()
{
    return CPUProfiler::Instance();
}

bool IsEnabled()
{
    return g_enabled;
}

void SetEnabled(bool enabled)
{
    g_enabled = enabled;
    // Also update the CPUProfiler's enabled state
    CPUProfiler::Instance().SetEnabled(enabled);
}

} // namespace xray::profiler
