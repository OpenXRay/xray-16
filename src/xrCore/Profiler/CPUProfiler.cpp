#include "stdafx.h"
#include "CPUProfiler.h"

#include "../MemoryStats.h"

namespace xray::profiler
{

// ============================================================================
//  ThreadZoneStack
// ============================================================================

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

// ============================================================================
//  CPUProfiler
// ============================================================================

static CPUProfiler* g_cpuProfiler = nullptr;

CPUProfiler::CPUProfiler()
{
    m_zones.reserve(512);  // Typical zone count
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
    // Fast path: already registered
    if (info->id != INVALID_ZONE_ID)
    {
        return info->id;
    }

    // Slow path: register new zone
    ScopeLock lock(&m_registryLock);

    // Double-check after acquiring lock
    if (info->id != INVALID_ZONE_ID)
    {
        return info->id;
    }

    u32 id = m_nextZoneId.fetch_add(1);

    // Ensure vector is large enough
    if (id >= m_zones.size())
    {
        m_zones.resize(id + 1);
    }

    m_zones[id].info = info;
    info->id = id;

    return id;
}

const ZoneInfo* CPUProfiler::RegisterDynamicZone(pcstr name)
{
    if (!m_enabled || !name)
        return nullptr;

    ScopeLock lock(&m_registryLock);

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

void CPUProfiler::BeginZone(u32 zoneId)
{
    if (!m_enabled || zoneId == INVALID_ZONE_ID)
        return;

    ThreadZoneStack& stack = GetThreadStack();

    // Record parent relationship
    u32 parentId = stack.CurrentParent();

    // Lock for zone data modification
    ScopeLock lock(&m_zoneLock);

    if (zoneId >= m_zones.size())
        return;

    ZoneData& zone = m_zones[zoneId];

    // Track hierarchy (only set once per frame for this zone)
    if (zone.timing.callCount == 0)
    {
        zone.parentId = parentId;

        if (parentId != INVALID_ZONE_ID && parentId < m_zones.size())
        {
            // Add as child of parent (avoid duplicates)
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

    memstats::ZoneEntered(zoneId);
}

void CPUProfiler::EndZone(u32 zoneId, float elapsedMs,
    u64 allocCalls, u64 allocBytes, u64 freeCalls, u64 freeBytes)
{
    if (!m_enabled || zoneId == INVALID_ZONE_ID)
        return;

    ThreadZoneStack& stack = GetThreadStack();
    stack.Pop();
    memstats::ZoneExited(zoneId, stack.CurrentParent());

    // Lock for zone data modification
    ScopeLock lock(&m_zoneLock);

    if (zoneId >= m_zones.size())
        return;

    ZoneTiming& timing = m_zones[zoneId].timing;
    if (timing.callCount == 0)
        timing.callCount = 1;
    timing.totalTimeMs += elapsedMs;
    timing.allocCalls += allocCalls;
    timing.allocBytes += allocBytes;
    timing.freeCalls += freeCalls;
    timing.freeBytes += freeBytes;
}

void CPUProfiler::FrameStart()
{
    if (!m_enabled)
        return;

    // Lock to ensure no zones are being modified during reset
    ScopeLock lock(&m_zoneLock);

    // Reset timing data but preserve zone registry
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

    // Lock to ensure consistent snapshot for display buffer
    ScopeLock lock(&m_zoneLock);

    // Finalize current frame data
    BuildHierarchy(m_zones, m_rootZones);
    ComputeSelfTimes(m_zones);

    // Copy to display buffer for rendering (previous frame's data)
    CopyToDisplayBuffer();
}

void CPUProfiler::CopyToDisplayBuffer()
{
    // Copy finalized frame data to display buffer
    m_displayFrameTimeMs = m_frameTimeMs;

    // Resize display buffer to match current zones
    m_displayZones.resize(m_zones.size());

    // Deep copy zone data (including timing and hierarchy)
    for (u32 i = 0; i < m_zones.size(); ++i)
    {
        m_displayZones[i].info = m_zones[i].info;
        m_displayZones[i].timing = m_zones[i].timing;
        m_displayZones[i].parentId = m_zones[i].parentId;
        m_displayZones[i].childIds = m_zones[i].childIds;
    }

    // Copy root zones
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

// ============================================================================
//  CPUZoneScope
// ============================================================================

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

    m_allocCalls0 = memstats::AllocCallsThread();
    m_allocBytes0 = memstats::AllocBytesThread();
    m_freeCalls0 = memstats::FreeCallsThread();
    m_freeBytes0 = memstats::FreeBytesThread();
}

CPUZoneScope::~CPUZoneScope()
{
    if (m_zoneId == INVALID_ZONE_ID)
        return;

    auto endTime = CTimerBase::Clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();

    CPUProfiler::Instance().EndZone(m_zoneId, elapsedMs,
        memstats::AllocCallsThread() - m_allocCalls0,
        memstats::AllocBytesThread() - m_allocBytes0,
        memstats::FreeCallsThread() - m_freeCalls0,
        memstats::FreeBytesThread() - m_freeBytes0);
}

} // namespace xray::profiler
