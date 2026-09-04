#include "stdafx.h"
#include "VSMPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

constexpr float kVSMRejectTol = 0.05f;
constexpr u32 kVSMCounterWords = 16;
constexpr u32 kReadbackBinOffset = kVSMCounterWords;
constexpr u32 kVSMDirtyListWords = kVSMStaticSlots;
constexpr u32 kReadbackDynOffset = kReadbackBinOffset + 8;
constexpr u32 kReadbackWords = kReadbackDynOffset + 16;
constexpr u32 kPairCaps[kVSMStreamCount] = { kVSMPairCapOpaque, kVSMPairCapTerrain, kVSMPairCapAT };
constexpr const char* kStreamNames[kVSMStreamCount] = { "Opaque", "Terrain", "AT" };

struct VsmMarkParams {
    Fmatrix invViewProj;
    Fvector4 screen;
    u32 markStep;
    u32 lodBias;
    u32 pad[2];
    Fvector4 hudScale;
};

struct VsmResidParams {
    s32 pageBase[12];
    u32 frame;
    u32 refreshBudget;
    u32 wrongBudget;
    u32 forceDirty;
    u32 interval[8];
    Fvector4 pivot;
    Fvector4 sun;
    Fvector4 levelOrigin[kVSMLevels];
};

struct VsmBinParams {
    u32 includeAT;
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    s32 winToBucket[12];
    u32 itemBase[8];
};

struct VsmBucketParams {
    u32 entryCount;
    u32 level;
    u32 itemBase;
    u32 itemCap;
    s32 shift[4];
    float errK;
    float pw;
    u32 pad[2];
};

struct VsmBucketScanParams {
    u32 level;
    u32 itemCap;
    u32 pad[2];
};

struct VsmArgsParams {
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    u32 pad;
};

struct VsmDynBinParams {
    u32 entryBase;
    u32 entryCount;
    u32 includeAT;
    u32 capOpaque;
    u32 capAT;
    u32 statsBase;
    u32 pad[2];
};

struct VsmDynArgsParams {
    u32 capOpaque;
    u32 capAT;
    u32 capSkinned;
    u32 pad;
};

struct VsmResolveParams {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fvector4 prevCamPos;
    Fvector4 curCamPos;
    Fvector4 screen;
    Fvector4 params;
    Fvector4 params2;
    Fvector4 params3;
    Fvector4 params4;
    Fvector4 hudScale;
};

