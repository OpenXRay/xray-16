#include "stdafx.h"
#include "TextureStreaming.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::resources {

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

void StreamingManager::RequestMips(
    TextureHandle handle,
    u32 targetMips,
    TexturePriority priority)
{
    auto it = m_handleToRequest.find(handle);
    if (it != m_handleToRequest.end()) {
        StreamingRequest& existing = m_pendingRequests[it->second];
        existing.priority = std::min(existing.priority, priority);
        existing.targetMips = std::max(existing.targetMips, targetMips);
        return;
    }

    const TextureMetadata* meta = m_texManager->GetMetadata(handle);
    if (!meta) {
        return;
    }

    if (meta->residentMips >= targetMips) {
        return;
    }

    StreamingRequest request;
    request.handle = handle;
    request.currentMips = meta->residentMips;
    request.targetMips = std::min(targetMips, meta->totalMips);
    request.priority = priority;
    request.requestTime = Device.fTimeGlobal;
    request.status = StreamingRequest::Pending;

    u32 index = (u32)m_pendingRequests.size();
    m_pendingRequests.push_back(request);
    m_handleToRequest[handle] = index;

    std::sort(m_pendingRequests.begin(), m_pendingRequests.end());

    m_stats.requestsPending++;

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

void StreamingManager::Update(float deltaTime) {
    m_stats.bytesStreamedThisFrame = 0;

    ProcessActiveRequests();
    ProcessPendingRequests();
}

void StreamingManager::ProcessPendingRequests() {
    while (m_activeRequests.size() < m_maxConcurrentStreams &&
           !m_pendingRequests.empty()) {

        StreamingRequest request = m_pendingRequests.front();
        m_pendingRequests.erase(m_pendingRequests.begin());

        StartRequest(request);

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

void StreamingManager::StartRequest(StreamingRequest& request) {

    request.status = StreamingRequest::InProgress;

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

    DDSData ddsData;
    if (!DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData))
        return UploadResult::Failed;

    u32 startMip = request.currentMips;
    u32 endMip = request.targetMips;

    nvrhi::ICommandList* cmdList = nullptr;
    std::unique_lock<std::mutex> streamingLock;
    bool needsExecute = false;

    if (GEnv.Backend && GEnv.Backend->IsInFrame()) {
        cmdList = GEnv.Backend->GetCommandList();
    } else {
        streamingLock = std::unique_lock<std::mutex>(s_streamingCmdListMutex);
        if (!s_streamingCmdList) {
            s_streamingCmdList = m_device->GetNativeDevice()->createCommandList();
        }
        s_streamingCmdList->open();
        cmdList = s_streamingCmdList.Get();
        needsExecute = true;
    }

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

    meta->residentMips = request.targetMips;
    m_texManager->NotifyContentChanged(*meta);

    m_stats.bytesStreamedThisFrame += request.stagingBuffer.size();
    m_stats.bytesStreamedTotal += request.stagingBuffer.size();

    return UploadResult::Uploaded;
}

void StreamingManager::CompleteRequest(StreamingRequest& request) {

    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsCompleted++;
}

void StreamingManager::FailRequest(StreamingRequest& request, const char* reason) {

    m_handleToRequest.erase(request.handle);

    m_stats.requestsInProgress--;
    m_stats.requestsFailed++;
}

void StreamingManager::OnAsyncLoadComplete(
    TextureHandle handle,
    AsyncIORequest& ioRequest)
{

    if (ioRequest.status == IOStatus::Complete) {
        for (auto& request : m_activeRequests) {
            if (request.handle == handle) {
                DDSData ddsData;

                const TextureMetadata* meta = m_texManager->GetMetadata(handle);
                if (meta && DDSLoader::LoadFromFile(meta->filePath.c_str(), ddsData)) {
                    u32 startMip = request.currentMips;
                    u32 endMip = request.targetMips;

                    request.stagingBuffer.clear();

                    for (u32 mip = startMip; mip < endMip; mip++) {
                        if (mip >= ddsData.mipLevels.size()) break;

                        const DDSMipLevel& mipData = ddsData.mipLevels[mip];

                        u32 offset = (u32)request.stagingBuffer.size();
                        request.stagingBuffer.resize(offset + mipData.size);
                        memcpy(
                            request.stagingBuffer.data() + offset,
                            mipData.data,
                            mipData.size
                        );
                    }

                    request.status = StreamingRequest::Uploading;
                } else {
                    request.status = StreamingRequest::Failed;
                }
                break;
            }
        }
    } else {
        for (auto& request : m_activeRequests) {
            if (request.handle == handle) {
                request.status = StreamingRequest::Failed;
                break;
            }
        }
    }
}

void StreamingManager::PrintStatistics() const {
    Msg("! [StreamingManager] Statistics:");
    Msg("!   Pending: %u, Active: %u", m_stats.requestsPending, m_stats.requestsInProgress);
    Msg("!   Completed: %u, Failed: %u", m_stats.requestsCompleted, m_stats.requestsFailed);
    Msg("!   Streamed: %llu MB total, %llu KB this frame",
        m_stats.bytesStreamedTotal / (1024 * 1024),
        m_stats.bytesStreamedThisFrame / 1024);
}

}
