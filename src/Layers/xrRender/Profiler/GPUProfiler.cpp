#include "stdafx.h"
#include "GPUProfiler.h"

namespace xray::profiler
{

GPUProfiler::GPUProfiler()
{
    m_queryPool.reserve(INITIAL_POOL_SIZE);
    m_freeQueries.reserve(INITIAL_POOL_SIZE);
    m_activePasses.reserve(32);
    m_pendingQueries.reserve(64);
    m_passTimings.reserve(32);
}

GPUProfiler::~GPUProfiler()
{
    Shutdown();
}

void GPUProfiler::Initialize(nvrhi::IDevice* device)
{
    if (m_device)
        return;

    m_device = device;

    // Pre-allocate timer queries
    for (u32 i = 0; i < INITIAL_POOL_SIZE; ++i)
    {
        auto query = m_device->createTimerQuery();
        if (query)
        {
            m_queryPool.push_back(query);
            m_freeQueries.push_back(query);
        }
    }

    Msg("* [GPUProfiler] Initialized with %u timer queries", static_cast<u32>(m_queryPool.size()));
}

void GPUProfiler::Shutdown()
{
    // Clear all references - NVRHI handles are ref-counted
    m_pendingQueries.clear();
    m_activePasses.clear();
    m_freeQueries.clear();
    m_queryPool.clear();
    m_passTimings.clear();
    m_device = nullptr;
}

nvrhi::TimerQueryHandle GPUProfiler::AcquireTimerQuery()
{
    if (!m_freeQueries.empty())
    {
        auto query = m_freeQueries.back();
        m_freeQueries.pop_back();
        m_device->resetTimerQuery(query);
        return query;
    }

    // Pool exhausted, create new query
    auto query = m_device->createTimerQuery();
    if (query)
    {
        m_queryPool.push_back(query);
    }
    return query;
}

void GPUProfiler::ReleaseTimerQuery(nvrhi::TimerQueryHandle query)
{
    if (query)
    {
        m_freeQueries.push_back(query);
    }
}

void GPUProfiler::BeginPass(nvrhi::ICommandList* cmdList, const char* name, bool isAsync)
{
    if (!m_enabled || !m_device || !cmdList || !name)
        return;

    auto query = AcquireTimerQuery();
    if (!query)
        return;

    cmdList->beginTimerQuery(query);

    ActivePass pass;
    pass.name = name;
    pass.query = query;
    pass.isAsync = isAsync;
    m_activePasses.push_back(pass);
}

void GPUProfiler::EndPass(nvrhi::ICommandList* cmdList, const char* name)
{
    if (!m_enabled || !m_device || !cmdList || !name)
        return;

    // Find matching active pass (should be the last one with this name)
    for (auto it = m_activePasses.rbegin(); it != m_activePasses.rend(); ++it)
    {
        if (it->name == name)
        {
            cmdList->endTimerQuery(it->query);

            PendingQuery pending;
            pending.name = it->name;
            pending.query = it->query;
            pending.frameSubmitted = m_currentFrame;
            pending.resolved = false;
            pending.isAsync = it->isAsync;
            m_pendingQueries.push_back(pending);

            // Remove from active (convert reverse iterator)
            m_activePasses.erase(std::next(it).base());
            return;
        }
    }
}

void GPUProfiler::FrameStart()
{
    if (!m_enabled)
        return;

    m_activePasses.clear();
}

void GPUProfiler::FrameEnd()
{
    if (!m_enabled)
        return;

    m_currentFrame++;

    // Resolve queries from previous frames
    ResolvePendingQueries();
}

void GPUProfiler::ResolvePendingQueries()
{
    if (!m_device)
        return;

    m_passTimings.clear();
    m_totalGPUTimeMs = 0.0f;

    // Process pending queries
    for (auto it = m_pendingQueries.begin(); it != m_pendingQueries.end(); )
    {
        // Skip if submitted too recently (GPU might not be done)
        if (m_currentFrame - it->frameSubmitted < 2)
        {
            ++it;
            continue;
        }

        // Check if query is ready
        if (m_device->pollTimerQuery(it->query))
        {
            float timeSeconds = m_device->getTimerQueryTime(it->query);
            float timeMs = timeSeconds * 1000.0f;

            GPUPassTiming timing;
            timing.name = it->name;
            timing.timeMs = timeMs;
            timing.pending = false;
            timing.isAsync = it->isAsync;
            m_passTimings.push_back(timing);

            // Sub-passes (names containing '.') are children of a parent pass
            // and should not be counted in the total to avoid double-counting
            bool isSubPass = (it->name.c_str() && strchr(it->name.c_str(), '.') != nullptr);
            if (!isSubPass)
                m_totalGPUTimeMs += timeMs;

            // Return query to pool
            ReleaseTimerQuery(it->query);

            it = m_pendingQueries.erase(it);
        }
        else if (m_currentFrame - it->frameSubmitted > MAX_PENDING_FRAMES)
        {
            // Query took too long, discard
            ReleaseTimerQuery(it->query);
            it = m_pendingQueries.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ============================================================================
//  GPUPassScope
// ============================================================================

GPUPassScope::GPUPassScope(GPUProfiler* profiler, nvrhi::ICommandList* cmdList, const char* name)
    : m_profiler(profiler)
    , m_cmdList(cmdList)
    , m_name(name)
{
    if (m_profiler && m_profiler->IsProfilingEnabled() && m_profiler->IsInitialized())
    {
        m_profiler->BeginPass(m_cmdList, m_name);
    }
}

GPUPassScope::~GPUPassScope()
{
    if (m_profiler && m_profiler->IsProfilingEnabled() && m_profiler->IsInitialized())
    {
        m_profiler->EndPass(m_cmdList, m_name);
    }
}

} // namespace xray::profiler