struct VSMResolveData {
    VirtualResourceHandle depth;
    VirtualResourceHandle atlas;
    VirtualResourceHandle mask;
    VirtualResourceHandle dynAtlas;
    VirtualResourceHandle dynTable;
    VSMState* state;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VsmDebugParams {
    Fmatrix invViewProj;
    Fvector4 screen;
    u32 mode;
    u32 pad[3];
    Fvector4 hudScale;
};

struct VSMMarkData {
    VirtualResourceHandle depth;
    VirtualResourceHandle order;
    VirtualResourceHandle needed;
    VSMState* state;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMResidData {
    VirtualResourceHandle needed;
    VirtualResourceHandle dirtyList;
    VirtualResourceHandle drawClear;
    VSMState* state;
    fg::RenderDevice* device;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMBinData {
    VirtualResourceHandle dirtyList;
    VirtualResourceHandle pageArgs[kVSMStreamCount];
    VSMState* state;
    fg::RenderDevice* device;
    VSMDrawConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMAtlasData {
    VirtualResourceHandle atlas;
    VirtualResourceHandle dirtyList;
    VirtualResourceHandle drawClear;
    VirtualResourceHandle pageArgs[kVSMStreamCount];
    VSMState* state;
    fg::RenderDevice* device;
    VSMDrawConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMDebugData {
    VirtualResourceHandle depth;
    VirtualResourceHandle dirtyList;
    VirtualResourceHandle mask;
    VirtualResourceHandle output;
    VSMState* state;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
};

struct VSMDynAllocData {
    VirtualResourceHandle needed;
    VirtualResourceHandle dynTable;
    VSMState* state;
    fg::RenderDevice* device;
    VSMDynConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMDynBinData {
    VirtualResourceHandle dynTable;
    VirtualResourceHandle order;
    VirtualResourceHandle dynArgs;
    VSMState* state;
    fg::RenderDevice* device;
    VSMDynConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMDynAtlasData {
    VirtualResourceHandle dynAtlas;
    VirtualResourceHandle dynArgs;
    VSMState* state;
    fg::RenderDevice* device;
    VSMDynConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

nvrhi::BufferHandle MakeUAVBuffer(nvrhi::IDevice* nvDevice, const char* name, u64 bytes, u32 stride, bool raw)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = bytes;
    desc.structStride = raw ? 0 : stride;
    desc.canHaveUAVs = true;
    desc.canHaveRawViews = raw;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return nvDevice->createBuffer(desc);
}

nvrhi::IBuffer* VsmParamsCB(fg::RenderDevice* device)
{
    return GetPassResourceCache().GetOrCreateVolatileCB("VSM", "VsmParams", sizeof(VsmParams), device, 64);
}

bool EnsurePipelines(fg::RenderDevice* device, VSMState& state)
{
    if (state.markPipeline && state.residPipeline && state.debugPipeline && state.clearPipeline
        && state.binPrepPipeline && state.binPipeline && state.argsPipeline && state.pagePipeline && state.pageATPipeline && state.resolvePipeline
        && state.allocPipeline && state.dynBinPipeline && state.dynArgsPipeline && state.touchPipeline
        && state.bucketCountPipeline && state.bucketScanPipeline && state.bucketFillPipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto markResult = shaderLoader->LoadComputeShader("vsm_mark");
    auto residResult = shaderLoader->LoadComputeShader("vsm_resid");
    auto debugResult = shaderLoader->LoadComputeShader("vsm_debug_view");
    auto clearVsResult = shaderLoader->LoadVertexShader("vsm_clear", "main");
    auto clearPsResult = shaderLoader->LoadPixelShader("vsm_clear", "main");
    auto binPrepResult = shaderLoader->LoadComputeShader("vsm_bin_prep");
    auto binResult = shaderLoader->LoadComputeShader("vsm_bin_cluster");
    auto argsResult = shaderLoader->LoadComputeShader("vsm_draw_args");
    auto bucketCountResult = shaderLoader->LoadComputeShader("vsm_bucket_count");
    auto bucketScanResult = shaderLoader->LoadComputeShader("vsm_bucket_scan");
    auto bucketFillResult = shaderLoader->LoadComputeShader("vsm_bucket_fill");
    if (!bucketCountResult.handle || !bucketCountResult.reflection || !bucketScanResult.handle || !bucketScanResult.reflection
        || !bucketFillResult.handle || !bucketFillResult.reflection) {
        Msg("! [VSM] bucket shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    auto pageVsResult = shaderLoader->LoadVertexShader("vsm_page_pull", "main");
    auto pagePsResult = shaderLoader->LoadPixelShader("vsm_page", "main");
    auto pageATPsResult = shaderLoader->LoadPixelShader("vsm_page_at", "main");
    auto resolveResult = shaderLoader->LoadComputeShader("vsm_resolve");
    if (!resolveResult.handle || !resolveResult.reflection) {
        Msg("! [VSM] resolve shader failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    auto allocResult = shaderLoader->LoadComputeShader("vsm_alloc");
    auto dynBinResult = shaderLoader->LoadComputeShader("vsm_bin_dyn");
    auto dynArgsResult = shaderLoader->LoadComputeShader("vsm_dyn_draw_args");
    auto touchResult = shaderLoader->LoadComputeShader("vsm_dyn_touch");
    if (!allocResult.handle || !allocResult.reflection || !dynBinResult.handle || !dynBinResult.reflection
        || !dynArgsResult.handle || !dynArgsResult.reflection || !touchResult.handle || !touchResult.reflection) {
        Msg("! [VSM] dynamic atlas shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    if (!markResult.handle || !markResult.reflection || !residResult.handle || !residResult.reflection
        || !debugResult.handle || !debugResult.reflection || !clearVsResult.handle || !clearVsResult.reflection
        || !clearPsResult.handle || !clearPsResult.reflection || !binResult.handle || !binResult.reflection
        || !binPrepResult.handle || !binPrepResult.reflection
        || !argsResult.handle || !argsResult.reflection || !pageVsResult.handle || !pageVsResult.reflection
        || !pagePsResult.handle || !pagePsResult.reflection || !pageATPsResult.handle || !pageATPsResult.reflection) {
        Msg("! [VSM] shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    state.pageVS = pageVsResult.handle;
    state.pagePS = pagePsResult.handle;
    state.pageATPS = pageATPsResult.handle;

    auto& cache = GetPassResourceCache();
    state.markLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMMark", *markResult.reflection, nvDevice);
    state.residLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMResid", *residResult.reflection, nvDevice);
    state.debugLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDebugView", *debugResult.reflection, nvDevice);
    state.clearLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMClear", *clearVsResult.reflection, *clearPsResult.reflection, nvDevice);
    state.binPrepLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBinPrep", *binPrepResult.reflection, nvDevice);
    state.binLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBin", *binResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMArgs", *argsResult.reflection, nvDevice);
    state.bucketCountLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBucketCount", *bucketCountResult.reflection, nvDevice);
    state.bucketScanLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBucketScan", *bucketScanResult.reflection, nvDevice);
    state.bucketFillLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBucketFill", *bucketFillResult.reflection, nvDevice);
    state.pageLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMPage", *pageVsResult.reflection, *pagePsResult.reflection, nvDevice);
    state.pageATLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMPageAT", *pageVsResult.reflection, *pageATPsResult.reflection, nvDevice);
    state.resolveLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMResolve", *resolveResult.reflection, nvDevice);
    state.allocLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMAlloc", *allocResult.reflection, nvDevice);
    state.dynBinLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynBin", *dynBinResult.reflection, nvDevice);
    state.dynArgsLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynArgs", *dynArgsResult.reflection, nvDevice);
    state.touchLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynTouch", *touchResult.reflection, nvDevice);
    if (!state.markLayout || !state.residLayout || !state.debugLayout || !state.clearLayout
        || !state.binPrepLayout || !state.binLayout || !state.argsLayout || !state.pageLayout || !state.pageATLayout || !state.resolveLayout
        || !state.allocLayout || !state.dynBinLayout || !state.dynArgsLayout || !state.touchLayout
        || !state.bucketCountLayout || !state.bucketScanLayout || !state.bucketFillLayout) {
        state.pipelinesFailed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc markDesc;
    markDesc.CS = markResult.handle;
    markDesc.bindingLayouts = { state.markLayout };
    state.markPipeline = cache.GetOrCreateComputePipeline("VSMMark", markDesc, nvDevice);

    nvrhi::ComputePipelineDesc residDesc;
    residDesc.CS = residResult.handle;
    residDesc.bindingLayouts = { state.residLayout };
    state.residPipeline = cache.GetOrCreateComputePipeline("VSMResid", residDesc, nvDevice);

    nvrhi::ComputePipelineDesc debugDesc;
    debugDesc.CS = debugResult.handle;
    debugDesc.bindingLayouts = { state.debugLayout };
    state.debugPipeline = cache.GetOrCreateComputePipeline("VSMDebugView", debugDesc, nvDevice);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D16;
    nvrhi::GraphicsPipelineDesc clearDesc;
    clearDesc.VS = clearVsResult.handle;
    clearDesc.PS = clearPsResult.handle;
    clearDesc.inputLayout = nullptr;
    clearDesc.bindingLayouts = { state.clearLayout };
    clearDesc.primType = nvrhi::PrimitiveType::TriangleList;
    clearDesc.renderState.depthStencilState.depthTestEnable = true;
    clearDesc.renderState.depthStencilState.depthWriteEnable = true;
    clearDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
    clearDesc.renderState.rasterState.frontCounterClockwise = false;
    clearDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    state.clearPipeline = cache.GetOrCreatePipeline("VSMClear", clearDesc, fbInfo, nvDevice);

    nvrhi::ComputePipelineDesc binPrepDesc;
    binPrepDesc.CS = binPrepResult.handle;
    binPrepDesc.bindingLayouts = { state.binPrepLayout };
    state.binPrepPipeline = cache.GetOrCreateComputePipeline("VSMBinPrep", binPrepDesc, nvDevice);

    nvrhi::ComputePipelineDesc binDesc;
    binDesc.CS = binResult.handle;
    binDesc.bindingLayouts = { state.binLayout };
    state.binPipeline = cache.GetOrCreateComputePipeline("VSMBin", binDesc, nvDevice);

    nvrhi::ComputePipelineDesc touchDesc;
    touchDesc.CS = touchResult.handle;
    touchDesc.bindingLayouts = { state.touchLayout };
    state.touchPipeline = cache.GetOrCreateComputePipeline("VSMDynTouch", touchDesc, nvDevice);

    nvrhi::ComputePipelineDesc bucketCountDesc;
    bucketCountDesc.CS = bucketCountResult.handle;
    bucketCountDesc.bindingLayouts = { state.bucketCountLayout };
    state.bucketCountPipeline = cache.GetOrCreateComputePipeline("VSMBucketCount", bucketCountDesc, nvDevice);

    nvrhi::ComputePipelineDesc bucketScanDesc;
    bucketScanDesc.CS = bucketScanResult.handle;
    bucketScanDesc.bindingLayouts = { state.bucketScanLayout };
    state.bucketScanPipeline = cache.GetOrCreateComputePipeline("VSMBucketScan", bucketScanDesc, nvDevice);

    nvrhi::ComputePipelineDesc bucketFillDesc;
    bucketFillDesc.CS = bucketFillResult.handle;
    bucketFillDesc.bindingLayouts = { state.bucketFillLayout };
    state.bucketFillPipeline = cache.GetOrCreateComputePipeline("VSMBucketFill", bucketFillDesc, nvDevice);

    nvrhi::ComputePipelineDesc argsDesc;
    argsDesc.CS = argsResult.handle;
    argsDesc.bindingLayouts = { state.argsLayout };
    state.argsPipeline = cache.GetOrCreateComputePipeline("VSMArgs", argsDesc, nvDevice);

    nvrhi::ComputePipelineDesc resolveDesc;
    resolveDesc.CS = resolveResult.handle;
    resolveDesc.bindingLayouts = { state.resolveLayout };
    state.resolvePipeline = cache.GetOrCreateComputePipeline("VSMResolve", resolveDesc, nvDevice);

    nvrhi::ComputePipelineDesc allocDesc;
    allocDesc.CS = allocResult.handle;
    allocDesc.bindingLayouts = { state.allocLayout };
    state.allocPipeline = cache.GetOrCreateComputePipeline("VSMAlloc", allocDesc, nvDevice);

    nvrhi::ComputePipelineDesc dynBinDesc;
    dynBinDesc.CS = dynBinResult.handle;
    dynBinDesc.bindingLayouts = { state.dynBinLayout };
    state.dynBinPipeline = cache.GetOrCreateComputePipeline("VSMDynBin", dynBinDesc, nvDevice);

    nvrhi::ComputePipelineDesc dynArgsDesc;
    dynArgsDesc.CS = dynArgsResult.handle;
    dynArgsDesc.bindingLayouts = { state.dynArgsLayout };
    state.dynArgsPipeline = cache.GetOrCreateComputePipeline("VSMDynArgs", dynArgsDesc, nvDevice);

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto makePageDesc = [&](nvrhi::IShader* ps, nvrhi::IBindingLayout* layout, bool withBindless) {
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = state.pageVS;
        desc.PS = ps;
        desc.inputLayout = nullptr;
        if (withBindless && bindlessLayout)
            desc.bindingLayouts = { layout, bindlessLayout };
        else
            desc.bindingLayouts = { layout };
        desc.primType = nvrhi::PrimitiveType::TriangleList;
        desc.renderState.depthStencilState.depthTestEnable = true;
        desc.renderState.depthStencilState.depthWriteEnable = true;
        desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        desc.renderState.rasterState.frontCounterClockwise = false;
        desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
        desc.renderState.rasterState.depthBias = -int(roundf(state.rasterBias));
        desc.renderState.rasterState.slopeScaledDepthBias = -state.rasterSlope;
        desc.renderState.rasterState.depthBiasClamp = 0.0f;
        return desc;
    };
    string128 name;
    xr_sprintf(name, "VSMPage_b%.2f_s%.2f", state.rasterBias, state.rasterSlope);
    state.pagePipeline = cache.GetOrCreatePipeline(name, makePageDesc(state.pagePS, state.pageLayout, false), fbInfo, nvDevice);
    xr_sprintf(name, "VSMPageAT_b%.2f_s%.2f", state.rasterBias, state.rasterSlope);
    state.pageATPipeline = cache.GetOrCreatePipeline(name, makePageDesc(state.pageATPS, state.pageATLayout, true), fbInfo, nvDevice);

    if (!state.markPipeline || !state.residPipeline || !state.debugPipeline || !state.clearPipeline
        || !state.binPrepPipeline || !state.binPipeline || !state.argsPipeline || !state.pagePipeline || !state.pageATPipeline || !state.resolvePipeline
        || !state.allocPipeline || !state.dynBinPipeline || !state.dynArgsPipeline) {
        Msg("! [VSM] pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }
    Msg("* [VSM] pipelines initialized (raster bias %.2f / %.2f)", state.rasterBias, state.rasterSlope);
    return true;
}

bool EnsureResources(nvrhi::IDevice* nvDevice, VSMState& state)
{
    if (state.needed && state.atlas)
        return true;

    state.needed = MakeUAVBuffer(nvDevice, "VSM_Needed", u64(kVSMPageCount) * sizeof(u32), sizeof(u32), false);
    state.pageTable = MakeUAVBuffer(nvDevice, "VSM_PageTable", u64(kVSMPageCount) * sizeof(u32), sizeof(u32), false);
    state.pageList = MakeUAVBuffer(nvDevice, "VSM_PageList", u64(kVSMStaticSlots) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.physTile = MakeUAVBuffer(nvDevice, "VSM_PhysTile", u64(kVSMStaticSlots) * sizeof(u32) * 2, sizeof(u32) * 2, false);
    state.slotDirty = MakeUAVBuffer(nvDevice, "VSM_SlotDirty", u64(kVSMStaticSlots) * sizeof(u32), sizeof(u32), false);
    state.dirtyList = MakeUAVBuffer(nvDevice, "VSM_DirtyList", u64(kVSMDirtyListWords) * sizeof(u32), sizeof(u32), false);
    state.slotFrame = MakeUAVBuffer(nvDevice, "VSM_SlotFrame", u64(kVSMStaticSlots) * sizeof(u32), sizeof(u32), false);
    state.slotPivot = MakeUAVBuffer(nvDevice, "VSM_SlotPivot", u64(kVSMStaticSlots) * sizeof(float) * 4, sizeof(float) * 4, false);
    state.slotSun = MakeUAVBuffer(nvDevice, "VSM_SlotSun", u64(kVSMStaticSlots) * sizeof(float) * 4, sizeof(float) * 4, false);
    state.dynPageTable = MakeUAVBuffer(nvDevice, "VSM_DynPageTable", u64(kVSMPageCount) * sizeof(u32), sizeof(u32), false);
    state.dynPageList = MakeUAVBuffer(nvDevice, "VSM_DynPageList", u64(kVSMMaxPhys) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.dynAllocInfo = MakeUAVBuffer(nvDevice, "VSM_DynAllocInfo", sizeof(u32) * 8, sizeof(u32), false);
    state.dynStats = MakeUAVBuffer(nvDevice, "VSM_DynStats", sizeof(u32) * 16, sizeof(u32), false);
    {
        static const u32 kDynCaps[kVSMDynStreamCount] = { kVSMDynPairCapOpaque, kVSMDynPairCapAT, kVSMDynPairCapSkinned };
        static const char* kDynNames[kVSMDynStreamCount] = { "Opaque", "AT", "Skinned" };
        for (u32 i = 0; i < kVSMDynStreamCount; ++i) {
            string64 nm;
            xr_sprintf(nm, "VSM_DynPairs%s", kDynNames[i]);
            state.dynPairs[i] = MakeUAVBuffer(nvDevice, nm, u64(kDynCaps[i]) * sizeof(u32) * 2, sizeof(u32) * 2, false);
            nvrhi::BufferDesc desc;
            xr_sprintf(nm, "VSM_DynArgs%s", kDynNames[i]);
            desc.debugName = nm;
            desc.byteSize = sizeof(u32) * 4;
            desc.canHaveUAVs = true;
            desc.canHaveRawViews = true;
            desc.isDrawIndirectArgs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            state.dynArgs[i] = nvDevice->createBuffer(desc);
            if (!state.dynPairs[i] || !state.dynArgs[i]) {
                Msg("! [VSM] dynamic pair arena creation failed");
                state.needed = nullptr;
                return false;
            }
        }
    }
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_Counters";
        desc.byteSize = sizeof(u32) * kVSMCounterWords;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.drawClear = nvDevice->createBuffer(desc);
        desc.debugName = "VSM_DynClearArgs";
        desc.byteSize = sizeof(u32) * 4;
        state.dynClearArgs = nvDevice->createBuffer(desc);
    }
    state.binStats = MakeUAVBuffer(nvDevice, "VSM_BinStats", sizeof(u32) * 8, sizeof(u32), false);
    state.bucketCount = MakeUAVBuffer(nvDevice, "VSM_BucketCount", u64(kVSMLevels) * kVSMBucketsPerLevel * sizeof(u32), sizeof(u32), false);
    state.bucketStart = MakeUAVBuffer(nvDevice, "VSM_BucketStart", u64(kVSMLevels) * kVSMBucketsPerLevel * sizeof(u32), sizeof(u32), false);
    state.bucketEnd = MakeUAVBuffer(nvDevice, "VSM_BucketEnd", u64(kVSMLevels) * kVSMBucketsPerLevel * sizeof(u32), sizeof(u32), false);
    state.bucketCursor = MakeUAVBuffer(nvDevice, "VSM_BucketCursor", u64(kVSMLevels) * kVSMBucketsPerLevel * sizeof(u32), sizeof(u32), false);
    state.bucketItems = MakeUAVBuffer(nvDevice, "VSM_BucketItems", u64(kVSMBucketItemTotal) * sizeof(u32), sizeof(u32), false);
    state.bucketInit = false;
    for (u32 L = 0; L < kVSMLevels; ++L)
        state.bucketValid[L] = false;
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_BinArgs";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.binArgs = nvDevice->createBuffer(desc);
    }
    for (u32 i = 0; i < kVSMStreamCount; ++i) {
        string64 nm;
        xr_sprintf(nm, "VSM_Pairs%s", kStreamNames[i]);
        state.pairs[i] = MakeUAVBuffer(nvDevice, nm, u64(kPairCaps[i]) * sizeof(u32) * 2, sizeof(u32) * 2, false);
        nvrhi::BufferDesc desc;
        xr_sprintf(nm, "VSM_PageArgs%s", kStreamNames[i]);
        desc.debugName = nm;
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.pageArgs[i] = nvDevice->createBuffer(desc);
        if (!state.pairs[i] || !state.pageArgs[i]) {
            Msg("! [VSM] pair arena creation failed");
            state.needed = nullptr;
            return false;
        }
    }
    {
        nvrhi::TextureDesc desc;
        desc.width = kVSMAtlasW * kVSMPageSize;
        desc.height = kVSMAtlasH * kVSMPageSize;
        desc.format = nvrhi::Format::D16;
        desc.debugName = "VSM_Atlas";
        desc.isShaderResource = true;
        desc.isRenderTarget = true;
        desc.isTypeless = true;
        desc.useClearValue = true;
        desc.clearValue = nvrhi::Color(0.0f);
        desc.initialState = nvrhi::ResourceStates::DepthWrite;
        desc.keepInitialState = true;
        state.atlas = nvDevice->createTexture(desc);
    }
    {
        nvrhi::TextureDesc desc;
        desc.width = kVSMAtlasWDyn * kVSMPageSize;
        desc.height = kVSMAtlasHDyn * kVSMPageSize;
        desc.format = nvrhi::Format::D16;
        desc.debugName = "VSM_AtlasDyn";
        desc.isShaderResource = true;
        desc.isRenderTarget = true;
        desc.isTypeless = true;
        desc.useClearValue = true;
        desc.clearValue = nvrhi::Color(0.0f);
        desc.initialState = nvrhi::ResourceStates::DepthWrite;
        desc.keepInitialState = true;
        state.dynAtlas = nvDevice->createTexture(desc);
    }
    for (u32 i = 0; i < VSMState::kReadbackSlots; ++i) {
        if (state.readback[i])
            continue;
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_Readback";
        desc.byteSize = u64(kReadbackWords) * sizeof(u32);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        state.readback[i] = nvDevice->createBuffer(desc);
    }
    state.readbackWrite = 0;
    state.readbackScheduled = 0;
    state.physInit = false;
    state.atlasFirst = true;
    state.primeFrames = kVSMPrimeFrames;
    state.primeTraceLeft = kVSMPrimeTraceFrames;
    state.primeTraceIdx = 0;
    state.primeTraceQuiet = 0;

    if (!state.needed || !state.dynClearArgs || !state.pageTable || !state.pageList || !state.physTile
        || !state.slotDirty || !state.dirtyList || !state.drawClear || !state.slotFrame || !state.slotPivot || !state.slotSun || !state.atlas || !state.binStats
        || !state.bucketCount || !state.bucketStart || !state.bucketEnd || !state.bucketCursor || !state.bucketItems || !state.binArgs
        || !state.dynPageTable || !state.dynPageList || !state.dynAllocInfo || !state.dynAtlas || !state.dynStats) {
        Msg("! [VSM] resource creation failed");
        state.needed = nullptr;
        state.atlas = nullptr;
        return false;
    }
    Msg("* [VSM] static atlas %ux%u D16, %u toroidal slots", kVSMAtlasW * kVSMPageSize, kVSMAtlasH * kVSMPageSize, kVSMStaticSlots);
    return true;
}

bool EnsureMaskTargets(nvrhi::IDevice* nvDevice, VSMState& state, u32 width, u32 height)
{
    if (state.mask[0] && state.mask[1] && state.maskWidth == width && state.maskHeight == height)
        return true;
    for (u32 i = 0; i < 2; ++i) {
        nvrhi::TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.debugName = i == 0 ? "VSM_Mask0" : "VSM_Mask1";
        desc.isShaderResource = true;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.mask[i] = nvDevice->createTexture(desc);
    }
    state.maskWidth = width;
    state.maskHeight = height;
    state.maskSlot = 0;
    state.resolveCount = 0;
    state.maskReady = false;
    if (!state.mask[0] || !state.mask[1]) {
        Msg("! [VSM] mask creation failed (%ux%u)", width, height);
        return false;
    }
    Msg("* [VSM] mask targets %ux%u RGBA16F x2", width, height);
    return true;
}

void ProcessReadback(nvrhi::IDevice* nvDevice, VSMState& state)
{
    if (state.readbackScheduled < VSMState::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    void* mapped = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* words = static_cast<const u32*>(mapped);
    u32 total = 0;
    for (u32 L = 0; L < kVSMLevels; ++L) {
        state.levelPages[L] = std::min(words[8 + L], kVSMPagesPerLevel);
        total += state.levelPages[L];
    }
    state.markPages = total;
    state.dirtyPages = std::min(words[1], kVSMStaticSlots);
    state.wrongPages = std::min(words[4], kVSMStaticSlots);
    state.refreshPages = std::min(words[5], kVSMStaticSlots);
    state.overduePages = std::min(words[6], kVSMStaticSlots);
    const u32* bin = words + kReadbackBinOffset;
    state.binDraws = bin[0];
    state.binInstances = bin[1];
    state.binMaxPages = bin[2];
    state.binLodCulled = bin[3];
    state.binDrops = bin[7];
    const u32* dyn = words + kReadbackDynOffset;
    state.dynCasters = dyn[0] + dyn[8];
    state.dynInstances = dyn[1] + dyn[9];
    state.dynMaxPages = std::max(dyn[2], dyn[10]);
    nvDevice->unmapBuffer(oldest);
}

void ScheduleReadback(nvrhi::ICommandList* cmdList, VSMState& state)
{
    nvrhi::IBuffer* slot = state.readback[state.readbackWrite];
    if (!slot)
        return;
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.dynStats, nvrhi::ResourceStates::CopySource);
    cmdList->copyBuffer(slot, 0, state.drawClear, 0, sizeof(u32) * kVSMCounterWords);
    cmdList->copyBuffer(slot, u64(kReadbackBinOffset) * sizeof(u32), state.binStats, 0, sizeof(u32) * 8);
    cmdList->copyBuffer(slot, u64(kReadbackDynOffset) * sizeof(u32), state.dynStats, 0, sizeof(u32) * 16);
    state.readbackWrite = (state.readbackWrite + 1) % VSMState::kReadbackSlots;
    if (state.readbackScheduled < VSMState::kReadbackSlots)
        ++state.readbackScheduled;
}

void LogPrimeTrace(VSMState& state)
{
    if (state.primeTraceLeft == 0 || state.readbackScheduled < VSMState::kReadbackSlots)
        return;
    --state.primeTraceLeft;
    const bool onScreen = Device.dwPrecacheFrame == 0;
    Msg("[VSM prime] f=%u primeLeft=%u wrong=%u dirty=%u budget=%d loadscreen=%d",
        state.primeTraceIdx++, state.primeFrames, state.wrongPages, state.dirtyPages, ps_r_vsm_dirty_budget, onScreen ? 0 : 1);
    state.primeTraceQuiet = (onScreen && state.dirtyPages == 0) ? state.primeTraceQuiet + 1 : 0;
    if (state.primeTraceQuiet >= kVSMPrimeTraceQuiet) {
        Msg("[VSM prime] drained: world visible and dirty==0 by frame %u", state.primeTraceIdx);
        state.primeTraceLeft = 0;
    }
}

void LogTelemetry(VSMState& state)
{
    if (ps_r_vsm_debug < 1)
        return;
    LogPrimeTrace(state);
    if (Device.dwTimeGlobal - state.lastLogTime < 2000)
        return;
    state.lastLogTime = Device.dwTimeGlobal;
    Msg("[VSM] mark: pages=%u | L0=%u L1=%u L2=%u L3=%u L4=%u L5=%u | sun %s rate %.2e rad/f step max %.3f deg | pivot (%.0f %.0f %.0f) relabels %u | base %.1f m k %.2f | inval %u boltHeld %u",
        state.markPages, state.levelPages[0], state.levelPages[1], state.levelPages[2],
        state.levelPages[3], state.levelPages[4], state.levelPages[5],
        state.sunMoving ? "moving" : "static", state.sunRate, state.sunStepMax,
        state.pivot.x, state.pivot.y, state.pivot.z, state.relabels,
        ps_r_vsm_base, ps_r_vsm_cluster_lod, state.invalidations, state.boltHeld);
    state.boltHeld = 0;
    state.relabels = 0;
    Msg("[VSM] static: dirty=%u/%u rendered | wrong=%u refresh=%u overdue=%u | wrong budget=%d refresh budget=%u (floor %d, stretch %.1f, lod bias %u) prime=%u | interval L0=%u L1=%u L2=%u L3=%u L4=%u L5=%u | cache %s",
        state.dirtyPages, state.markPages, state.wrongPages, state.refreshPages, state.overduePages, ps_r_vsm_dirty_budget, state.refreshBudget, ps_r_vsm_refresh_budget, state.refreshStretch, state.lodBias, state.primeFrames,
        state.refreshInterval[0], state.refreshInterval[1], state.refreshInterval[2], state.refreshInterval[3], state.refreshInterval[4], state.refreshInterval[5],
        ps_r_vsm_cache ? "on" : "off");
    Msg("[VSM] bin: pages=%u pairs=%u maxBucket=%u drops=%u rebuilds=%u | k=%.2f at=%d",
        state.binDraws, state.binInstances, state.binMaxPages, state.binDrops, state.bucketRebuilds, ps_r_vsm_cluster_lod, ps_r_vsm_at);
    state.bucketRebuilds = 0;
    Msg("[VSM] dyn: casters=%u instances=%u maxPages=%u | blend_dyn %.2f",
        state.dynCasters, state.dynInstances, state.dynMaxPages, ps_r_vsm_ta_blend_dyn);
    state.sunStepMax = 0.0f;
}

void ExecuteMark(fg::RenderContext* ctx, const FrameGraph& fg, const VSMMarkData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
    if (!cmdList || !nvDevice || !depth)
        return;
    if (!EnsurePipelines(data.device, state))
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* markRefl = shaderLoader->GetCachedReflection("vsm_mark", ".cs");
    if (!markRefl)
        return;

    auto vsmCB = VsmParamsCB(data.device);
    cmdList->writeBuffer(vsmCB, &state.params, sizeof(VsmParams));

    const u32 markStep = ps_r_vsm_mark_half ? 2u : 1u;
    VsmMarkParams mp = {};
    mp.invViewProj = Device.mInvFullTransform;
    mp.hudScale.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, psHUD_FOV);
    mp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    mp.markStep = markStep;
    mp.lodBias = state.lodBias;
    auto markCB = cache.GetOrCreateVolatileCB("VSM", "MarkParams", sizeof(VsmMarkParams), data.device);
    cmdList->writeBuffer(markCB, &mp, sizeof(mp));

    const bool dyn = state.dynActive;
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::CopyDest);
    if (dyn) {
        cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.dynAllocInfo, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.dynStats, nvrhi::ResourceStates::CopyDest);
    }
    if (!state.physInit) {
        cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.slotFrame, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.slotPivot, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.slotSun, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::CopyDest);
    }
    cmdList->clearBufferUInt(state.needed, 0);
    cmdList->clearBufferUInt(state.drawClear, 0);
    cmdList->clearBufferUInt(state.binStats, 0);
    if (dyn) {
        cmdList->clearBufferUInt(state.dynPageTable, 0xFFFFFFFFu);
        cmdList->clearBufferUInt(state.dynAllocInfo, 0);
        cmdList->clearBufferUInt(state.dynStats, 0);
    }
    if (!state.physInit) {
        cmdList->clearBufferUInt(state.physTile, 0xFFFFFFFFu);
        cmdList->clearBufferUInt(state.slotFrame, 0);
        cmdList->clearBufferUInt(state.slotPivot, 0);
        cmdList->clearBufferUInt(state.slotSun, 0);
        cmdList->clearBufferUInt(state.slotDirty, 0);
        cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.slotFrame, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.slotPivot, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.slotSun, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::UnorderedAccess);
        state.physInit = true;
    }
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::UnorderedAccess);
    if (dyn) {
        cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.dynAllocInfo, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.dynStats, nvrhi::ResourceStates::UnorderedAccess);
    }

    BindingSetBuilder bsb(*markRefl, nvDevice, "VSM.Mark");
    bsb.ConstantBuffer("VsmParams", vsmCB)
       .ConstantBuffer("VsmMarkParams", markCB)
       .Texture("g_Depth", depth)
       .BufferUAV("g_Needed", state.needed);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.markLayout, nvDevice);
    if (!bindingSet)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.markPipeline;
    cs.bindings = { bindingSet };
    cmdList->setComputeState(cs);
    const u32 mw = (data.width + markStep - 1) / markStep;
    const u32 mh = (data.height + markStep - 1) / markStep;
    cmdList->dispatch((mw + 7) / 8, (mh + 7) / 8, 1);
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::ShaderResource);
}

void ExecuteResid(fg::RenderContext* ctx, const VSMResidData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.residPipeline)
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_resid", ".cs");
    if (!refl)
        return;

    VsmResidParams rp = {};
    float demand = 0.0f;
    for (u32 L = 0; L < kVSMLevels; ++L)
        demand += float(state.levelPages[L]) / float(std::max(state.refreshInterval[L], 1u));
    const u32 floorBudget = u32(std::max(ps_r_vsm_refresh_budget, 0));
    state.refreshBudget = std::min(std::max(u32(ceilf(demand * 1.25f)) + 2u, floorBudget), kVSMRefreshBudgetMax);
    const float unbiased = demand * float(1u << (2u * state.lodBias)) / float(kVSMRefreshBudgetMax);
    u32 target = 0;
    while (target < kVSMLodBiasMax && unbiased > float(1u << (2u * target)))
        ++target;
    if (state.lodBiasHold > 0) {
        --state.lodBiasHold;
    } else if (target > state.lodBias) {
        state.lodBias = target;
        state.lodBiasHold = kVSMLodBiasHold;
    } else if (target < state.lodBias && unbiased * 2.0f <= float(1u << (2u * (state.lodBias - 1u)))) {
        --state.lodBias;
        state.lodBiasHold = kVSMLodBiasHold;
    }
    state.refreshStretch = std::max(unbiased / float(1u << (2u * state.lodBias)), 1.0f);
    for (u32 L = 0; L < kVSMLevels; ++L) {
        rp.pageBase[2 * L] = state.pageBase[L][0] + state.tileBias[L][0];
        rp.pageBase[2 * L + 1] = state.pageBase[L][1] + state.tileBias[L][1];
        const float stretched = float(state.refreshInterval[L]) * state.refreshStretch;
        rp.interval[L] = stretched >= float(kVSMRefreshIntervalMax) ? kVSMRefreshIntervalMax : u32(stretched);
    }
    rp.frame = state.frame;
    rp.refreshBudget = state.refreshBudget;
    rp.forceDirty = ps_r_vsm_cache ? 0u : 1u;
    const int wrongBudget = state.primeFrames > 0 ? 0 : std::max(ps_r_vsm_dirty_budget, 0);
    if (state.primeFrames > 0)
        --state.primeFrames;
    rp.wrongBudget = u32(wrongBudget);
    rp.pivot.set(state.pivot.x, state.pivot.y, state.pivot.z, 0.0f);
    rp.sun.set(state.sunDir.x, state.sunDir.y, state.sunDir.z, 0.0f);
    for (u32 L = 0; L < kVSMLevels; ++L)
        rp.levelOrigin[L].set(state.params.level[L].x, state.params.level[L].y, state.params.level[L].z / float(kVSMPagesAxis), 0.0f);
    auto residCB = cache.GetOrCreateVolatileCB("VSM", "ResidParams", sizeof(VsmResidParams), data.device);
    cmdList->writeBuffer(residCB, &rp, sizeof(rp));

    cmdList->setBufferState(state.pageTable, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.pageList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotFrame, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotPivot, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotSun, nvrhi::ResourceStates::UnorderedAccess);

    BindingSetBuilder bsb(*refl, nvDevice, "VSM.Resid");
    bsb.ConstantBuffer("VsmResidParams", residCB)
       .BufferSRV("g_Needed", state.needed)
       .BufferUAV("g_PageTable", state.pageTable)
       .BufferUAV("g_PageList", state.pageList)
       .BufferUAV("g_PhysTile", state.physTile)
       .BufferUAV("g_SlotDirty", state.slotDirty)
       .BufferUAV("g_DirtyList", state.dirtyList)
       .BufferUAV("g_Counters", state.drawClear)
       .BufferUAV("g_SlotFrame", state.slotFrame)
       .BufferUAV("g_SlotPivot", state.slotPivot)
       .BufferUAV("g_SlotSun", state.slotSun);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.residLayout, nvDevice);
    if (!bindingSet)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.residPipeline;
    cs.bindings = { bindingSet };
    cmdList->setComputeState(cs);
    cmdList->dispatch((kVSMPageCount + 63) / 64, 1, 1);

    cmdList->setBufferState(state.pageTable, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.pageList, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.slotPivot, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.slotSun, nvrhi::ResourceStates::ShaderResource);
}

void ExecuteBin(fg::RenderContext* ctx, const VSMBinData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.binPrepPipeline || !state.binPipeline || !state.argsPipeline)
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* prepRefl = shaderLoader->GetCachedReflection("vsm_bin_prep", ".cs");
    auto* binRefl = shaderLoader->GetCachedReflection("vsm_bin_cluster", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("vsm_draw_args", ".cs");
    if (!prepRefl || !binRefl || !argsRefl)
        return;

    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::UnorderedAccess);
    for (u32 i = 0; i < kVSMStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::UnorderedAccess);

    const bool haveEntries = data.config.entryBuffer && data.config.entryCount > 0;
    if (haveEntries) {
        auto vsmCB = VsmParamsCB(data.device);
        if (!state.bucketInit) {
            cmdList->setBufferState(state.bucketCount, nvrhi::ResourceStates::CopyDest);
            cmdList->setBufferState(state.bucketStart, nvrhi::ResourceStates::CopyDest);
            cmdList->setBufferState(state.bucketEnd, nvrhi::ResourceStates::CopyDest);
            cmdList->setBufferState(state.bucketCursor, nvrhi::ResourceStates::CopyDest);
            cmdList->clearBufferUInt(state.bucketCount, 0);
            cmdList->clearBufferUInt(state.bucketStart, 0);
            cmdList->clearBufferUInt(state.bucketEnd, 0);
            cmdList->clearBufferUInt(state.bucketCursor, 0);
            state.bucketInit = true;
        }

        const float errK = std::max(0.1f, ps_r_vsm_cluster_lod);
        const bool entriesChanged = state.bucketEntryCount != data.config.entryCount || state.bucketErrK != errK;
        state.bucketEntryCount = data.config.entryCount;
        state.bucketErrK = errK;
        s32 stableBase[kVSMLevels][2];
        u32 itemBase[kVSMLevels];
        u32 acc = 0;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            stableBase[L][0] = state.pageBase[L][0] + state.tileBias[L][0];
            stableBase[L][1] = state.pageBase[L][1] + state.tileBias[L][1];
            itemBase[L] = acc;
            acc += kVSMBucketItemCap[L];
        }

        bool anyRebuild = false;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            const float ext = ps_r_vsm_base * float(1u << L);
            const float pw = ext / float(kVSMPagesAxis);
            bool rebuild = entriesChanged || !state.bucketValid[L];
            if (!rebuild) {
                const s32 offX = stableBase[L][0] - state.bucketBase[L][0];
                const s32 offY = stableBase[L][1] - state.bucketBase[L][1];
                const s32 slackMax = s32(kVSMBucketAxis - kVSMPagesAxis) - 2;
                if (offX < 2 || offY < 2 || offX > slackMax || offY > slackMax)
                    rebuild = true;
                Fvector cr;
                cr.crossproduct(state.sunDir, state.bucketSun[L]);
                if (cr.magnitude() * kVSMZFar > 0.5f * pw)
                    rebuild = true;
            }
            if (!rebuild)
                continue;
            anyRebuild = true;
            state.bucketRebuilds++;
            state.bucketValid[L] = true;
            state.bucketBase[L][0] = stableBase[L][0] - s32(kVSMBucketMargin);
            state.bucketBase[L][1] = stableBase[L][1] - s32(kVSMBucketMargin);
            state.bucketSun[L] = state.sunDir;

            VsmBucketParams bp = {};
            bp.entryCount = data.config.entryCount;
            bp.level = L;
            bp.itemBase = itemBase[L];
            bp.itemCap = kVSMBucketItemCap[L];
            bp.shift[0] = state.tileBias[L][0] - state.bucketBase[L][0];
            bp.shift[1] = state.tileBias[L][1] - state.bucketBase[L][1];
            bp.errK = errK;
            bp.pw = pw;
            auto bucketCB = cache.GetOrCreateVolatileCB("VSM", "BucketParams", sizeof(VsmBucketParams), data.device, 16);
            cmdList->writeBuffer(bucketCB, &bp, sizeof(bp));
            VsmBucketScanParams sp = {};
            sp.level = L;
            sp.itemCap = kVSMBucketItemCap[L];
            auto scanCB = cache.GetOrCreateVolatileCB("VSM", "BucketScanParams", sizeof(VsmBucketScanParams), data.device, 16);
            cmdList->writeBuffer(scanCB, &sp, sizeof(sp));

            cmdList->setBufferState(state.bucketCount, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(state.bucketStart, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(state.bucketEnd, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(state.bucketCursor, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(state.bucketItems, nvrhi::ResourceStates::UnorderedAccess);
            const u32 groups = (data.config.entryCount + 63) / 64;
            {
                auto* refl = shaderLoader->GetCachedReflection("vsm_bucket_count", ".cs");
                if (!refl)
                    return;
                BindingSetBuilder cbs(*refl, nvDevice, "VSM.BucketCount");
                cbs.ConstantBuffer("VsmParams", vsmCB)
                   .ConstantBuffer("VsmBucketParams", bucketCB)
                   .BufferSRV("g_Entries", data.config.entryBuffer)
                   .BufferUAV("g_BucketCount", state.bucketCount);
                auto set = cache.GetOrCreateBindingSet(cbs.Build(), state.bucketCountLayout, nvDevice);
                if (!set)
                    return;
                nvrhi::ComputeState cs;
                cs.pipeline = state.bucketCountPipeline;
                cs.bindings = { set };
                cmdList->setComputeState(cs);
                cmdList->dispatch(groups, 1, 1);
            }
            {
                auto* refl = shaderLoader->GetCachedReflection("vsm_bucket_scan", ".cs");
                if (!refl)
                    return;
                BindingSetBuilder sbs(*refl, nvDevice, "VSM.BucketScan");
                sbs.ConstantBuffer("VsmBucketScanParams", scanCB)
                   .BufferUAV("g_BucketCount", state.bucketCount)
                   .BufferUAV("g_BucketStart", state.bucketStart)
                   .BufferUAV("g_BucketEnd", state.bucketEnd)
                   .BufferUAV("g_BucketCursor", state.bucketCursor);
                auto set = cache.GetOrCreateBindingSet(sbs.Build(), state.bucketScanLayout, nvDevice);
                if (!set)
                    return;
                nvrhi::ComputeState cs;
                cs.pipeline = state.bucketScanPipeline;
                cs.bindings = { set };
                cmdList->setComputeState(cs);
                cmdList->dispatch(1, 1, 1);
            }
            {
                auto* refl = shaderLoader->GetCachedReflection("vsm_bucket_fill", ".cs");
                if (!refl)
                    return;
                BindingSetBuilder fbs(*refl, nvDevice, "VSM.BucketFill");
                fbs.ConstantBuffer("VsmParams", vsmCB)
                   .ConstantBuffer("VsmBucketParams", bucketCB)
                   .BufferSRV("g_Entries", data.config.entryBuffer)
                   .BufferUAV("g_BucketCursor", state.bucketCursor)
                   .BufferUAV("g_Items", state.bucketItems)
                   .BufferUAV("g_Stats", state.binStats);
                auto set = cache.GetOrCreateBindingSet(fbs.Build(), state.bucketFillLayout, nvDevice);
                if (!set)
                    return;
                nvrhi::ComputeState cs;
                cs.pipeline = state.bucketFillPipeline;
                cs.bindings = { set };
                cmdList->setComputeState(cs);
                cmdList->dispatch(groups, 1, 1);
            }
        }
        (void)anyRebuild;

        cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.pageList, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.binArgs, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(state.bucketStart, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.bucketEnd, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.bucketItems, nvrhi::ResourceStates::ShaderResource);

        BindingSetBuilder pbs(*prepRefl, nvDevice, "VSM.BinPrep");
        pbs.BufferSRV("g_Counters", state.drawClear)
           .BufferUAV("g_BinArgs", state.binArgs);
        auto prepSet = cache.GetOrCreateBindingSet(pbs.Build(), state.binPrepLayout, nvDevice);
        if (prepSet) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binPrepPipeline;
            cs.bindings = { prepSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch(1, 1, 1);
        }
        cmdList->setBufferState(state.binArgs, nvrhi::ResourceStates::IndirectArgument);

        VsmBinParams bp = {};
        bp.includeAT = ps_r_vsm_at ? 1u : 0u;
        bp.capOpaque = kVSMPairCapOpaque;
        bp.capTerrain = kVSMPairCapTerrain;
        bp.capAT = kVSMPairCapAT;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            bp.winToBucket[2 * L] = stableBase[L][0] - state.bucketBase[L][0];
            bp.winToBucket[2 * L + 1] = stableBase[L][1] - state.bucketBase[L][1];
            bp.itemBase[L] = itemBase[L];
        }
        auto binCB = cache.GetOrCreateVolatileCB("VSM", "BinParams", sizeof(VsmBinParams), data.device);
        cmdList->writeBuffer(binCB, &bp, sizeof(bp));

        BindingSetBuilder bsb(*binRefl, nvDevice, "VSM.Bin");
        bsb.ConstantBuffer("VsmParams", vsmCB)
           .ConstantBuffer("VsmBinParams", binCB)
           .BufferSRV("g_Entries", data.config.entryBuffer)
           .BufferSRV("g_DirtyList", state.dirtyList)
           .BufferSRV("g_PageList", state.pageList)
           .BufferSRV("g_BucketStart", state.bucketStart)
           .BufferSRV("g_BucketEnd", state.bucketEnd)
           .BufferSRV("g_Items", state.bucketItems)
           .BufferUAV("g_Stats", state.binStats)
           .BufferUAV("g_PairsOpaque", state.pairs[0])
           .BufferUAV("g_PairsTerrain", state.pairs[1])
           .BufferUAV("g_PairsAT", state.pairs[2]);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.binLayout, nvDevice);
        if (bindingSet) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binPipeline;
            cs.bindings = { bindingSet };
            cs.indirectParams = state.binArgs;
            cmdList->setComputeState(cs);
            cmdList->dispatchIndirect(0);
        }
    }

    VsmArgsParams ap = {};
    ap.capOpaque = kVSMPairCapOpaque;
    ap.capTerrain = kVSMPairCapTerrain;
    ap.capAT = kVSMPairCapAT;
    auto argsCB = cache.GetOrCreateVolatileCB("VSM", "ArgsParams", sizeof(VsmArgsParams), data.device);
    cmdList->writeBuffer(argsCB, &ap, sizeof(ap));
    for (u32 i = 0; i < kVSMStreamCount; ++i)
        cmdList->setBufferState(state.pageArgs[i], nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::ShaderResource);

    BindingSetBuilder abs(*argsRefl, nvDevice, "VSM.Args");
    abs.ConstantBuffer("VsmArgsParams", argsCB)
       .BufferSRV("g_Stats", state.binStats)
       .BufferUAV("g_ArgsOpaque", state.pageArgs[0])
       .BufferUAV("g_ArgsTerrain", state.pageArgs[1])
       .BufferUAV("g_ArgsAT", state.pageArgs[2]);
    auto argsSet = cache.GetOrCreateBindingSet(abs.Build(), state.argsLayout, nvDevice);
    if (argsSet) {
        nvrhi::ComputeState cs;
        cs.pipeline = state.argsPipeline;
        cs.bindings = { argsSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch(1, 1, 1);
    }

    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::IndirectArgument);
    for (u32 i = 0; i < kVSMStreamCount; ++i) {
        cmdList->setBufferState(state.pageArgs[i], nvrhi::ResourceStates::IndirectArgument);
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::ShaderResource);
    }
}

void DrawPages(fg::RenderContext* ctx, const VSMAtlasData& data, nvrhi::IFramebuffer* framebuffer,
    const nvrhi::Viewport& viewport, const nvrhi::Rect& scissor)
{
    VSMState& state = *data.state;
    const VSMDrawConfig& cfg = data.config;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cfg.entryBuffer || !cfg.megaVertexBuffer || !cfg.megaIndexBuffer || !state.pagePipeline)
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("vsm_page_pull", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_page", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!vsRefl || !psRefl || !atRefl)
        return;

    auto& matBuffer = bindless::MaterialBuffer::Instance();
    auto vsmCB = VsmParamsCB(data.device);
    auto* backend = data.device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

    auto drawStream = [&](u32 stream, nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingLayout* layout,
                          const ExtractedReflection& ps, nvrhi::IBuffer* instanceBuffer, bool withBindless, const char* label) {
        if (!pipeline || !layout || !instanceBuffer)
            return;
        BindingSetBuilder bsb(*vsRefl, ps, nvDevice, label);
        bsb.ConstantBuffer("VsmParams", vsmCB);
        bsb.BufferSRV("g_InstanceData", instanceBuffer);
        bsb.BufferSRV("g_Pairs", state.pairs[stream]);
        bsb.BufferSRV("g_Entries", cfg.entryBuffer);
        bsb.BufferSRV("g_PageList", state.pageList);
        bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        if (withBindless)
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), layout, nvDevice);
        if (!bindingSet)
            return;
        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        if (withBindless && bindlessTable)
            gs.addBindingSet(bindlessTable);
        gs.indirectParams = state.pageArgs[stream];
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(0, 1);
    };

