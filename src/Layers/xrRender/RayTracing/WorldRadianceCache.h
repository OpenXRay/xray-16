#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
class RenderPassBuilder;
}

namespace xray::render::fg
{
class RenderDevice;

class WorldRadianceCacheConfig
{
public:
    bool enabled = false;
    bool jitter = true;
    u32 bounce = 1;
    u32 debug = 0;
    u32 capacityLog2 = 18;
    u32 lifetime = 30;
    u32 updateTarget = 32768;
    u32 maxSamples = 32;
    float cellSize = 0.25f;
    float lodScale = 8.0f;
};

class WorldRadianceCacheCB
{
public:
    Fvector4 camera;
    float cellSize;
    float lodScale;
    u32 capacityMask;
    u32 lifetime;
    u32 frame;
    u32 updateTarget;
    u32 maxSamples;
    u32 flags;
    u32 bounce;
    u32 epoch;
    u32 pad1;
    u32 pad2;
};
static_assert(sizeof(WorldRadianceCacheCB) == 64);

enum class WorldRadianceCacheAccess : u8
{
    Maintain,
    Update,
    Query
};

class WorldRadianceCacheResources
{
public:
    framegraph::VirtualResourceHandle checksum;
    framegraph::VirtualResourceHandle life;
    framegraph::VirtualResourceHandle radiance;
    framegraph::VirtualResourceHandle snapshot;
    framegraph::VirtualResourceHandle position;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle stats;
    WorldRadianceCacheAccess access = WorldRadianceCacheAccess::Query;
    bool Valid() const;
};

class WorldRadianceCacheBuffers
{
public:
    nvrhi::IBuffer* checksum = nullptr;
    nvrhi::IBuffer* life = nullptr;
    nvrhi::IBuffer* radiance = nullptr;
    nvrhi::IBuffer* snapshot = nullptr;
    nvrhi::IBuffer* radianceInput = nullptr;
    nvrhi::IBuffer* position = nullptr;
    nvrhi::IBuffer* normal = nullptr;
    nvrhi::IBuffer* stats = nullptr;
    bool Valid() const;
};

class WorldRadianceCacheStats
{
public:
    static constexpr u32 kEventCount = 4;

    u32 capacity = 0;
    u32 liveCells = 0;
    u32 events[kEventCount] = {};
    u64 bytes = 0;
    bool liveCellsKnown = false;
    bool eventsKnown = false;
};

class WorldRadianceCacheReadback
{
public:
    nvrhi::BufferHandle buffer;
    u64 lease = 0;
    u64 sequence = 0;
    u32 epoch = 0;
    bool events = false;
};

class WorldRadianceCache
{
public:
    static constexpr u32 kReadbackSlots = 3;
    static constexpr u32 kStatsWords = 8;
    static constexpr u32 kFlagEnabled = 1u;
    static constexpr u32 kFlagJitter = 2u;
    static constexpr u32 kDebugShift = 4u;
    static constexpr u32 kMinCapacityLog2 = 10;
    static constexpr u32 kMaxCapacityLog2 = 22;

    void Initialize(RenderDevice* device);
    void Shutdown();
    void Invalidate();
    bool Prepare(const WorldRadianceCacheConfig& config);
    bool IsAllocated() const;
    bool IsContentReady() const;
    void RecordPendingClear(nvrhi::ICommandList* commandList, const WorldRadianceCacheBuffers& buffers);
    void TrackRecordedWork();
    WorldRadianceCacheResources Use(framegraph::FrameGraph& graph, framegraph::RenderPassBuilder& builder,
        WorldRadianceCacheAccess access) const;
    static WorldRadianceCacheBuffers Resolve(const framegraph::FrameGraph& graph,
        const WorldRadianceCacheResources& resources);
    WorldRadianceCacheCB BuildConstants(const Fvector& cameraPos, u32 frame, bool active) const;
    void ScheduleStatsReadback(nvrhi::ICommandList* commandList, nvrhi::IBuffer* stats, bool events);
    const WorldRadianceCacheConfig& GetConfig() const;
    u32 GetCapacity() const;
    u32 GetEpoch() const;
    WorldRadianceCacheStats GetStats() const;

private:
    bool Allocate(u32 capacity);
    void Release();
    void PollSubmittedWork();
    void ProcessStatsReadback();
    void ReleaseLeases();
    nvrhi::BufferHandle CreateCellBuffer(const char* name, u32 capacity, u32 stride) const;

    RenderDevice* m_device = nullptr;
    WorldRadianceCacheConfig m_config;
    nvrhi::BufferHandle m_checksum;
    nvrhi::BufferHandle m_life;
    nvrhi::BufferHandle m_radiance;
    nvrhi::BufferHandle m_snapshot;
    nvrhi::BufferHandle m_position;
    nvrhi::BufferHandle m_normal;
    nvrhi::BufferHandle m_stats;
    WorldRadianceCacheReadback m_readback[kReadbackSlots];
    xr_vector<u64> m_workLeases;
    u64 m_frameLease = 0;
    u64 m_readbackSequence = 0;
    u64 m_readbackLatest = 0;
    u32 m_capacity = 0;
    u32 m_epoch = 0;
    u32 m_readbackWrite = 0;
    u32 m_liveCells = 0;
    u32 m_events[WorldRadianceCacheStats::kEventCount] = {};
    bool m_liveCellsKnown = false;
    bool m_eventsKnown = false;
    bool m_clearPending = false;
};
}
