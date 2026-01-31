#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"

namespace xray::profiler
{

// ImGui-based stats overlay for displaying profiling data
class StatsOverlay
{
public:
    StatsOverlay();
    ~StatsOverlay();

    // Set GPU profiler reference (optional - for GPU timing display)
    void SetGPUProfiler(GPUProfiler* gpuProfiler) { m_gpuProfiler = gpuProfiler; }

    // Render the overlay (call during ImGui pass)
    void Render();

    // Visibility control
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void ToggleVisible() { m_visible = !m_visible; }

private:
    void RenderCPUSection();
    void RenderGPUSection();
    void RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime);

    // Helper to format time with appropriate precision
    // Use different slot values (0-3) when calling multiple times in one statement
    static const char* FormatTime(float ms, int slot = -1);
    static u32 GetTimeColor(float ms, float parentMs);

private:
    GPUProfiler* m_gpuProfiler = nullptr;
    bool m_visible = false;

    // UI state
    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
};

} // namespace xray::profiler
