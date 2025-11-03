#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace xray::render::resources {

enum class IOStatus : u8 {
    Pending,
    InProgress,
    Complete,
    Failed
};

struct AsyncIORequest {
    shared_str filePath;
    u64 offset = 0;
    u64 size = 0;

    xr_vector<u8> buffer;

    IOStatus status = IOStatus::Pending;
    float requestTime = 0.0f;

    using Callback = std::function<void(AsyncIORequest&)>;
    Callback callback;

    void* userData = nullptr;

    shared_str errorMessage;

    u32 requestID = 0;
};

class AsyncIOManager {
public:
    AsyncIOManager();
    ~AsyncIOManager();

    u32 ReadAsync(
        const char* filePath,
        u64 offset,
        u64 size,
        AsyncIORequest::Callback callback,
        void* userData = nullptr
    );

    void CancelRequest(u32 requestID);

    IOStatus GetRequestStatus(u32 requestID) const;
    bool IsRequestComplete(u32 requestID) const;

    void ProcessCompletedRequests();

    void SetMaxConcurrentRequests(u32 count) { m_maxConcurrent = count; }
    u32 GetMaxConcurrentRequests() const { return m_maxConcurrent; }

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

    struct WorkerThread {
        std::thread thread;
        bool shouldExit = false;
    };

    xr_vector<WorkerThread> m_workers;

    void WorkerThreadFunc(u32 workerID);

    mutable std::mutex m_pendingMutex;
    xr_vector<AsyncIORequest> m_pendingRequests;
    xr_map<u32, u32> m_idToIndex;

    mutable std::mutex m_activeMutex;
    xr_vector<AsyncIORequest> m_activeRequests;

    mutable std::mutex m_completedMutex;
    xr_vector<AsyncIORequest> m_completedRequests;

    u32 m_maxConcurrent = 4;
    u32 m_nextRequestID = 1;

    std::condition_variable m_workAvailable;
    std::mutex m_workMutex;

    mutable Statistics m_stats;
};

}
