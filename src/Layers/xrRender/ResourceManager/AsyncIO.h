#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// Async I/O Manager
// Week 3 - Day 5: Task 5.1

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  ASYNC I/O REQUEST
// ═══════════════════════════════════════════════════

enum class IOStatus : u8 {
    Pending,        // Waiting in queue
    InProgress,     // Reading from disk
    Complete,       // Successfully read
    Failed          // Error occurred
};

struct AsyncIORequest {
    // Request data
    shared_str filePath;
    u64 offset = 0;           // File offset to start reading
    u64 size = 0;             // Bytes to read

    // Output buffer
    xr_vector<u8> buffer;     // Filled when complete

    // Status
    IOStatus status = IOStatus::Pending;
    float requestTime = 0.0f;

    // Callback (called when complete)
    using Callback = std::function<void(AsyncIORequest&)>;
    Callback callback;

    // User data (optional)
    void* userData = nullptr;

    // Error info
    shared_str errorMessage;

    // Request ID (for tracking)
    u32 requestID = 0;
};

// ═══════════════════════════════════════════════════
//  ASYNC I/O MANAGER
// ═══════════════════════════════════════════════════

class AsyncIOManager {
public:
    AsyncIOManager();
    ~AsyncIOManager();

    // ═══════════════════════════════════════════════════
    //  REQUEST SUBMISSION
    // ═══════════════════════════════════════════════════

    // Submit async read request
    u32 ReadAsync(
        const char* filePath,
        u64 offset,
        u64 size,
        AsyncIORequest::Callback callback,
        void* userData = nullptr
    );

    // Cancel pending request
    void CancelRequest(u32 requestID);

    // ═══════════════════════════════════════════════════
    //  STATUS CHECKING
    // ═══════════════════════════════════════════════════

    IOStatus GetRequestStatus(u32 requestID) const;
    bool IsRequestComplete(u32 requestID) const;

    // ═══════════════════════════════════════════════════
    //  UPDATE (Call from Main Thread)
    // ═══════════════════════════════════════════════════

    // Process completed requests (invoke callbacks)
    void ProcessCompletedRequests();

    // ═══════════════════════════════════════════════════
    //  CONFIGURATION
    // ═══════════════════════════════════════════════════

    void SetMaxConcurrentRequests(u32 count) { m_maxConcurrent = count; }
    u32 GetMaxConcurrentRequests() const { return m_maxConcurrent; }

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        u32 requestsPending = 0;
        u32 requestsInProgress = 0;
        u32 requestsCompleted = 0;
        u32 requestsFailed = 0;

        u64 bytesRead = 0;
        float avgReadTime = 0.0f;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    // ═══════════════════════════════════════════════════
    //  THREADING
    // ═══════════════════════════════════════════════════

    struct WorkerThread {
        std::thread thread;
        bool shouldExit = false;
    };

    xr_vector<WorkerThread> m_workers;

    // Worker thread function
    void WorkerThreadFunc(u32 workerID);

    // ═══════════════════════════════════════════════════
    //  REQUEST QUEUES (Thread-Safe)
    // ═══════════════════════════════════════════════════

    // Pending requests (not yet started)
    mutable std::mutex m_pendingMutex;
    xr_vector<AsyncIORequest> m_pendingRequests;
    xr_map<u32, u32> m_idToIndex;  // RequestID → index in pending

    // Active requests (being processed)
    mutable std::mutex m_activeMutex;
    xr_vector<AsyncIORequest> m_activeRequests;

    // Completed requests (ready for callback)
    mutable std::mutex m_completedMutex;
    xr_vector<AsyncIORequest> m_completedRequests;

    // ═══════════════════════════════════════════════════
    //  CONFIGURATION
    // ═══════════════════════════════════════════════════

    u32 m_maxConcurrent = 4;      // Max parallel I/O operations
    u32 m_nextRequestID = 1;

    // ═══════════════════════════════════════════════════
    //  SYNCHRONIZATION
    // ═══════════════════════════════════════════════════

    std::condition_variable m_workAvailable;
    std::mutex m_workMutex;

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources
