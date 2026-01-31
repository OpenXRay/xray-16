#pragma once

#include "ProfilerTypes.h"
#include "../FTimer.h"
#include "../Threading/Lock.hpp"

#include <atomic>
#include <chrono>

namespace xray::profiler
{

// Thread-local zone stack for building hierarchy
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

// CPU Profiler - manages zone registration and timing
class XRCORE_API CPUProfiler
{
public:
    CPUProfiler();
    ~CPUProfiler();

    // Enable/disable profiling (disabled = no overhead)
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // Throttle interval: profile every N frames (1 = every frame, higher = less overhead)
    void SetThrottleInterval(u32 interval) { m_throttleInterval = (interval > 0) ? interval : 1; }
    u32 GetThrottleInterval() const { return m_throttleInterval; }

    // Zone registration (thread-safe, called once per source location)
    u32 RegisterZone(const ZoneInfo* info);

    // Zone timing (called per invocation)
    void BeginZone(u32 zoneId);
    void EndZone(u32 zoneId, float elapsedMs);

    // Frame lifecycle
    void FrameStart();
    void FrameEnd();

    // Access results - returns PREVIOUS frame's data for display
    // (current frame is still being collected)
    const xr_vector<ZoneData>& GetZones() const { return m_displayZones; }
    const xr_vector<u32>& GetRootZones() const { return m_displayRootZones; }
    float GetFrameTimeMs() const { return m_displayFrameTimeMs; }

    // Singleton access
    static CPUProfiler& Instance();

private:
    void ComputeSelfTimes(xr_vector<ZoneData>& zones);
    void BuildHierarchy(const xr_vector<ZoneData>& zones, xr_vector<u32>& rootZones);
    void CopyToDisplayBuffer();
    ThreadZoneStack& GetThreadStack();

private:
    // Zone registry (grows, never shrinks) - shared between frames
    xr_vector<ZoneData> m_zones;           // Current frame being collected
    xr_vector<u32> m_rootZones;            // Current frame root zones
    Lock m_registryLock;
    std::atomic<u32> m_nextZoneId{0};

    // Display buffer (previous frame's finalized data)
    xr_vector<ZoneData> m_displayZones;
    xr_vector<u32> m_displayRootZones;
    float m_displayFrameTimeMs = 0.0f;

    // Frame timing
    CTimerBase m_frameTimer;
    float m_frameTimeMs = 0.0f;

    // Thread-local stacks (indexed by thread ID)
    // Using simple approach: map of thread ID -> stack
    xr_map<u32, ThreadZoneStack> m_threadStacks;
    Lock m_stackLock;

    // Lock for zone data modifications (BeginZone/EndZone)
    Lock m_zoneLock;

    // Enabled flag (when false, all operations are no-ops)
    bool m_enabled = false;

    // Throttle interval: profile every N frames (default 30 = ~2% overhead)
    u32 m_throttleInterval = 30;
};

// RAII scope for CPU zone timing
class XRCORE_API CPUZoneScope
{
public:
    explicit CPUZoneScope(const ZoneInfo* info);
    ~CPUZoneScope();

    // Non-copyable
    CPUZoneScope(const CPUZoneScope&) = delete;
    CPUZoneScope& operator=(const CPUZoneScope&) = delete;

private:
    u32 m_zoneId;
    CTimerBase::Time m_startTime;
};

} // namespace xray::profiler
