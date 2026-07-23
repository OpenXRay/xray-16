#include "stdafx.h"
#include "Profiler.h"

#include <thread>
#include <chrono>

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
#if XRAY_TRACY_ENABLED
    // Stop Tracy worker before static teardown — otherwise libc++ throws
    // "mutex lock failed: Invalid argument" when joining a destroyed mutex.
    tracy::GetProfiler().RequestShutdown();
    for (int i = 0; i < 200 && !tracy::GetProfiler().HasShutdownFinished(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
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