    drawStream(0, state.pagePipeline, state.pageLayout, *psRefl, cfg.staticInstanceBuffer, false, "VSM.PageOpaque");
    drawStream(1, state.pagePipeline, state.pageLayout, *psRefl, cfg.terrainInstanceBuffer, false, "VSM.PageTerrain");
    drawStream(2, state.pageATPipeline, state.pageATLayout, *atRefl, cfg.staticInstanceBuffer, true, "VSM.PageAT");
}

void ExecuteAtlas(fg::RenderContext* ctx, const FrameGraph& fg, const VSMAtlasData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.atlas);
    if (!cmdList || !nvDevice || !atlas || !state.clearPipeline)
        return;

    if (state.atlasFirst) {
        cmdList->clearDepthStencilTexture(atlas, nvrhi::AllSubresources, true, 0.0f, false, 0);
        state.atlasFirst = false;
    }
    if (data.config.materialCache)
        data.config.materialCache->FinalizePendingMaterials(ctx);
    bindless::MaterialBuffer::Instance().Upload(ctx);

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("vsm_clear", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_clear", ".ps");
    if (!vsRefl || !psRefl)
        return;

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(atlas);
    auto framebuffer = cache.GetOrCreateFramebuffer("VSMAtlas", fbDesc, nvDevice);
    if (!framebuffer)
        return;

    BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "VSM.Clear");
    bsb.BufferSRV("g_DirtyList", state.dirtyList);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.clearLayout, nvDevice);
    if (!bindingSet)
        return;

    const auto& rtDesc = atlas->getDesc();
    nvrhi::GraphicsState gs;
    gs.pipeline = state.clearPipeline;
    gs.framebuffer = framebuffer;
    gs.bindings = { bindingSet };
    gs.indirectParams = state.drawClear;
    gs.viewport.addViewport(nvrhi::Viewport(0.0f, float(rtDesc.width), 0.0f, float(rtDesc.height), 0.0f, 1.0f));
    gs.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));
    cmdList->setGraphicsState(gs);
    cmdList->drawIndirect(0, 1);

    DrawPages(ctx, data, framebuffer, nvrhi::Viewport(0.0f, float(rtDesc.width), 0.0f, float(rtDesc.height), 0.0f, 1.0f),
        nvrhi::Rect(rtDesc.width, rtDesc.height));
}

