#include "stdafx.h"
#include "SkyVisibilityGrid.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IRenderBackend.h"
#include <cmath>

namespace xray::render::fg
{
namespace
{
constexpr u32 kCacheMagic = 0x56594B53u;
constexpr u32 kCacheVersion = 8;
constexpr u64 kUploadSlice = 4ull << 20;

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
        StartBake();
    }
    PollReadback();
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
}
