#pragma once

#include "ProfilerTypes.h"
#include "../FTimer.h"
#include "../Threading/Lock.hpp"
#include "../MemoryStats.h"
#include "xrCommon/xr_deque.h"

#include <atomic>
#include <chrono>

namespace xray::profiler
{

class ThreadZoneStack
{
public:
    struct Entry
    {
        u32 zoneId;
        u32 nodeId;
        u32 previousMemoryZone;
        u64 epoch;
        memstats::AllocationSpikeWatch allocationWatch;
    };

    bool Push(u32 zoneId, u32 nodeId, u64 epoch, u32 previousMemoryZone);
    Entry Pop();
    u32 CurrentNode(u64 epoch) const;
    bool Empty() const { return m_depth == 0; }
    memstats::AllocationSpikeWatch& CurrentAllocationWatch() { return m_stack[m_depth - 1].allocationWatch; }

private:
    static constexpr u32 MAX_DEPTH = 128;
    Entry m_stack[MAX_DEPTH] = {};
    u32 m_depth = 0;
};

class XRCORE_API CPUProfiler
{
public:
    CPUProfiler();
    ~CPUProfiler();

    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled.load(std::memory_order_acquire); }
    bool IsSamplingFrame() const { return m_captureEpoch.load(std::memory_order_acquire) != 0; }

    void SetThrottleInterval(u32 interval);
    u32 GetThrottleInterval() const { return m_throttleInterval.load(std::memory_order_relaxed); }

    u32 RegisterZone(const ZoneInfo* info);
    const ZoneInfo* RegisterDynamicZone(pcstr name);

    u64 BeginZone(u32 zoneId);
    void EndZone(u32 zoneId, u64 epoch, float elapsedMs,
        u64 allocCalls, u64 allocBytes, u64 freeCalls, u64 freeBytes);

    void FrameStart();
    void FrameEnd();
    void ResetTree();

    const xr_vector<ZoneData>& GetZones() const { return m_displayZones; }
    const xr_vector<u32>& GetRootZones() const { return m_displayRootZones; }
    float GetFrameTimeMs() const { return m_displayFrameTimeMs; }

    static CPUProfiler& Instance();

private:
    friend class CPUZoneScope;

    u32 FindOrCreateNode(u32 parentNode, u32 zoneId);
    void ComputeSelfTimes(xr_vector<ZoneData>& zones);
    void CopyToDisplayBuffer();
    ThreadZoneStack& GetThreadStack();

    xr_vector<const ZoneInfo*> m_infos;
    xr_vector<ZoneData> m_nodes;
    xr_deque<memstats::AllocationSpikeCounter> m_allocationSpikeCounters;
    xr_vector<u32> m_rootNodes;
    xr_map<shared_str, ZoneInfo*> m_dynamicZones;
    Lock m_zoneLock;

    xr_vector<ZoneData> m_displayZones;
    xr_vector<u32> m_displayRootZones;
    float m_displayFrameTimeMs = 0.0f;

    CTimerBase m_frameTimer;
    float m_frameTimeMs = 0.0f;
    std::atomic<bool> m_enabled{false};
    std::atomic<u32> m_throttleInterval{30};
    std::atomic<u64> m_captureEpoch{0};
    u64 m_nextEpoch = 0;
    std::atomic<bool> m_resetPending{false};
    std::atomic<u32> m_framesUntilSample{0};
};

class XRCORE_API CPUZoneScope
{
public:
    explicit CPUZoneScope(const ZoneInfo* info);
    ~CPUZoneScope();

    CPUZoneScope(const CPUZoneScope&) = delete;
    CPUZoneScope& operator=(const CPUZoneScope&) = delete;

private:
    u32 m_zoneId = INVALID_ZONE_ID;
    u64 m_epoch = 0;
    CTimerBase::Time m_startTime;
    u64 m_allocCalls0;
    u64 m_allocBytes0;
    u64 m_freeCalls0;
    u64 m_freeBytes0;
};

}