bool EnsureDynPagePipelines(fg::RenderDevice* device, VSMState& state)
{
    if (state.dynPipelinesReady)
        return true;
    if (state.dynPipelinesFailed || !state.pagePS || !state.pageATPS)
        return false;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;
    auto& cache = GetPassResourceCache();
    if (!state.dynPageVS) {
        auto vsResult = shaderLoader->LoadVertexShader("vsm_page_pull_dyn", "main");
        if (vsResult.handle && vsResult.reflection)
            state.dynPageVS = vsResult.handle;
    }
    if (!state.dynSkinPageVS) {
        auto vsResult = shaderLoader->LoadVertexShader("vsm_page_pull_skinned", "main");
        if (vsResult.handle && vsResult.reflection)
            state.dynSkinPageVS = vsResult.handle;
    }
    if (!state.dynClearVS) {
        auto vsResult = shaderLoader->LoadVertexShader("vsm_clear_dyn", "main");
        if (vsResult.handle && vsResult.reflection)
            state.dynClearVS = vsResult.handle;
    }
    auto* dynVsRefl = shaderLoader->GetCachedReflection("vsm_page_pull_dyn", ".vs");
    auto* skinVsRefl = shaderLoader->GetCachedReflection("vsm_page_pull_skinned", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_page", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    auto* clearVsRefl = shaderLoader->GetCachedReflection("vsm_clear_dyn", ".vs");
    auto* clearPsRefl = shaderLoader->GetCachedReflection("vsm_clear", ".ps");
    if (!state.dynPageVS || !state.dynSkinPageVS || !state.dynClearVS || !dynVsRefl || !skinVsRefl || !psRefl || !atRefl || !clearVsRefl || !clearPsRefl) {
        Msg("! [VSM] dynamic page shaders failed to load");
        state.dynPipelinesFailed = true;
        return false;
    }
    state.dynPageLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynPage", *dynVsRefl, *psRefl, nvDevice);
    state.dynPageATLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynPageAT", *dynVsRefl, *atRefl, nvDevice);
    state.dynSkinPageLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynSkinPage", *skinVsRefl, *psRefl, nvDevice);
    state.dynClearLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDynClear", *clearVsRefl, *clearPsRefl, nvDevice);
    if (!state.dynPageLayout || !state.dynPageATLayout || !state.dynSkinPageLayout || !state.dynClearLayout) {
        state.dynPipelinesFailed = true;
        return false;
    }
    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D16;
    auto makeDesc = [&](nvrhi::IShader* vs, nvrhi::IShader* ps, nvrhi::IBindingLayout* layout, bool withBindless) {
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = vs;
        desc.PS = ps;
        desc.inputLayout = nullptr;
        if (withBindless && bindlessLayout)
            desc.bindingLayouts = { layout, bindlessLayout };
        else
            desc.bindingLayouts = { layout };
        desc.primType = nvrhi::PrimitiveType::TriangleList;
        desc.renderState.depthStencilState.depthTestEnable = true;
        desc.renderState.depthStencilState.depthWriteEnable = true;
        desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        desc.renderState.rasterState.frontCounterClockwise = false;
        desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
        desc.renderState.rasterState.depthBias = -int(roundf(state.rasterBias));
        desc.renderState.rasterState.slopeScaledDepthBias = -state.rasterSlope;
        desc.renderState.rasterState.depthBiasClamp = 0.0f;
        return desc;
    };
    string128 name;
    xr_sprintf(name, "VSMDynPage_b%.2f_s%.2f", state.rasterBias, state.rasterSlope);
    state.dynPagePipeline = cache.GetOrCreatePipeline(name, makeDesc(state.dynPageVS, state.pagePS, state.dynPageLayout, false), fbInfo, nvDevice);
    xr_sprintf(name, "VSMDynPageAT_b%.2f_s%.2f", state.rasterBias, state.rasterSlope);
    state.dynPageATPipeline = cache.GetOrCreatePipeline(name, makeDesc(state.dynPageVS, state.pageATPS, state.dynPageATLayout, true), fbInfo, nvDevice);
    xr_sprintf(name, "VSMDynSkinPage_b%.2f_s%.2f", state.rasterBias, state.rasterSlope);
    state.dynSkinPagePipeline = cache.GetOrCreatePipeline(name, makeDesc(state.dynSkinPageVS, state.pagePS, state.dynSkinPageLayout, false), fbInfo, nvDevice);
    {
        nvrhi::GraphicsPipelineDesc clearDesc;
        clearDesc.VS = state.dynClearVS;
        clearDesc.PS = state.clearPipeline ? state.clearPipeline->getDesc().PS : nullptr;
        clearDesc.inputLayout = nullptr;
        clearDesc.bindingLayouts = { state.dynClearLayout };
        clearDesc.primType = nvrhi::PrimitiveType::TriangleList;
        clearDesc.renderState.depthStencilState.depthTestEnable = true;
        clearDesc.renderState.depthStencilState.depthWriteEnable = true;
        clearDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
        clearDesc.renderState.rasterState.frontCounterClockwise = false;
        clearDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
        state.dynClearPipeline = cache.GetOrCreatePipeline("VSMDynClear", clearDesc, fbInfo, nvDevice);
    }
    if (!state.dynPagePipeline || !state.dynPageATPipeline || !state.dynSkinPagePipeline || !state.dynClearPipeline) {
        Msg("! [VSM] dynamic page pipelines failed");
        state.dynPipelinesFailed = true;
        return false;
    }
    state.dynPipelinesReady = true;
    Msg("* [VSM] dynamic page pipelines initialized");
    return true;
}

void ExecuteDynAlloc(fg::RenderContext* ctx, const VSMDynAllocData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.allocPipeline)
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_alloc", ".cs");
    auto* touchRefl = shaderLoader->GetCachedReflection("vsm_dyn_touch", ".cs");
    if (!refl || !touchRefl || !state.touchPipeline || !data.config.gpuCulling)
        return;

    cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dynAllocInfo, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dynPageList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::ShaderResource);

    auto vsmCB = VsmParamsCB(data.device);
    GPUCullingManager& gpuCulling = *data.config.gpuCulling;
    auto touch = [&](nvrhi::IBuffer* entries, u32 entryBase, u32 entryCount, const char* label) {
        if (!entries || entryCount == 0)
            return;
        VsmDynBinParams tp = {};
        tp.entryBase = entryBase;
        tp.entryCount = entryCount;
        tp.includeAT = ps_r_vsm_at ? 1u : 0u;
        auto touchCB = cache.GetOrCreateVolatileCB("VSM", "DynTouchParams", sizeof(VsmDynBinParams), data.device, 64);
        cmdList->writeBuffer(touchCB, &tp, sizeof(tp));
        cmdList->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);
        BindingSetBuilder tbs(*touchRefl, nvDevice, label);
        tbs.ConstantBuffer("VsmParams", vsmCB)
           .ConstantBuffer("VsmDynBinParams", touchCB)
           .BufferSRV("g_Entries", entries)
           .BufferUAV("g_DynPageTable", state.dynPageTable);
        auto set = cache.GetOrCreateBindingSet(tbs.Build(), state.touchLayout, nvDevice);
        if (!set)
            return;
        nvrhi::ComputeState cs;
        cs.pipeline = state.touchPipeline;
        cs.bindings = { set };
        cmdList->setComputeState(cs);
        cmdList->dispatch((entryCount + 63) / 64, 1, 1);
    };
    touch(data.config.entryBuffer, gpuCulling.GetClusterEntryCount(), gpuCulling.GetDynamicClusterEntryCount(), "VSM.DynTouch");
    touch(gpuCulling.GetSkinnedEntryBuffer(), 0u, gpuCulling.GetSkinnedEntryCount(), "VSM.DynTouchSkinned");

    BindingSetBuilder bsb(*refl, nvDevice, "VSM.DynAlloc");
    bsb.BufferSRV("g_Needed", state.needed)
       .BufferUAV("g_DynPageTable", state.dynPageTable)
       .BufferUAV("g_DynPageList", state.dynPageList)
       .BufferUAV("g_DynAllocInfo", state.dynAllocInfo);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.allocLayout, nvDevice);
    if (!bindingSet)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.allocPipeline;
    cs.bindings = { bindingSet };
    cmdList->setComputeState(cs);
    cmdList->dispatch((kVSMPageCount + 63) / 64, 1, 1);

    cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.dynPageList, nvrhi::ResourceStates::ShaderResource);
}

