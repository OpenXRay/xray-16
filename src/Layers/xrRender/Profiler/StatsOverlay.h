#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"

namespace xray::profiler
{

class StatsOverlay
{
public:
    StatsOverlay();
    ~StatsOverlay();

    void SetGPUProfiler(GPUProfiler* gpuProfiler) { m_gpuProfiler = gpuProfiler; }

    void Render();

    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void ToggleVisible() { m_visible = !m_visible; }

private:
    void RenderCPUSection();
    void RenderGPUSection();
    void RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime);

    static const char* FormatTime(float ms, int slot = -1);
    static u32 GetTimeColor(float ms, float parentMs);

private:
    GPUProfiler* m_gpuProfiler = nullptr;
    bool m_visible = false;

    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
};

}
