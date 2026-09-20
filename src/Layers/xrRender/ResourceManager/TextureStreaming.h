#pragma once

#include "TextureManager.h"
#include "AsyncIO.h"

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::resources {

struct StreamingRequest {
    TextureHandle handle;
    u32 currentMips;
    u32 targetMips;
    TexturePriority priority;
    float requestTime;

    enum Status {
        Pending,
        InProgress,
        Uploading,
        Complete,
        Failed
    };
    Status status = Pending;

    u64 ioRequest = 0;
    xr_vector<u8> stagingBuffer;

    bool operator<(const StreamingRequest& other) const {
        if (priority != other.priority)
            return priority < other.priority;
        return requestTime < other.requestTime;
    }
};

class StreamingManager {
public:
    StreamingManager(xray::render::fg::RenderDevice* device, TextureManager* texManager);
    ~StreamingManager();

    void RequestMips(TextureHandle handle, u32 targetMips, TexturePriority priority);

    void CancelRequest(TextureHandle handle);

    bool HasPendingRequest(TextureHandle handle) const;

    void Update(float deltaTime);

    void SetMaxConcurrentStreams(u32 count) { m_maxConcurrentStreams = count; }
    u32 GetMaxConcurrentStreams() const { return m_maxConcurrentStreams; }

    void SetBandwidthLimit(u64 bytesPerFrame) { m_bandwidthLimit = bytesPerFrame; }
    u64 GetBandwidthLimit() const { return m_bandwidthLimit; }

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
    enum class UploadResult : u8 {
        Uploaded,
        Deferred,
        Failed
    };

    AsyncIOManager* IOService();

    xray::render::fg::RenderDevice* m_device;
    TextureManager* m_texManager;
    AsyncIOManager* m_asyncIO = nullptr;
    u32 m_ioEpoch = 0;

    xr_vector<StreamingRequest> m_pendingRequests;
    xr_vector<StreamingRequest> m_activeRequests;

    xr_map<TextureHandle, u32> m_handleToRequest;

    u32 m_maxConcurrentStreams = 4;
    u64 m_bandwidthLimit = 32 * 1024 * 1024;

    void ProcessPendingRequests();
    void ProcessActiveRequests();
    void StartRequest(StreamingRequest& request);
    void CompleteRequest(StreamingRequest& request);
    void FailRequest(StreamingRequest& request, const char* reason);

    bool LoadMipsFromDisk(StreamingRequest& request);
    UploadResult UploadMipsToGPU(StreamingRequest& request);

    void OnAsyncLoadComplete(TextureHandle handle, AsyncIORequest& ioRequest);

    mutable Statistics m_stats;
};

}
