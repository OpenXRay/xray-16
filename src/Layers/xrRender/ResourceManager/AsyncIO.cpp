#include "stdafx.h"
#include "AsyncIO.h"

// Async I/O Manager Implementation
// Week 3 - Day 5: Task 5.2

namespace xray::render::resources {

AsyncIOManager::AsyncIOManager() {
    // Create worker threads
    u32 numThreads = std::min(4u, std::thread::hardware_concurrency());

    Msg("! [AsyncIOManager] Creating %u worker threads...", numThreads);

    for (u32 i = 0; i < numThreads; i++) {
        WorkerThread worker;
        worker.thread = std::thread(&AsyncIOManager::WorkerThreadFunc, this, i);
        m_workers.push_back(std::move(worker));
    }

    Msg("! [AsyncIOManager] ✅ Created");
}

AsyncIOManager::~AsyncIOManager() {
    Msg("! [AsyncIOManager] Shutting down...");

    // Signal all threads to exit
    {
        std::lock_guard<std::mutex> lock(m_workMutex);
        for (auto& worker : m_workers) {
            worker.shouldExit = true;
        }
    }

    // Wake all threads
    m_workAvailable.notify_all();

    // Wait for threads to finish
    for (auto& worker : m_workers) {
        if (worker.thread.joinable()) {
            worker.thread.join();
        }
    }

    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  REQUEST SUBMISSION
// ═══════════════════════════════════════════════════

u32 AsyncIOManager::ReadAsync(
    const char* filePath,
    u64 offset,
    u64 size,
    AsyncIORequest::Callback callback,
    void* userData)
{
    // Create request
    AsyncIORequest request;
    request.filePath = filePath;
    request.offset = offset;
    request.size = size;
    request.callback = callback;
    request.userData = userData;
    request.status = IOStatus::Pending;
    request.requestTime = Device.fTimeGlobal;

    // Assign ID
    u32 requestID = m_nextRequestID++;
    request.requestID = requestID;

    // Add to pending queue
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        u32 index = (u32)m_pendingRequests.size();
        m_pendingRequests.push_back(request);
        m_idToIndex[requestID] = index;

        m_stats.requestsPending++;
    }

    // Wake a worker thread
    m_workAvailable.notify_one();

    Msg("! [AsyncIOManager] Queued: %s (offset=%llu, size=%llu, id=%u)",
        filePath, offset, size, requestID);

    return requestID;
}

void AsyncIOManager::CancelRequest(u32 requestID) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);

    auto it = m_idToIndex.find(requestID);
    if (it != m_idToIndex.end()) {
        u32 index = it->second;

        if (index < m_pendingRequests.size()) {
            m_pendingRequests.erase(m_pendingRequests.begin() + index);
            m_idToIndex.erase(it);
            m_stats.requestsPending--;

            Msg("! [AsyncIOManager] Cancelled request: %u", requestID);
        }
    }
}

IOStatus AsyncIOManager::GetRequestStatus(u32 requestID) const {
    // Check pending
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        auto it = m_idToIndex.find(requestID);
        if (it != m_idToIndex.end()) {
            return IOStatus::Pending;
        }
    }

    // Check active
    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        for (const auto& req : m_activeRequests) {
            if (req.requestID == requestID) {
                return req.status;
            }
        }
    }

    // Check completed
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        for (const auto& req : m_completedRequests) {
            if (req.requestID == requestID) {
                return req.status;
            }
        }
    }

    return IOStatus::Failed;  // Not found
}

bool AsyncIOManager::IsRequestComplete(u32 requestID) const {
    IOStatus status = GetRequestStatus(requestID);
    return status == IOStatus::Complete || status == IOStatus::Failed;
}

// ═══════════════════════════════════════════════════
//  WORKER THREAD
// ═══════════════════════════════════════════════════

