#include "stdafx.h"
#include "GPUProfiler.h"

namespace xray::profiler
{

GPUProfiler::GPUProfiler()
{
    m_queryPool.reserve(INITIAL_POOL_SIZE);
    m_freeQueries.reserve(INITIAL_POOL_SIZE);
    m_activePasses.reserve(32);
    m_pendingFrames.reserve(4);
    m_passTimings.reserve(32);
}

GPUProfiler::~GPUProfiler()
{
    Shutdown();
}

void GPUProfiler::Initialize(nvrhi::IDevice* device)
{
    if (m_device || !device)
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
    if (m_device)
        m_device->waitForIdle();

    m_pendingFrames.clear();
    m_activePasses.clear();
    m_freeQueries.clear();
    m_queryPool.clear();
    m_passTimings.clear();
    m_totalGPUTimeMs = 0.0f;
    m_currentFrame = 0;
    m_completedSampleId = 0;
    m_recordingFrame = NO_RECORDING_FRAME;
    m_enabled = false;
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

void GPUProfiler::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled && m_recordingFrame != NO_RECORDING_FRAME)
        m_pendingFrames[m_recordingFrame].valid = false;
}

void GPUProfiler::BeginPass(nvrhi::ICommandList* cmdList, const char* name, bool isAsync)
{
    if (!m_device || m_recordingFrame == NO_RECORDING_FRAME || !cmdList || !name)
        return;

    auto& frame = m_pendingFrames[m_recordingFrame];
    frame.queries.emplace_back();
    auto& pending = frame.queries.back();
    pending.timing.name = name;
    pending.timing.isAsync = isAsync;
    if (frame.valid)
    {
        pending.query = AcquireTimerQuery();
        if (pending.query)
        {
            cmdList->beginTimerQuery(pending.query);
            pending.timing.pending = true;
        }
        else
        {
            frame.valid = false;
        }
    }

    ActivePass pass;
    pass.cmdList = cmdList;
    pass.queryIndex = frame.queries.size() - 1;
    m_activePasses.push_back(pass);
}

void GPUProfiler::EndPass(nvrhi::ICommandList* cmdList, const char* name)
{
    if (!m_device || m_recordingFrame == NO_RECORDING_FRAME || !cmdList || !name)
        return;

    auto& frame = m_pendingFrames[m_recordingFrame];
    for (auto it = m_activePasses.rbegin(); it != m_activePasses.rend(); ++it)
    {
        auto& pending = frame.queries[it->queryIndex];
        if (it->cmdList == cmdList && pending.timing.name == name)
        {
            if (pending.query)
                cmdList->endTimerQuery(pending.query);

            m_activePasses.erase(std::next(it).base());
            return;
        }
    }
}

void GPUProfiler::SealRecordingFrame()
{
    if (m_recordingFrame == NO_RECORDING_FRAME)
        return;

    auto& frame = m_pendingFrames[m_recordingFrame];
    if (!m_activePasses.empty())
    {
        frame.valid = false;
        for (const auto& pass : m_activePasses)
        {
            auto& pending = frame.queries[pass.queryIndex];
            pending.query = nullptr;
            pending.timing.pending = false;
        }
        m_activePasses.clear();
    }
    frame.sealed = true;
    m_recordingFrame = NO_RECORDING_FRAME;
}

void GPUProfiler::FrameStart(bool sampleFrame)
{
    SealRecordingFrame();
    ++m_currentFrame;
    if (!m_enabled || !m_device || !sampleFrame)
        return;

    size_t frameIndex = 0;
    while (frameIndex < m_pendingFrames.size() && m_pendingFrames[frameIndex].sampleId != 0)
        ++frameIndex;
    if (frameIndex == m_pendingFrames.size())
    {
        m_pendingFrames.emplace_back();
        m_pendingFrames.back().queries.reserve(32);
    }

    auto& frame = m_pendingFrames[frameIndex];
    frame.sampleId = m_currentFrame;
    frame.sealed = false;
    frame.valid = true;
    m_recordingFrame = frameIndex;
}

void GPUProfiler::FrameEnd()
{
    SealRecordingFrame();
    ResolvePendingQueries();
}

void GPUProfiler::ResolvePendingQueries()
{
    if (!m_device)
        return;

    PendingFrame* newestFrame = nullptr;
    u64 newestSampleId = m_completedSampleId;
    for (auto& frame : m_pendingFrames)
    {
        if (frame.sampleId == 0 || !frame.sealed || m_currentFrame - frame.sampleId < 2)
            continue;

        bool complete = true;
        for (auto& pending : frame.queries)
        {
            if (!pending.query)
                continue;
            if (!m_device->pollTimerQuery(pending.query))
            {
                complete = false;
                continue;
            }

            pending.timing.timeMs = m_device->getTimerQueryTime(pending.query) * 1000.0f;
            pending.timing.pending = false;
            ReleaseTimerQuery(pending.query);
            pending.query = nullptr;
        }

        if (!complete)
            continue;

        if (frame.valid && frame.sampleId > newestSampleId)
        {
            newestFrame = &frame;
            newestSampleId = frame.sampleId;
        }
        frame.sampleId = 0;
    }

    if (newestFrame)
    {
        m_passTimings.clear();
        m_totalGPUTimeMs = 0.0f;
        for (const auto& pending : newestFrame->queries)
        {
            m_passTimings.push_back(pending.timing);
            if (strchr(pending.timing.name.c_str(), '.') == nullptr)
                m_totalGPUTimeMs += pending.timing.timeMs;
        }
        m_completedSampleId = newestSampleId;
    }

    for (auto& frame : m_pendingFrames)
    {
        if (frame.sampleId == 0)
            frame.queries.clear();
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
    if (m_profiler)
    {
        m_profiler->BeginPass(m_cmdList, m_name);
    }
}

GPUPassScope::~GPUPassScope()
{
    if (m_profiler)
    {
        m_profiler->EndPass(m_cmdList, m_name);
    }
}

} // namespace xray::profiler