void ExecuteDynBin(fg::RenderContext* ctx, const VSMDynBinData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.dynBinPipeline || !state.dynArgsPipeline || !data.config.gpuCulling)
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* binRefl = shaderLoader->GetCachedReflection("vsm_bin_dyn", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("vsm_dyn_draw_args", ".cs");
    if (!binRefl || !argsRefl)
        return;

    cmdList->setBufferState(state.dynStats, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::ShaderResource);
    for (u32 i = 0; i < kVSMDynStreamCount; ++i)
        cmdList->setBufferState(state.dynPairs[i], nvrhi::ResourceStates::UnorderedAccess);

    auto vsmCB = VsmParamsCB(data.device);
    GPUCullingManager& gpuCulling = *data.config.gpuCulling;
    const VSMDynConfig& cfg = data.config;
    auto dispatchSource = [&](nvrhi::IBuffer* entries, u32 entryBase, u32 entryCount, u32 statsBase,
                              nvrhi::IBuffer* pairsOpaque, nvrhi::IBuffer* pairsAT, u32 capOpaque, u32 capAT, const char* label) {
        if (!entries || entryCount == 0)
            return;
        VsmDynBinParams bp = {};
        bp.entryBase = entryBase;
        bp.entryCount = entryCount;
        bp.includeAT = ps_r_vsm_at ? 1u : 0u;
        bp.capOpaque = capOpaque;
        bp.capAT = capAT;
        bp.statsBase = statsBase;
        auto binCB = cache.GetOrCreateVolatileCB("VSM", "DynBinParams", sizeof(VsmDynBinParams), data.device, 64);
        cmdList->writeBuffer(binCB, &bp, sizeof(bp));
        cmdList->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);

        BindingSetBuilder bsb(*binRefl, nvDevice, label);
        bsb.ConstantBuffer("VsmParams", vsmCB)
           .ConstantBuffer("VsmDynBinParams", binCB)
           .BufferSRV("g_Entries", entries)
           .BufferSRV("g_DynPageTable", state.dynPageTable)
           .BufferUAV("g_Stats", state.dynStats)
           .BufferUAV("g_PairsOpaque", pairsOpaque)
           .BufferUAV("g_PairsAT", pairsAT);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.dynBinLayout, nvDevice);
        if (!bindingSet)
            return;
        nvrhi::ComputeState cs;
        cs.pipeline = state.dynBinPipeline;
        cs.bindings = { bindingSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch((entryCount + 63) / 64, 1, 1);
    };
    dispatchSource(cfg.entryBuffer, gpuCulling.GetClusterEntryCount(), gpuCulling.GetDynamicClusterEntryCount(), 0u,
        state.dynPairs[0], state.dynPairs[1], kVSMDynPairCapOpaque, kVSMDynPairCapAT, "VSM.DynBin");
    dispatchSource(gpuCulling.GetSkinnedEntryBuffer(), 0u, gpuCulling.GetSkinnedEntryCount(), 8u,
        state.dynPairs[2], state.dynPairs[1], kVSMDynPairCapSkinned, 0u, "VSM.DynBinSkinned");

    VsmDynArgsParams ap = {};
    ap.capOpaque = kVSMDynPairCapOpaque;
    ap.capAT = kVSMDynPairCapAT;
    ap.capSkinned = kVSMDynPairCapSkinned;
    auto argsCB = cache.GetOrCreateVolatileCB("VSM", "DynArgsParams", sizeof(VsmDynArgsParams), data.device);
    cmdList->writeBuffer(argsCB, &ap, sizeof(ap));
    for (u32 i = 0; i < kVSMDynStreamCount; ++i)
        cmdList->setBufferState(state.dynArgs[i], nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dynClearArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dynStats, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.dynAllocInfo, nvrhi::ResourceStates::ShaderResource);

    BindingSetBuilder abs(*argsRefl, nvDevice, "VSM.DynArgs");
    abs.ConstantBuffer("VsmDynArgsParams", argsCB)
       .BufferSRV("g_Stats", state.dynStats)
       .BufferSRV("g_DynAllocInfo", state.dynAllocInfo)
       .BufferUAV("g_ArgsOpaque", state.dynArgs[0])
       .BufferUAV("g_ArgsAT", state.dynArgs[1])
       .BufferUAV("g_ArgsSkinned", state.dynArgs[2])
       .BufferUAV("g_ArgsClear", state.dynClearArgs);
    auto argsSet = cache.GetOrCreateBindingSet(abs.Build(), state.dynArgsLayout, nvDevice);
    if (argsSet) {
        nvrhi::ComputeState cs;
        cs.pipeline = state.dynArgsPipeline;
        cs.bindings = { argsSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch(1, 1, 1);
    }

    for (u32 i = 0; i < kVSMDynStreamCount; ++i) {
        cmdList->setBufferState(state.dynArgs[i], nvrhi::ResourceStates::IndirectArgument);
        cmdList->setBufferState(state.dynPairs[i], nvrhi::ResourceStates::ShaderResource);
    }
    cmdList->setBufferState(state.dynClearArgs, nvrhi::ResourceStates::IndirectArgument);
}

