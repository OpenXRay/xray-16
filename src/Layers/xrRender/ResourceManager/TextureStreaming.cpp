#include "stdafx.h"
#include "TextureStreaming.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"
#include "xrEngine/IFrameGraphRender.h"

// Texture Streaming System Implementation
// Week 2 - Day 3: Task 3.2
// Week 3 - Day 5: Task 5.3 - Async I/O integration

namespace xray::render::resources {

StreamingManager::StreamingManager(xray::render::ng::RenderDevice* device, TextureManager* texManager)
    : m_device(device)
    , m_texManager(texManager)
    , m_asyncIO(nullptr)
{
    VERIFY(m_texManager);

    // Create async I/O manager
    m_asyncIO = xr_new<AsyncIOManager>();

    // Msg("! [StreamingManager] Created (max concurrent: %u, bandwidth: %llu MB/frame)",
    //     m_maxConcurrentStreams,
    //     m_bandwidthLimit / (1024 * 1024));
}

StreamingManager::~StreamingManager() {
    // Cancel all pending requests
    for (auto& request : m_activeRequests) {
        if (request.ioHandle) {
            u32 requestID = (u32)(uintptr_t)request.ioHandle;
            m_asyncIO->CancelRequest(requestID);
        }
    }

    PrintStatistics();

    // Destroy async I/O manager
    xr_delete(m_asyncIO);
}

// ═══════════════════════════════════════════════════
//  REQUEST MANAGEMENT
// ═══════════════════════════════════════════════════

void StreamingManager::RequestMips(
    TextureHandle handle,
    u32 targetMips,
    TexturePriority priority)
{
    // Check if already requested
    auto it = m_handleToRequest.find(handle);
    if (it != m_handleToRequest.end()) {
        // Already have a request, update priority
        StreamingRequest& existing = m_pendingRequests[it->second];
        existing.priority = std::min(existing.priority, priority);  // Higher priority wins
        existing.targetMips = std::max(existing.targetMips, targetMips);
        return;
    }

    // Get current state
    const TextureMetadata* meta = m_texManager->GetMetadata(handle);
    if (!meta) {
        // Msg("! [StreamingManager] ⚠️ Invalid handle");
        return;
    }

    // Already have enough mips?
    if (meta->residentMips >= targetMips) {
        return;
    }

    // Create request
    StreamingRequest request;
    request.handle = handle;
    request.currentMips = meta->residentMips;
    request.targetMips = std::min(targetMips, meta->totalMips);
    request.priority = priority;
    request.requestTime = Device.fTimeGlobal;
    request.status = StreamingRequest::Pending;

    // Add to pending queue
    u32 index = (u32)m_pendingRequests.size();
    m_pendingRequests.push_back(request);
    m_handleToRequest[handle] = index;

    // Sort by priority
    std::sort(m_pendingRequests.begin(), m_pendingRequests.end());

    m_stats.requestsPending++;

    // Msg("! [StreamingManager] Request: %s (%u → %u mips, priority=%d)",
    //     meta->filePath.c_str(),
    //     request.currentMips,
    //     request.targetMips,
    //     (int)priority);
}

void StreamingManager::CancelRequest(TextureHandle handle) {
    auto it = m_handleToRequest.find(handle);
    if (it == m_handleToRequest.end()) {
        return;
    }

    u32 index = it->second;

    // Remove from pending
    if (index < m_pendingRequests.size()) {
        m_pendingRequests.erase(m_pendingRequests.begin() + index);
        m_handleToRequest.erase(it);
        m_stats.requestsPending--;
    }

    // TODO: Cancel active request if in progress
}

bool StreamingManager::HasPendingRequest(TextureHandle handle) const {
    return m_handleToRequest.find(handle) != m_handleToRequest.end();
}

// ═══════════════════════════════════════════════════
//  UPDATE (Called Every Frame)
// ═══════════════════════════════════════════════════

void StreamingManager::Update(float deltaTime) {
    m_stats.bytesStreamedThisFrame = 0;

    // Process completed async I/O requests first
    m_asyncIO->ProcessCompletedRequests();

    // Process active requests (check for completion)
    ProcessActiveRequests();

    // Start new requests if we have bandwidth
    ProcessPendingRequests();
}

void StreamingManager::ProcessPendingRequests() {
    // Start new requests up to concurrent limit
    while (m_activeRequests.size() < m_maxConcurrentStreams &&
           !m_pendingRequests.empty()) {

        // Get highest priority request
        StreamingRequest request = m_pendingRequests.front();
        m_pendingRequests.erase(m_pendingRequests.begin());

        // Start it
        StartRequest(request);

        // Move to active
        m_activeRequests.push_back(request);
        m_stats.requestsPending--;
        m_stats.requestsInProgress++;
    }
}

void StreamingManager::ProcessActiveRequests() {
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ) {
        StreamingRequest& request = *it;

        switch (request.status) {
            case StreamingRequest::Pending:
                // Shouldn't happen
                StartRequest(request);
                ++it;
                break;

            case StreamingRequest::InProgress:
                // Check if async I/O is complete
                if (request.ioHandle) {
                    u32 requestID = (u32)(uintptr_t)request.ioHandle;

                    if (m_asyncIO->IsRequestComplete(requestID)) {
                        // I/O completed - data should be in stagingBuffer now
                        if (!request.stagingBuffer.empty()) {
                            request.status = StreamingRequest::Uploading;
                        } else {
                            FailRequest(request, "Async I/O failed or returned empty buffer");
                            it = m_activeRequests.erase(it);
                            continue;
                        }
                    }
                } else {
                    // No async handle yet - try to start async load
                    if (!LoadMipsFromDisk(request)) {
                        FailRequest(request, "Failed to start async load");
                        it = m_activeRequests.erase(it);
                        continue;
                    }
                }
                ++it;
                break;

            case StreamingRequest::Uploading:
                // Upload to GPU
                if (UploadMipsToGPU(request)) {
                    CompleteRequest(request);
                    it = m_activeRequests.erase(it);
                    continue;
                } else {
                    FailRequest(request, "Failed to upload to GPU");
                    it = m_activeRequests.erase(it);
                    continue;
                }
                break;

            case StreamingRequest::Complete:
            case StreamingRequest::Failed:
                // Remove from active
                it = m_activeRequests.erase(it);
                continue;
        }
    }
}

