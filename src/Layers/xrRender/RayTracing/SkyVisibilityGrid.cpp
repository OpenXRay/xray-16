#include "stdafx.h"
#include "SkyVisibilityGrid.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/device.h"
#include <imgui.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace xray::render::fg
{
namespace
{
constexpr u32 kCacheMagic = 0x56594B53u;
constexpr u32 kCacheVersion = 8;
constexpr u64 kUploadSlice = 4ull << 20;
constexpr u32 kDebugFailureLimit = 3;
constexpr u32 kDebugCaptureWaitFrames = 240;

struct SkyVisibilityCacheHeader
{
    u32 magic;
    u32 version;
    u64 geometryStamp;
    Fvector origin;
    float spacing;
    u32 dims[3];
    u32 count;
    u32 rays;
    float rayDistance;
    float backfaceLimit;
};
}

bool SkyVisibilityLayout::Matches(const SkyVisibilityLayout& other) const
{
    return origin.similar(other.origin, EPS_L) && spacing == other.spacing && dims[0] == other.dims[0] &&
        dims[1] == other.dims[1] && dims[2] == other.dims[2] && count == other.count && rays == other.rays &&
        rayDistance == other.rayDistance && backfaceLimit == other.backfaceLimit;
}

void SkyVisibilityGrid::Initialize(RenderDevice* device)
{
    m_device = device;
}

void SkyVisibilityGrid::Shutdown()
{
    EndLevel();
    m_device = nullptr;
}

void SkyVisibilityGrid::BeginLevel(const Fbox& bounds, u64 geometryStamp)
{
    EndLevel();
    if (!m_device || !bounds.is_valid())
        return;
    m_bounds = bounds;
    m_geometryStamp = geometryStamp;
    m_levelActive = true;
    FS.update_path(m_cachePath, "$level$", "level.skyv");

    const SkyVisibilityLayout layout = BuildLayout();
    if (layout.count && LoadCache(layout) && Allocate(layout))
    {
        m_state = SkyVisibilityState::Uploading;
        Msg("* [SkyVisibility] cache hit: %u x %u x %u probes at %.2f m (%s)", layout.dims[0], layout.dims[1],
            layout.dims[2], layout.spacing, m_cachePath);
        return;
    }
    m_upload.clear();
    StartBake();
}

void SkyVisibilityGrid::EndLevel()
{
    ResetDebugSnapshots();
    ReleaseReadback();
    m_buffer = nullptr;
    m_upload.clear();
    m_upload.shrink_to_fit();
    m_layout = {};
    m_state = SkyVisibilityState::Empty;
    m_bakeNext = 0;
    m_progressStep = 0;
    m_clearPending = false;
    m_rebakeRequested = false;
    m_levelActive = false;
}

void SkyVisibilityGrid::RequestRebake()
{
    m_rebakeRequested = true;
}

void SkyVisibilityGrid::Update()
{
    if (m_rebakeRequested && m_levelActive)
    {
        m_rebakeRequested = false;
        ReleaseReadback();
        ResetDebugSnapshots();
        StartBake();
    }
    PollReadback();
    ConsumeDebugControls();
    PollDebugReadback();
}

bool SkyVisibilityGrid::NeedsScene() const
{
    return m_state == SkyVisibilityState::Baking;
}

bool SkyVisibilityGrid::HasPendingWork() const
{
    return m_buffer && (m_clearPending || m_state == SkyVisibilityState::Uploading);
}

bool SkyVisibilityGrid::TakeBakeSlice(u32 budget, u32& first, u32& count) const
{
    if (m_state != SkyVisibilityState::Baking || !m_buffer || m_bakeNext >= m_layout.count)
        return false;
    first = m_bakeNext;
    count = std::min(budget, m_layout.count - m_bakeNext);
    return count != 0;
}

void SkyVisibilityGrid::RecordPending(nvrhi::ICommandList* commandList)
{
    if (!commandList || !m_buffer)
        return;
    if (m_clearPending)
    {
        commandList->clearBufferUInt(m_buffer, 0xFFFFFFFFu);
        m_clearPending = false;
    }
    if (m_state != SkyVisibilityState::Uploading)
        return;
    const u64 bytes = u64(m_upload.size());
    for (u64 offset = 0; offset < bytes; offset += kUploadSlice)
        commandList->writeBuffer(m_buffer, m_upload.data() + offset, size_t(std::min(kUploadSlice, bytes - offset)), offset);
    m_upload.clear();
    m_upload.shrink_to_fit();
    m_state = SkyVisibilityState::Ready;
}

void SkyVisibilityGrid::CommitBakeSlice(nvrhi::ICommandList* commandList, u32 first, u32 count)
{
    if (m_state != SkyVisibilityState::Baking || first != m_bakeNext || !commandList)
        return;
    m_bakeNext = first + count;
    const u32 step = u32(u64(m_bakeNext) * 10 / std::max(m_layout.count, 1u));
    if (step != m_progressStep)
    {
        m_progressStep = step;
        Msg("* [SkyVisibility] bake %u%% (%u / %u probes)", step * 10, m_bakeNext, m_layout.count);
    }
    if (m_bakeNext < m_layout.count)
        return;

    Msg("* [SkyVisibility] bake finished: %u probes x %u rays in %.1f s", m_layout.count, m_layout.rays,
        m_bakeTimer.GetElapsed_sec());
    m_state = SkyVisibilityState::Ready;
    IRenderBackend* backend = GEnv.Backend;
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!backend || !nvDevice || !backend->SupportsSubmissionLeases())
        return;
    const u64 bytes = u64(m_layout.count) * kProbeBytes;
    nvrhi::BufferDesc desc;
    desc.byteSize = bytes;
    desc.debugName = "SkyVisibility_Readback";
    desc.cpuAccess = nvrhi::CpuAccessMode::Read;
    desc.initialState = nvrhi::ResourceStates::CopyDest;
    desc.keepInitialState = true;
    m_readback = nvDevice->createBuffer(desc);
    if (!m_readback)
        return;
    m_readbackLease = backend->OpenSubmissionLease();
    if (!m_readbackLease)
    {
        m_readback = nullptr;
        return;
    }
    commandList->copyBuffer(m_readback, 0, m_buffer, 0, bytes);
    m_state = SkyVisibilityState::Saving;
}

const SkyVisibilityLayout& SkyVisibilityGrid::GetLayout() const
{
    return m_layout;
}

nvrhi::IBuffer* SkyVisibilityGrid::GetBuffer() const
{
    return m_buffer;
}

SkyVisibilityState SkyVisibilityGrid::GetState() const
{
    return m_state;
}

SkyVisibilityLayout SkyVisibilityGrid::BuildLayout() const
{
    SkyVisibilityLayout layout;
    layout.rays = u32(std::clamp(ps_r_sky_probe_rays, 16, 1024));
    layout.rayDistance = std::clamp(ps_r_sky_probe_ray_distance, 10.0f, 10000.0f);
    layout.backfaceLimit = std::clamp(ps_r_sky_probe_backface, 0.0f, 1.0f);
    if (!m_bounds.is_valid())
        return layout;

    Fvector size;
    m_bounds.getsize(size);
    const u64 maxProbes = u64(std::clamp(ps_r_sky_probe_max, 65536, 33554432));
    float spacing = std::clamp(ps_r_sky_probe_spacing, 0.5f, 16.0f);
    const auto fit = [&](float cell, u32* dims)
    {
        dims[0] = u32(std::floor(size.x / cell)) + 1;
        dims[1] = u32(std::floor(size.y / cell)) + 1;
        dims[2] = u32(std::floor(size.z / cell)) + 1;
        return u64(dims[0]) * dims[1] * dims[2];
    };
    while (fit(spacing, layout.dims) > maxProbes)
        spacing *= 1.25f;

    layout.spacing = spacing;
    layout.count = layout.dims[0] * layout.dims[1] * layout.dims[2];
    Fvector centre;
    m_bounds.getcenter(centre);
    layout.origin.set(centre.x - 0.5f * spacing * float(layout.dims[0] - 1),
        centre.y - 0.5f * spacing * float(layout.dims[1] - 1),
        centre.z - 0.5f * spacing * float(layout.dims[2] - 1));
    return layout;
}

bool SkyVisibilityGrid::Allocate(const SkyVisibilityLayout& layout)
{
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!nvDevice || !layout.count)
        return false;
    const u64 bytes = u64(layout.count) * kProbeBytes;
    if (!m_buffer || m_buffer->getDesc().byteSize != bytes)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkyVisibility_Probes";
        desc.byteSize = bytes;
        desc.structStride = kProbeBytes;
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_buffer = nvDevice->createBuffer(desc);
        if (!m_buffer)
        {
            Msg("! [SkyVisibility] failed to allocate %u probes (%.1f MB)", layout.count, float(bytes) / (1024.0f * 1024.0f));
            return false;
        }
    }
    m_layout = layout;
    return true;
}

void SkyVisibilityGrid::StartBake()
{
    const SkyVisibilityLayout layout = BuildLayout();
    if (!layout.count || !Allocate(layout))
    {
        m_state = SkyVisibilityState::Empty;
        return;
    }
    m_clearPending = true;
    m_bakeNext = 0;
    m_progressStep = 0;
    m_state = SkyVisibilityState::Baking;
    m_bakeTimer.Start();
    Msg("* [SkyVisibility] baking %u x %u x %u probes at %.2f m, %u rays (%.1f MB)", layout.dims[0], layout.dims[1],
        layout.dims[2], layout.spacing, layout.rays, float(u64(layout.count) * kProbeBytes) / (1024.0f * 1024.0f));
}