void ExecuteDynAtlas(fg::RenderContext* ctx, const FrameGraph& fg, const VSMDynAtlasData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.dynAtlas);
    if (!cmdList || !nvDevice || !atlas)
        return;
    state.dynRendered = false;

    if (state.dynAtlasFirst) {
        cmdList->clearDepthStencilTexture(atlas, nvrhi::AllSubresources, true, 0.0f, false, 0);
        state.dynAtlasFirst = false;
    }

    const VSMDynConfig& cfg = data.config;
    if (!cfg.gpuCulling)
        return;
    if (!EnsureDynPagePipelines(data.device, state))
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* dynVsRefl = shaderLoader->GetCachedReflection("vsm_page_pull_dyn", ".vs");
    auto* skinVsRefl = shaderLoader->GetCachedReflection("vsm_page_pull_skinned", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_page", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!dynVsRefl || !skinVsRefl || !psRefl || !atRefl)
        return;

    auto vsmCB = VsmParamsCB(data.device);
    auto* backend = data.device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
    auto& matBuffer = bindless::MaterialBuffer::Instance();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(atlas);
    auto framebuffer = cache.GetOrCreateFramebuffer("VSMAtlasDyn", fbDesc, nvDevice);
    if (!framebuffer)
        return;
    const auto& rtDesc = atlas->getDesc();
    const nvrhi::Viewport viewport(0.0f, float(rtDesc.width), 0.0f, float(rtDesc.height), 0.0f, 1.0f);
    const nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

    auto draw = [&](nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingSet* bindingSet, nvrhi::IBuffer* args, bool withBindless) {
        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        if (withBindless && bindlessTable)
            gs.addBindingSet(bindlessTable);
        gs.indirectParams = args;
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(0, 1);
        state.dynRendered = true;
    };
    {
        nvrhi::GraphicsState gs;
        gs.pipeline = state.dynClearPipeline;
        gs.framebuffer = framebuffer;
        gs.indirectParams = state.dynClearArgs;
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(0, 1);
    }

    GPUCullingManager& gpuCulling = *cfg.gpuCulling;
    if (gpuCulling.GetDynamicClusterEntryCount() > 0 && cfg.entryBuffer && cfg.dynamicInstanceBuffer && cfg.megaVertexBuffer && cfg.megaIndexBuffer) {
        BindingSetBuilder bsb(*dynVsRefl, *psRefl, nvDevice, "VSM.DynPage");
        bsb.ConstantBuffer("VsmParams", vsmCB)
           .BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer)
           .BufferSRV("g_Pairs", state.dynPairs[0])
           .BufferSRV("g_Entries", cfg.entryBuffer)
           .BufferSRV("g_PageList", state.dynPageList)
           .BufferSRV("g_MegaVB", cfg.megaVertexBuffer)
           .BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.dynPageLayout, nvDevice))
            draw(state.dynPagePipeline, bindingSet, state.dynArgs[0], false);

        BindingSetBuilder atBsb(*dynVsRefl, *atRefl, nvDevice, "VSM.DynPageAT");
        atBsb.ConstantBuffer("VsmParams", vsmCB)
             .BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer)
             .BufferSRV("g_Pairs", state.dynPairs[1])
             .BufferSRV("g_Entries", cfg.entryBuffer)
             .BufferSRV("g_PageList", state.dynPageList)
             .BufferSRV("g_MegaVB", cfg.megaVertexBuffer)
             .BufferSRV("g_MegaIB", cfg.megaIndexBuffer)
             .BufferSRV("g_Materials", matBuffer.GetBuffer());
        if (auto bindingSet = cache.GetOrCreateBindingSet(atBsb.Build(), state.dynPageATLayout, nvDevice))
            draw(state.dynPageATPipeline, bindingSet, state.dynArgs[1], true);
    }

    nvrhi::IBuffer* skinnedEntries = gpuCulling.GetSkinnedEntryBuffer();
    nvrhi::IBuffer* preVB = gpuCulling.GetSkinnedPreVertexBuffer();
    nvrhi::IBuffer* skinnedIB = gpuCulling.GetSkinnedPools().GetCombinedIndexBuffer();
    if (gpuCulling.GetSkinnedEntryCount() > 0 && skinnedEntries && preVB && skinnedIB) {
        BindingSetBuilder bsb(*skinVsRefl, *psRefl, nvDevice, "VSM.DynSkinPage");
        bsb.ConstantBuffer("VsmParams", vsmCB)
           .BufferSRV("g_Pairs", state.dynPairs[2])
           .BufferSRV("g_Entries", skinnedEntries)
           .BufferSRV("g_PageList", state.dynPageList)
           .BufferSRV("g_SkinnedVB", preVB)
           .BufferSRV("g_SkinnedIB", skinnedIB);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.dynSkinPageLayout, nvDevice))
            draw(state.dynSkinPagePipeline, bindingSet, state.dynArgs[2], false);
    }
}

