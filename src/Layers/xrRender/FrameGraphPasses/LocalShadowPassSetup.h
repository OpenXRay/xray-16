#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
        class light;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::fg::passes {

constexpr u32 kLocalShadowAtlas = 4096;
constexpr u32 kLocalAtlasLevels = 6;
constexpr u32 kLocalAtlasNodes = 1365;
constexpr u32 kLocalSpotTileMax = 1024;
constexpr u32 kLocalSpotTileMin = 256;
constexpr u32 kLocalPointFaceMax = 512;
constexpr u32 kLocalPointFaceMin = 128;
constexpr u32 kLocalSpotSlotsMax = 64;
constexpr u32 kLocalPointLightsMax = 40;
constexpr u32 kLocalTileCount = 256;
constexpr u32 kLocalPullVertices = 384;
constexpr u32 kLocalStatWords = 48;
constexpr u32 kLocalStreamCount = 6;
constexpr u32 kLocalPairCapOpaque = 1u << 18;
constexpr u32 kLocalPairCapTerrain = 1u << 16;
constexpr u32 kLocalPairCapAT = 1u << 17;
constexpr u32 kLocalPairCapDynOpaque = 1u << 16;
constexpr u32 kLocalPairCapDynAT = 1u << 15;
constexpr u32 kLocalPairCapSkinned = 1u << 16;

struct LocalShadowViewGPU {
    Fmatrix viewProj;
    Fvector4 rect;
    Fvector4 zparams;
    Fvector4 lightPos;
    Fvector4 planes[6];
    Fvector4 shape;
    u32 meta[4];
};
static_assert(sizeof(LocalShadowViewGPU) == 240, "LocalShadowViewGPU is shader-visible");

struct LocalAtlasAllocator {
    u16 nodeX[kLocalAtlasNodes] = {};
    u16 nodeY[kLocalAtlasNodes] = {};
    u8 nodeLevel[kLocalAtlasNodes] = {};
    u8 nodeState[kLocalAtlasNodes] = {};
    u16 nodeSlot[kLocalAtlasNodes] = {};
    xr_vector<u32> freeList[kLocalAtlasLevels];
    u32 usedTexels = 0;
    bool built = false;

    void Reset();
    u32 Alloc(u32 level);
    void Free(u32 node);
    void Rect(u32 node, u32& x, u32& y, u32& size) const;
    u32 UsedTexels() const { return usedTexels; }

    static u32 SizeOf(u32 level) { return kLocalShadowAtlas >> level; }
    static u32 LevelOf(u32 size);

private:
    void Build();
    void ListPush(u32 node);
    void ListRemove(u32 node);
    u32 Take(u32 level);
};

struct LocalShadowState {
    // Each page has a bounded work list; additional pages preserve all admitted lights.
    xr_vector<xr_unique_ptr<LocalShadowState>> overflowPages;
    u32 activePages = 0;
    u32 atlasLayers = 0;
    u32 atlasLayer = 0;
    nvrhi::BufferHandle receiverTiles;
    const light* owners[kLocalTileCount] = {};
    LocalAtlasAllocator atlas;
    LocalShadowViewGPU request[kLocalTileCount] = {};
    u32 candList[kLocalTileCount][4] = {};
    u32 candCount = 0;
    u32 dirtyViews = 0;
    u32 nextSerial = 0;
    bool stateReset = true;
    u32 pairCapacity[kLocalStreamCount] = {};
    nvrhi::IBuffer* lastEntryBuffer = nullptr;
    nvrhi::IBuffer* lastBvhNodeBuffer = nullptr;
    xr_vector<u32> slotOfLight;
    u32 pooledSpots = 0;
    u32 pooledPoints = 0;
    u32 frame = 0;
    u32 statAccepted = 0;
    u32 statDeferred = 0;
    u32 statUpToDate = 0;
    u32 statPairs = 0;
    u32 statDynPairs = 0;
    u32 statSkinnedPairs = 0;
    u32 statDrops = 0;
    u32 statDynDrops = 0;
    u32 statMaxVisited = 0;
    u32 statDynRefresh = 0;
    u32 statAtlasPercent = 0;
    u32 statOverflowViews = 0;
    u32 statCasterBatches = 0;

    static constexpr u32 kReadbackSlots = 4;
    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;

    nvrhi::BufferHandle requestBuffer;
    nvrhi::BufferHandle stateBuffer;
    nvrhi::BufferHandle candListBuffer;
    nvrhi::BufferHandle tileCount;
    nvrhi::BufferHandle schedule;
    nvrhi::BufferHandle dirtyList;
    nvrhi::BufferHandle refreshDynBuffer;
    nvrhi::BufferHandle pairBase;
    nvrhi::BufferHandle emitArgs;
    nvrhi::BufferHandle stats;
    nvrhi::BufferHandle pairs[kLocalStreamCount];
    nvrhi::BufferHandle args;
    nvrhi::BufferHandle clearArgs;
    nvrhi::TextureHandle staticAtlas;
    nvrhi::TextureHandle dynAtlas;
    bool staticAtlasFirst = true;
    bool dynAtlasFirst = true;

    nvrhi::ComputePipelineHandle binCountPipeline;
    nvrhi::BindingLayoutHandle binCountLayout;
    nvrhi::ComputePipelineHandle binReservePipeline;
    nvrhi::BindingLayoutHandle binReserveLayout;
    nvrhi::ComputePipelineHandle binEmitPipeline;
    nvrhi::BindingLayoutHandle binEmitLayout;
    nvrhi::ComputePipelineHandle binDynPipeline;
    nvrhi::BindingLayoutHandle binDynLayout;
    nvrhi::ComputePipelineHandle argsPipeline;
    nvrhi::BindingLayoutHandle argsLayout;
    nvrhi::GraphicsPipelineHandle clearPipeline;
    nvrhi::BindingLayoutHandle clearLayout;
    nvrhi::GraphicsPipelineHandle pagePipeline;
    nvrhi::BindingLayoutHandle pageLayout;
    nvrhi::GraphicsPipelineHandle pageATPipeline;
    nvrhi::BindingLayoutHandle pageATLayout;
    nvrhi::GraphicsPipelineHandle skinPagePipeline;
    nvrhi::BindingLayoutHandle skinPageLayout;
    nvrhi::ShaderHandle pageVS;
    nvrhi::ShaderHandle skinPageVS;
    bool pipelinesFailed = false;
    bool resourcesFailed = false;
};

struct LocalShadowConfig {
    GPUCullingManager* gpuCulling = nullptr;
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* bvhNodeBuffer = nullptr;
    nvrhi::IBuffer* bvhIndexBuffer = nullptr;
    u32 bvhNodeCount = 0;
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* dynamicInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
};

struct LocalShadowOutput {
    framegraph::VirtualResourceHandle tiles;
    framegraph::VirtualResourceHandle staticAtlas;
    framegraph::VirtualResourceHandle dynAtlas;
    LocalShadowState* state = nullptr;
    bool active = false;
};

void ResetLocalShadowPool(LocalShadowState& state);

void WarmLocalShadowPool(fg::RenderDevice* device, LocalShadowState& state);

void ProcessLocalShadowStats(LocalShadowState& state, nvrhi::IDevice* device);

void SelectLocalShadowLights(
    LocalShadowState& state,
    const xr_vector<const light*>& lights,
    const Fvector& camPos,
    float projScale);

LocalShadowOutput setupLocalShadowPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    const LocalShadowConfig& config,
    LocalShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler);

void ResolveLocalShadowBindings(
    const framegraph::FrameGraph& fg,
    const LocalShadowOutput& out,
    nvrhi::IDevice* device,
    nvrhi::IBuffer*& tiles,
    nvrhi::ITexture*& staticAtlas,
    nvrhi::ITexture*& dynAtlas);

}
