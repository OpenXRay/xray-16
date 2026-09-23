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
    u32 pad0;
    u32 pad1;
    u32 pad2;
};
static_assert(sizeof(WorldRadianceCacheCB) == 64);

class WorldRadianceCacheResources
{
public:
    framegraph::VirtualResourceHandle checksum;
    framegraph::VirtualResourceHandle life;
    framegraph::VirtualResourceHandle radiance;
    framegraph::VirtualResourceHandle position;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle stats;
    bool Valid() const;
};

class WorldRadianceCacheBuffers
{
public:
    nvrhi::IBuffer* checksum = nullptr;
    nvrhi::IBuffer* life = nullptr;
    nvrhi::IBuffer* radiance = nullptr;
    nvrhi::IBuffer* position = nullptr;
    nvrhi::IBuffer* normal = nullptr;
    nvrhi::IBuffer* stats = nullptr;
    bool Valid() const;
};

class WorldRadianceCacheStats
{
public:
    u32 capacity = 0;
    u32 liveCells = 0;
    u64 bytes = 0;
    bool liveCellsKnown = false;
};

class WorldRadianceCache
{
public:
    static constexpr u32 kReadbackSlots = 3;
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
    bool ConsumePendingClear();
    WorldRadianceCacheResources Use(framegraph::FrameGraph& graph, framegraph::RenderPassBuilder& builder) const;
    static WorldRadianceCacheBuffers Resolve(const framegraph::FrameGraph& graph,
        const WorldRadianceCacheResources& resources);
    WorldRadianceCacheCB BuildConstants(const Fvector& cameraPos, u32 frame, bool active) const;
    void ScheduleStatsReadback(nvrhi::ICommandList* commandList, nvrhi::IBuffer* stats);
    void ProcessStatsReadback();
    const WorldRadianceCacheConfig& GetConfig() const;
    u32 GetCapacity() const;
    WorldRadianceCacheStats GetStats() const;

private:
    bool Allocate(u32 capacity);
    void Release();
    nvrhi::BufferHandle CreateCellBuffer(const char* name, u32 capacity, u32 stride) const;

    RenderDevice* m_device = nullptr;
    WorldRadianceCacheConfig m_config;
    nvrhi::BufferHandle m_checksum;
    nvrhi::BufferHandle m_life;
    nvrhi::BufferHandle m_radiance;
    nvrhi::BufferHandle m_position;
    nvrhi::BufferHandle m_normal;
    nvrhi::BufferHandle m_stats;
    nvrhi::BufferHandle m_readback[kReadbackSlots];
    u32 m_capacity = 0;
    u32 m_readbackWrite = 0;
    u32 m_readbackScheduled = 0;
    u32 m_liveCells = 0;
    bool m_liveCellsKnown = false;
    bool m_clearPending = false;
};
}