bool SkyVisibilityGrid::LoadCache(const SkyVisibilityLayout& layout)
{
    if (!FS.exist(m_cachePath))
        return false;
    IReader* reader = FS.r_open(m_cachePath);
    if (!reader)
        return false;
    SkyVisibilityCacheHeader header = {};
    const u64 bytes = u64(layout.count) * kProbeBytes;
    bool valid = u64(reader->length()) == sizeof(header) + bytes;
    if (valid)
    {
        reader->r(&header, sizeof(header));
        SkyVisibilityLayout cached;
        cached.origin = header.origin;
        cached.spacing = header.spacing;
        cached.dims[0] = header.dims[0];
        cached.dims[1] = header.dims[1];
        cached.dims[2] = header.dims[2];
        cached.count = header.count;
        cached.rays = header.rays;
        cached.rayDistance = header.rayDistance;
        cached.backfaceLimit = header.backfaceLimit;
        valid = header.magic == kCacheMagic && header.version == kCacheVersion &&
            header.geometryStamp == m_geometryStamp && cached.Matches(layout);
    }
    if (valid)
    {
        m_upload.resize(size_t(bytes));
        reader->r(m_upload.data(), size_t(bytes));
    }
    FS.r_close(reader);
    if (!valid)
        Msg("* [SkyVisibility] cache stale, rebaking: %s", m_cachePath);
    return valid;
}

void SkyVisibilityGrid::SaveCache(const void* data, u64 bytes) const
{
    IWriter* writer = FS.w_open(m_cachePath);
    if (!writer)
    {
        Msg("! [SkyVisibility] failed to write cache: %s", m_cachePath);
        return;
    }
    SkyVisibilityCacheHeader header = {};
    header.magic = kCacheMagic;
    header.version = kCacheVersion;
    header.geometryStamp = m_geometryStamp;
    header.origin = m_layout.origin;
    header.spacing = m_layout.spacing;
    header.dims[0] = m_layout.dims[0];
    header.dims[1] = m_layout.dims[1];
    header.dims[2] = m_layout.dims[2];
    header.count = m_layout.count;
    header.rays = m_layout.rays;
    header.rayDistance = m_layout.rayDistance;
    header.backfaceLimit = m_layout.backfaceLimit;
    writer->w(&header, sizeof(header));
    writer->w(data, size_t(bytes));
    FS.w_close(writer);
    Msg("* [SkyVisibility] cache saved: %s", m_cachePath);
}

void SkyVisibilityGrid::PollReadback()
{
    IRenderBackend* backend = GEnv.Backend;
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!m_readbackLease || !backend || !nvDevice)
        return;
    const auto state = backend->PollSubmissionLease(m_readbackLease);
    if (state == IRenderBackend::SubmissionLeaseState::Open || state == IRenderBackend::SubmissionLeaseState::Pending)
        return;
    if (state == IRenderBackend::SubmissionLeaseState::Complete && m_readback)
    {
        const u64 bytes = u64(m_layout.count) * kProbeBytes;
        if (const void* data = nvDevice->mapBuffer(m_readback, nvrhi::CpuAccessMode::Read))
        {
            SaveCache(data, bytes);
            nvDevice->unmapBuffer(m_readback);
        }
    }
    ReleaseReadback();
    m_state = SkyVisibilityState::Ready;
}

void SkyVisibilityGrid::ReleaseReadback()
{
    if (m_readbackLease && GEnv.Backend)
        GEnv.Backend->ReleaseSubmissionLease(m_readbackLease);
    m_readbackLease = 0;
    m_readback = nullptr;
    if (m_state == SkyVisibilityState::Saving)
        m_state = SkyVisibilityState::Ready;
}

bool SkyVisibilityGrid::PrepareDebugSnapshot()
{
    if (ps_r_sky_probe_debug != 3)
        return false;
    IRenderBackend* backend = GEnv.Backend;
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!nvDevice || !m_levelActive || !m_buffer)
        return false;
    if (!backend || !backend->SupportsSubmissionLeases())
    {
        if (m_debugError.empty())
        {
            m_debugError = "render backend has no submission leases; GPU snapshot readback is unsupported";
            Msg("! [SkyProbeInspect] %s", m_debugError.c_str());
        }
        return false;
    }
    if (m_debugAllocFailed)
        return false;
    if (m_debugBuffer && m_debugReadback)
        return true;

    if (!m_debugBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkyProbe_DebugSnapshot";
        desc.byteSize = sizeof(SkyProbeDebugSnapshot);
        desc.structStride = SkyProbeDebugSnapshot::kBufferStride;
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_debugBuffer = nvDevice->createBuffer(desc);
    }
    if (m_debugBuffer && !m_debugReadback)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkyProbe_DebugSnapshotReadback";
        desc.byteSize = sizeof(SkyProbeDebugSnapshot);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        m_debugReadback = nvDevice->createBuffer(desc);
    }
    if (!m_debugBuffer || !m_debugReadback)
    {
        m_debugBuffer = nullptr;
        m_debugReadback = nullptr;
        m_debugAllocFailed = true;
        m_debugError = "failed to allocate the sky probe diagnostic snapshot buffers";
        Msg("! [SkyProbeInspect] %s", m_debugError.c_str());
        return false;
    }
    return true;
}

bool SkyVisibilityGrid::NeedsDebugSnapshot() const
{
    if (ps_r_sky_probe_debug != 3 || !m_levelActive || !m_buffer || m_debugLease || m_debugAllocFailed)
        return false;
    if (!GEnv.Backend || !GEnv.Backend->SupportsSubmissionLeases())
        return false;
    if (ps_r_sky_probe_debug_freeze != 0 && m_debugGpuValid)
        return false;
    if (m_debugCaptureRequested)
        return true;
    if (m_debugFailures >= kDebugFailureLimit)
        return false;
    return true;
}

void SkyVisibilityGrid::ReportDebugFailure(const char* reason)
{
    const bool changed = !m_debugFailed || m_debugError != reason;
    m_debugFailed = true;
    m_debugFailures = std::min(m_debugFailures + 1u, kDebugFailureLimit);
    m_debugError = reason;
    if (m_debugCaptureRequested || m_debugPendingCapture)
        FailDebugCapture(reason);
    else if (changed)
        Msg("! [SkyProbeInspect] %s", reason);
}

bool SkyVisibilityGrid::HasDebugSnapshot() const
{
    return m_debugGpuValid && m_debugBuffer;
}

nvrhi::IBuffer* SkyVisibilityGrid::GetDebugBuffer() const
{
    return m_debugBuffer;
}

void SkyVisibilityGrid::RecordDebugSnapshot(nvrhi::ICommandList* commandList)
{
    if (!commandList || !m_debugBuffer || !m_debugReadback || ps_r_sky_probe_debug != 3)
        return;
    IRenderBackend* backend = GEnv.Backend;
    m_debugGpuValid = true;
    m_debugRecordFrame = Device.dwFrame;
    if (!backend || !backend->SupportsSubmissionLeases() || m_debugLease)
    {
        m_debugDesync = true;
        return;
    }
    const u64 lease = backend->OpenSubmissionLease();
    if (!lease)
    {
        m_debugDesync = true;
        m_debugFailed = true;
        m_debugError = "submission lease unavailable for the diagnostic snapshot readback";
        if (m_debugCaptureRequested)
            FailDebugCapture("submission lease unavailable");
        return;
    }
    commandList->copyBuffer(m_debugReadback, 0, m_debugBuffer, 0, sizeof(SkyProbeDebugSnapshot));
    m_debugLease = lease;
    m_debugDesync = false;
    m_debugPendingCapture = m_debugCaptureRequested;
    if (m_debugCaptureRequested)
    {
        m_debugCaptureRequested = false;
        ps_r_sky_probe_debug_freeze = 1;
        m_debugExportStatus = "capture recorded; waiting for GPU readback completion";
    }
}

void SkyVisibilityGrid::RequestDebugCapture()
{
    ps_r_sky_probe_debug = 3;
    if (ps_r_sky_probe_debug_freeze != 0)
    {
        m_debugCaptureRequested = false;
        if (!m_levelActive || !m_buffer || !m_debugGpuValid)
        {
            FailDebugCapture("no frozen snapshot is available; unfreeze to select a point");
            return;
        }
        if (m_debugLease)
        {
            m_debugPendingCapture = true;
            m_debugExportStatus = "saving the frozen snapshot; waiting for its GPU readback";
            return;
        }
        if (!m_debugDecoded || m_debugDesync || m_debugFailed || m_debugPacket.metadata[1] != m_debugRecordFrame)
        {
            FailDebugCapture("the frozen snapshot has no matching completed readback; unfreeze to select a new point");
            return;
        }
        ExportDebugCapture();
        return;
    }
    m_debugCaptureRequested = true;
    m_debugRequestFrame = Device.dwFrame;
    m_debugExportStatus = "capture requested; waiting for the next successful snapshot";
}

void SkyVisibilityGrid::ResetDebugSnapshots()
{
    const bool active = m_debugBuffer || m_debugReadback || m_debugLease || m_debugDecoded || m_debugGpuValid ||
        m_debugCaptureRequested || !m_debugError.empty();
    ReleaseDebugReadback();
    m_debugBuffer = nullptr;
    m_debugReadback = nullptr;
    m_debugPacket = SkyProbeDebugSnapshot();
    m_debugError.clear();
    m_debugIssues.clear();
    m_debugWarnings.clear();
    m_debugExportStatus.clear();
    m_debugReport.clear();
    m_debugRecordFrame = 0;
    m_debugRequestFrame = 0;
    m_debugReadbackFrame = 0;
    m_debugReadbackCount = 0;
    m_debugFailures = 0;
    m_debugGpuValid = false;
    m_debugDecoded = false;
    m_debugDesync = false;
    m_debugFailed = false;
    m_debugAllocFailed = false;
    m_debugCaptureRequested = false;
    ps_r_sky_probe_debug_freeze = 0;
    if (active)
        Msg("* [SkyProbeInspect] diagnostic snapshots reset");
}

