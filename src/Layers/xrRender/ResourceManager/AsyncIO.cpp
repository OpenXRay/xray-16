#include "stdafx.h"
#include "AsyncIO.h"

namespace xray::render::resources {

namespace {

constexpr u32 kMaxWorkers = 4;

u32 WorkerCount()
{
    const u32 hardware = std::thread::hardware_concurrency();
    if (hardware < 2u)
        return 1u;
    return std::min(kMaxWorkers, hardware - 1u);
}

} // namespace

AsyncIOManager::AsyncIOManager()
{
    const u32 numThreads = WorkerCount();
    m_workers.reserve(numThreads);
    for (u32 i = 0; i < numThreads; ++i)
        m_workers.emplace_back(&AsyncIOManager::WorkerThreadFunc, this, i);

    Msg("* [AsyncIO] service started with %u worker threads", numThreads);
}

AsyncIOManager::~AsyncIOManager()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_requests) {
            pair.second.canceled = true;
            pair.second.request.callback = nullptr;
        }
        m_pending.clear();
        m_finished.clear();
        m_shouldExit = true;
    }
    m_workAvailable.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable())
            worker.join();
    }
    m_workers.clear();

    PrintStatistics();
}

u32 AsyncIOManager::AcquireEpoch()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nextEpoch++;
}

void AsyncIOManager::DropLocked(u64 requestID, Entry& entry)
{
    if (entry.retained && m_retainedCount)
        --m_retainedCount;
    const auto finished = std::find(m_finished.begin(), m_finished.end(), requestID);
    if (finished != m_finished.end())
        m_finished.erase(finished);
    RemovePendingLocked(requestID);
    m_requests.erase(requestID);
}

void AsyncIOManager::CancelEpoch(u32 epoch)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_requests.begin(); it != m_requests.end();) {
        Entry& entry = it->second;
        if (entry.request.epoch != epoch) {
            ++it;
            continue;
        }

        entry.canceled = true;
        entry.request.callback = nullptr;

        if (entry.dispatched && !entry.settled) {
            ++it;
            continue;
        }

        const u64 id = it->first;
        if (entry.retained && m_retainedCount)
            --m_retainedCount;
        const auto finished = std::find(m_finished.begin(), m_finished.end(), id);
        if (finished != m_finished.end())
            m_finished.erase(finished);
        RemovePendingLocked(id);
        m_stats.requestsCanceled++;
        it = m_requests.erase(it);
    }
}

void AsyncIOManager::DrainEpoch(u32 epoch)
{
    CancelEpoch(epoch);

    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle.wait(lock, [&] {
        for (const auto& pair : m_requests) {
            if (pair.second.request.epoch == epoch && pair.second.dispatched && !pair.second.settled)
                return false;
        }
        return true;
    });

    for (auto it = m_requests.begin(); it != m_requests.end();) {
        if (it->second.request.epoch != epoch) {
            ++it;
            continue;
        }
        const u64 id = it->first;
        if (it->second.retained && m_retainedCount)
            --m_retainedCount;
        const auto finished = std::find(m_finished.begin(), m_finished.end(), id);
        if (finished != m_finished.end())
            m_finished.erase(finished);
        RemovePendingLocked(id);
        it = m_requests.erase(it);
    }
}

void AsyncIOManager::DrainAll()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto& pair : m_requests) {
        pair.second.canceled = true;
        pair.second.request.callback = nullptr;
    }
    m_pending.clear();
    m_idle.wait(lock, [&] { return m_activeCount == 0; });
    m_requests.clear();
    m_finished.clear();
    m_retainedCount = 0;
}

u64 AsyncIOManager::ReadAsync(
    const char* filePath,
    u64 offset,
    u64 size,
    u32 epoch,
    AsyncIORequest::Callback callback,
    void* userData,
    u64 contentTag)
{
    if (!filePath || !filePath[0] || size == 0)
        return 0;

    u64 requestID;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        requestID = m_nextRequestID++;

        Entry entry;
        entry.request.filePath = filePath;
        entry.request.offset = offset;
        entry.request.size = size;
        entry.request.status = IOStatus::Pending;
        entry.request.requestTime = Device.fTimeGlobal;
        entry.request.callback = std::move(callback);
        entry.request.userData = userData;
        entry.request.contentTag = contentTag;
        entry.request.requestID = requestID;
        entry.request.epoch = epoch;
        entry.retained = !entry.request.callback;
        if (entry.retained)
            ++m_retainedCount;

        m_requests.emplace(requestID, std::move(entry));
        m_pending.push_back(requestID);
    }

    m_workAvailable.notify_one();
    return requestID;
}