void AsyncIOManager::WorkerThreadFunc(u32 workerID) {
    Msg("! [AsyncIOManager] Worker %u started", workerID);

    while (true) {
        AsyncIORequest request;
        bool hasWork = false;

        // Get next request
        {
            std::unique_lock<std::mutex> lock(m_workMutex);

            // Wait for work or exit signal
            m_workAvailable.wait(lock, [this, workerID]() {
                std::lock_guard<std::mutex> pendingLock(m_pendingMutex);
                return !m_pendingRequests.empty() || m_workers[workerID].shouldExit;
            });

            // Check exit condition
            if (m_workers[workerID].shouldExit) {
                break;
            }

            // Get pending request
            {
                std::lock_guard<std::mutex> pendingLock(m_pendingMutex);

                if (!m_pendingRequests.empty()) {
                    request = m_pendingRequests.back();
                    m_pendingRequests.pop_back();

                    // Remove from ID map
                    m_idToIndex.erase(request.requestID);

                    hasWork = true;

                    m_stats.requestsPending--;
                    m_stats.requestsInProgress++;
                }
            }
        }

        if (!hasWork) continue;

        // ═══════════════════════════════════════════════════
        //  PERFORM I/O (Outside of Locks)
        // ═══════════════════════════════════════════════════

        request.status = IOStatus::InProgress;

        Msg("! [AsyncIOManager] Worker %u reading: %s",
            workerID, request.filePath.c_str());

        // Open file
        IReader* reader = FS.r_open(request.filePath.c_str());

        if (!reader) {
            request.status = IOStatus::Failed;
            request.errorMessage = "Failed to open file";

            Msg("! [AsyncIOManager] ❌ Worker %u failed: %s",
                workerID, request.filePath.c_str());
        } else {
            // Check file size
            u64 fileSize = reader->length();

            if (request.offset + request.size > fileSize) {
                request.status = IOStatus::Failed;
                request.errorMessage = "Read past end of file";

                Msg("! [AsyncIOManager] ❌ Worker %u: Read past EOF: %s",
                    workerID, request.filePath.c_str());
            } else {
                // Seek to offset
                reader->seek((u32)request.offset);

                // Read data
                request.buffer.resize((size_t)request.size);
                reader->r(request.buffer.data(), (u32)request.size);

                request.status = IOStatus::Complete;

                m_stats.bytesRead += request.size;

                Msg("! [AsyncIOManager] ✅ Worker %u complete: %s (%llu KB)",
                    workerID, request.filePath.c_str(), request.size / 1024);
            }

            FS.r_close(reader);
        }

        // ═══════════════════════════════════════════════════
        //  MOVE TO COMPLETED QUEUE
        // ═══════════════════════════════════════════════════

        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            m_completedRequests.push_back(request);

            m_stats.requestsInProgress--;

            if (request.status == IOStatus::Complete) {
                m_stats.requestsCompleted++;
            } else {
                m_stats.requestsFailed++;
            }
        }
    }

    Msg("! [AsyncIOManager] Worker %u exiting", workerID);
}

// ═══════════════════════════════════════════════════
//  PROCESS COMPLETED REQUESTS (Main Thread)
// ═══════════════════════════════════════════════════

void AsyncIOManager::ProcessCompletedRequests() {
    xr_vector<AsyncIORequest> completed;

    // Grab all completed requests
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        completed = std::move(m_completedRequests);
        m_completedRequests.clear();
    }

    // Invoke callbacks (on main thread)
    for (auto& request : completed) {
        if (request.callback) {
            request.callback(request);
        }
    }
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

AsyncIOManager::Statistics AsyncIOManager::GetStatistics() const {
    return m_stats;
}

void AsyncIOManager::PrintStatistics() const {
    Msg("! [AsyncIOManager] Statistics:");
    Msg("!   Pending: %u, Active: %u", m_stats.requestsPending, m_stats.requestsInProgress);
    Msg("!   Completed: %u, Failed: %u", m_stats.requestsCompleted, m_stats.requestsFailed);
    Msg("!   Bytes read: %llu MB", m_stats.bytesRead / (1024 * 1024));
}

} // namespace xray::render::resources