void SkyVisibilityGrid::ReleaseDebugReadback()
{
    if (m_debugLease && GEnv.Backend)
        GEnv.Backend->ReleaseSubmissionLease(m_debugLease);
    m_debugLease = 0;
    m_debugPendingCapture = false;
}

void SkyVisibilityGrid::ConsumeDebugControls()
{
    if (ps_r_sky_probe_capture != 0)
    {
        ps_r_sky_probe_capture = 0;
        RequestDebugCapture();
    }
    if (!m_debugCaptureRequested)
        return;
    if (ps_r_sky_probe_debug != 3)
        FailDebugCapture("diagnostic mode was disabled before the capture completed");
    else if (!m_levelActive || !m_buffer)
        FailDebugCapture("no sky probe grid is loaded");
    else if (ps_r_sky_probe_debug_freeze != 0)
        RequestDebugCapture();
    else if (!GEnv.Backend || !GEnv.Backend->SupportsSubmissionLeases())
        FailDebugCapture("render backend has no submission leases");
    else if (m_debugAllocFailed)
        FailDebugCapture("diagnostic snapshot buffers could not be allocated");
    else if (Device.dwFrame - m_debugRequestFrame >= kDebugCaptureWaitFrames)
        ReportDebugFailure(m_debugLease ? "the previous diagnostic readback is still pending; new capture could not start" :
            "no diagnostic snapshot was recorded; the inspector pass did not run");
}

void SkyVisibilityGrid::PollDebugReadback()
{
    if (!m_debugLease)
        return;
    IRenderBackend* backend = GEnv.Backend;
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!backend || !nvDevice)
    {
        ReportDebugFailure("render backend or device became unavailable during diagnostic readback");
        ReleaseDebugReadback();
        return;
    }
    const auto state = backend->PollSubmissionLease(m_debugLease);
    if (state == IRenderBackend::SubmissionLeaseState::Open || state == IRenderBackend::SubmissionLeaseState::Pending)
    {
        if (m_debugPendingCapture && Device.dwFrame - m_debugRecordFrame >= kDebugCaptureWaitFrames)
            ReportDebugFailure(state == IRenderBackend::SubmissionLeaseState::Open ?
                "diagnostic submission lease was not sealed; readback is still retained" :
                "diagnostic GPU readback did not complete; readback is still retained");
        return;
    }

    const bool capture = m_debugPendingCapture;
    bool decoded = false;
    if (state == IRenderBackend::SubmissionLeaseState::Complete && m_debugReadback)
    {
        if (const void* data = nvDevice->mapBuffer(m_debugReadback, nvrhi::CpuAccessMode::Read))
        {
            std::memcpy(&m_debugPacket, data, sizeof(SkyProbeDebugSnapshot));
            nvDevice->unmapBuffer(m_debugReadback);
            decoded = true;
        }
    }
    ReleaseDebugReadback();

    if (!decoded)
    {
        ++m_debugFailures;
        m_debugFailed = true;
        m_debugError = state == IRenderBackend::SubmissionLeaseState::Complete
            ? "diagnostic snapshot readback buffer could not be mapped"
            : "diagnostic snapshot submission did not complete";
        if (capture)
        {
            FailDebugCapture(m_debugError.c_str());
        }
        return;
    }

    m_debugDecoded = true;
    m_debugFailed = false;
    m_debugFailures = 0;
    m_debugError.clear();
    m_debugReadbackFrame = Device.dwFrame;
    ++m_debugReadbackCount;
    SkyProbeDebugReport::Validate(m_debugPacket, m_layout, m_debugIssues, m_debugWarnings);
    if (capture)
        ExportDebugCapture();
}

void SkyVisibilityGrid::FailDebugCapture(const char* reason)
{
    m_debugCaptureRequested = false;
    m_debugPendingCapture = false;
    m_debugExportStatus = "capture FAILED: ";
    m_debugExportStatus += reason;
    Msg("! [SkyProbeInspect] %s", m_debugExportStatus.c_str());
}

void SkyVisibilityGrid::ExportDebugCapture()
{
    ++m_debugCaptureCount;
    m_debugReport = SkyProbeDebugReport::Build(m_debugPacket, BuildDebugContext(true));

    Msg("* [SkyProbeInspect] BEGIN capture %u", m_debugCaptureCount);
    size_t begin = 0;
    while (begin < m_debugReport.size())
    {
        size_t end = m_debugReport.find('\n', begin);
        if (end == xr_string::npos)
            end = m_debugReport.size();
        const xr_string line = m_debugReport.substr(begin, end - begin);
        Msg("[SkyProbeInspect] %s", line.c_str());
        begin = end + 1;
    }
    Msg("* [SkyProbeInspect] END capture %u", m_debugCaptureCount);

    string_path path = {};
    FS.update_path(path, "$app_data_root$", "sky_probe_capture.txt");
    IWriter* writer = FS.w_open(path);
    if (writer)
    {
        writer->w(m_debugReport.data(), m_debugReport.size());
        FS.w_close(writer);
        m_debugExportStatus = "capture ";
        char index[16];
        xr_sprintf(index, "%u", m_debugCaptureCount);
        m_debugExportStatus += index;
        m_debugExportStatus += m_debugIssues.empty() ? " saved: " : " saved (packet INVALID): ";
        m_debugExportStatus += path;
        Msg("* [SkyProbeInspect] capture saved: %s", path);
    }
    else
    {
        m_debugExportStatus = "capture logged but the file could not be written: ";
        m_debugExportStatus += path;
        Msg("! [SkyProbeInspect] failed to write capture file: %s", path);
    }
}

SkyProbeDebugStatus SkyVisibilityGrid::GetDebugStatus() const
{
    if (!m_levelActive || !m_buffer)
        return SkyProbeDebugStatus::Unavailable;
    if (!m_debugDecoded)
    {
        if (!m_debugError.empty())
            return SkyProbeDebugStatus::Error;
        return m_debugLease ? SkyProbeDebugStatus::Pending : SkyProbeDebugStatus::NoData;
    }
    if (!m_debugIssues.empty())
        return SkyProbeDebugStatus::Invalid;
    if (m_debugFailed && !m_debugLease)
        return SkyProbeDebugStatus::Error;
    if (m_debugLease)
        return SkyProbeDebugStatus::Pending;
    return ps_r_sky_probe_debug_freeze != 0 ? SkyProbeDebugStatus::Frozen : SkyProbeDebugStatus::Live;
}

SkyProbeDebugReportContext SkyVisibilityGrid::BuildDebugContext(bool explicitCapture) const
{
    SkyProbeDebugReportContext context;
    context.layout = m_layout;
    context.gridState = m_state;
    context.bakeNext = m_bakeNext;
    context.currentFrame = Device.dwFrame;
    context.readbackFrame = m_debugReadbackFrame;
    context.captureIndex = explicitCapture ? m_debugCaptureCount : 0;
    context.readbackCount = m_debugReadbackCount;
    context.cacheVersion = kCacheVersion;
    context.probeBytes = kProbeBytes;
    context.frozen = ps_r_sky_probe_debug_freeze != 0;
    context.explicitCapture = explicitCapture;
    context.statusText = SkyProbeDebugReport::StatusName(GetDebugStatus());
    context.cachePath = m_cachePath;
    context.geometryStamp = m_geometryStamp;
    context.backendName = GEnv.Backend && GEnv.Backend->GetAPIName() ? GEnv.Backend->GetAPIName() : "unknown";
    context.issues = m_debugIssues;
    context.warnings = m_debugWarnings;
    return context;
}

namespace
{
constexpr u32 kDebugNone = 0xFFFFFFFFu;
constexpr float kFloorLimited = SkyProbeDebugSnapshot::kVisibilityFloor;

void AppendF(xr_string& out, const char* format, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    out += buffer;
}

bool Finite4(const Fvector4& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
}

float Length3(const Fvector4& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void FormatId(char (&buffer)[16], u32 id)
{
    if (id == kDebugNone)
        xr_strcpy(buffer, "none");
    else
        xr_sprintf(buffer, "%u", id);
}

bool ReconstructHit(const SkyProbeDebugSnapshot& packet, u32 corner, u32 ray, Fvector& out, float& remaining)
{
    const SkyProbeDebugRay& r = packet.corners[corner].rays[ray];
    if (r.hit[0] != 2 || !(r.metrics.y >= 0.0f) || !(r.metrics.x > 1e-8f))
        return false;
    const Fvector4& origin = (ray & 1u) ? packet.controlOrigin : packet.rayOrigin;
    const Fvector4& probe = packet.corners[corner].position;
    const float t = r.metrics.y / r.metrics.x;
    out.set(origin.x + (probe.x - origin.x) * t, origin.y + (probe.y - origin.y) * t,
        origin.z + (probe.z - origin.z) * t);
    remaining = r.metrics.x - r.metrics.y;
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

const char* RayRelation(const SkyProbeDebugRay& a, const SkyProbeDebugRay& b)
{
    if (a.hit[0] == 0 || b.hit[0] == 0)
        return "n/a (not traced)";
    if (a.hit[0] != b.hit[0])
        return "DIFFERS (hit vs miss status)";
    if (a.hit[0] == 2 && (a.hit[1] != b.hit[1] || a.hit[2] != b.hit[2] || a.hit[3] != b.hit[3]))
        return "DIFFERS (different instance/geometry/primitive)";
    return "same";
}

void AppendTexels(xr_string& out, const char* label, const u32 (&words)[4])
{
    AppendF(out, "%s:", label);
    for (u32 texel = 0; texel < 16; ++texel)
        AppendF(out, " %u", (words[texel >> 2] >> ((texel & 3u) * 8u)) & 0xFFu);
    AppendF(out, "\n");
}

const char* ProductionSourceName(u32 source)
{
    return source == SkyProbeDebugSnapshot::kProductionMoments ? "continuous depth moments (Chebyshev)" : "unknown";
}
}

const char* SkyProbeDebugReport::SurfaceStatusName(u32 status)
{
    switch (status)
    {
    case 0: return "no grid";
    case 1: return "background or invalid depth";
    case 2: return "HUD depth (labelled, still sampled)";
    case 3: return "invalid surface data (non-finite P/N/depth)";
    case 4: return "valid opaque world";
    default: return "unknown";
    }
}

const char* SkyProbeDebugReport::CornerStateName(u32 state)
{
    switch (state)
    {
    case 0: return "unset";
    case 1: return "zero trilinear weight";
    case 2: return "unbaked";
    case 3: return "invalid probe";
    case 4: return "valid";
    default: return "unknown";
    }
}

const char* SkyProbeDebugReport::RayStatusName(u32 status)
{
    switch (status)
    {
    case 0: return "unavailable/not traced";
    case 1: return "miss";
    case 2: return "hit";
    case 3: return "degenerate";
    default: return "unknown";
    }
}

char SkyProbeDebugReport::RayStatusLetter(u32 status)
{
    switch (status)
    {
    case 0: return '-';
    case 1: return 'M';
    case 2: return 'H';
    case 3: return 'D';
    default: return '?';
    }
}

const char* SkyProbeDebugReport::RayLabel(u32 ray)
{
    switch (ray)
    {
    case 0: return "ray origin     | STATIC 0x04 | CULL_NON_OPAQUE";
    case 1: return "control origin | STATIC 0x04 | CULL_NON_OPAQUE";
    case 2: return "ray origin     | STATIC 0x04 | FORCE_OPAQUE";
    case 3: return "control origin | STATIC 0x04 | FORCE_OPAQUE";
    case 4: return "ray origin     | WORLD 0x01  | FORCE_OPAQUE";
    case 5: return "control origin | WORLD 0x01  | FORCE_OPAQUE";
    default: return "unknown";
    }
}

const char* SkyProbeDebugReport::SceneStatusName(u32 status)
{
    switch (status)
    {
    case 0: return "not recorded for current frame";
    case 1: return "recorded, all tile classes dispatched";
    case 2: return "partial/incomplete tile dispatch";
    default: return "unknown";
    }
}

const char* SkyProbeDebugReport::GridStateName(SkyVisibilityState state)
{
    switch (state)
    {
    case SkyVisibilityState::Empty: return "Empty";
    case SkyVisibilityState::Uploading: return "Uploading";
    case SkyVisibilityState::Baking: return "Baking";
    case SkyVisibilityState::Saving: return "Saving";
    case SkyVisibilityState::Ready: return "Ready";
    default: return "Unknown";
    }
}

const char* SkyProbeDebugReport::StatusName(SkyProbeDebugStatus status)
{
    switch (status)
    {
    case SkyProbeDebugStatus::Unavailable: return "UNAVAILABLE (no sky probe grid)";
    case SkyProbeDebugStatus::NoData: return "NO DATA (no snapshot read back yet)";
    case SkyProbeDebugStatus::Live: return "LIVE";
    case SkyProbeDebugStatus::Frozen: return "FROZEN";
    case SkyProbeDebugStatus::Pending: return "PENDING (readback in flight)";
    case SkyProbeDebugStatus::Invalid: return "INVALID PACKET";
    case SkyProbeDebugStatus::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}

void SkyProbeDebugReport::Validate(const SkyProbeDebugSnapshot& packet, const SkyVisibilityLayout& layout,
    xr_string& issues, xr_string& warnings)
{
    issues.clear();
    warnings.clear();
    if (packet.metadata[0] != SkyProbeDebugSnapshot::kVersion)
        AppendF(issues, "packet version %u does not match expected %u\n", packet.metadata[0],
            SkyProbeDebugSnapshot::kVersion);
    if (packet.metadata[2] != SkyProbeDebugSnapshot::kProductionMoments)
        AppendF(issues, "production visibility source %u is not depth moments (%u)\n", packet.metadata[2],
            SkyProbeDebugSnapshot::kProductionMoments);
    if (packet.metadata[3] > 4)
        AppendF(issues, "surface status %u is out of range\n", packet.metadata[3]);
    if (packet.scene[3] == 0)
        AppendF(warnings, "deferred lighting was not recorded for the current frame: the production comparison may come from a different frame\n");
    else if (packet.scene[3] == 2)
        AppendF(warnings, "deferred lighting dispatch was partial/incomplete: production comparison is not exact\n");
    else if (packet.scene[3] != 1)
        AppendF(issues, "deferred lighting status %u is out of range\n", packet.scene[3]);
    if (packet.scene[2] == 0)
        AppendF(warnings, "diagnostic ray-query pipeline/TLAS is unavailable: reference rays R0-R5 were not traced; production moment visibility is unaffected\n");
    if (packet.metadata[3] == 2)
        AppendF(warnings, "surface is HUD depth: labelled only, still sampled like deferred lighting\n");
    if (packet.selection[2] == 0 || packet.selection[3] == 0)
        AppendF(issues, "selection extent is zero (%u x %u)\n", packet.selection[2], packet.selection[3]);
    else if (packet.selection[0] >= packet.selection[2] || packet.selection[1] >= packet.selection[3])
        AppendF(issues, "selected pixel (%u,%u) is outside %u x %u\n", packet.selection[0], packet.selection[1],
            packet.selection[2], packet.selection[3]);
    if (u64(packet.gridDims[0]) * packet.gridDims[1] * packet.gridDims[2] != u64(packet.gridDims[3]))
        AppendF(issues, "grid dims %u x %u x %u do not multiply to count %u\n", packet.gridDims[0],
            packet.gridDims[1], packet.gridDims[2], packet.gridDims[3]);
    if (layout.count &&
        (packet.gridDims[0] != layout.dims[0] || packet.gridDims[1] != layout.dims[1] ||
            packet.gridDims[2] != layout.dims[2] || packet.gridDims[3] != layout.count))
        AppendF(warnings, "packet grid dims differ from the current grid layout (grid changed since the snapshot)\n");

    if (packet.metadata[3] < 2 || packet.metadata[3] > 4)
        return;
    if (packet.metadata[3] == 3)
        return;

    const char* vectorNames[] = {"surface", "shadingNormal", "geometricNormal", "view", "biased", "rayOrigin",
        "controlOrigin", "sampleSH", "summary", "sky", "reflection"};
    const Fvector4* vectorValues[] = {&packet.surface, &packet.shadingNormal, &packet.geometricNormal, &packet.view,
        &packet.biased, &packet.rayOrigin, &packet.controlOrigin, &packet.sampleSH, &packet.summary, &packet.sky,
        &packet.reflection};
    for (u32 i = 0; i < sizeof(vectorNames) / sizeof(vectorNames[0]); ++i)
        if (!Finite4(*vectorValues[i]))
            AppendF(issues, "%s contains non-finite values\n", vectorNames[i]);
    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = packet.corners[i];
        if (!Finite4(k.position) || !Finite4(k.weights) || !Finite4(k.contribution) || !Finite4(k.sky) ||
            !Finite4(k.moment) || !Finite4(k.offset))
            AppendF(issues, "corner %u contains non-finite values\n", i);
        if ((k.identity[1] == 3 || k.identity[1] == 4) && !Finite4(k.coefficients))
            AppendF(issues, "corner %u coefficients are non-finite for a baked probe state\n", i);
        if (k.identity[1] == 0)
            AppendF(warnings, "corner %u state is unset\n", i);
        for (u32 r = 0; r < 6; ++r)
            if (!Finite4(k.rays[r].metrics))
                AppendF(issues, "corner %u ray %u metrics are non-finite\n", i, r);
    }
    if (!issues.empty())
        return;

    if (std::fabs(Length3(packet.shadingNormal) - 1.0f) > 0.05f)
        AppendF(warnings, "shading normal length is %.4f (not normalized)\n", Length3(packet.shadingNormal));
    if (std::fabs(Length3(packet.geometricNormal) - 1.0f) > 0.05f)
        AppendF(warnings, "geometric normal length is %.4f (not normalized)\n", Length3(packet.geometricNormal));

    float rawSum = 0.0f;
    float normSum = 0.0f;
    float responseSum = 0.0f;
    for (u32 i = 0; i < 8; ++i)
    {
        rawSum += packet.corners[i].weights.w;
        normSum += packet.corners[i].contribution.x;
        responseSum += packet.corners[i].contribution.y;
    }
    const float saturatedResponse = std::clamp(responseSum, 0.0f, 1.0f);
    if (std::fabs(saturatedResponse - packet.sky.x) > 1e-3f)
        AppendF(warnings, "saturated sum of corner weighted responses %.6g differs from final sky toward Ns %.6g\n",
            saturatedResponse, packet.sky.x);
    if (std::fabs(rawSum - packet.summary.x) > 1e-3f * std::max(1.0f, std::fabs(packet.summary.x)))
        AppendF(warnings, "sum of corner raw weights %.6g differs from summary raw weight %.6g\n", rawSum,
            packet.summary.x);
    if (rawSum > 1e-12f && std::fabs(normSum - 1.0f) > 1e-3f)
        AppendF(warnings, "sum of normalized corner weights is %.6g, expected 1\n", normSum);
}

xr_string SkyProbeDebugReport::Build(const SkyProbeDebugSnapshot& p, const SkyProbeDebugReportContext& c)
{
    xr_string out;
    out.reserve(1u << 16);
    const bool surface = p.metadata[3] == 2 || p.metadata[3] == 4;
    const u64 sceneRevision = u64(p.scene[0]) | (u64(p.scene[1]) << 32);
    const u32 age = c.readbackFrame - p.metadata[1];

    AppendF(out, "SKY PROBE INSPECTOR REPORT\n");
    AppendF(out, "packet version %u (expected %u) | snapshot bytes %u (expected %u) | corner bytes %u | probe record bytes %u | grid cache version %u\n",
        p.metadata[0], SkyProbeDebugSnapshot::kVersion, u32(sizeof(SkyProbeDebugSnapshot)),
        SkyProbeDebugSnapshot::kBufferStride * SkyProbeDebugSnapshot::kBufferRows, u32(sizeof(SkyProbeDebugCorner)),
        c.probeBytes, c.cacheVersion);
    AppendF(out, "GPU snapshot storage: %u uint4 records at %u bytes each\n",
        SkyProbeDebugSnapshot::kBufferRows, SkyProbeDebugSnapshot::kBufferStride);
    AppendF(out, "backend %s | cache file %s | geometry stamp 0x%016llX\n", c.backendName.c_str(), c.cachePath.c_str(),
        static_cast<unsigned long long>(c.geometryStamp));
    AppendF(out, "capture index %u | %s\n", c.captureIndex,
        c.explicitCapture ? "explicit capture of the frozen GPU snapshot that mode 3 is drawing" : "copy of current snapshot");
    AppendF(out, "status: %s | freeze %s\n", c.statusText.c_str(), c.frozen ? "ON" : "off");
    AppendF(out, "GPU packet recorded at Device frame %u | readback decoded at frame %u | report built at frame %u | age at readback %u frame(s) | readbacks this level %u\n",
        p.metadata[1], c.readbackFrame, c.currentFrame, age, c.readbackCount);
    if (c.issues.empty())
        AppendF(out, "packet validity: VALID\n");
    else
        AppendF(out, "packet validity: INVALID\n%s", c.issues.c_str());
    if (!c.warnings.empty())
        AppendF(out, "warnings:\n%s", c.warnings.c_str());

    AppendF(out, "\n-- Grid --\n");
    AppendF(out, "grid state %s (bake progress %u / %u probes)\n", GridStateName(c.gridState), c.bakeNext,
        c.layout.count);
    AppendF(out, "current layout: dims %u x %u x %u = %u probes | origin (%.9g, %.9g, %.9g) | spacing %.9g | bake rays %u | rayDistance %.9g | backfaceLimit %.9g\n",
        c.layout.dims[0], c.layout.dims[1], c.layout.dims[2], c.layout.count, c.layout.origin.x, c.layout.origin.y,
        c.layout.origin.z, c.layout.spacing, c.layout.rays, c.layout.rayDistance, c.layout.backfaceLimit);
    AppendF(out, "packet grid: dims %u x %u x %u = %u | origin (%.9g, %.9g, %.9g) | spacing %.9g\n", p.gridDims[0],
        p.gridDims[1], p.gridDims[2], p.gridDims[3], p.gridOrigin.x, p.gridOrigin.y, p.gridOrigin.z, p.gridOrigin.w);
    AppendF(out, "spacing: requested %.9g | effective (packet) %.9g | effective (current layout) %.9g\n", p.settings.x,
        p.gridOrigin.w, c.layout.spacing);
    AppendF(out, "rays: configured cvar %.0f | captured baked rays (camera.w) %.0f | current layout rays %u\n",
        p.settings.y, p.camera.w, c.layout.rays);
    AppendF(out, "settings: requested spacing %.9g | configured sky rays %.0f | configured probe max %.0f\n",
        p.settings.x, p.settings.y, p.settings.z);
    AppendF(out, "lighting: sky trace distance %.9g | backface limit %.9g | sky IBL flag %.0f | effective lighting mode %.0f\n",
        p.lighting.x, p.lighting.y, p.lighting.z, p.lighting.w);
    AppendF(out, "production visibility: %s | requires runtime ray tracing: no\n", ProductionSourceName(p.metadata[2]));
    AppendF(out, "scene: revision %llu | diagnostic ray-query TLAS available %u | deferred lighting status %u (%s)\n",
        static_cast<unsigned long long>(sceneRevision), p.scene[2], p.scene[3], SceneStatusName(p.scene[3]));
    AppendF(out, "moments: 48-byte probe record = visibility uint4 + depthMean uint4 + depthSigma uint4; raw words and decoded interpolation inputs are listed per corner\n");

    AppendF(out, "\n-- Surface --\n");
    AppendF(out, "surface status %u (%s)\n", p.metadata[3], SurfaceStatusName(p.metadata[3]));
    AppendF(out, "selected pixel (%u, %u) of %u x %u\n", p.selection[0], p.selection[1], p.selection[2], p.selection[3]);
    AppendF(out, "camera eye (%.9g, %.9g, %.9g)\n", p.camera.x, p.camera.y, p.camera.z);
    AppendF(out, "P (%.9g, %.9g, %.9g) | depth %.9g\n", p.surface.x, p.surface.y, p.surface.z, p.surface.w);
    AppendF(out, "Ns (%.9g, %.9g, %.9g) | roughness %.9g\n", p.shadingNormal.x, p.shadingNormal.y, p.shadingNormal.z,
        p.shadingNormal.w);
    AppendF(out, "Ng (%.9g, %.9g, %.9g) | dot(Ng,V) %.9g\n", p.geometricNormal.x, p.geometricNormal.y,
        p.geometricNormal.z, p.geometricNormal.w);
    AppendF(out, "V (%.9g, %.9g, %.9g) | distance from camera %.9g\n", p.view.x, p.view.y, p.view.z, p.view.w);
    AppendF(out, "gbuffer material (%.9g, %.9g, %.9g, %.9g)\n", p.material.x, p.material.y, p.material.z, p.material.w);
    AppendF(out, "grid selection position (biased) (%.9g, %.9g, %.9g) | |biased-P| %.9g\n", p.biased.x, p.biased.y,
        p.biased.z, p.biased.w);
    AppendF(out, "diagnostic ray origin (%.9g, %.9g, %.9g) | |origin-P| %.9g (moments are evaluated at B, not here)\n", p.rayOrigin.x,
        p.rayOrigin.y, p.rayOrigin.z, p.rayOrigin.w);
    AppendF(out, "control ray origin P+Ng*0.001 (%.9g, %.9g, %.9g) | offset %.9g\n", p.controlOrigin.x,
        p.controlOrigin.y, p.controlOrigin.z, p.controlOrigin.w);
    AppendF(out, "reflection R (%.9g, %.9g, %.9g) | grid-clamped flag %.0f\n", p.reflection.x, p.reflection.y,
        p.reflection.z, p.reflection.w);

    AppendF(out, "\n-- Production result --\n");
    AppendF(out, "sample SH after production confidence: c0 %.9g | c1 (%.9g, %.9g, %.9g)\n", p.sampleSH.x, p.sampleSH.y,
        p.sampleSH.z, p.sampleSH.w);
    AppendF(out, "raw weight sum %.9g | best moment visibility %.9g | confidence %.9g | floor-limited weight share %.9g\n",
        p.summary.x, p.summary.y, p.summary.z, p.summary.w);
    AppendF(out, "final sky: toward Ns %.9g | toward Ng %.9g | toward -Ns %.9g | toward R %.9g\n", p.sky.x, p.sky.y,
        p.sky.z, p.sky.w);

    float rawSum = 0.0f;
    float normSum = 0.0f;
    float blockedNormSum = 0.0f;
    float retainedRaw = 0.0f;
    float weightedResponse = 0.0f;
    u32 retained = 0;
    u32 validCorners = 0;
    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = p.corners[i];
        rawSum += k.weights.w;
        normSum += k.contribution.x;
        blockedNormSum += k.contribution.z;
        weightedResponse += k.contribution.y;
        if (k.identity[1] == 4)
            ++validCorners;
        if (k.identity[1] == 4 && k.weights.z < kFloorLimited)
        {
            ++retained;
            retainedRaw += k.weights.w;
        }
    }

    AppendF(out, "\n-- Moment visibility weights (raw vs normalized) --\n");
    AppendF(out, "%-3s %-22s %-12s %-12s %-12s %-12s %-12s %-12s %-12s\n", "#", "state", "trilinear", "facing+0.05",
        "momentVis", "rawWeight", "normWeight", "floorNorm", "preCubic");
    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = p.corners[i];
        AppendF(out, "%-3u %-22s %-12.6g %-12.6g %-12.6g %-12.6g %-12.6g %-12.6g %-12.6g%s\n", i,
            CornerStateName(k.identity[1]), k.weights.x, k.weights.y, k.weights.z, k.weights.w, k.contribution.x,
            k.contribution.z, k.contribution.w,
            (k.identity[1] == 4 && k.weights.z < kFloorLimited) ? "  <-- moment visibility below floor, weight clamped" : "");
    }
    AppendF(out, "sum rawWeight %.9g (summary %.9g) | sum normWeight %.9g | sum floorNorm %.9g (summary floor-limited share %.9g)\n",
        rawSum, p.summary.x, normSum, blockedNormSum, p.summary.w);
    AppendF(out, "valid corners %u of 8 | corners below the visibility floor %u | their raw weight %.9g (%.4f of raw sum)\n",
        validCorners, retained, retainedRaw, rawSum > 0.0f ? retainedRaw / rawSum : 0.0f);
    AppendF(out, "sum of weighted SH responses toward Ns (normalized weight * unclamped sky * confidence) %.9g\n", weightedResponse);
    if (retained != 0)
        AppendF(out, "NOTE: %u corner(s) have moment visibility below %.2f; production clamps their visibility factor to the floor rather than discarding them.\n",
            retained, kFloorLimited);

    AppendF(out, "\n-- Corners --\n");
    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = p.corners[i];
        AppendF(out, "CORNER %u: index %u | state %u (%s) | cell (%u, %u, %u) | rawFlags 0x%08X | failureReason %u\n", i,
            k.identity[0], k.identity[1], CornerStateName(k.identity[1]), k.identity[2], k.identity[3],
            k.cellAndFlags[0], k.cellAndFlags[1], k.cellAndFlags[2]);
        AppendF(out, "  raw visibility uint4: %08X %08X %08X %08X\n", k.rawVisibility[0], k.rawVisibility[1],
            k.rawVisibility[2], k.rawVisibility[3]);
        AppendF(out, "  raw depthMean uint4:  %08X %08X %08X %08X\n", k.rawDepthMean[0], k.rawDepthMean[1],
            k.rawDepthMean[2], k.rawDepthMean[3]);
        AppendTexels(out, "  depthMean texels (0-255 of 3*spacing)", k.rawDepthMean);
        AppendF(out, "  raw depthSigma uint4: %08X %08X %08X %08X\n", k.rawDepthSigma[0], k.rawDepthSigma[1],
            k.rawDepthSigma[2], k.rawDepthSigma[3]);
        AppendTexels(out, "  depthSigma texels (0-255 of 2*sigma)", k.rawDepthSigma);
        AppendF(out, "  moment: |sample B - probe| %.9g | interpolated mean depth %.9g | variance+tolerance^2 %.9g | chebyshev^3 visibility %.9g\n",
            k.moment.x, k.moment.y, k.moment.z, k.moment.w);
        AppendF(out, "  relocated position (%.9g, %.9g, %.9g) | distance(P,probe) %.9g\n", k.position.x, k.position.y,
            k.position.z, k.position.w);
        AppendF(out, "  relocation offset (%.9g, %.9g, %.9g) | length %.9g\n", k.offset.x, k.offset.y, k.offset.z,
            k.offset.w);
        AppendF(out, "  SH c0 %.9g | c1 (%.9g, %.9g, %.9g)\n", k.coefficients.x, k.coefficients.y, k.coefficients.z,
            k.coefficients.w);
        AppendF(out, "  weights: trilinear %.9g | facing(+0.05) %.9g | moment visibility %.9g | final raw (floor+cubic) %.9g\n",
            k.weights.x, k.weights.y, k.weights.z, k.weights.w);
        AppendF(out, "  contribution: normalized weight %.9g | weighted SH response toward Ns (normalized weight * unclamped sky * confidence) %.9g | normalized floor-limited weight %.9g | weight before cubic %.9g\n",
            k.contribution.x, k.contribution.y, k.contribution.z, k.contribution.w);
        AppendF(out, "  sky response: toward Ns unclamped %.9g clamped %.9g | toward Ng unclamped %.9g | toward R clamped %.9g\n",
            k.sky.x, k.sky.y, k.sky.z, k.sky.w);
        for (u32 r = 0; r < 6; ++r)
        {
            const SkyProbeDebugRay& ray = k.rays[r];
            char inst[16];
            char geom[16];
            char prim[16];
            FormatId(inst, ray.hit[1]);
            FormatId(geom, ray.hit[2]);
            FormatId(prim, ray.hit[3]);
            AppendF(out, "  R%u [%s] status %u (%s) | segmentLength %.9g | hitT %.9g (first accepted hit, not nearest) | binary segment visibility %.9g (diagnostic only) | frontFace %.0f | instance %s geometry %s primitive %s",
                r, RayLabel(r), ray.hit[0], RayStatusName(ray.hit[0]), ray.metrics.x, ray.metrics.y, ray.metrics.z,
                ray.metrics.w, inst, geom, prim);
            Fvector hitPosition;
            float remaining = 0.0f;
            if (ReconstructHit(p, i, r, hitPosition, remaining))
                AppendF(out, " | derived hit position (%.9g, %.9g, %.9g) | remaining distance to probe %.9g (%.4f of segment)",
                    hitPosition.x, hitPosition.y, hitPosition.z, remaining,
                    ray.metrics.x > 0.0f ? remaining / ray.metrics.x : 0.0f);
            else if (ray.hit[0] == 2)
                AppendF(out, " | derived hit position unavailable (degenerate segment or hitT)");
            AppendF(out, "\n");
        }
        AppendF(out, "  ray relations: R0 vs R1 (origin) %s | R0 vs R2 (non-opaque culling) %s | R2 vs R4 (instance mask) %s | R1 vs R3 %s | R3 vs R5 %s\n",
            RayRelation(k.rays[0], k.rays[1]), RayRelation(k.rays[0], k.rays[2]), RayRelation(k.rays[2], k.rays[4]),
            RayRelation(k.rays[1], k.rays[3]), RayRelation(k.rays[3], k.rays[5]));
        if (k.identity[1] == 4 && k.weights.z < kFloorLimited)
            AppendF(out, "  moment visibility %.9g is below the %.2f floor: the weight is clamped to the floor, so this corner is down-weighted but not removed\n",
                k.weights.z, kFloorLimited);
    }

    if (!surface)
        AppendF(out, "\nNOTE: surface status %u (%s): corner and ray data are not meaningful for this pixel.\n",
            p.metadata[3], SurfaceStatusName(p.metadata[3]));

    AppendF(out, "\n-- Legend --\n");
    AppendF(out, "PRODUCTION visibility is the continuous depth-moment (Chebyshev) weight read from the 48-byte probe record (raw visibility, depthMean and depthSigma uint4 shown above). It is evaluated at the biased position B and needs no runtime ray tracing.\n");
    AppendF(out, "R0-R5 are binary geometric REFERENCE segments traced only by this inspector (ray-query diagnostic variant). They are NOT production occlusion and never change the weights, SH or sky response.\n");
    AppendF(out, "Rays travel from an origin to the probe; a hit means that diagnostic segment is blocked, segment visibility 1 means clear.\n");
    AppendF(out, "R0 ray origin STATIC mask 0x04 CULL_NON_OPAQUE; R1 control origin (P+Ng*0.001) same flags; R2/R3 same with FORCE_OPAQUE; R4/R5 WORLD mask 0x01 FORCE_OPAQUE.\n");
    AppendF(out, "Interpretation: R0 vs R1 changes only the origin; R0 vs R2 tests excluded non-opaque geometry; R2 vs R4 tests the instance mask. Forced-opaque hits are not proof that alpha-tested pixels are opaque.\n");
    AppendF(out, "ACCEPT_FIRST_HIT does not promise the nearest hit: hitT, instance, geometry and primitive identify some blocker, not necessarily the closest one.\n");
    AppendF(out, "Forced-opaque controls deliberately ignore alpha textures.\n");
    AppendF(out, "Derived hit position = origin + (probePos - origin) * (hitT / segmentLength), with origin = ray origin for R0/R2/R4 and control origin for R1/R3/R5; remaining distance = segmentLength - hitT. Compare the derived world hit position against the actual wall and receiver geometry.\n");
    AppendF(out, "Mode 3 viewport: striped or dimmed probe markers are occluded from the camera only. Dashed R->corner lines are x-ray guides to the diagnostic segment endpoint. The scene background is drawn normally; there is no full-surface heatmap.\n");
    AppendF(out, "Hit status: 0 unavailable/not traced, 1 miss, 2 hit, 3 degenerate. IDs 0xFFFFFFFF mean none.\n");
    AppendF(out, "Corner state: 0 unset, 1 zero trilinear weight, 2 unbaked, 3 invalid, 4 valid.\n");
    AppendF(out, "Weights: trilinear * facing(+0.05) * max(moment visibility, %.2f), then cubic shaping below 0.2 gives the final raw weight; normalized weight = raw / sum(raw). Moment visibility below %.2f is clamped to the floor, not removed.\n",
        kFloorLimited, kFloorLimited);
    AppendF(out, "Moment columns: |sample-probe| is the distance from B to the relocated probe; mean depth and variance are the bilinearly interpolated octahedral depth moments in world units (variance includes the 0.03*spacing tolerance); chebyshev^3 is 1 inside the mean depth.\n");
    return out;
}

