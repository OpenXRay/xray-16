#include "stdafx.h"
#include "TextureStreaming.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"

// Texture Streaming System Implementation
// Week 2 - Day 3: Task 3.2
// Week 3 - Day 5: Task 5.3 - Async I/O integration

namespace xray::render::resources {

// Persistent upload command list for streaming (prevents exhausting D3D12 allocator pool)
static nvrhi::CommandListHandle s_streamingCmdList;
static std::mutex s_streamingCmdListMutex;

StreamingManager::StreamingManager(xray::render::fg::RenderDevice* device, TextureManager* texManager)
    : m_device(device)
    , m_texManager(texManager)
{
    VERIFY(m_texManager);
}

StreamingManager::~StreamingManager() {
    if (m_asyncIO && m_ioEpoch)
        m_asyncIO->DrainEpoch(m_ioEpoch);

    PrintStatistics();

    s_streamingCmdList = nullptr;
}

AsyncIOManager* StreamingManager::IOService() {
    if (m_asyncIO)
        return m_asyncIO;
    if (!m_device)
        return nullptr;
    FGResourceManager* owner = m_device->GetFGResourceManager();
    if (!owner)
        return nullptr;
    m_asyncIO = owner->GetIOService();
    if (m_asyncIO && !m_ioEpoch)
        m_ioEpoch = m_asyncIO->AcquireEpoch();
    return m_asyncIO;
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
    if (it == m_handleToRequest.end())
        return;

    for (auto pending = m_pendingRequests.begin(); pending != m_pendingRequests.end();) {
        if (pending->handle == handle) {
            pending = m_pendingRequests.erase(pending);
            if (m_stats.requestsPending)
                m_stats.requestsPending--;
            continue;
        }
        ++pending;
    }

    for (auto active = m_activeRequests.begin(); active != m_activeRequests.end();) {
        if (active->handle == handle) {
            if (active->ioRequest && m_asyncIO)
                m_asyncIO->CancelRequest(active->ioRequest);
            active = m_activeRequests.erase(active);
            if (m_stats.requestsInProgress)
                m_stats.requestsInProgress--;
            continue;
        }
        ++active;
    }

    m_handleToRequest.erase(it);
}

bool StreamingManager::HasPendingRequest(TextureHandle handle) const {
    return m_handleToRequest.find(handle) != m_handleToRequest.end();
}

// ═══════════════════════════════════════════════════
//  UPDATE (Called Every Frame)
// ═══════════════════════════════════════════════════

void StreamingManager::Update(float deltaTime) {
    m_stats.bytesStreamedThisFrame = 0;

    ProcessActiveRequests();
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
                StartRequest(request);
                ++it;
                break;

            case StreamingRequest::InProgress:
                if (!request.ioRequest) {
                    if (!LoadMipsFromDisk(request)) {
                        FailRequest(request, "failed to start the async load");
                        it = m_activeRequests.erase(it);
                        continue;
                    }
                }
                ++it;
                break;

            case StreamingRequest::Uploading:
            {
                const UploadResult result = UploadMipsToGPU(request);
                if (result == UploadResult::Deferred) {
                    ++it;
                    break;
                }
                if (result == UploadResult::Uploaded)
                    CompleteRequest(request);
                else
                    FailRequest(request, "failed to upload the streamed mips");
                it = m_activeRequests.erase(it);
                continue;
            }

            case StreamingRequest::Complete:
            case StreamingRequest::Failed:
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
    if (!meta)
        return false;

    if (request.ioRequest)
        return true;

    AsyncIOManager* io = IOService();
    if (!io)
        return false;

    IReader* reader = FS.r_open(meta->filePath.c_str());
    if (!reader)
        return false;
    const u64 size = u64(reader->length());
    FS.r_close(reader);
    if (size == 0)
        return false;

    request.ioRequest = io->ReadAsync(
        meta->filePath.c_str(),
        0,
        size,
        m_ioEpoch,
        [this, handle = request.handle](AsyncIORequest& ioRequest) {
            this->OnAsyncLoadComplete(handle, ioRequest);
        },
        (void*)(uintptr_t)request.handle.index);

    return request.ioRequest != 0;
}

StreamingManager::UploadResult StreamingManager::UploadMipsToGPU(StreamingRequest& request) {
    TextureMetadata* meta = const_cast<TextureMetadata*>(
        m_texManager->GetMetadata(request.handle)
    );

    if (!meta || !meta->nvrhiTexture)
        return UploadResult::Failed;

    if (m_stats.bytesStreamedThisFrame != 0
        && m_stats.bytesStreamedThisFrame + request.stagingBuffer.size() > m_bandwidthLimit)
        return UploadResult::Deferred;

    // Load DDS again to get proper mip layout info
    DDSData ddsData;
    if (!DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData))
        return UploadResult::Failed;

    // Upload texture mips
    u32 startMip = request.currentMips;
    u32 endMip = request.targetMips;

    // Use backend command list if in-frame, otherwise use persistent streaming cmdlist
    nvrhi::ICommandList* cmdList = nullptr;
    std::unique_lock<std::mutex> streamingLock;
    bool needsExecute = false;

    if (GEnv.Backend && GEnv.Backend->IsInFrame()) {
        cmdList = GEnv.Backend->GetCommandList();
    } else {
        // Use persistent streaming command list (prevents exhausting D3D12 allocator pool)
        streamingLock = std::unique_lock<std::mutex>(s_streamingCmdListMutex);
        if (!s_streamingCmdList) {
            s_streamingCmdList = m_device->GetNativeDevice()->createCommandList();
        }
        s_streamingCmdList->open();
        cmdList = s_streamingCmdList.Get();
        needsExecute = true;
    }

    // Add debug marker for texture streaming uploads
    cmdList->beginMarker("Texture Streaming Upload");

    for (u32 mip = startMip; mip < endMip; mip++) {
        if (mip >= ddsData.mipLevels.size()) break;

        const DDSMipLevel& mipData = ddsData.mipLevels[mip];
        cmdList->writeTexture(meta->nvrhiTexture, 0, mip, mipData.data, mipData.rowPitch, mipData.slicePitch);
    }

    cmdList->endMarker();

    if (needsExecute) {
        s_streamingCmdList->close();
        GEnv.Backend->ExecuteCommandList(s_streamingCmdList);
    }

    // Update metadata
    meta->residentMips = request.targetMips;
    m_texManager->NotifyContentChanged(*meta);

    m_stats.bytesStreamedThisFrame += request.stagingBuffer.size();
    m_stats.bytesStreamedTotal += request.stagingBuffer.size();

    // Msg("! [StreamingManager] ✅ Upload complete");

    return UploadResult::Uploaded;
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