void AsyncIOManager::CancelRequest(u64 requestID)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Entry* entry = FindLocked(requestID);
    if (!entry)
        return;

    entry->canceled = true;
    entry->request.callback = nullptr;

    if (entry->dispatched && !entry->settled)
        return;

    m_stats.requestsCanceled++;
    DropLocked(requestID, *entry);
}

IOStatus AsyncIOManager::GetRequestStatus(u64 requestID) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const Entry* entry = FindLocked(requestID);
    if (!entry)
        return IOStatus::Unknown;
    if (entry->canceled && !entry->settled)
        return IOStatus::Canceled;
    return entry->request.status;
}

bool AsyncIOManager::IsRequestSettled(u64 requestID) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const Entry* entry = FindLocked(requestID);
    return entry ? entry->settled : false;
}

bool AsyncIOManager::TryTakeResult(u64 requestID, AsyncIORequest& out)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Entry* entry = FindLocked(requestID);
    if (!entry || !entry->settled || entry->canceled)
        return false;

    out = std::move(entry->request);
    out.requestID = requestID;
    DropLocked(requestID, *entry);
    return true;
}

void AsyncIOManager::Pump()
{
    m_dispatchScratch.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_windowBytes = 0;

        for (auto it = m_finished.begin(); it != m_finished.end();) {
            const u64 id = *it;
            Entry* entry = FindLocked(id);
            if (!entry) {
                it = m_finished.erase(it);
                continue;
            }
            if (entry->retained || entry->canceled || !entry->request.callback) {
                ++it;
                continue;
            }
            m_dispatchScratch.push_back(std::move(entry->request));
            m_requests.erase(id);
            it = m_finished.erase(it);
        }
    }

    m_workAvailable.notify_all();

    for (AsyncIORequest& request : m_dispatchScratch) {
        if (request.callback)
            request.callback(request);
    }
    m_dispatchScratch.clear();
}

void AsyncIOManager::SetMaxConcurrentRequests(u32 count)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxConcurrent = std::max(1u, count);
    }
    m_workAvailable.notify_all();
}

u32 AsyncIOManager::GetMaxConcurrentRequests() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxConcurrent;
}

void AsyncIOManager::SetBandwidthLimit(u64 bytesPerWindow)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bandwidthLimit = bytesPerWindow;
    }
    m_workAvailable.notify_all();
}

u64 AsyncIOManager::GetBandwidthLimit() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bandwidthLimit;
}

AsyncIOManager::Statistics AsyncIOManager::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Statistics stats = m_stats;
    stats.bytesDispatchedThisWindow = m_windowBytes;
    stats.requestsInProgress = m_activeCount;
    stats.requestsPending = u32(m_pending.size());
    stats.requestsRetained = m_retainedCount;
    stats.avgReadTime = m_readTimeSamples ? float(m_readTimeTotal / double(m_readTimeSamples)) : 0.0f;
    return stats;
}

void AsyncIOManager::PrintStatistics() const
{
    const Statistics stats = GetStatistics();
    Msg("* [AsyncIO] pending %u, in flight %u, retained %u, deferrals %u",
        stats.requestsPending, stats.requestsInProgress, stats.requestsRetained, stats.requestsDeferred);
    Msg("* [AsyncIO] completed %u, failed %u, canceled %u, %llu MB read, avg %.2f ms",
        stats.requestsCompleted, stats.requestsFailed, stats.requestsCanceled,
        (unsigned long long)(stats.bytesRead / (1024ull * 1024ull)), stats.avgReadTime);
}

AsyncIOManager::Entry* AsyncIOManager::FindLocked(u64 requestID)
{
    const auto it = m_requests.find(requestID);
    return it == m_requests.end() ? nullptr : &it->second;
}

const AsyncIOManager::Entry* AsyncIOManager::FindLocked(u64 requestID) const
{
    const auto it = m_requests.find(requestID);
    return it == m_requests.end() ? nullptr : &it->second;
}