namespace
{
const ImVec4 kColorGood(0.35f, 0.9f, 0.4f, 1.0f);
const ImVec4 kColorWarn(1.0f, 0.75f, 0.25f, 1.0f);
const ImVec4 kColorBad(1.0f, 0.35f, 0.3f, 1.0f);
const ImVec4 kColorInfo(0.6f, 0.8f, 1.0f, 1.0f);

ImVec4 StatusColor(SkyProbeDebugStatus status)
{
    switch (status)
    {
    case SkyProbeDebugStatus::Live: return kColorGood;
    case SkyProbeDebugStatus::Frozen: return kColorInfo;
    case SkyProbeDebugStatus::Pending:
    case SkyProbeDebugStatus::NoData: return kColorWarn;
    default: return kColorBad;
    }
}

void TextLines(const xr_string& text, const ImVec4& color)
{
    size_t begin = 0;
    while (begin < text.size())
    {
        size_t end = text.find('\n', begin);
        if (end == xr_string::npos)
            end = text.size();
        if (end > begin)
            ImGui::TextColored(color, "%.*s", int(end - begin), text.c_str() + begin);
        begin = end + 1;
    }
}

void RayLetters(const SkyProbeDebugCorner& corner)
{
    for (u32 r = 0; r < 6; ++r)
    {
        const u32 status = corner.rays[r].hit[0];
        const ImVec4 color = status == 2 ? kColorBad : status == 1 ? kColorGood : status == 3 ? kColorWarn
                                                                                              : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        if (r != 0)
            ImGui::SameLine(0.0f, 4.0f);
        ImGui::TextColored(color, "%c", SkyProbeDebugReport::RayStatusLetter(status));
    }
}
}

nvrhi::IBuffer* SkyVisibilityGrid::GetDebugReadbackBuffer() const
{
    return m_debugReadback;
}

void SkyVisibilityGrid::DrawDebugInspector()
{
    if (ps_r_sky_probe_debug != 3)
        return;
    auto* context = Device.GetImGuiContext();
    if (!context)
        return;
    ImGui::SetCurrentContext(context);
    if (m_debugDecoded && m_levelActive && m_buffer)
        DrawDebugLabels(m_debugPacket);
    ImGui::SetNextWindowPos(ImVec2(420.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(820.0f, 760.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sky Probe Inspector"))
    {
        ImGui::End();
        return;
    }

    const SkyProbeDebugStatus status = GetDebugStatus();
    ImGui::TextColored(StatusColor(status), "%s", SkyProbeDebugReport::StatusName(status));
    ImGui::SameLine();
    ImGui::Text("| grid %s | freeze %s", SkyProbeDebugReport::GridStateName(m_state),
        ps_r_sky_probe_debug_freeze != 0 ? "ON" : "off");

    const bool frozen = ps_r_sky_probe_debug_freeze != 0;
    if (ImGui::Button(frozen ? "Unfreeze" : "Freeze"))
        ps_r_sky_probe_debug_freeze = frozen ? 0 : 1;
    ImGui::SameLine();
    if (ImGui::Button("Snapshot"))
        RequestDebugCapture();
    ImGui::SameLine();
    if (ImGui::Button("Copy report") && m_debugDecoded)
    {
        const xr_string report = SkyProbeDebugReport::Build(m_debugPacket, BuildDebugContext(false));
        ImGui::SetClipboardText(report.c_str());
    }
    ImGui::SameLine();
    ImGui::Checkbox("Raw records", &m_debugShowRaw);
    ImGui::TextDisabled("console: r_sky_probe_capture 1 | r_sky_probe_debug_freeze 0/1 | file: sky_probe_capture.txt");
    ImGui::TextDisabled("Freeze a point, then take snapshots. Unfreeze to select another point.");
    if (!m_debugExportStatus.empty())
        ImGui::TextWrapped("%s", m_debugExportStatus.c_str());
    if (m_debugCaptureRequested)
        ImGui::TextColored(kColorWarn, "capture waiting for the next successful snapshot");
    if (!m_debugError.empty())
        ImGui::TextColored(kColorBad, "error: %s", m_debugError.c_str());

    if (!m_levelActive || !m_buffer)
    {
        ImGui::TextColored(kColorBad, "no sky probe grid is loaded");
        ImGui::End();
        return;
    }
    if (!m_debugDecoded)
    {
        ImGui::TextColored(kColorWarn, m_debugLease ? "first snapshot readback is in flight"
                                                    : "no snapshot has been read back yet (diagnostic dispatch not recorded)");
        if (m_debugGpuValid)
            ImGui::TextColored(kColorWarn, "GPU packet exists and is drawn, but its values are not yet decoded");
        ImGui::End();
        return;
    }

    const SkyProbeDebugSnapshot& p = m_debugPacket;
    const u32 now = Device.dwFrame;
    ImGui::Text("packet frame %u | now %u | age %u | readback decoded at %u | %u decoded | version %u", p.metadata[1], now,
        now - p.metadata[1], m_debugReadbackFrame, m_debugReadbackCount, p.metadata[0]);
    if (m_debugDesync)
        ImGui::TextColored(kColorWarn, "GPU packet is newer than the decoded values shown here");
    if (m_debugFailed)
        ImGui::TextColored(kColorBad, "last readback failed; showing the previous decoded packet");
    if (!m_debugIssues.empty())
        TextLines(m_debugIssues, kColorBad);
    if (!m_debugWarnings.empty())
        TextLines(m_debugWarnings, kColorWarn);

    DrawDebugLegend();
    DrawDebugSurface(p);
    DrawDebugCorners(p);
    ImGui::End();
}

void SkyVisibilityGrid::DrawDebugSurface(const SkyProbeDebugSnapshot& p)
{
    if (ImGui::CollapsingHeader("Surface, origins and settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("surface status %u: %s", p.metadata[3], SkyProbeDebugReport::SurfaceStatusName(p.metadata[3]));
        ImGui::Text("pixel (%u, %u) of %u x %u", p.selection[0], p.selection[1], p.selection[2], p.selection[3]);
        ImGui::Text("camera (%.4f, %.4f, %.4f)", p.camera.x, p.camera.y, p.camera.z);
        ImGui::Text("P (%.5f, %.5f, %.5f) depth %.6f", p.surface.x, p.surface.y, p.surface.z, p.surface.w);
        ImGui::Text("Ns (%.5f, %.5f, %.5f) roughness %.5f", p.shadingNormal.x, p.shadingNormal.y, p.shadingNormal.z,
            p.shadingNormal.w);
        ImGui::Text("Ng (%.5f, %.5f, %.5f) dot(Ng,V) %.5f", p.geometricNormal.x, p.geometricNormal.y,
            p.geometricNormal.z, p.geometricNormal.w);
        ImGui::Text("V (%.5f, %.5f, %.5f) distance %.4f", p.view.x, p.view.y, p.view.z, p.view.w);
        ImGui::Text("material (%.4f, %.4f, %.4f, %.4f)", p.material.x, p.material.y, p.material.z, p.material.w);
        ImGui::Text("grid position (%.5f, %.5f, %.5f) |biased-P| %.5f", p.biased.x, p.biased.y, p.biased.z, p.biased.w);
        ImGui::Text("ray origin (diagnostic) (%.5f, %.5f, %.5f) |origin-P| %.5f", p.rayOrigin.x, p.rayOrigin.y, p.rayOrigin.z,
            p.rayOrigin.w);
        ImGui::Text("control origin (%.5f, %.5f, %.5f) offset %.4f", p.controlOrigin.x, p.controlOrigin.y,
            p.controlOrigin.z, p.controlOrigin.w);
        ImGui::Text("reflection R (%.5f, %.5f, %.5f) grid-clamped %.0f", p.reflection.x, p.reflection.y,
            p.reflection.z, p.reflection.w);
        ImGui::Text("grid %u x %u x %u = %u | origin (%.3f, %.3f, %.3f) | spacing %.4f", p.gridDims[0], p.gridDims[1],
            p.gridDims[2], p.gridDims[3], p.gridOrigin.x, p.gridOrigin.y, p.gridOrigin.z, p.gridOrigin.w);
        ImGui::Text("requested spacing %.3f | rays %.0f | probe max %.0f", p.settings.x, p.settings.y, p.settings.z);
        ImGui::Text("sky trace %.3f | backface %.3f | sky IBL %.0f | lighting mode %.0f", p.lighting.x, p.lighting.y,
            p.lighting.z, p.lighting.w);
        ImGui::TextColored(p.metadata[2] == SkyProbeDebugSnapshot::kProductionMoments ? kColorGood : kColorBad,
            "production visibility: %s", ProductionSourceName(p.metadata[2]));
        ImGui::Text("scene revision %llu | diagnostic ray-query TLAS %u | deferred lighting %u (%s)",
            static_cast<unsigned long long>(u64(p.scene[0]) | (u64(p.scene[1]) << 32)), p.scene[2], p.scene[3],
            SkyProbeDebugReport::SceneStatusName(p.scene[3]));
        ImGui::TextDisabled("Rays R0-R5 are binary geometric reference segments, not production occlusion. Raw depthMean/depthSigma are in each corner.");
    }
    if (ImGui::CollapsingHeader("Production result", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("raw weight sum %.6f | best moment visibility %.6f | confidence %.6f | floor-limited share %.6f", p.summary.x,
            p.summary.y, p.summary.z, p.summary.w);
        ImGui::Text("SH after confidence: c0 %.5f | c1 (%.5f, %.5f, %.5f)", p.sampleSH.x, p.sampleSH.y, p.sampleSH.z,
            p.sampleSH.w);
        ImGui::Text("sky: Ns %.5f | Ng %.5f | -Ns %.5f | R %.5f", p.sky.x, p.sky.y, p.sky.z, p.sky.w);
    }
}

void SkyVisibilityGrid::DrawDebugCorners(const SkyProbeDebugSnapshot& p)
{
    if (p.metadata[3] < 2 || p.metadata[3] > 4 || p.metadata[3] == 3)
    {
        ImGui::TextColored(kColorWarn, "corner data is not meaningful: %s", SkyProbeDebugReport::SurfaceStatusName(p.metadata[3]));
        return;
    }
    if (!ImGui::CollapsingHeader("Corners (moment visibility, raw vs normalized weight)", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    float rawSum = 0.0f;
    float normSum = 0.0f;
    float blockedSum = 0.0f;
    u32 retained = 0;
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("sky_probe_corners", 14, flags))
    {
        const char* headers[] = {"#", "State", "Cell", "Tri", "Facing", "Moment vis", "Raw w", "Norm w", "Floor norm",
            "SH Ns", "Weighted", "Sky clamp", "Rays R0-R5", "Note"};
        for (const char* header : headers)
            ImGui::TableSetupColumn(header);
        ImGui::TableHeadersRow();
        for (u32 i = 0; i < 8; ++i)
        {
            const SkyProbeDebugCorner& k = p.corners[i];
            const bool floorLimited = k.identity[1] == 4 && k.weights.z < kFloorLimited;
            rawSum += k.weights.w;
            normSum += k.contribution.x;
            blockedSum += k.contribution.z;
            retained += floorLimited ? 1u : 0u;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%u", i);
            ImGui::TableNextColumn();
            ImGui::Text("%s", SkyProbeDebugReport::CornerStateName(k.identity[1]));
            ImGui::TableNextColumn();
            ImGui::Text("%u,%u,%u", k.identity[2], k.identity[3], k.cellAndFlags[0]);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f", k.weights.x);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f", k.weights.y);
            ImGui::TableNextColumn();
            ImGui::TextColored(floorLimited ? kColorWarn : kColorGood, "%.4f", k.weights.z);
            ImGui::TableNextColumn();
            ImGui::Text("%.5f", k.weights.w);
            ImGui::TableNextColumn();
            ImGui::Text("%.5f", k.contribution.x);
            ImGui::TableNextColumn();
            ImGui::Text("%.5f", k.contribution.z);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f", k.sky.x);
            ImGui::TableNextColumn();
            ImGui::Text("%.5f", k.contribution.y);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f", k.sky.y);
            ImGui::TableNextColumn();
            RayLetters(k);
            ImGui::TableNextColumn();
            if (floorLimited)
                ImGui::TextColored(kColorWarn, "moment vis < floor, weight clamped");
            else if (k.identity[1] != 4)
                ImGui::TextColored(kColorWarn, "%s", SkyProbeDebugReport::CornerStateName(k.identity[1]));
        }
        ImGui::EndTable();
    }
    ImGui::Text("sum raw %.5f (summary %.5f) | sum norm %.5f | sum floor-limited norm %.5f (summary share %.5f)", rawSum,
        p.summary.x, normSum, blockedSum, p.summary.w);
    if (retained != 0)
        ImGui::TextColored(kColorWarn, "%u corner(s) have moment visibility below 0.05: their weight is clamped to the floor", retained);
    ImGui::TextDisabled("Rays: H hit, M miss, D degenerate, - not traced. Order R0..R5; binary reference only, not production visibility.");

    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = p.corners[i];
        if (!ImGui::TreeNode(reinterpret_cast<void*>(uintptr_t(i)), "Corner %u details", i))
            continue;
        ImGui::Text("probe position (%.4f, %.4f, %.4f) distance %.4f", k.position.x, k.position.y, k.position.z,
            k.position.w);
        ImGui::Text("relocation (%.4f, %.4f, %.4f) length %.4f", k.offset.x, k.offset.y, k.offset.z, k.offset.w);
        ImGui::Text("flags 0x%08X | failure reason %u | index %u", k.cellAndFlags[1], k.cellAndFlags[2], k.identity[0]);
        ImGui::Text("SH c0 %.5f | c1 (%.5f, %.5f, %.5f)", k.coefficients.x, k.coefficients.y, k.coefficients.z,
            k.coefficients.w);
        ImGui::Text("sky Ns unclamped %.5f clamped %.5f | Ng unclamped %.5f | R clamped %.5f", k.sky.x, k.sky.y,
            k.sky.z, k.sky.w);
        ImGui::Text("weight before cubic %.6f", k.contribution.w);
        ImGui::Text("moment: |sample-probe| %.5f | mean depth %.5f | variance+tolerance %.6f | chebyshev^3 visibility %.6f",
            k.moment.x, k.moment.y, k.moment.z, k.moment.w);
        if (m_debugShowRaw)
        {
            ImGui::Text("raw visibility uint4 %08X %08X %08X %08X", k.rawVisibility[0], k.rawVisibility[1],
                k.rawVisibility[2], k.rawVisibility[3]);
            ImGui::Text("raw depthMean  uint4 %08X %08X %08X %08X", k.rawDepthMean[0], k.rawDepthMean[1],
                k.rawDepthMean[2], k.rawDepthMean[3]);
            ImGui::Text("raw depthSigma uint4 %08X %08X %08X %08X", k.rawDepthSigma[0], k.rawDepthSigma[1],
                k.rawDepthSigma[2], k.rawDepthSigma[3]);
        }
        ImGui::TextDisabled("Rays below are binary geometric reference segments (diagnostic only, not production visibility).");
        const ImGuiTableFlags rayFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX;
        if (ImGui::BeginTable("sky_probe_rays", 8, rayFlags))
        {
            const char* rayHeaders[] = {"Ray", "Config", "Status", "Segment", "HitT", "Seg vis", "Front", "Instance/Geom/Prim"};
            for (const char* header : rayHeaders)
                ImGui::TableSetupColumn(header);
            ImGui::TableHeadersRow();
            for (u32 r = 0; r < 6; ++r)
            {
                const SkyProbeDebugRay& ray = k.rays[r];
                char inst[16];
                char geom[16];
                char prim[16];
                FormatId(inst, ray.hit[1]);
                FormatId(geom, ray.hit[2]);
                FormatId(prim, ray.hit[3]);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("R%u", r);
                ImGui::TableNextColumn();
                ImGui::Text("%s", SkyProbeDebugReport::RayLabel(r));
                ImGui::TableNextColumn();
                ImGui::Text("%s", SkyProbeDebugReport::RayStatusName(ray.hit[0]));
                ImGui::TableNextColumn();
                ImGui::Text("%.4f", ray.metrics.x);
                ImGui::TableNextColumn();
                ImGui::Text("%.4f", ray.metrics.y);
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", ray.metrics.z);
                ImGui::TableNextColumn();
                ImGui::Text("%.0f", ray.metrics.w);
                ImGui::TableNextColumn();
                ImGui::Text("%s / %s / %s", inst, geom, prim);
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
}

void SkyVisibilityGrid::DrawDebugLegend() const
{
    if (!ImGui::CollapsingHeader("Legend"))
        return;
    ImGui::TextWrapped("Production sky visibility is the continuous depth-moment (Chebyshev) weight stored per probe. "
                       "The six rays R0-R5 are binary geometric reference segments and do NOT enter production lighting.");
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 1.0f, 1.0f), "magenta corner: moment visibility at or above the 0.05 weight floor");
    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.1f, 1.0f),
        "orange corner: moment visibility below 0.05 (weight is clamped to the floor, not removed)");
    ImGui::TextColored(kColorBad, "red corner: invalid probe");
    ImGui::TextColored(ImVec4(0.35f, 0.5f, 1.0f, 1.0f), "blue corner: unbaked");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "gray corner: zero trilinear weight");
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 1.0f, 1.0f), "cyan rim: relocated probe | R: diagnostic ray origin");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "P: actual surface position");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "B: grid selection and moment evaluation (biased) position");
    ImGui::TextDisabled("Striped/dim probe markers are occluded from the camera only. Dashed R->corner lines are x-ray guides to the diagnostic segment endpoint.");
    ImGui::TextDisabled("World labels C0..C7 are projected on the CPU with the current view; '(stale)' means the decoded values lag the GPU packet.");
}

