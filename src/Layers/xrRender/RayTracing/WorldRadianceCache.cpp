#include "stdafx.h"
#include "WorldRadianceCache.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg
{
bool WorldRadianceCacheResources::Valid() const
{
    return checksum.is_valid() && life.is_valid() && radiance.is_valid() && position.is_valid() &&
        normal.is_valid() && stats.is_valid();
}

bool WorldRadianceCacheBuffers::Valid() const
{
    return checksum && life && radiance && position && normal && stats;
}

void WorldRadianceCache::Initialize(RenderDevice* device)
{
    m_device = device;
    m_config = {};
    m_clearPending = true;
}

void WorldRadianceCache::Shutdown()
{
    Release();
    for (auto& slot : m_readback)
        slot = nullptr;
    m_readbackWrite = 0;
    m_readbackScheduled = 0;
    m_liveCells = 0;
    m_liveCellsKnown = false;
    m_device = nullptr;
}

void WorldRadianceCache::Invalidate()
{
    m_clearPending = true;
    m_liveCells = 0;
    m_liveCellsKnown = false;
}

bool WorldRadianceCache::Prepare(const WorldRadianceCacheConfig& config)
{
    const bool wasEnabled = m_config.enabled;
    m_config = config;
    m_config.capacityLog2 = std::clamp(config.capacityLog2, kMinCapacityLog2, kMaxCapacityLog2);
    m_config.lifetime = std::max(config.lifetime, 2u);
    m_config.maxSamples = std::max(config.maxSamples, 1u);
    m_config.updateTarget = std::max(config.updateTarget, 1u);
    m_config.cellSize = std::max(config.cellSize, 0.01f);
    m_config.lodScale = std::max(config.lodScale, 0.01f);
    if (!m_device || !m_device->GetNVRHIDevice())
        return false;
    const u32 capacity = m_config.enabled ? (1u << m_config.capacityLog2) : 1u;
    if (m_capacity != capacity)
    {
        if (!Allocate(capacity))
            return false;
    }
    if (m_config.enabled && !wasEnabled)
        Invalidate();
    return true;
}

bool WorldRadianceCache::IsAllocated() const
{
    return m_capacity != 0 && m_checksum && m_life && m_radiance && m_position && m_normal && m_stats;
}

bool WorldRadianceCache::ConsumePendingClear()
{
    const bool pending = m_clearPending;
    m_clearPending = false;
    return pending;
}

nvrhi::BufferHandle WorldRadianceCache::CreateCellBuffer(const char* name, u32 capacity, u32 stride) const
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = u64(capacity) * stride;
    desc.structStride = stride;
    desc.canHaveUAVs = true;
    desc.canHaveRawViews = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return m_device->GetNVRHIDevice()->createBuffer(desc);
}

bool WorldRadianceCache::Allocate(u32 capacity)
{
    Release();
    m_checksum = CreateCellBuffer("WorldCache_Checksum", capacity, sizeof(u32));
    m_life = CreateCellBuffer("WorldCache_Life", capacity, sizeof(u32));
    m_radiance = CreateCellBuffer("WorldCache_Radiance", capacity, sizeof(Fvector4));
    m_position = CreateCellBuffer("WorldCache_Position", capacity, sizeof(Fvector4));
    m_normal = CreateCellBuffer("WorldCache_Normal", capacity, sizeof(Fvector4));
    m_stats = CreateCellBuffer("WorldCache_Stats", 4, sizeof(u32));
    if (!m_checksum || !m_life || !m_radiance || !m_position || !m_normal || !m_stats)
    {
        Release();
        return false;
    }
    m_capacity = capacity;
    m_clearPending = true;
    m_liveCells = 0;
    m_liveCellsKnown = false;
    return true;
}

void WorldRadianceCache::Release()
{
    m_checksum = nullptr;
    m_life = nullptr;
    m_radiance = nullptr;
    m_position = nullptr;
    m_normal = nullptr;
    m_stats = nullptr;
    m_capacity = 0;
}

static framegraph::VirtualResourceHandle ImportCacheBuffer(framegraph::FrameGraph& graph, const char* name,
    nvrhi::IBuffer* buffer)
{
    if (!buffer)
        return {};
    framegraph::ResourceDesc desc;
    desc.type = framegraph::ResourceDesc::Type::Buffer;
    desc.debugName = name;
    desc.bufferSize = buffer->getDesc().byteSize;
    desc.structStride = buffer->getDesc().structStride;
    desc.isUAV = true;
    desc.allowUAV = true;
    desc.isImported = true;
    desc.isTransient = false;
    return graph.ImportBuffer(name, buffer, desc);
}

