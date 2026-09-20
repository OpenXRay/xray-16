#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace xray::render::resources {

enum class IOStatus : u8 {
    Unknown,
    Pending,
    Deferred,
    InProgress,
    Complete,
    Failed,
    Canceled
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
    u64 contentTag = 0;

    shared_str errorMessage;

    u64 requestID = 0;
    u32 epoch = 0;
};

class AsyncIOManager {
public:
    AsyncIOManager();
    ~AsyncIOManager();

    u32 AcquireEpoch();
    void CancelEpoch(u32 epoch);
    void DrainEpoch(u32 epoch);
    void DrainAll();

    u64 ReadAsync(
        const char* filePath,
        u64 offset,
        u64 size,
        u32 epoch,
        AsyncIORequest::Callback callback,
        void* userData = nullptr,
        u64 contentTag = 0
    );

    void CancelRequest(u64 requestID);

    IOStatus GetRequestStatus(u64 requestID) const;
    bool IsRequestSettled(u64 requestID) const;
    bool TryTakeResult(u64 requestID, AsyncIORequest& out);

    void Pump();

    void SetMaxConcurrentRequests(u32 count);
    u32 GetMaxConcurrentRequests() const;
    void SetBandwidthLimit(u64 bytesPerWindow);
    u64 GetBandwidthLimit() const;

    struct Statistics {
        u32 requestsPending = 0;
        u32 requestsInProgress = 0;
        u32 requestsRetained = 0;
        u32 requestsCompleted = 0;
        u32 requestsFailed = 0;
        u32 requestsCanceled = 0;
        u32 requestsDeferred = 0;

        u64 bytesRead = 0;
        u64 bytesDispatchedThisWindow = 0;
        float avgReadTime = 0.0f;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    class Entry
    {
    public:
        AsyncIORequest request;
        bool canceled = false;
        bool settled = false;
        bool retained = false;
        bool dispatched = false;
    };

    void WorkerThreadFunc(u32 workerID);
    bool PickRequestLocked(u64& outID);
    bool HasDispatchableLocked() const;
    Entry* FindLocked(u64 requestID);
    const Entry* FindLocked(u64 requestID) const;
    void RemovePendingLocked(u64 requestID);
    void SettleLocked(Entry& entry, IOStatus status, const char* error);
    void DropLocked(u64 requestID, Entry& entry);

    mutable std::mutex m_mutex;
    std::condition_variable m_workAvailable;
    std::condition_variable m_idle;

    xr_unordered_map<u64, Entry> m_requests;
    xr_vector<u64> m_pending;
    xr_vector<u64> m_finished;

    xr_vector<std::thread> m_workers;
    bool m_shouldExit = false;

    u32 m_maxConcurrent = 4;
    u32 m_activeCount = 0;
    u32 m_retainedCount = 0;
    u64 m_nextRequestID = 1;
    u32 m_nextEpoch = 1;

    u64 m_bandwidthLimit = 16ull * 1024ull * 1024ull;
    u64 m_windowBytes = 0;

    Statistics m_stats;
    double m_readTimeTotal = 0.0;
    u64 m_readTimeSamples = 0;

    xr_vector<AsyncIORequest> m_dispatchScratch;
};

} // namespace xray::render::resources