void SkyVisibilityGrid::DrawDebugLabels(const SkyProbeDebugSnapshot& p) const
{
    if (p.metadata[3] < 2 || p.metadata[3] > 4 || p.metadata[3] == 3 || !m_debugIssues.empty())
        return;
    ImDrawList* list = ImGui::GetForegroundDrawList();
    if (!list)
        return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (!(display.x > 0.0f) || !(display.y > 0.0f))
        return;
    const bool stale = m_debugLease != 0 || m_debugDesync || m_debugFailed;

    const auto project = [&](const Fvector4& world, ImVec2& screen)
    {
        Fvector position;
        position.set(world.x, world.y, world.z);
        Fvector4 clip;
        Device.mFullTransform.transform(clip, position);
        if (!(clip.w > 0.0f) || !std::isfinite(clip.x) || !std::isfinite(clip.y) || std::fabs(clip.x) > 1.1f ||
            std::fabs(clip.y) > 1.1f)
            return false;
        screen = ImVec2((clip.x * 0.5f + 0.5f) * display.x, (0.5f - clip.y * 0.5f) * display.y);
        return true;
    };
    const auto label = [&](const Fvector4& world, ImU32 color, const char* text)
    {
        ImVec2 screen;
        if (!project(world, screen))
            return;
        char buffer[64];
        xr_sprintf(buffer, "%s%s", text, stale ? " (stale)" : "");
        list->AddText(ImVec2(screen.x + 6.0f, screen.y - 6.0f), IM_COL32(0, 0, 0, 255), buffer);
        list->AddText(ImVec2(screen.x + 5.0f, screen.y - 7.0f), color, buffer);
    };

    label(p.surface, IM_COL32(255, 255, 255, 255), "P");
    label(p.biased, IM_COL32(255, 255, 50, 255), "B");
    label(p.rayOrigin, IM_COL32(80, 255, 255, 255), "R");
    for (u32 i = 0; i < 8; ++i)
    {
        const SkyProbeDebugCorner& k = p.corners[i];
        ImU32 color = IM_COL32(255, 50, 255, 255);
        switch (k.identity[1])
        {
        case 1: color = IM_COL32(150, 150, 150, 255); break;
        case 2: color = IM_COL32(90, 130, 255, 255); break;
        case 3: color = IM_COL32(255, 80, 70, 255); break;
        case 4: color = k.weights.z < kFloorLimited ? IM_COL32(255, 140, 25, 255) : IM_COL32(255, 50, 255, 255); break;
        default: color = IM_COL32(150, 150, 150, 255); break;
        }
        char text[48];
        xr_sprintf(text, "C%u w=%.3f", i, k.contribution.x);
        label(k.position, color, text);
    }
}
}
