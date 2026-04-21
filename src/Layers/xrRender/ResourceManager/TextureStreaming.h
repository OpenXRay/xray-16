#pragma once

#include "TextureManager.h"
#include "AsyncIO.h"

// Texture Streaming System
// Week 2 - Day 3: Task 3.1
// Week 3 - Day 5: Task 5.3 - Async I/O integration

namespace xray::render::fg {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  STREAMING REQUEST
// ═══════════════════════════════════════════════════

struct StreamingRequest {
    TextureHandle handle;
    u32 currentMips;        // Currently resident
    u32 targetMips;         // Want to reach
    TexturePriority priority;
    float requestTime;      // When requested (for timeout)

    enum Status {
        Pending,            // Waiting to start
        InProgress,         // Loading from disk
        Uploading,          // Uploading to GPU
        Complete,           // Done
        Failed              // Error occurred
    };
    Status status = Pending;

    // For async I/O (Week 3)
    void* ioHandle = nullptr;  // Platform-specific async I/O handle
    xr_vector<u8> stagingBuffer;  // CPU memory for loaded data

    // Comparison for priority queue
    bool operator<(const StreamingRequest& other) const {
        // Higher priority = process first
        if (priority != other.priority)
            return priority < other.priority;
        return requestTime < other.requestTime;
    }
};

// ═══════════════════════════════════════════════════
//  STREAMING MANAGER
// ═══════════════════════════════════════════════════

class StreamingManager {
public:
    StreamingManager(xray::render::fg::RenderDevice* device, TextureManager* texManager);
    ~StreamingManager();

    // ═══════════════════════════════════════════════════
    //  REQUEST MANAGEMENT
    // ═══════════════════════════════════════════════════

    // Request specific mip levels
    void RequestMips(TextureHandle handle, u32 targetMips, TexturePriority priority);

    // Cancel pending request
    void CancelRequest(TextureHandle handle);

    // Check if request is active
    bool HasPendingRequest(TextureHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  UPDATE (Per Frame)
    // ═══════════════════════════════════════════════════

    void Update(float deltaTime);

    // ═══════════════════════════════════════════════════
    //  CONFIGURATION
    // ═══════════════════════════════════════════════════

    // Max concurrent streaming operations
    void SetMaxConcurrentStreams(u32 count) { m_maxConcurrentStreams = count; }
    u32 GetMaxConcurrentStreams() const { return m_maxConcurrentStreams; }

    // Bandwidth limit (bytes per frame)
    void SetBandwidthLimit(u64 bytesPerFrame) { m_bandwidthLimit = bytesPerFrame; }
    u64 GetBandwidthLimit() const { return m_bandwidthLimit; }

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Statistics {
        u32 requestsPending = 0;
        u32 requestsInProgress = 0;
        u32 requestsCompleted = 0;
        u32 requestsFailed = 0;

        u64 bytesStreamedThisFrame = 0;
        u64 bytesStreamedTotal = 0;

        float avgStreamTime = 0.0f;
    };

    Statistics GetStatistics() const { return m_stats; }
    void PrintStatistics() const;

private:
    xray::render::fg::RenderDevice* m_device;
    TextureManager* m_texManager;
    AsyncIOManager* m_asyncIO;  // Async I/O manager

    // ═══════════════════════════════════════════════════
    //  REQUEST QUEUE (Priority-based)
    // ═══════════════════════════════════════════════════

    xr_vector<StreamingRequest> m_pendingRequests;  // Sorted by priority
    xr_vector<StreamingRequest> m_activeRequests;   // Currently processing

    // Handle → Request lookup (for cancellation)
    xr_map<TextureHandle, u32> m_handleToRequest;

    // ═══════════════════════════════════════════════════
    //  CONFIGURATION
    // ═══════════════════════════════════════════════════

    u32 m_maxConcurrentStreams = 4;       // Max parallel loads
    u64 m_bandwidthLimit = 32 * 1024 * 1024;  // 32 MB/frame

    // ═══════════════════════════════════════════════════
    //  INTERNAL METHODS
    // ═══════════════════════════════════════════════════

    void ProcessPendingRequests();
    void ProcessActiveRequests();
    void StartRequest(StreamingRequest& request);
    void CompleteRequest(StreamingRequest& request);
    void FailRequest(StreamingRequest& request, const char* reason);

    // Mip loading
    bool LoadMipsFromDisk(StreamingRequest& request);
    bool UploadMipsToGPU(StreamingRequest& request);

    // Async I/O callback
    void OnAsyncLoadComplete(TextureHandle handle, AsyncIORequest& ioRequest);

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources
