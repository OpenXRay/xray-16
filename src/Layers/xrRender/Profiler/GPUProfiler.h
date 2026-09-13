#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler
{

class GPUProfiler
{
public:
    GPUProfiler();
    ~GPUProfiler();

    void Initialize(nvrhi::IDevice* device);
    void Shutdown();

    void SetEnabled(bool enabled);
    bool IsProfilingEnabled() const { return m_enabled; }

    void BeginPass(nvrhi::ICommandList* cmdList, const char* name, bool isAsync = false);
    void EndPass(nvrhi::ICommandList* cmdList, const char* name);

    void FrameStart();
    void FrameEnd();

    void ResolvePendingQueries();

    const xr_vector<GPUPassTiming>& GetPassTimings() const { return m_passTimings; }
    float GetTotalGPUTimeMs() const { return m_totalGPUTimeMs; }
    u64 GetCompletedSampleId() const { return m_completedSampleId; }

    bool IsInitialized() const { return m_device != nullptr; }

private:
    nvrhi::TimerQueryHandle AcquireTimerQuery();
    void ReleaseTimerQuery(nvrhi::TimerQueryHandle query);
    void SealRecordingFrame();

private:
    nvrhi::IDevice* m_device = nullptr;

    xr_vector<nvrhi::TimerQueryHandle> m_queryPool;
    xr_vector<nvrhi::TimerQueryHandle> m_freeQueries;
    static constexpr u32 INITIAL_POOL_SIZE = 64;

    struct PendingQuery
    {
        GPUPassTiming timing;
        nvrhi::TimerQueryHandle query;
    };

    struct PendingFrame
    {
        xr_vector<PendingQuery> queries;
        u64 sampleId = 0;
        bool sealed = false;
        bool valid = true;
    };

    struct ActivePass
    {
        nvrhi::ICommandList* cmdList = nullptr;
        size_t queryIndex = 0;
    };
    xr_vector<ActivePass> m_activePasses;
    xr_vector<PendingFrame> m_pendingFrames;
    static constexpr size_t NO_RECORDING_FRAME = static_cast<size_t>(-1);
    size_t m_recordingFrame = NO_RECORDING_FRAME;

    xr_vector<GPUPassTiming> m_passTimings;
    float m_totalGPUTimeMs = 0.0f;
    u64 m_currentFrame = 0;
    u64 m_completedSampleId = 0;
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
