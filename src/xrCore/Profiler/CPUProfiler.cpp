#include "stdafx.h"
#include "CPUProfiler.h"

#include "../MemoryStats.h"

namespace xray::profiler
{

bool ThreadZoneStack::Push(u32 zoneId, u32 nodeId, u64 epoch, u32 previousMemoryZone)
{
    if (m_depth == MAX_DEPTH)
        return false;
    m_stack[m_depth++] = {zoneId, nodeId, previousMemoryZone, epoch};
    return true;
}

ThreadZoneStack::Entry ThreadZoneStack::Pop()
{
    if (m_depth != 0)
        return m_stack[--m_depth];
    return {INVALID_ZONE_ID, INVALID_ZONE_ID, INVALID_ZONE_ID, 0};
}

u32 ThreadZoneStack::CurrentNode(u64 epoch) const
{
    if (m_depth != 0 && m_stack[m_depth - 1].epoch == epoch)
        return m_stack[m_depth - 1].nodeId;
    return INVALID_ZONE_ID;
}

CPUProfiler::CPUProfiler()
{
    m_infos.reserve(512);
    m_nodes.reserve(2048);
    m_rootNodes.reserve(32);
    m_displayZones.reserve(2048);
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

    id = static_cast<u32>(m_infos.size());
    m_infos.push_back(info);
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

u32 CPUProfiler::FindOrCreateNode(u32 parentNode, u32 zoneId)
{
    xr_vector<u32>& siblings = parentNode == INVALID_ZONE_ID ? m_rootNodes : m_nodes[parentNode].childIds;
    for (u32 nodeId : siblings)
        if (m_nodes[nodeId].zoneId == zoneId)
            return nodeId;

    const u32 nodeId = static_cast<u32>(m_nodes.size());
    ZoneData& node = m_nodes.emplace_back();
    node.info = m_infos[zoneId];
    node.zoneId = zoneId;
    node.parentId = parentNode;
    (parentNode == INVALID_ZONE_ID ? m_rootNodes : m_nodes[parentNode].childIds).push_back(nodeId);
    return nodeId;
}

u64 CPUProfiler::BeginZone(u32 zoneId)
{
    const u64 epoch = m_captureEpoch.load(std::memory_order_acquire);
    if (epoch == 0 || zoneId == INVALID_ZONE_ID)
        return 0;

    ScopeLock lock(&m_zoneLock);
    if (m_captureEpoch.load(std::memory_order_relaxed) != epoch || zoneId >= m_infos.size())
        return 0;
    ThreadZoneStack& stack = GetThreadStack();
    const u32 nodeId = FindOrCreateNode(stack.CurrentNode(epoch), zoneId);
    if (!stack.Push(zoneId, nodeId, epoch, memstats::CurrentZone()))
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
    if (m_captureEpoch.load(std::memory_order_acquire) != epoch || entry.epoch != epoch)
        return;

    ScopeLock lock(&m_zoneLock);
    if (m_captureEpoch.load(std::memory_order_relaxed) != epoch || entry.nodeId >= m_nodes.size())
        return;

    ZoneTiming& timing = m_nodes[entry.nodeId].timing;
    ++timing.callCount;
    timing.totalTimeMs += elapsedMs;
    timing.allocCalls += allocCalls;
    timing.allocBytes += allocBytes;
    timing.freeCalls += freeCalls;
    timing.freeBytes += freeBytes;
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

    if (m_resetPending.exchange(false, std::memory_order_acq_rel))
    {
        m_nodes.clear();
        m_rootNodes.clear();
    }
    for (auto& node : m_nodes)
        node.timing.Reset();
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
    ComputeSelfTimes(m_nodes);
    CopyToDisplayBuffer();
}

void CPUProfiler::CopyToDisplayBuffer()
{
    m_displayFrameTimeMs = m_frameTimeMs;
    m_displayZones = m_nodes;
    m_displayRootZones = m_rootNodes;
}

void CPUProfiler::ResetTree()
{
    ScopeLock lock(&m_zoneLock);
    m_resetPending.store(true, std::memory_order_release);
    m_displayZones.clear();
    m_displayRootZones.clear();
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