// ═══════════════════════════════════════════════════
//  REQUEST PROCESSING
// ═══════════════════════════════════════════════════

void StreamingManager::StartRequest(StreamingRequest& request) {
    // Msg("! [StreamingManager] Starting: handle=%u.%u",
    //     request.handle.index, request.handle.generation);

    request.status = StreamingRequest::InProgress;

    // Async I/O will be started in LoadMipsFromDisk()
}

bool StreamingManager::LoadMipsFromDisk(StreamingRequest& request) {
    const TextureMetadata* meta = m_texManager->GetMetadata(request.handle);
    if (!meta) {
        return false;
    }

    // ═══════════════════════════════════════════════════
    //  ASYNC I/O VERSION
    // ═══════════════════════════════════════════════════

    if (!request.ioHandle) {
        // First call - kick off async read

        // Msg("! [StreamingManager] Starting async load: %s", meta->filePath.c_str());

        // Calculate file size
        // For now, just read entire file (later can optimize to read only specific mips)
        IReader* reader = FS.r_open(meta->filePath.c_str());
        if (!reader) {
            return false;
        }

        u64 size = reader->length();
        FS.r_close(reader);

        // Submit async read
        u32 requestID = m_asyncIO->ReadAsync(
            meta->filePath.c_str(),
            0,  // offset (read entire file for now)
            size,
            [this, handle = request.handle](AsyncIORequest& ioRequest) {
                // Callback when complete
                this->OnAsyncLoadComplete(handle, ioRequest);
            },
            (void*)(uintptr_t)request.handle.index
        );

        request.ioHandle = (void*)(uintptr_t)requestID;
        return true;  // Async load started
    }

    // Already started - waiting for completion
    return true;
}

