#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler
{

// Ring buffer entry for pending GPU queries
struct PendingQuery
{
    shared_str name;
    nvrhi::TimerQueryHandle query;
    u32 frameSubmitted = 0;
    bool resolved = false;
};

// GPU Profiler - manages NVRHI timer queries for FrameGraph passes
class GPUProfiler
{
public:
    GPUProfiler();
    ~GPUProfiler();

    // Initialize with NVRHI device
    void Initialize(nvrhi::IDevice* device);
    void Shutdown();

    // Begin/End timing for a named pass
    void BeginPass(nvrhi::ICommandList* cmdList, const char* name);
    void EndPass(nvrhi::ICommandList* cmdList, const char* name);

    // Frame lifecycle
    void FrameStart();
    void FrameEnd();

    // Resolve pending queries (call after GPU work completes)
    void ResolvePendingQueries();

    // Access results
    const xr_vector<GPUPassTiming>& GetPassTimings() const { return m_passTimings; }
    float GetTotalGPUTimeMs() const { return m_totalGPUTimeMs; }

    // Check if initialized
    bool IsInitialized() const { return m_device != nullptr; }

private:
    nvrhi::TimerQueryHandle AcquireTimerQuery();
    void ReleaseTimerQuery(nvrhi::TimerQueryHandle query);

private:
    nvrhi::IDevice* m_device = nullptr;

    // Timer query pool
    xr_vector<nvrhi::TimerQueryHandle> m_queryPool;
    xr_vector<nvrhi::TimerQueryHandle> m_freeQueries;
    static constexpr u32 INITIAL_POOL_SIZE = 64;

    // Active passes this frame
    struct ActivePass
    {
        shared_str name;
        nvrhi::TimerQueryHandle query;
    };
    xr_vector<ActivePass> m_activePasses;

    // Pending queries from previous frames
    xr_vector<PendingQuery> m_pendingQueries;
    static constexpr u32 MAX_PENDING_FRAMES = 4;

    // Resolved timings for current frame display
    xr_vector<GPUPassTiming> m_passTimings;
    float m_totalGPUTimeMs = 0.0f;

    u32 m_currentFrame = 0;
};

// RAII scope for GPU pass timing
class GPUPassScope
{
public:
    GPUPassScope(GPUProfiler* profiler, nvrhi::ICommandList* cmdList, const char* name);
    ~GPUPassScope();

    // Non-copyable
    GPUPassScope(const GPUPassScope&) = delete;
    GPUPassScope& operator=(const GPUPassScope&) = delete;

private:
    GPUProfiler* m_profiler;
    nvrhi::ICommandList* m_cmdList;
    const char* m_name;
};

} // namespace xray::profiler
