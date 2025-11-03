#include "stdafx.h"
#include "TextureStreaming.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"

// Texture Streaming System Implementation
// Week 2 - Day 3: Task 3.2

namespace xray::render::resources {

StreamingManager::StreamingManager(xray::render::ng::RenderDevice* device, TextureManager* texManager)
    : m_device(device)
    , m_texManager(texManager)
{
    VERIFY(m_device);
    VERIFY(m_texManager);

    Msg("! [StreamingManager] Created (max concurrent: %u, bandwidth: %llu MB/frame)",
        m_maxConcurrentStreams,
        m_bandwidthLimit / (1024 * 1024));
}

StreamingManager::~StreamingManager() {
    // Cancel all pending requests
    for (auto& request : m_activeRequests) {
        if (request.ioHandle) {
            // TODO: Cancel async I/O (Week 3)
        }
    }

    PrintStatistics();
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
        Msg("! [StreamingManager] ⚠️ Invalid handle");
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

    Msg("! [StreamingManager] Request: %s (%u → %u mips, priority=%d)",
        meta->filePath.c_str(),
        request.currentMips,
        request.targetMips,
        (int)priority);
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
                // Check if I/O complete
                // TODO: Check async I/O status (Week 3)
                // For now, assume synchronous
                if (LoadMipsFromDisk(request)) {
                    request.status = StreamingRequest::Uploading;
                } else {
                    FailRequest(request, "Failed to load from disk");
                    it = m_activeRequests.erase(it);
                    continue;
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
    Msg("! [StreamingManager] Starting: handle=%u.%u",
        request.handle.index, request.handle.generation);

    request.status = StreamingRequest::InProgress;

    // TODO: Kick off async I/O (Week 3)
    // For now, will load synchronously in next frame
}

bool StreamingManager::LoadMipsFromDisk(StreamingRequest& request) {
    const TextureMetadata* meta = m_texManager->GetMetadata(request.handle);
    if (!meta) {
        return false;
    }

    Msg("! [StreamingManager] Loading mips from disk: %s", meta->filePath.c_str());

    // Load DDS file
    DDSData ddsData;

    if (!DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData)) {
        return false;
    }

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

    Msg("! [StreamingManager] Loaded %u mips (%llu KB)",
        endMip - startMip,
        request.stagingBuffer.size() / 1024);

    return true;
}

bool StreamingManager::UploadMipsToGPU(StreamingRequest& request) {
    TextureMetadata* meta = const_cast<TextureMetadata*>(
        m_texManager->GetMetadata(request.handle)
    );

    if (!meta || !meta->nvrhiTexture) {
        return false;
    }

    Msg("! [StreamingManager] Uploading mips to GPU...");

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

        // Calculate row pitch for this mip level
        const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(meta->desc.format);
        u32 mipWidth = (meta->desc.width >> mip) > 0 ? (meta->desc.width >> mip) : 1;

        u32 rowPitch;
        if (formatInfo.blockSize > 1) {
            // Block-compressed format (4x4 blocks)
            rowPitch = ((mipWidth + 3) / 4) * formatInfo.bytesPerBlock;
        } else {
            // Uncompressed format
            rowPitch = mipWidth * formatInfo.bytesPerBlock;
        }

        // Write texture data
        cmdList->writeTexture(meta->nvrhiTexture, 0, mip, mipData.data, rowPitch);
    }

    cmdList->close();
    m_device->GetNativeDevice()->executeCommandList(cmdList);

    // Update metadata
    meta->residentMips = request.targetMips;

    m_stats.bytesStreamedThisFrame += request.stagingBuffer.size();
    m_stats.bytesStreamedTotal += request.stagingBuffer.size();

    Msg("! [StreamingManager] ✅ Upload complete");

    return true;
}

void StreamingManager::CompleteRequest(StreamingRequest& request) {
    Msg("! [StreamingManager] ✅ Request complete: handle=%u.%u",
        request.handle.index, request.handle.generation);

    // Remove from lookup
    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsCompleted++;
}

void StreamingManager::FailRequest(StreamingRequest& request, const char* reason) {
    Msg("! [StreamingManager] ❌ Request failed: %s", reason);

    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsFailed++;
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
