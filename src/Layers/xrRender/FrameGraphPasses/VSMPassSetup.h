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
constexpr u32 kVSMSkinnedCap = 256;
constexpr u32 kVSMMaxSkinned = 256;
constexpr u32 kVSMSkinnedFormats = 6;
constexpr u32 kVSMPairCapOpaque = 1u << 20;
constexpr u32 kVSMPairCapTerrain = 1u << 19;
constexpr u32 kVSMPairCapAT = 1u << 19;
constexpr u32 kVSMStreamCount = 3;
constexpr float kVSMZNear = -1000.0f;
constexpr float kVSMZFar = 1000.0f;
constexpr float kVSMZSnap = 256.0f;
constexpr u32 kVSMPrimeFrames = 8;
constexpr u32 kVSMPrimeTraceFrames = 400;
constexpr u32 kVSMPrimeTraceQuiet = 10;

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
    s32 pageBasePrev[kVSMLevels][2] = {};
    bool pageBasePrevValid = false;
    Fvector prevSunDir;
    bool prevSunValid = false;
    bool sunMoving = false;
    bool sunDown = false;
    bool nightFrozen = false;
    float zCentre = 0.0f;
    bool zCentreValid = false;
    u32 frame = 0;
    u32 sunEpoch = 0;
    u32 invalidations = 0;
    u32 snapMax = 0;
    float sunStepMax = 0.0f;
    bool physInit = false;
    bool atlasFirst = true;
    bool behindLoadScreen = false;
    u32 primeFrames = 0;
    u32 primeTraceLeft = 0;
    u32 primeTraceIdx = 0;
    u32 primeTraceQuiet = 0;

    nvrhi::BufferHandle needed;
    nvrhi::BufferHandle counter;
    nvrhi::BufferHandle pageTable;
    nvrhi::BufferHandle pageList;
    nvrhi::BufferHandle physTile;
    nvrhi::BufferHandle slotDirty;
    nvrhi::BufferHandle dirtyList;
    nvrhi::BufferHandle drawClear;
    nvrhi::BufferHandle slotEpoch;
    nvrhi::TextureHandle atlas;
    nvrhi::TextureHandle dynAtlas;
    nvrhi::BufferHandle dynPageTable;
    nvrhi::BufferHandle dynPageList;
    nvrhi::BufferHandle dynAllocInfo;
    nvrhi::BufferHandle dynUsed;
    nvrhi::BufferHandle skinPages;
    nvrhi::BufferHandle skinArgs;
    nvrhi::BufferHandle skinStats;
    bool dynActive = false;
    bool dynRendered = false;
    u32 dynCasters = 0;
    u32 dynInstances = 0;
    u32 dynMaxPages = 0;
    framegraph::VirtualResourceHandle fgNeeded;
    framegraph::VirtualResourceHandle fgDirtyList;
    framegraph::VirtualResourceHandle fgAtlas;
    framegraph::VirtualResourceHandle fgDynAtlas;
    framegraph::VirtualResourceHandle fgDynTable;
    framegraph::VirtualResourceHandle fgDynUsed;
    nvrhi::TextureHandle mask[2];
    u32 maskWidth = 0;
    u32 maskHeight = 0;
    u32 maskSlot = 0;
    u32 resolveCount = 0;
    bool maskReady = false;
    Fmatrix prevViewProj;
    Fvector prevCamPos;
    nvrhi::BufferHandle binStats;
    nvrhi::BufferHandle pairs[kVSMStreamCount];
    nvrhi::BufferHandle pageArgs[kVSMStreamCount];
    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;

    u32 markPages = 0;
    u32 levelPages[kVSMLevels] = {};
    u32 dirtyPages = 0;
    u32 wrongPages = 0;
    u32 stalePages = 0;
    u32 staleMaxAge = 0;
    u32 binDraws = 0;
    u32 binInstances = 0;
    u32 binMaxPages = 0;
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
    nvrhi::ComputePipelineHandle skinBinPipeline;
    nvrhi::BindingLayoutHandle skinBinLayout;
    nvrhi::GraphicsPipelineHandle skinPagePipeline;
    nvrhi::BindingLayoutHandle skinPageLayout;
    nvrhi::ShaderHandle skinPageVS;
    bool skinPipelinesReady = false;
    bool skinPipelinesFailed = false;
    nvrhi::GraphicsPipelineHandle clearPipeline;
    nvrhi::BindingLayoutHandle clearLayout;
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
    nvrhi::IBuffer* staticInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    MaterialCache* materialCache = nullptr;
};

struct VSMDynConfig {
    GPUCullingManager* gpuCulling = nullptr;
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
float VSMReceiverExtent();
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
