#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    namespace fg {
        class RenderDevice;
        class GPUCullingManager;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::fg::passes {

constexpr u32 kVSMLevels = 6;
constexpr u32 kVSMVirtualRes = 4096;
constexpr u32 kVSMPageSize = 128;
constexpr u32 kVSMPagesAxis = kVSMVirtualRes / kVSMPageSize;
constexpr u32 kVSMPagesPerLevel = kVSMPagesAxis * kVSMPagesAxis;
constexpr u32 kVSMPageCount = kVSMLevels * kVSMPagesPerLevel;
constexpr u32 kVSMAtlasW = 64;
constexpr u32 kVSMAtlasH = 96;
constexpr u32 kVSMStaticSlots = kVSMAtlasW * kVSMAtlasH;
constexpr u32 kVSMMaxPhys = 2048;
constexpr u32 kVSMAtlasWDyn = 64;
constexpr u32 kVSMAtlasHDyn = 32;
constexpr u32 kVSMDynPairCapOpaque = 1u << 16;
constexpr u32 kVSMDynPairCapAT = 1u << 15;
constexpr u32 kVSMDynPairCapSkinned = 1u << 16;
constexpr u32 kVSMDynStreamCount = 3;
constexpr u32 kVSMPairCapOpaque = 1u << 20;
constexpr u32 kVSMPairCapTerrain = 1u << 19;
constexpr u32 kVSMPairCapAT = 1u << 19;
constexpr u32 kVSMStreamCount = 3;
constexpr float kVSMZNear = -1000.0f;
constexpr float kVSMZFar = 1000.0f;
constexpr float kVSMRefreshTexels = 0.5f;
constexpr u32 kVSMRefreshIntervalMax = 1u << 20;
constexpr u32 kVSMRefreshBudgetMax = 512;
constexpr u32 kVSMLodBiasMax = 3;
constexpr u32 kVSMLodBiasHold = 12;
constexpr u32 kVSMPrimeFrames = 8;
constexpr u32 kVSMPrimeTraceFrames = 400;
constexpr u32 kVSMPrimeTraceQuiet = 10;
constexpr u32 kVSMHudMapSize = 2048;
constexpr float kVSMHudMargin = 0.05f;

struct VsmParams {
    Fmatrix view;
    Fvector4 level[kVSMLevels];
    Fvector4 zparams;
};
static_assert(sizeof(VsmParams) == 176, "VsmParams layout is shader-visible");

struct VSMState {
    static constexpr u32 kReadbackSlots = 6;

    bool active = false;
    VsmParams params;
    Fmatrix sunView;
    s32 pageBase[kVSMLevels][2] = {};
    s32 tileBias[kVSMLevels][2] = {};
    Fvector pivot;
    Fvector sunDir;
    bool pivotValid = false;
    u32 relabels = 0;
    Fvector prevSunDir;
    bool prevSunValid = false;
    bool sunMoving = false;
    float sunRate = 0.0f;
    u32 refreshInterval[kVSMLevels] = {};
    u32 stretchedInterval[kVSMLevels] = {};
    u32 refreshBudget = 0;
    float refreshStretch = 1.0f;
    u32 lodBias = 0;
    u32 lodBiasHold = 0;
    bool sunDown = false;
    bool nightFrozen = false;
    u32 frame = 0;
    u32 invalidations = 0;
    float sunStepMax = 0.0f;
    bool physInit = false;
    bool atlasFirst = true;
    bool dynAtlasFirst = true;
    bool behindLoadScreen = false;
    u32 primeFrames = 0;
    u32 primeTraceLeft = 0;
    u32 primeTraceIdx = 0;
    u32 primeTraceQuiet = 0;

    nvrhi::BufferHandle needed;
    nvrhi::BufferHandle pageTable;
    nvrhi::BufferHandle pageList;
    nvrhi::BufferHandle physTile;
    nvrhi::BufferHandle slotDirty;
    nvrhi::BufferHandle dirtyList;
    nvrhi::BufferHandle drawClear;
    nvrhi::BufferHandle slotFrame;
    nvrhi::BufferHandle slotPivot;
    nvrhi::BufferHandle slotSun;
    nvrhi::TextureHandle atlas;
    nvrhi::TextureHandle dynAtlas;
    nvrhi::BufferHandle dynPageTable;
    nvrhi::BufferHandle dynPageList;
    nvrhi::BufferHandle dynAllocInfo;
    nvrhi::BufferHandle dynClearArgs;
    nvrhi::BufferHandle dynStats;
    nvrhi::BufferHandle dynPairs[kVSMDynStreamCount];
    nvrhi::BufferHandle dynArgs[kVSMDynStreamCount];
    bool dynActive = false;
    bool dynRendered = false;
    nvrhi::TextureHandle hudMap;
    nvrhi::GraphicsPipelineHandle hudPipeline;
    nvrhi::BindingLayoutHandle hudLayout;
    nvrhi::ShaderHandle hudVS;
    bool hudPipelineFailed = false;
    bool hudRendered = false;
    Fmatrix hudViewProj;
    float hudTexelWorld = 0.0f;
    float hudDepthRange = 1.0f;
    framegraph::VirtualResourceHandle fgHudMap;
    u32 dynCasters = 0;
    u32 dynInstances = 0;
    u32 dynMaxPages = 0;
    framegraph::VirtualResourceHandle fgNeeded;
    framegraph::VirtualResourceHandle fgDirtyList;
    framegraph::VirtualResourceHandle fgAtlas;
    framegraph::VirtualResourceHandle fgDynAtlas;
    framegraph::VirtualResourceHandle fgDynTable;
    framegraph::VirtualResourceHandle fgDynArgs;
    nvrhi::TextureHandle mask[2];
    u32 maskWidth = 0;
    u32 maskHeight = 0;
    u32 maskSlot = 0;
    u32 resolveCount = 0;
    bool maskReady = false;
    Fmatrix prevViewProj;
    Fvector prevCamPos;
    nvrhi::BufferHandle binStats;
    nvrhi::BufferHandle binArgs;
    nvrhi::BufferHandle emitArgs;
    nvrhi::BufferHandle candList;
    nvrhi::BufferHandle pageCount;
    nvrhi::BufferHandle pairBase;
    nvrhi::BufferHandle pairs[kVSMStreamCount];
    u32 pairCapacity[kVSMStreamCount] = {};
    nvrhi::BufferHandle pageArgs[kVSMStreamCount];
    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;

    u32 markPages = 0;
    u32 levelPages[kVSMLevels] = {};
    u32 dirtyPages = 0;
    u32 deferredPages = 0;
    u32 wrongPages = 0;
    u32 refreshPages = 0;
    u32 overduePages = 0;
    u32 binDraws = 0;
    u32 binInstances = 0;
    u32 binMaxVisited = 0;
    u32 binLodCulled = 0;
    u32 binDrops = 0;
    float rasterBias = -1.0f;
    float rasterSlope = -1.0f;
    int atMode = -1;
    float lastBase = -1.0f;
    float lastClusterLod = -1.0f;
    u32 boltHeld = 0;

    nvrhi::ComputePipelineHandle markPipeline;
    nvrhi::BindingLayoutHandle markLayout;
    nvrhi::ComputePipelineHandle residPipeline;
    nvrhi::BindingLayoutHandle residLayout;
    nvrhi::ComputePipelineHandle debugPipeline;
    nvrhi::BindingLayoutHandle debugLayout;
    nvrhi::ComputePipelineHandle resolvePipeline;
    nvrhi::BindingLayoutHandle resolveLayout;
    nvrhi::ComputePipelineHandle allocPipeline;
    nvrhi::BindingLayoutHandle allocLayout;
    nvrhi::ComputePipelineHandle dynBinPipeline;
    nvrhi::BindingLayoutHandle dynBinLayout;
    nvrhi::ComputePipelineHandle dynArgsPipeline;
    nvrhi::BindingLayoutHandle dynArgsLayout;
    nvrhi::ComputePipelineHandle touchPipeline;
    nvrhi::BindingLayoutHandle touchLayout;
    nvrhi::GraphicsPipelineHandle dynClearPipeline;
    nvrhi::BindingLayoutHandle dynClearLayout;
    nvrhi::ShaderHandle dynClearVS;
    nvrhi::GraphicsPipelineHandle dynPagePipeline;
    nvrhi::GraphicsPipelineHandle dynPageATPipeline;
    nvrhi::GraphicsPipelineHandle dynSkinPagePipeline;
    nvrhi::BindingLayoutHandle dynPageLayout;
    nvrhi::BindingLayoutHandle dynPageATLayout;
    nvrhi::BindingLayoutHandle dynSkinPageLayout;
    nvrhi::ShaderHandle dynPageVS;
    nvrhi::ShaderHandle dynSkinPageVS;
    bool dynPipelinesReady = false;
    bool dynPipelinesFailed = false;
    nvrhi::GraphicsPipelineHandle clearPipeline;
    nvrhi::BindingLayoutHandle clearLayout;
    nvrhi::ComputePipelineHandle binPrepPipeline;
    nvrhi::BindingLayoutHandle binPrepLayout;
    nvrhi::ComputePipelineHandle binCountPipeline;
    nvrhi::BindingLayoutHandle binCountLayout;
    nvrhi::ComputePipelineHandle binReservePipeline;
    nvrhi::BindingLayoutHandle binReserveLayout;
    nvrhi::ComputePipelineHandle binPipeline;
    nvrhi::BindingLayoutHandle binLayout;
    nvrhi::ComputePipelineHandle argsPipeline;
    nvrhi::BindingLayoutHandle argsLayout;
    nvrhi::GraphicsPipelineHandle pagePipeline;
    nvrhi::GraphicsPipelineHandle pageATPipeline;
    nvrhi::BindingLayoutHandle pageLayout;
    nvrhi::BindingLayoutHandle pageATLayout;
    nvrhi::ShaderHandle pageVS;
    nvrhi::ShaderHandle pagePS;
    nvrhi::ShaderHandle pageATPS;
    bool pipelinesFailed = false;
    u32 lastLogTime = 0;
};

struct VSMDrawConfig {
    nvrhi::IBuffer* entryBuffer = nullptr;
    u32 entryCount = 0;
    nvrhi::IBuffer* bvhNodeBuffer = nullptr;
    nvrhi::IBuffer* bvhIndexBuffer = nullptr;
    u32 bvhNodeCount = 0;
    u32 minimumPairCapacity[kVSMStreamCount] = {};
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
};

struct VSMDynConfig {
    GPUCullingManager* gpuCulling = nullptr;
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* dynamicInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
};

struct VSMOutput {
    framegraph::VirtualResourceHandle atlas;
    framegraph::VirtualResourceHandle mask;
    framegraph::VirtualResourceHandle debugView;
    bool active = false;
};

void InvalidateVSMCache(VSMState& state);
bool VSMLoadScreenFrozen();
Fmatrix VSMSunView(const Fvector& sunDir);
Fmatrix VSMSunView(const Fvector& sunDir, const Fvector& eye);
float VSMReceiverExtent();
nvrhi::ITexture* ResolveSunMask(const framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle mask, nvrhi::IDevice* device);
void VSMBeginFrame(VSMState& state, const Fvector& camPos, const Fvector& sunDir);

VSMOutput setupVSMPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle orderAfter,
    const VSMDrawConfig& config,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler);

void setupVSMDynamicPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const VSMDynConfig& config,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler);

framegraph::VirtualResourceHandle setupVSMResolvePasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler,
    framegraph::VirtualResourceHandle* outDebugView);

}