WorldRadianceCacheResources WorldRadianceCache::Use(framegraph::FrameGraph& graph,
    framegraph::RenderPassBuilder& builder) const
{
    WorldRadianceCacheResources resources;
    if (!IsAllocated())
        return resources;
    resources.checksum = ImportCacheBuffer(graph, "WorldCache_Checksum", m_checksum);
    resources.life = ImportCacheBuffer(graph, "WorldCache_Life", m_life);
    resources.radiance = ImportCacheBuffer(graph, "WorldCache_Radiance", m_radiance);
    resources.position = ImportCacheBuffer(graph, "WorldCache_Position", m_position);
    resources.normal = ImportCacheBuffer(graph, "WorldCache_Normal", m_normal);
    resources.stats = ImportCacheBuffer(graph, "WorldCache_Stats", m_stats);
    if (!resources.Valid())
        return {};
    builder.readWrite(resources.checksum, framegraph::ResourceState::UnorderedAccess);
    builder.readWrite(resources.life, framegraph::ResourceState::UnorderedAccess);
    builder.readWrite(resources.radiance, framegraph::ResourceState::UnorderedAccess);
    builder.readWrite(resources.position, framegraph::ResourceState::UnorderedAccess);
    builder.readWrite(resources.normal, framegraph::ResourceState::UnorderedAccess);
    builder.readWrite(resources.stats, framegraph::ResourceState::UnorderedAccess);
    return resources;
}

WorldRadianceCacheBuffers WorldRadianceCache::Resolve(const framegraph::FrameGraph& graph,
    const WorldRadianceCacheResources& resources)
{
    auto buffer = [&](framegraph::VirtualResourceHandle handle)
    {
        return handle.is_valid() ? graph.GetPhysicalBuffer(handle) : nullptr;
    };
    WorldRadianceCacheBuffers buffers;
    buffers.checksum = buffer(resources.checksum);
    buffers.life = buffer(resources.life);
    buffers.radiance = buffer(resources.radiance);
    buffers.position = buffer(resources.position);
    buffers.normal = buffer(resources.normal);
    buffers.stats = buffer(resources.stats);
    return buffers;
}

WorldRadianceCacheCB WorldRadianceCache::BuildConstants(const Fvector& cameraPos, u32 frame, bool active) const
{
    WorldRadianceCacheCB cb = {};
    cb.camera = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f };
    cb.cellSize = m_config.cellSize;
    cb.lodScale = m_config.lodScale;
    cb.capacityMask = m_capacity ? m_capacity - 1 : 0;
    cb.lifetime = m_config.lifetime;
    cb.frame = frame;
    cb.updateTarget = m_config.updateTarget;
    cb.maxSamples = m_config.maxSamples;
    cb.flags = 0;
    if (active && m_config.enabled && IsAllocated())
        cb.flags |= kFlagEnabled;
    if (m_config.jitter)
        cb.flags |= kFlagJitter;
    cb.flags |= (std::min(m_config.debug, 15u) << kDebugShift);
    cb.bounce = m_config.bounce;
    return cb;
}

void WorldRadianceCache::ScheduleStatsReadback(nvrhi::ICommandList* commandList, nvrhi::IBuffer* stats)
{
    if (!commandList || !stats || !m_device)
        return;
    nvrhi::BufferHandle& slot = m_readback[m_readbackWrite];
    if (!slot)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32) * 4;
        desc.debugName = "WorldCache_StatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = m_device->GetNVRHIDevice()->createBuffer(desc);
        if (!slot)
            return;
    }
    commandList->copyBuffer(slot, 0, stats, 0, sizeof(u32) * 4);
    m_readbackWrite = (m_readbackWrite + 1) % kReadbackSlots;
    if (m_readbackScheduled < kReadbackSlots)
        ++m_readbackScheduled;
}

void WorldRadianceCache::ProcessStatsReadback()
{
    if (m_readbackScheduled < kReadbackSlots || !m_device)
        return;
    nvrhi::IBuffer* oldest = m_readback[m_readbackWrite];
    if (!oldest)
        return;
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    const u32* words = static_cast<const u32*>(nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read));
    if (!words)
        return;
    m_liveCells = words[0];
    m_liveCellsKnown = true;
    nvDevice->unmapBuffer(oldest);
}

const WorldRadianceCacheConfig& WorldRadianceCache::GetConfig() const
{
    return m_config;
}

u32 WorldRadianceCache::GetCapacity() const
{
    return m_capacity;
}

WorldRadianceCacheStats WorldRadianceCache::GetStats() const
{
    WorldRadianceCacheStats stats;
    stats.capacity = m_capacity;
    stats.liveCells = m_liveCells;
    stats.liveCellsKnown = m_liveCellsKnown;
    if (IsAllocated())
        stats.bytes = u64(m_capacity) * (2 * sizeof(u32) + 3 * sizeof(Fvector4)) + 4 * sizeof(u32);
    return stats;
}
}