bool StreamingManager::UploadMipsToGPU(StreamingRequest& request) {
    TextureMetadata* meta = const_cast<TextureMetadata*>(
        m_texManager->GetMetadata(request.handle)
    );

    if (!meta || !meta->nvrhiTexture) {
        return false;
    }

    // Msg("! [StreamingManager] Uploading mips to GPU...");

    // Check bandwidth limit
    if (m_stats.bytesStreamedThisFrame + request.stagingBuffer.size() > m_bandwidthLimit) {
        // Defer to next frame
        return false;
    }

    // Load DDS again to get proper mip layout info
    DDSData ddsData;
    if (!DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData)) {
        return false;
    }

    // Upload directly via NVRHI
    nvrhi::ICommandList* cmdList = m_device->GetNativeDevice()->createCommandList();
    cmdList->open();

    u32 startMip = request.currentMips;
    u32 endMip = request.targetMips;

    for (u32 mip = startMip; mip < endMip; mip++) {
        if (mip >= ddsData.mipLevels.size()) break;

        const DDSMipLevel& mipData = ddsData.mipLevels[mip];

        // Use pitch values calculated by DDSLoader (DRY principle)
        // writeTexture signature: (texture, arraySlice, mipLevel, data, rowPitch, depthPitch)
        cmdList->writeTexture(meta->nvrhiTexture, 0, mip, mipData.data, mipData.rowPitch, mipData.slicePitch);
    }

    cmdList->close();
    m_device->GetNativeDevice()->executeCommandList(cmdList);
    cmdList->Release();  // CRITICAL: Release to avoid massive leak during streaming!

    // Update metadata
    meta->residentMips = request.targetMips;

    m_stats.bytesStreamedThisFrame += request.stagingBuffer.size();
    m_stats.bytesStreamedTotal += request.stagingBuffer.size();

    // Msg("! [StreamingManager] ✅ Upload complete");

    return true;
}

void StreamingManager::CompleteRequest(StreamingRequest& request) {
    // Msg("! [StreamingManager] ✅ Request complete: handle=%u.%u",
    //     request.handle.index, request.handle.generation);

    // Remove from lookup
    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsCompleted++;
}

void StreamingManager::FailRequest(StreamingRequest& request, const char* reason) {
    // Msg("! [StreamingManager] ❌ Request failed: %s", reason);

    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsFailed++;
}

// ═══════════════════════════════════════════════════
//  ASYNC I/O CALLBACK
// ═══════════════════════════════════════════════════

void StreamingManager::OnAsyncLoadComplete(
    TextureHandle handle,
    AsyncIORequest& ioRequest)
{
    // Msg("! [StreamingManager] Async load complete: handle=%u.%u, status=%d",
    //     handle.index, handle.generation, (int)ioRequest.status);

    if (ioRequest.status == IOStatus::Complete) {
        // Find streaming request
        for (auto& request : m_activeRequests) {
            if (request.handle == handle) {
                // Parse DDS data from loaded buffer
                DDSData ddsData;

                // Create temporary IReader from buffer
                // (DDSLoader expects IReader, so we need to adapt)
                // For now, re-load from file path to get DDS structure
                const TextureMetadata* meta = m_texManager->GetMetadata(handle);
                if (meta && DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData)) {
                    // Extract only the mips we need
                    u32 startMip = request.currentMips;
                    u32 endMip = request.targetMips;

                    request.stagingBuffer.clear();

                    for (u32 mip = startMip; mip < endMip; mip++) {
                        if (mip >= ddsData.mipLevels.size()) break;

                        const DDSMipLevel& mipData = ddsData.mipLevels[mip];

                        // Copy to staging buffer
                        u32 offset = (u32)request.stagingBuffer.size();
                        request.stagingBuffer.resize(offset + mipData.size);
                        memcpy(
                            request.stagingBuffer.data() + offset,
                            mipData.data,
                            mipData.size
                        );
                    }

                    // Msg("! [StreamingManager] Extracted %u mips (%llu KB)",
                    //     endMip - startMip,
                    //     request.stagingBuffer.size() / 1024);

                    request.status = StreamingRequest::Uploading;
                } else {
                    // Msg("! [StreamingManager] ❌ Failed to parse DDS data");
                    request.status = StreamingRequest::Failed;
                }
                break;
            }
        }
    } else {
        // Failed - mark request as failed
        for (auto& request : m_activeRequests) {
            if (request.handle == handle) {
                request.status = StreamingRequest::Failed;
                // Msg("! [StreamingManager] ❌ Async I/O failed: %s",
                //     ioRequest.errorMessage.c_str());
                break;
            }
        }
    }
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

void StreamingManager::PrintStatistics() const {
    Msg("! [StreamingManager] Statistics:");
    Msg("!   Pending: %u, Active: %u", m_stats.requestsPending, m_stats.requestsInProgress);
    Msg("!   Completed: %u, Failed: %u", m_stats.requestsCompleted, m_stats.requestsFailed);
    Msg("!   Streamed: %llu MB total, %llu KB this frame",
        m_stats.bytesStreamedTotal / (1024 * 1024),
        m_stats.bytesStreamedThisFrame / 1024);
}

} // namespace xray::render::resources
