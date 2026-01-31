#include "stdafx.h"
#include "CPUProfiler.h"

#include <thread>
#include <functional>

namespace xray::profiler
{

void ThreadZoneStack::Push(u32 zoneId)
{
    if (m_depth < MAX_DEPTH)
    {
        m_stack[m_depth++] = zoneId;
    }
}

u32 ThreadZoneStack::Pop()
{
    if (m_depth > 0)
    {
        return m_stack[--m_depth];
    }
    return INVALID_ZONE_ID;
}

u32 ThreadZoneStack::CurrentParent() const
{
    if (m_depth > 0)
    {
        return m_stack[m_depth - 1];
    }
    return INVALID_ZONE_ID;
}

static CPUProfiler* g_cpuProfiler = nullptr;

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
    if (!g_cpuProfiler)
    {
        g_cpuProfiler = new CPUProfiler();
    }
    return *g_cpuProfiler;
}

u32 CPUProfiler::RegisterZone(const ZoneInfo* info)
{
    if (info->id != INVALID_ZONE_ID)
    {
        return info->id;
    }

    ScopeLock lock(&m_registryLock);

    if (info->id != INVALID_ZONE_ID)
    {
        return info->id;
    }

    u32 id = m_nextZoneId.fetch_add(1);

    if (id >= m_zones.size())
    {
        m_zones.resize(id + 1);
    }

    m_zones[id].info = info;
    info->id = id;

    return id;
}

ThreadZoneStack& CPUProfiler::GetThreadStack()
{
    u32 threadId = static_cast<u32>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

    ScopeLock lock(&m_stackLock);
    return m_threadStacks[threadId];
}

void CPUProfiler::BeginZone(u32 zoneId)
{
    if (!m_enabled || zoneId == INVALID_ZONE_ID)
        return;

    ThreadZoneStack& stack = GetThreadStack();

    u32 parentId = stack.CurrentParent();

    ScopeLock lock(&m_zoneLock);

    if (zoneId >= m_zones.size())
        return;

    ZoneData& zone = m_zones[zoneId];

    if (zone.timing.callCount == 0)
    {
        zone.parentId = parentId;

        if (parentId != INVALID_ZONE_ID && parentId < m_zones.size())
        {
            auto& parentChildren = m_zones[parentId].childIds;
            bool found = false;
            for (u32 childId : parentChildren)
            {
                if (childId == zoneId)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                parentChildren.push_back(zoneId);
            }
        }
    }

    stack.Push(zoneId);
    zone.timing.callCount++;
}

void CPUProfiler::EndZone(u32 zoneId, float elapsedMs)
{
    if (!m_enabled || zoneId == INVALID_ZONE_ID)
        return;

    ThreadZoneStack& stack = GetThreadStack();
    stack.Pop();

    ScopeLock lock(&m_zoneLock);

    if (zoneId >= m_zones.size())
        return;

    m_zones[zoneId].timing.totalTimeMs += elapsedMs;
}

void CPUProfiler::FrameStart()
{
    if (!m_enabled)
        return;

    ScopeLock lock(&m_zoneLock);

    for (auto& zone : m_zones)
    {
        zone.timing.Reset();
        zone.parentId = INVALID_ZONE_ID;
        zone.childIds.clear();
    }
    m_rootZones.clear();

    m_frameTimer.Start();
}

void CPUProfiler::FrameEnd()
{
    if (!m_enabled)
        return;

    m_frameTimeMs = m_frameTimer.GetElapsed_sec() * 1000.0f;

    ScopeLock lock(&m_zoneLock);

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

void CPUProfiler::BuildHierarchy(const xr_vector<ZoneData>& zones, xr_vector<u32>& rootZones)
{
    rootZones.clear();

    for (u32 i = 0; i < zones.size(); ++i)
    {
        const ZoneData& zone = zones[i];
        if (zone.timing.callCount > 0 && zone.parentId == INVALID_ZONE_ID)
        {
            rootZones.push_back(i);
        }
    }
}

void CPUProfiler::ComputeSelfTimes(xr_vector<ZoneData>& zones)
{
    for (auto& zone : zones)
    {
        if (zone.timing.callCount == 0)
            continue;

        float childTime = 0.0f;
        for (u32 childId : zone.childIds)
        {
            if (childId < zones.size())
            {
                childTime += zones[childId].timing.totalTimeMs;
            }
        }
        zone.timing.selfTimeMs = zone.timing.totalTimeMs - childTime;
        if (zone.timing.selfTimeMs < 0.0f)
            zone.timing.selfTimeMs = 0.0f;
    }
}

CPUZoneScope::CPUZoneScope(const ZoneInfo* info)
    : m_zoneId(INVALID_ZONE_ID)
{
    if (!info)
        return;

    CPUProfiler& profiler = CPUProfiler::Instance();
    if (!profiler.IsEnabled())
        return;

    m_zoneId = profiler.RegisterZone(info);
    m_startTime = CTimerBase::Clock::now();
    profiler.BeginZone(m_zoneId);
}

CPUZoneScope::~CPUZoneScope()
{
    if (m_zoneId == INVALID_ZONE_ID)
        return;

    auto endTime = CTimerBase::Clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();

    CPUProfiler::Instance().EndZone(m_zoneId, elapsedMs);
}

}