void ExecuteResolve(fg::RenderContext* ctx, const FrameGraph& fg, const VSMResolveData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.atlas);
    nvrhi::ITexture* mask = fg.GetPhysicalTexture(data.mask);
    if (!cmdList || !nvDevice || !depth || !atlas || !mask || !state.resolvePipeline)
        return;
    nvrhi::ITexture* atlasDyn = data.dynAtlas.is_valid() ? fg.GetPhysicalTexture(data.dynAtlas) : nullptr;
    if (!atlasDyn)
        atlasDyn = state.dynAtlas;
    if (!atlasDyn)
        return;

    ScheduleReadback(cmdList, state);

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_resolve", ".cs");
    if (!refl)
        return;

    auto vsmCB = VsmParamsCB(data.device);

    const u32 prev = (state.maskSlot + 1) % 2;
    const bool histOK = ps_r_vsm_temporal && state.resolveCount >= 1;
    VsmResolveParams rp = {};
    rp.invViewProj = Device.mInvFullTransform;
    rp.hudScale.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, psHUD_FOV);
    rp.prevViewProj = state.prevViewProj;
    const bool dynOn = state.dynActive && state.dynRendered;
    rp.prevCamPos.set(state.prevCamPos.x, state.prevCamPos.y, state.prevCamPos.z, dynOn ? float(std::max(ps_r_vsm_debug_dyn, 0)) : 0.0f);
    rp.curCamPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, ps_r_vsm_ta_blend_dyn);
    rp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    rp.params.set(histOK ? ps_r_vsm_ta_blend : 0.0f, kVSMRejectTol, histOK ? 1.0f : 0.0f, 1.0f);
    const bool softOn = ps_r_vsm_soft >= 1;
    rp.params2.set(softOn ? ps_r_vsm_soft_clamp : ps_r_vsm_ta_clamp, ps_r_vsm_ta_motion, ps_r_vsm_ta_motion_floor, ps_r_vsm_bias_min);
    rp.params3.set(softOn ? float(ps_r_vsm_soft) : 0.0f, float(ps_r_vsm_soft_search), tanf(deg2rad(ps_r_vsm_soft_angle)), ps_r_vsm_soft_range);
    const float diagSel = (dynOn && ps_r_vsm_debug_dyn > 0) ? 2.0f : (ps_r_vsm_debug == 4 ? 1.0f : 0.0f);
    rp.params4.set(float(state.resolveCount & 63u), histOK ? ps_r_vsm_ta_carry : 0.0f, diagSel, dynOn ? 1.0f : 0.0f);
    auto resolveCB = cache.GetOrCreateVolatileCB("VSM", "ResolveParams", sizeof(VsmResolveParams), data.device);
    cmdList->writeBuffer(resolveCB, &rp, sizeof(rp));

    cmdList->setBufferState(state.dynPageTable, nvrhi::ResourceStates::ShaderResource);
    BindingSetBuilder bsb(*refl, nvDevice, "VSM.Resolve");
    bsb.ConstantBuffer("VsmParams", vsmCB)
       .ConstantBuffer("VsmResolveParams", resolveCB)
       .Texture("g_Depth", depth)
       .Texture("g_Atlas", atlas)
       .BufferSRV("g_PageTable", state.pageTable)
       .Texture("g_History", state.mask[prev])
       .Texture("g_AtlasDyn", atlasDyn)
       .BufferSRV("g_DynPageTable", state.dynPageTable)
       .BufferSRV("g_SlotPivot", state.slotPivot)
       .BufferSRV("g_SlotSun", state.slotSun)
       .TextureUAV("g_Mask", mask);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.resolveLayout, nvDevice);
    if (!bindingSet)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.resolvePipeline;
    cs.bindings = { bindingSet };
    cmdList->setComputeState(cs);
    cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);

    state.prevViewProj = Device.mFullTransform;
    state.prevCamPos = Device.vCameraPosition;
    if (state.resolveCount < 0xFFFF)
        state.resolveCount++;
    state.maskReady = true;
}

void ExecuteDebugView(fg::RenderContext* ctx, const FrameGraph& fg, const VSMDebugData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
    nvrhi::ITexture* output = fg.GetPhysicalTexture(data.output);
    nvrhi::ITexture* mask = data.mask.is_valid() ? fg.GetPhysicalTexture(data.mask) : nullptr;
    if (!cmdList || !nvDevice || !depth || !output || !state.debugPipeline)
        return;
    if (!mask)
        mask = GetPassResourceCache().GetDummyShadowMap2D(nvDevice);

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_debug_view", ".cs");
    if (!refl)
        return;

    auto vsmCB = VsmParamsCB(data.device);

    VsmDebugParams dp = {};
    dp.invViewProj = Device.mInvFullTransform;
    dp.hudScale.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, psHUD_FOV);
    dp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    dp.mode = u32(ps_r_vsm_debug);
    dp.pad[0] = (state.dynActive && state.dynRendered) ? u32(std::max(ps_r_vsm_debug_dyn, 0)) : 0u;
    auto debugCB = cache.GetOrCreateVolatileCB("VSM", "DebugParams", sizeof(VsmDebugParams), data.device);
    cmdList->writeBuffer(debugCB, &dp, sizeof(dp));

    BindingSetBuilder bsb(*refl, nvDevice, "VSM.DebugView");
    bsb.ConstantBuffer("VsmParams", vsmCB)
       .ConstantBuffer("VsmDebugParams", debugCB)
       .Texture("g_Depth", depth)
       .BufferSRV("g_Needed", state.needed)
       .BufferSRV("g_PageTable", state.pageTable)
       .BufferSRV("g_SlotDirty", state.slotDirty)
       .Texture("g_Mask", mask)
       .TextureUAV("g_Output", output);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.debugLayout, nvDevice);
    if (!bindingSet)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.debugPipeline;
    cs.bindings = { bindingSet };
    cmdList->setComputeState(cs);
    cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
}

}

bool VSMLoadScreenFrozen()
{
    return ps_r_vsm_load_freeze != 0 && Device.dwPrecacheFrame != 0;
}

void InvalidateVSMCache(VSMState& state)
{
    state.resolveCount = 0;
    state.prevSunValid = false;
    state.physInit = false;
    for (u32 L = 0; L < kVSMLevels; ++L)
        state.bucketValid[L] = false;
    state.invalidations++;
    state.primeFrames = kVSMPrimeFrames;
    state.primeTraceLeft = kVSMPrimeTraceFrames;
    state.primeTraceIdx = 0;
    state.primeTraceQuiet = 0;
}

Fmatrix VSMSunView(const Fvector& sunDir, const Fvector& eye)
{
    Fvector sd = sunDir;
    if (sd.magnitude() < 1e-4f)
        sd.set(0.0f, -1.0f, 0.0f);
    sd.normalize();
    Fvector up;
    up.set(0.0f, 1.0f, 0.0f);
    if (_abs(sd.y) > 0.99f)
        up.set(0.0f, 0.0f, 1.0f);
    Fmatrix view;
    view.build_camera_dir(eye, sd, up);
    return view;
}

Fmatrix VSMSunView(const Fvector& sunDir)
{
    Fvector eye;
    eye.set(0.0f, 0.0f, 0.0f);
    return VSMSunView(sunDir, eye);
}

static void ComputePageBases(const Fvector& camL, s32 (&pb)[kVSMLevels][2])
{
    for (u32 L = 0; L < kVSMLevels; ++L) {
        const float ext = ps_r_vsm_base * float(1u << L);
        const float pageTexel = ext / float(kVSMPagesAxis);
        pb[L][0] = (s32)floorf((camL.x - 0.5f * ext) / pageTexel);
        pb[L][1] = (s32)floorf((camL.y - 0.5f * ext) / pageTexel);
    }
}

nvrhi::ITexture* ResolveSunMask(const FrameGraph& fg, VirtualResourceHandle mask, nvrhi::IDevice* device)
{
    nvrhi::ITexture* tex = mask.is_valid() ? fg.GetPhysicalTexture(mask) : nullptr;
    return tex ? tex : GetPassResourceCache().GetDummyShadowMap2D(device);
}

float VSMReceiverExtent()
{
    return ps_r_vsm_base * float(1u << (kVSMLevels - 1));
}

void VSMBeginFrame(VSMState& state, const Fvector& camPos, const Fvector& sunDirIn)
{
    state.frame++;
    if (g_pGamePersistent && g_pGamePersistent->Environment().IsThunderboltActive())
        state.boltHeld++;

    Fvector sd = sunDirIn;
    if (sd.magnitude() < 1e-4f)
        sd.set(0.0f, -1.0f, 0.0f);
    sd.normalize();
    {
        float sunLum = 1.0f;
        if (g_pGamePersistent) {
            const Fvector& c = g_pGamePersistent->Environment().CurrentEnv.sun_color;
            sunLum = c.x * 0.299f + c.y * 0.587f + c.z * 0.114f;
        }
        const float toSunY = -sd.y;
        state.sunDown = ps_r_sun_night_freeze && (toSunY < ps_r_sun_night_alt || sunLum < ps_r_sun_night_lum);
    }
    if (!state.pivotValid) {
        state.pivot = camPos;
        state.pivotValid = true;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            state.tileBias[L][0] = 0;
            state.tileBias[L][1] = 0;
        }
    }

    float rate = 0.0f;
    if (state.prevSunValid) {
        Fvector cr;
        cr.crossproduct(sd, state.prevSunDir);
        rate = cr.magnitude();
        const float stepDeg = rate * 57.29578f;
        if (stepDeg > state.sunStepMax)
            state.sunStepMax = stepDeg;
        state.sunRate += (rate - state.sunRate) * 0.1f;
    } else {
        state.sunRate = 0.0f;
    }
    state.prevSunDir = sd;
    state.prevSunValid = true;
    state.sunMoving = state.sunRate > 1e-9f;

    const float relabelStep = ps_r_vsm_base;
    Fmatrix view = VSMSunView(sd, state.pivot);
    Fvector camL;
    view.transform_tiny(camL, camPos);
    s32 pbOld[kVSMLevels][2];
    ComputePageBases(camL, pbOld);
    const s32 kx = (s32)floorf(camL.x / relabelStep + 0.5f);
    const s32 ky = (s32)floorf(camL.y / relabelStep + 0.5f);
    const s32 kz = (s32)floorf(camL.z / relabelStep + 0.5f);
    if (kx != 0 || ky != 0 || kz != 0) {
        Fvector right, up, fwd;
        right.set(view._11, view._21, view._31);
        up.set(view._12, view._22, view._32);
        fwd.set(view._13, view._23, view._33);
        Fvector shift;
        shift.mul(right, float(kx) * relabelStep);
        shift.mad(up, float(ky) * relabelStep);
        shift.mad(fwd, float(kz) * relabelStep);
        state.pivot.add(shift);
        view = VSMSunView(sd, state.pivot);
        view.transform_tiny(camL, camPos);
        s32 pbNew[kVSMLevels][2];
        ComputePageBases(camL, pbNew);
        for (u32 L = 0; L < kVSMLevels; ++L) {
            state.tileBias[L][0] += pbOld[L][0] - pbNew[L][0];
            state.tileBias[L][1] += pbOld[L][1] - pbNew[L][1];
        }
        state.relabels++;
    }
    state.sunView = view;

    VsmParams& params = state.params;
    params.view = view;
    ComputePageBases(camL, state.pageBase);
    const float camDist = camL.magnitude();
    for (u32 L = 0; L < kVSMLevels; ++L) {
        const float ext = ps_r_vsm_base * float(1u << L);
        const float texel = ext / float(kVSMVirtualRes);
        const float pageTexel = texel * float(kVSMPageSize);
        params.level[L].set(float(state.pageBase[L][0]) * pageTexel, float(state.pageBase[L][1]) * pageTexel, ext, 0.0f);
        const float reach = camDist + 0.7071f * ext;
        const float drift = reach * state.sunRate;
        float interval = float(kVSMRefreshIntervalMax);
        if (drift > 1e-12f)
            interval = kVSMRefreshTexels * texel / drift;
        if (interval < 1.0f)
            interval = 1.0f;
        if (interval > float(kVSMRefreshIntervalMax))
            interval = float(kVSMRefreshIntervalMax);
        state.refreshInterval[L] = u32(interval);
    }
    params.zparams.set(kVSMZNear, 1.0f / (kVSMZFar - kVSMZNear), ps_r_vsm_bias, ps_r_vsm_bias_dyn);
    state.sunDir = sd;
}

