#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler
{

struct PendingQuery
{
    shared_str name;
    nvrhi::TimerQueryHandle query;
    u32 frameSubmitted = 0;
    bool resolved = false;
};

class GPUProfiler
{
public:
    GPUProfiler();
    ~GPUProfiler();

    void Initialize(nvrhi::IDevice* device);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsProfilingEnabled() const { return m_enabled; }

    void BeginPass(nvrhi::ICommandList* cmdList, const char* name);
    void EndPass(nvrhi::ICommandList* cmdList, const char* name);

    void FrameStart();
    void FrameEnd();

    void ResolvePendingQueries();

    const xr_vector<GPUPassTiming>& GetPassTimings() const { return m_passTimings; }
    float GetTotalGPUTimeMs() const { return m_totalGPUTimeMs; }

    bool IsInitialized() const { return m_device != nullptr; }

private:
    nvrhi::TimerQueryHandle AcquireTimerQuery();
    void ReleaseTimerQuery(nvrhi::TimerQueryHandle query);

private:
    nvrhi::IDevice* m_device = nullptr;

    xr_vector<nvrhi::TimerQueryHandle> m_queryPool;
    xr_vector<nvrhi::TimerQueryHandle> m_freeQueries;
    static constexpr u32 INITIAL_POOL_SIZE = 64;

    struct ActivePass
    {
        shared_str name;
        nvrhi::TimerQueryHandle query;
    };
    xr_vector<ActivePass> m_activePasses;

    xr_vector<PendingQuery> m_pendingQueries;
    static constexpr u32 MAX_PENDING_FRAMES = 4;

    xr_vector<GPUPassTiming> m_passTimings;
    float m_totalGPUTimeMs = 0.0f;

    u32 m_currentFrame = 0;

    bool m_enabled = false;
};

class GPUPassScope
{
public:
    GPUPassScope(GPUProfiler* profiler, nvrhi::ICommandList* cmdList, const char* name);
    ~GPUPassScope();

    GPUPassScope(const GPUPassScope&) = delete;
    GPUPassScope& operator=(const GPUPassScope&) = delete;

private:
    GPUProfiler* m_profiler;
    nvrhi::ICommandList* m_cmdList;
    const char* m_name;
};

}