void AsyncIOManager::RemovePendingLocked(u64 requestID)
{
    const auto it = std::find(m_pending.begin(), m_pending.end(), requestID);
    if (it != m_pending.end())
        m_pending.erase(it);
}

bool AsyncIOManager::HasDispatchableLocked() const
{
    if (m_pending.empty() || m_activeCount >= m_maxConcurrent)
        return false;

    for (u64 id : m_pending) {
        const Entry* entry = FindLocked(id);
        if (!entry || entry->canceled)
            return true;
        if (m_activeCount == 0)
            return true;
        if (m_windowBytes + entry->request.size <= m_bandwidthLimit)
            return true;
    }
    return false;
}

bool AsyncIOManager::PickRequestLocked(u64& outID)
{
    outID = 0;
    if (m_activeCount >= m_maxConcurrent)
        return false;

    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        const u64 id = *it;
        Entry* entry = FindLocked(id);
        if (!entry) {
            m_pending.erase(it);
            return true;
        }
        if (entry->canceled) {
            m_pending.erase(it);
            m_stats.requestsCanceled++;
            if (entry->retained && m_retainedCount)
                --m_retainedCount;
            m_requests.erase(id);
            return true;
        }

        const bool fitsWindow = m_windowBytes + entry->request.size <= m_bandwidthLimit;
        if (!fitsWindow && m_activeCount != 0) {
            entry->request.status = IOStatus::Deferred;
            continue;
        }

        m_pending.erase(it);
        entry->dispatched = true;
        entry->request.status = IOStatus::InProgress;
        m_windowBytes += entry->request.size;
        ++m_activeCount;
        outID = id;
        return true;
    }

    if (!m_pending.empty())
        m_stats.requestsDeferred++;
    return false;
}

void AsyncIOManager::SettleLocked(Entry& entry, IOStatus status, const char* error)
{
    entry.settled = true;
    entry.request.status = status;
    if (error)
        entry.request.errorMessage = error;

    if (status == IOStatus::Complete)
        m_stats.requestsCompleted++;
    else if (status == IOStatus::Failed)
        m_stats.requestsFailed++;

    m_finished.push_back(entry.request.requestID);
}

void AsyncIOManager::WorkerThreadFunc(u32 workerID)
{
    (void)workerID;

    for (;;) {
        u64 requestID = 0;
        shared_str path;
        u64 offset = 0;
        u64 size = 0;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_workAvailable.wait(lock, [&] { return m_shouldExit || HasDispatchableLocked(); });
            if (m_shouldExit)
                return;
            if (!PickRequestLocked(requestID) || requestID == 0)
                continue;

            const Entry* entry = FindLocked(requestID);
            if (!entry) {
                --m_activeCount;
                m_idle.notify_all();
                continue;
            }
            path = entry->request.filePath;
            offset = entry->request.offset;
            size = entry->request.size;
        }

        xr_vector<u8> buffer;
        IOStatus status = IOStatus::Failed;
        const char* error = nullptr;
        CTimer timer;
        timer.Start();

        IReader* reader = FS.r_open(path.c_str());
        if (!reader) {
            error = "failed to open source";
        } else {
            const u64 fileLength = u64(reader->length());
            if (offset > fileLength) {
                error = "read offset past end of file";
            } else if (size > fileLength - offset) {
                error = "read range past end of file";
            } else {
                buffer.resize(size_t(size));
                reader->seek(size_t(offset));
                reader->r(buffer.data(), size_t(size));
                status = IOStatus::Complete;
            }
            FS.r_close(reader);
        }

        const double elapsed = double(timer.GetElapsed_sec()) * 1000.0;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_activeCount;
            m_readTimeTotal += elapsed;
            ++m_readTimeSamples;
            if (status == IOStatus::Complete)
                m_stats.bytesRead += size;

            Entry* entry = FindLocked(requestID);
            if (entry) {
                if (entry->canceled) {
                    m_stats.requestsCanceled++;
                    if (entry->retained && m_retainedCount)
                        --m_retainedCount;
                    m_requests.erase(requestID);
                } else {
                    if (status == IOStatus::Complete)
                        entry->request.buffer = std::move(buffer);
                    SettleLocked(*entry, status, error);
                }
            }
        }

        m_idle.notify_all();
        m_workAvailable.notify_one();
    }
}

} // namespace xray::render::resources