VSMOutput setupVSMPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle orderAfter,
    const VSMDrawConfig& config,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    VSMOutput out;
    if (!state || !device || !depth.is_valid() || width == 0 || height == 0)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;

    ProcessReadback(nvDevice, *state);
    if (!EnsureResources(nvDevice, *state))
        return out;
    if (!EnsureMaskTargets(nvDevice, *state, width, height))
        return out;
    if (state->rasterBias != ps_r_vsm_raster_bias || state->rasterSlope != ps_r_vsm_raster_slope) {
        const bool live = state->rasterBias >= 0.0f;
        state->rasterBias = ps_r_vsm_raster_bias;
        state->rasterSlope = ps_r_vsm_raster_slope;
        state->pagePipeline = nullptr;
        state->pageATPipeline = nullptr;
        state->dynPipelinesReady = false;
        state->dynPagePipeline = nullptr;
        state->dynPageATPipeline = nullptr;
        state->dynSkinPagePipeline = nullptr;
        if (live)
            InvalidateVSMCache(*state);
    }
    if (state->atMode != (ps_r_vsm_at ? 1 : 0)) {
        if (state->atMode >= 0)
            InvalidateVSMCache(*state);
        state->atMode = ps_r_vsm_at ? 1 : 0;
    }
    {
        const float k = std::max(0.1f, ps_r_vsm_cluster_lod);
        if (state->lastBase != ps_r_vsm_base || state->lastClusterLod != k) {
            if (state->lastBase >= 0.0f)
                InvalidateVSMCache(*state);
            state->lastBase = ps_r_vsm_base;
            state->lastClusterLod = k;
        }
    }
    {
        const bool behind = VSMLoadScreenFrozen();
        if (state->behindLoadScreen && !behind) {
            state->primeFrames = kVSMPrimeFrames;
            state->primeTraceLeft = kVSMPrimeTraceFrames;
            state->primeTraceIdx = 0;
            state->primeTraceQuiet = 0;
        }
        state->behindLoadScreen = behind;
        const bool night = state->sunDown && !state->atlasFirst && state->maskReady;
        if (night != state->nightFrozen) {
            state->nightFrozen = night;
            if (ps_r_vsm_debug >= 1)
                Msg("[VSM] night-freeze: %s", night ? "FROZEN (sun down, VSM update skipped)" : "active (sun up)");
        }
        if (behind || night) {
            if (behind && state->maskReady) {
                ResourceDesc heldDesc;
                heldDesc.type = ResourceDesc::Type::Texture2D;
                heldDesc.width = width;
                heldDesc.height = height;
                heldDesc.format = nvrhi::Format::RGBA16_FLOAT;
                heldDesc.isImported = true;
                heldDesc.isTransient = false;
                heldDesc.debugName = "rt_VSMMask";
                out.mask = fg.ImportTexture("rt_VSMMask", state->mask[state->maskSlot], heldDesc);
                fg.GetRTRegistry().RegisterRT("rt_VSMMask", out.mask);
            }
            return out;
        }
    }
    LogTelemetry(*state);
    state->active = true;

    auto bufferDesc = [](const char* name, u64 bytes, u32 stride) {
        ResourceDesc d;
        d.type = ResourceDesc::Type::Buffer;
        d.bufferSize = bytes;
        d.structStride = stride;
        d.isUAV = true;
        d.allowUAV = true;
        d.isImported = true;
        d.isTransient = false;
        d.debugName = name;
        return d;
    };
    VirtualResourceHandle neededHandle = fg.ImportBuffer("vsm_needed", state->needed, bufferDesc("vsm_needed", u64(kVSMPageCount) * sizeof(u32), sizeof(u32)));
    VirtualResourceHandle dirtyHandle = fg.ImportBuffer("vsm_dirty_list", state->dirtyList, bufferDesc("vsm_dirty_list", u64(kVSMDirtyListWords) * sizeof(u32), sizeof(u32)));
    VirtualResourceHandle clearHandle = fg.ImportBuffer("vsm_draw_clear", state->drawClear, bufferDesc("vsm_draw_clear", sizeof(u32) * kVSMCounterWords, 0));
    VirtualResourceHandle argsHandles[kVSMStreamCount];
    {
        static const char* kArgsNames[kVSMStreamCount] = { "vsm_page_args_opaque", "vsm_page_args_terrain", "vsm_page_args_at" };
        for (u32 i = 0; i < kVSMStreamCount; ++i)
            argsHandles[i] = fg.ImportBuffer(kArgsNames[i], state->pageArgs[i], bufferDesc(kArgsNames[i], sizeof(u32) * 4, 0));
    }

    ResourceDesc atlasDesc;
    atlasDesc.type = ResourceDesc::Type::Texture2D;
    atlasDesc.width = kVSMAtlasW * kVSMPageSize;
    atlasDesc.height = kVSMAtlasH * kVSMPageSize;
    atlasDesc.format = nvrhi::Format::D16;
    atlasDesc.isDepthStencil = true;
    atlasDesc.isImported = true;
    atlasDesc.isTransient = false;
    atlasDesc.debugName = "rt_VSMAtlas";
    VirtualResourceHandle atlasHandle = fg.ImportTexture("rt_VSMAtlas", state->atlas, atlasDesc);

    auto& markData = fg.addCallbackPass<VSMMarkData>(
        "VSM Mark",
        [&, depth, orderAfter, neededHandle, width, height, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMMarkData& data) {
            data.state = state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::ShaderResource);
            data.needed = passBuilder.write(neededHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMMarkData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteMark(ctx, fg, data);
        });

    auto& residData = fg.addCallbackPass<VSMResidData>(
        "VSM Residency",
        [&, dirtyHandle, clearHandle, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMResidData& data) {
            data.state = state;
            data.device = device;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.needed = passBuilder.read(markData.needed, ResourceState::ShaderResource);
            data.dirtyList = passBuilder.write(dirtyHandle, ResourceState::UnorderedAccess);
            data.drawClear = passBuilder.write(clearHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMResidData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteResid(ctx, data);
        });

    auto& binData = fg.addCallbackPass<VSMBinData>(
        "VSM Bin",
        [&, argsHandles, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMBinData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.dirtyList = passBuilder.read(residData.dirtyList, ResourceState::ShaderResource);
            for (u32 i = 0; i < kVSMStreamCount; ++i)
                data.pageArgs[i] = passBuilder.write(argsHandles[i], ResourceState::UnorderedAccess);
        },
        [](const VSMBinData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteBin(ctx, data);
        });

    auto& atlasData = fg.addCallbackPass<VSMAtlasData>(
        "VSM Static Atlas",
        [&, atlasHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMAtlasData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.atlas = passBuilder.write(atlasHandle, ResourceState::DepthStencilWrite);
            data.dirtyList = passBuilder.read(residData.dirtyList, ResourceState::ShaderResource);
            data.drawClear = passBuilder.read(residData.drawClear, ResourceState::IndirectArgument);
            for (u32 i = 0; i < kVSMStreamCount; ++i)
                data.pageArgs[i] = passBuilder.read(binData.pageArgs[i], ResourceState::IndirectArgument);
        },
        [](const VSMAtlasData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteAtlas(ctx, fg, data);
        });

    fg.GetRTRegistry().RegisterRT("rt_VSMAtlas", atlasData.atlas);
    out.atlas = atlasData.atlas;
    out.active = true;
    state->fgNeeded = markData.needed;
    state->fgDirtyList = residData.dirtyList;
    state->fgAtlas = atlasData.atlas;
    state->fgDynAtlas = VirtualResourceHandle{};
    state->fgDynTable = VirtualResourceHandle{};
    state->fgDynArgs = VirtualResourceHandle{};
    state->dynActive = false;
    state->dynRendered = false;
    return out;
}

void setupVSMDynamicPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    const VSMDynConfig& config,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    if (!state || !device || !state->active || !config.gpuCulling)
        return;
    if (!state->dynAtlas || !state->dynPageTable || !state->dynClearArgs)
        return;
    if (config.gpuCulling->GetDynamicClusterEntryCount() == 0 && config.gpuCulling->GetSkinnedEntryCount() == 0)
        return;

    auto bufferDesc = [](const char* name, u64 bytes, u32 stride) {
        ResourceDesc d;
        d.type = ResourceDesc::Type::Buffer;
        d.bufferSize = bytes;
        d.structStride = stride;
        d.isUAV = true;
        d.allowUAV = true;
        d.isImported = true;
        d.isTransient = false;
        d.debugName = name;
        return d;
    };
    VirtualResourceHandle dynTableHandle = fg.ImportBuffer("vsm_dyn_page_table", state->dynPageTable, bufferDesc("vsm_dyn_page_table", u64(kVSMPageCount) * sizeof(u32), sizeof(u32)));
    VirtualResourceHandle dynArgsHandle = fg.ImportBuffer("vsm_dyn_args", state->dynArgs[0], bufferDesc("vsm_dyn_args", sizeof(u32) * 4, 0));

    ResourceDesc dynAtlasDesc;
    dynAtlasDesc.type = ResourceDesc::Type::Texture2D;
    dynAtlasDesc.width = kVSMAtlasWDyn * kVSMPageSize;
    dynAtlasDesc.height = kVSMAtlasHDyn * kVSMPageSize;
    dynAtlasDesc.format = nvrhi::Format::D16;
    dynAtlasDesc.isDepthStencil = true;
    dynAtlasDesc.isImported = true;
    dynAtlasDesc.isTransient = false;
    dynAtlasDesc.debugName = "rt_VSMAtlasDyn";
    VirtualResourceHandle dynAtlasHandle = fg.ImportTexture("rt_VSMAtlasDyn", state->dynAtlas, dynAtlasDesc);

    auto& allocData = fg.addCallbackPass<VSMDynAllocData>(
        "VSM Dyn Alloc",
        [&, dynTableHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMDynAllocData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.needed = passBuilder.read(state->fgNeeded, ResourceState::ShaderResource);
            data.dynTable = passBuilder.write(dynTableHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMDynAllocData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteDynAlloc(ctx, data);
        });

    auto& binData = fg.addCallbackPass<VSMDynBinData>(
        "VSM Dyn Bin",
        [&, skinnedDrawArgs, dynArgsHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMDynBinData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.dynTable = passBuilder.read(allocData.dynTable, ResourceState::ShaderResource);
            if (skinnedDrawArgs.is_valid())
                data.order = passBuilder.read(skinnedDrawArgs, ResourceState::ShaderResource);
            data.dynArgs = passBuilder.write(dynArgsHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMDynBinData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteDynBin(ctx, data);
        });

    auto& atlasData = fg.addCallbackPass<VSMDynAtlasData>(
        "VSM Dyn Atlas",
        [&, dynAtlasHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMDynAtlasData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.dynAtlas = passBuilder.write(dynAtlasHandle, ResourceState::DepthStencilWrite);
            data.dynArgs = passBuilder.read(binData.dynArgs, ResourceState::IndirectArgument);
        },
        [](const VSMDynAtlasData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteDynAtlas(ctx, fg, data);
        });

    fg.GetRTRegistry().RegisterRT("rt_VSMAtlasDyn", atlasData.dynAtlas);
    state->fgDynAtlas = atlasData.dynAtlas;
    state->fgDynTable = allocData.dynTable;
    state->fgDynArgs = binData.dynArgs;
    state->dynActive = true;
}

framegraph::VirtualResourceHandle setupVSMResolvePasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler,
    framegraph::VirtualResourceHandle* outDebugView)
{
    if (outDebugView)
        *outDebugView = VirtualResourceHandle{};
    if (!state || !device || !state->active || !depth.is_valid() || !state->fgAtlas.is_valid())
        return VirtualResourceHandle{};

    state->maskSlot = (state->maskSlot + 1) % 2;
    ResourceDesc maskDesc;
    maskDesc.type = ResourceDesc::Type::Texture2D;
    maskDesc.width = width;
    maskDesc.height = height;
    maskDesc.format = nvrhi::Format::RGBA16_FLOAT;
    maskDesc.isUAV = true;
    maskDesc.allowUAV = true;
    maskDesc.isImported = true;
    maskDesc.isTransient = false;
    maskDesc.debugName = "rt_VSMMask";
    VirtualResourceHandle maskHandle = fg.ImportTexture("rt_VSMMask", state->mask[state->maskSlot], maskDesc);

    auto& resolveData = fg.addCallbackPass<VSMResolveData>(
        "VSM Resolve",
        [&, depth, maskHandle, width, height, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMResolveData& data) {
            data.state = state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            data.atlas = passBuilder.read(state->fgAtlas, ResourceState::ShaderResource);
            if (state->dynActive) {
                data.dynAtlas = passBuilder.read(state->fgDynAtlas, ResourceState::ShaderResource);
                data.dynTable = passBuilder.read(state->fgDynTable, ResourceState::ShaderResource);
            }
            data.mask = passBuilder.write(maskHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMResolveData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteResolve(ctx, fg, data);
        });
    fg.GetRTRegistry().RegisterRT("rt_VSMMask", resolveData.mask);

    if (ps_r_vsm_debug >= 2) {
        ResourceDesc viewDesc;
        viewDesc.type = ResourceDesc::Type::Texture2D;
        viewDesc.width = width;
        viewDesc.height = height;
        viewDesc.format = nvrhi::Format::RGBA8_UNORM;
        viewDesc.isUAV = true;
        viewDesc.allowUAV = true;
        viewDesc.isTransient = true;
        viewDesc.debugName = "rt_VSMDebug";
        VirtualResourceHandle viewHandle = fg.CreateTexture("rt_VSMDebug", viewDesc);

        auto& debugData = fg.addCallbackPass<VSMDebugData>(
            "VSM Debug View",
            [&, depth, viewHandle, width, height, state, resolveData](FrameGraph& builder, PassHandle passHandle, VSMDebugData& data) {
                data.state = state;
                data.device = device;
                data.width = width;
                data.height = height;
                RenderPassBuilder passBuilder(builder, passHandle);
                data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
                data.dirtyList = passBuilder.read(state->fgDirtyList, ResourceState::ShaderResource);
                data.mask = passBuilder.read(resolveData.mask, ResourceState::ShaderResource);
                data.output = passBuilder.write(viewHandle, ResourceState::UnorderedAccess);
            },
            [](const VSMDebugData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                ExecuteDebugView(ctx, fg, data);
            });
        if (outDebugView)
            *outDebugView = debugData.output;
    }

    return resolveData.mask;
}

}
