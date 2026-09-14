#include "stdafx.h"
#include "CPUProfiler.h"

#include "../MemoryStats.h"

namespace xray::profiler
{

bool ThreadZoneStack::Push(u32 zoneId, u64 epoch, u32 previousMemoryZone)
{
    if (m_depth == MAX_DEPTH)
        return false;
    const u32 parentId = CurrentParent(epoch);
    m_stack[m_depth++] = {zoneId, parentId, previousMemoryZone, epoch};
    return true;
}

ThreadZoneStack::Entry ThreadZoneStack::Pop()
{
    if (m_depth != 0)
        return m_stack[--m_depth];
    return {INVALID_ZONE_ID, INVALID_ZONE_ID, INVALID_ZONE_ID, 0};
}

u32 ThreadZoneStack::CurrentParent(u64 epoch) const
{
    if (m_depth != 0 && m_stack[m_depth - 1].epoch == epoch)
        return m_stack[m_depth - 1].zoneId;
    return INVALID_ZONE_ID;
}

CPUProfiler::CPUProfiler()
{
    m_zones.reserve(512);
    m_rootZones.reserve(32);
    m_displayZones.reserve(512);
    m_displayRootZones.reserve(32);
}

CPUProfiler::~CPUProfiler() = default;

CPUProfiler& CPUProfiler::Instance()
{
    static CPUProfiler* const profiler = new CPUProfiler();
    return *profiler;
}

void CPUProfiler::SetEnabled(bool enabled)
{
    ScopeLock lock(&m_zoneLock);
    if (m_enabled.load(std::memory_order_relaxed) == enabled)
        return;
    m_captureEpoch.store(0, std::memory_order_release);
    m_framesUntilSample.store(0, std::memory_order_relaxed);
    m_enabled.store(enabled, std::memory_order_release);
}

void CPUProfiler::SetThrottleInterval(u32 interval)
{
    interval = interval != 0 ? interval : 1;
    ScopeLock lock(&m_zoneLock);
    if (m_throttleInterval.load(std::memory_order_relaxed) == interval)
        return;
    m_throttleInterval.store(interval, std::memory_order_relaxed);
    m_framesUntilSample.store(0, std::memory_order_relaxed);
}

u32 CPUProfiler::RegisterZone(const ZoneInfo* info)
{
    if (!info || !IsSamplingFrame())
        return INVALID_ZONE_ID;
    u32 id = info->id.load(std::memory_order_acquire);
    if (id != INVALID_ZONE_ID)
        return id;

    ScopeLock lock(&m_zoneLock);
    if (!IsSamplingFrame())
        return INVALID_ZONE_ID;
    id = info->id.load(std::memory_order_relaxed);
    if (id != INVALID_ZONE_ID)
        return id;

    id = static_cast<u32>(m_zones.size());
    m_zones.resize(id + 1);
    m_zones[id].info = info;
    info->id.store(id, std::memory_order_release);
    return id;
}

const ZoneInfo* CPUProfiler::RegisterDynamicZone(pcstr name)
{
    if (!name || !IsSamplingFrame())
        return nullptr;

    ScopeLock lock(&m_zoneLock);
    if (!IsSamplingFrame())
        return nullptr;
    shared_str key(name);
    auto it = m_dynamicZones.find(key);
    if (it != m_dynamicZones.end())
        return it->second;

    auto* info = xr_new<ZoneInfo>();
    info->name = key.c_str();
    info->file = "<dynamic>";
    info->line = 0;
    m_dynamicZones.emplace(key, info);
    return info;
}

ThreadZoneStack& CPUProfiler::GetThreadStack()
{
    static thread_local ThreadZoneStack stack;
    return stack;
}

u64 CPUProfiler::BeginZone(u32 zoneId)
{
    const u64 epoch = m_captureEpoch.load(std::memory_order_acquire);
    if (epoch == 0 || zoneId == INVALID_ZONE_ID)
        return 0;

    ScopeLock lock(&m_zoneLock);
    if (m_captureEpoch.load(std::memory_order_relaxed) != epoch || zoneId >= m_zones.size())
        return 0;
    if (!GetThreadStack().Push(zoneId, epoch, memstats::CurrentZone()))
        return 0;
    memstats::ZoneEntered(zoneId);
    return epoch;
}

void CPUProfiler::EndZone(u32 zoneId, u64 epoch, float elapsedMs,
    u64 allocCalls, u64 allocBytes, u64 freeCalls, u64 freeBytes)
{
    if (epoch == 0 || zoneId == INVALID_ZONE_ID)
        return;

    const auto entry = GetThreadStack().Pop();
    memstats::ZoneExited(zoneId, entry.previousMemoryZone);
    if (m_captureEpoch.load(std::memory_order_acquire) != epoch)
        return;

    ScopeLock lock(&m_zoneLock);
    if (m_captureEpoch.load(std::memory_order_relaxed) != epoch || zoneId >= m_zones.size())
        return;

    ZoneData& zone = m_zones[zoneId];
    if (zone.timing.callCount == 0)
        zone.parentId = entry.parentId;
    ++zone.timing.callCount;
    zone.timing.totalTimeMs += elapsedMs;
    zone.timing.allocCalls += allocCalls;
    zone.timing.allocBytes += allocBytes;
    zone.timing.freeCalls += freeCalls;
    zone.timing.freeBytes += freeBytes;
}

void CPUProfiler::FrameStart()
{
    m_captureEpoch.store(0, std::memory_order_release);
    if (!m_enabled.load(std::memory_order_acquire))
        return;
    u32 remaining = m_framesUntilSample.load(std::memory_order_relaxed);
    while (remaining != 0)
    {
        if (m_framesUntilSample.compare_exchange_weak(remaining, remaining - 1, std::memory_order_relaxed))
            return;
    }

    ScopeLock lock(&m_zoneLock);
    if (!m_enabled.load(std::memory_order_relaxed))
        return;
    m_framesUntilSample.store(m_throttleInterval.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);

    for (auto& zone : m_zones)
    {
        zone.timing.Reset();
        zone.parentId = INVALID_ZONE_ID;
        zone.childIds.clear();
    }
    m_rootZones.clear();
    m_frameTimer.Start();
    if (++m_nextEpoch == 0)
        ++m_nextEpoch;
    m_captureEpoch.store(m_nextEpoch, std::memory_order_release);
}

void CPUProfiler::FrameEnd()
{
    if (!IsSamplingFrame())
        return;
    ScopeLock lock(&m_zoneLock);
    if (m_captureEpoch.exchange(0, std::memory_order_acq_rel) == 0)
        return;

    m_frameTimeMs = m_frameTimer.GetElapsed_sec() * 1000.0f;
    BuildHierarchy(m_zones, m_rootZones);
    ComputeSelfTimes(m_zones);
    CopyToDisplayBuffer();
}

void CPUProfiler::CopyToDisplayBuffer()
{
    m_displayFrameTimeMs = m_frameTimeMs;
    m_displayZones.resize(m_zones.size());
    for (u32 i = 0; i < m_zones.size(); ++i)
    {
        m_displayZones[i].info = m_zones[i].info;
        m_displayZones[i].timing = m_zones[i].timing;
        m_displayZones[i].parentId = m_zones[i].parentId;
        m_displayZones[i].childIds = m_zones[i].childIds;
    }
    m_displayRootZones = m_rootZones;
}

void CPUProfiler::BuildHierarchy(xr_vector<ZoneData>& zones, xr_vector<u32>& rootZones)
{
    rootZones.clear();
    for (u32 i = 0; i < zones.size(); ++i)
    {
        auto& zone = zones[i];
        if (zone.timing.callCount == 0)
            continue;
        u32 parentId = zone.parentId;
        for (u32 depth = 0; parentId != INVALID_ZONE_ID; ++depth)
        {
            if (parentId >= zones.size() || zones[parentId].timing.callCount == 0 ||
                parentId == i || depth == zones.size())
            {
                zone.parentId = INVALID_ZONE_ID;
                break;
            }
            parentId = zones[parentId].parentId;
        }
    }
    for (u32 i = 0; i < zones.size(); ++i)
    {
        const auto& zone = zones[i];
        if (zone.timing.callCount == 0)
            continue;
        if (zone.parentId == INVALID_ZONE_ID)
            rootZones.push_back(i);
        else
            zones[zone.parentId].childIds.push_back(i);
    }
}

void CPUProfiler::ComputeSelfTimes(xr_vector<ZoneData>& zones)
{
    for (auto& zone : zones)
    {
        if (zone.timing.callCount == 0)
            continue;

        float childTime = 0.0f;
        u64 childAllocCalls = 0;
        u64 childAllocBytes = 0;
        for (u32 childId : zone.childIds)
        {
            if (childId < zones.size())
            {
                childTime += zones[childId].timing.totalTimeMs;
                childAllocCalls += zones[childId].timing.allocCalls;
                childAllocBytes += zones[childId].timing.allocBytes;
            }
        }
        zone.timing.selfTimeMs = zone.timing.totalTimeMs - childTime;
        if (zone.timing.selfTimeMs < 0.0f)
            zone.timing.selfTimeMs = 0.0f;
        zone.timing.selfAllocCalls =
            zone.timing.allocCalls > childAllocCalls ? zone.timing.allocCalls - childAllocCalls : 0;
        zone.timing.selfAllocBytes =
            zone.timing.allocBytes > childAllocBytes ? zone.timing.allocBytes - childAllocBytes : 0;
    }
}

CPUZoneScope::CPUZoneScope(const ZoneInfo* info)
{
    if (!info)
        return;
    CPUProfiler& profiler = CPUProfiler::Instance();
    if (!profiler.IsSamplingFrame())
        return;
    m_zoneId = profiler.RegisterZone(info);
    m_epoch = profiler.BeginZone(m_zoneId);
    if (m_epoch == 0)
        return;

    m_startTime = CTimerBase::Clock::now();
    m_allocCalls0 = memstats::AllocCallsThread();
    m_allocBytes0 = memstats::AllocBytesThread();
    m_freeCalls0 = memstats::FreeCallsThread();
    m_freeBytes0 = memstats::FreeBytesThread();
}

CPUZoneScope::~CPUZoneScope()
{
    if (m_epoch == 0)
        return;
    CPUProfiler& profiler = CPUProfiler::Instance();
    if (profiler.m_captureEpoch.load(std::memory_order_acquire) != m_epoch)
    {
        profiler.EndZone(m_zoneId, m_epoch, 0.0f, 0, 0, 0, 0);
        return;
    }

    const auto endTime = CTimerBase::Clock::now();
    const float elapsedMs = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
    profiler.EndZone(m_zoneId, m_epoch, elapsedMs,
        memstats::AllocCallsThread() - m_allocCalls0,
        memstats::AllocBytesThread() - m_allocBytes0,
        memstats::FreeCallsThread() - m_freeCalls0,
        memstats::FreeBytesThread() - m_freeBytes0);
}

}
