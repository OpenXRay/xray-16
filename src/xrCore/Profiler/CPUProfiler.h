#pragma once

#include "ProfilerTypes.h"
#include "../FTimer.h"
#include "../Threading/Lock.hpp"

#include <atomic>
#include <chrono>

namespace xray::profiler
{

class ThreadZoneStack
{
public:
    void Push(u32 zoneId);
    u32 Pop();
    u32 CurrentParent() const;
    bool Empty() const { return m_depth == 0; }

private:
    static constexpr u32 MAX_DEPTH = 128;
    u32 m_stack[MAX_DEPTH] = {};
    u32 m_depth = 0;
};

class XRCORE_API CPUProfiler
{
public:
    CPUProfiler();
    ~CPUProfiler();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetThrottleInterval(u32 interval) { m_throttleInterval = (interval > 0) ? interval : 1; }
    u32 GetThrottleInterval() const { return m_throttleInterval; }

    u32 RegisterZone(const ZoneInfo* info);

    void BeginZone(u32 zoneId);
    void EndZone(u32 zoneId, float elapsedMs,
        u64 allocCalls, u64 allocBytes, u64 freeCalls, u64 freeBytes);

    void FrameStart();
    void FrameEnd();

    const xr_vector<ZoneData>& GetZones() const { return m_displayZones; }
    const xr_vector<u32>& GetRootZones() const { return m_displayRootZones; }
    float GetFrameTimeMs() const { return m_displayFrameTimeMs; }

    static CPUProfiler& Instance();

private:
    void ComputeSelfTimes(xr_vector<ZoneData>& zones);
    void BuildHierarchy(const xr_vector<ZoneData>& zones, xr_vector<u32>& rootZones);
    void CopyToDisplayBuffer();
    ThreadZoneStack& GetThreadStack();

private:
    xr_vector<ZoneData> m_zones;
    xr_vector<u32> m_rootZones;
    Lock m_registryLock;
    std::atomic<u32> m_nextZoneId{0};

    xr_vector<ZoneData> m_displayZones;
    xr_vector<u32> m_displayRootZones;
    float m_displayFrameTimeMs = 0.0f;

    CTimerBase m_frameTimer;
    float m_frameTimeMs = 0.0f;

    Lock m_zoneLock;

    bool m_enabled = false;

    u32 m_throttleInterval = 30;
};

class XRCORE_API CPUZoneScope
{
public:
    explicit CPUZoneScope(const ZoneInfo* info);
    ~CPUZoneScope();

    CPUZoneScope(const CPUZoneScope&) = delete;
    CPUZoneScope& operator=(const CPUZoneScope&) = delete;

private:
    u32 m_zoneId;
    CTimerBase::Time m_startTime;
    u64 m_allocCalls0;
    u64 m_allocBytes0;
    u64 m_freeCalls0;
    u64 m_freeBytes0;
};

}
