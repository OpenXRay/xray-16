#include "stdafx.h"
#include "VSMPassSetup.h"
#include "PassCommon.h"
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

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

constexpr u32 kReadbackResidOffset = 4 + kVSMPageCount;
constexpr u32 kReadbackBinOffset = kReadbackResidOffset + 4;
constexpr u32 kReadbackWords = kReadbackBinOffset + 8;
constexpr u32 kPairCaps[kVSMStreamCount] = { kVSMPairCapOpaque, kVSMPairCapTerrain, kVSMPairCapAT };
constexpr const char* kStreamNames[kVSMStreamCount] = { "Opaque", "Terrain", "AT" };

struct VsmMarkParams {
    Fmatrix invViewProj;
    Fvector4 screen;
    u32 markStep;
    u32 lodBias;
    u32 pad[2];
};

struct VsmResidParams {
    s32 pageBase[12];
    u32 frame;
    u32 refreshN;
    u32 sunMoving;
    u32 forceDirty;
    float inval[16];
};

struct VsmBinParams {
    u32 entryCount;
    u32 includeAT;
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    float errK;
    u32 pad[2];
};

struct VsmArgsParams {
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
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
};

struct VSMResolveData {
    VirtualResourceHandle depth;
    VirtualResourceHandle atlas;
    VirtualResourceHandle mask;
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

struct VSMZone {
    xray::profiler::GPUProfiler* profiler;
    nvrhi::ICommandList* cmdList;
    const char* name;
    VSMZone(xray::profiler::GPUProfiler* p, nvrhi::ICommandList* c, const char* n) : profiler(p), cmdList(c), name(n)
    {
        if (profiler && cmdList)
            profiler->BeginPass(cmdList, name);
    }
    ~VSMZone()
    {
        if (profiler && cmdList)
            profiler->EndPass(cmdList, name);
    }
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
        && state.binPipeline && state.argsPipeline && state.pagePipeline && state.pageATPipeline && state.resolvePipeline)
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
    auto binResult = shaderLoader->LoadComputeShader("vsm_bin_cluster");
    auto argsResult = shaderLoader->LoadComputeShader("vsm_draw_args");
    auto pageVsResult = shaderLoader->LoadVertexShader("vsm_page_pull", "main");
    auto pagePsResult = shaderLoader->LoadPixelShader("vsm_page", "main");
    auto pageATPsResult = shaderLoader->LoadPixelShader("vsm_page_at", "main");
    auto resolveResult = shaderLoader->LoadComputeShader("vsm_resolve");
    if (!resolveResult.handle || !resolveResult.reflection) {
        Msg("! [VSM] resolve shader failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    if (!markResult.handle || !markResult.reflection || !residResult.handle || !residResult.reflection
        || !debugResult.handle || !debugResult.reflection || !clearVsResult.handle || !clearVsResult.reflection
        || !clearPsResult.handle || !clearPsResult.reflection || !binResult.handle || !binResult.reflection
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
    state.binLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMBin", *binResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMArgs", *argsResult.reflection, nvDevice);
    state.pageLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMPage", *pageVsResult.reflection, *pagePsResult.reflection, nvDevice);
    state.pageATLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMPageAT", *pageVsResult.reflection, *pageATPsResult.reflection, nvDevice);
    state.resolveLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMResolve", *resolveResult.reflection, nvDevice);
    if (!state.markLayout || !state.residLayout || !state.debugLayout || !state.clearLayout
        || !state.binLayout || !state.argsLayout || !state.pageLayout || !state.pageATLayout || !state.resolveLayout) {
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

    nvrhi::ComputePipelineDesc binDesc;
    binDesc.CS = binResult.handle;
    binDesc.bindingLayouts = { state.binLayout };
    state.binPipeline = cache.GetOrCreateComputePipeline("VSMBin", binDesc, nvDevice);

    nvrhi::ComputePipelineDesc argsDesc;
    argsDesc.CS = argsResult.handle;
    argsDesc.bindingLayouts = { state.argsLayout };
    state.argsPipeline = cache.GetOrCreateComputePipeline("VSMArgs", argsDesc, nvDevice);

    nvrhi::ComputePipelineDesc resolveDesc;
    resolveDesc.CS = resolveResult.handle;
    resolveDesc.bindingLayouts = { state.resolveLayout };
    state.resolvePipeline = cache.GetOrCreateComputePipeline("VSMResolve", resolveDesc, nvDevice);

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
        || !state.binPipeline || !state.argsPipeline || !state.pagePipeline || !state.pageATPipeline || !state.resolvePipeline) {
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
    state.counter = MakeUAVBuffer(nvDevice, "VSM_Counter", sizeof(u32) * 4, sizeof(u32), true);
    state.pageTable = MakeUAVBuffer(nvDevice, "VSM_PageTable", u64(kVSMPageCount) * sizeof(u32), sizeof(u32), false);
    state.pageList = MakeUAVBuffer(nvDevice, "VSM_PageList", u64(kVSMStaticSlots) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.physTile = MakeUAVBuffer(nvDevice, "VSM_PhysTile", u64(kVSMStaticSlots) * sizeof(u32) * 2, sizeof(u32) * 2, false);
    state.slotDirty = MakeUAVBuffer(nvDevice, "VSM_SlotDirty", u64(kVSMStaticSlots) * sizeof(u32), sizeof(u32), false);
    state.dirtyList = MakeUAVBuffer(nvDevice, "VSM_DirtyList", u64(kVSMStaticSlots + 2) * sizeof(u32), sizeof(u32), false);
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_DrawClear";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.drawClear = nvDevice->createBuffer(desc);
    }
    state.binStats = MakeUAVBuffer(nvDevice, "VSM_BinStats", sizeof(u32) * 8, sizeof(u32), false);
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

    if (!state.needed || !state.counter || !state.pageTable || !state.pageList || !state.physTile
        || !state.slotDirty || !state.dirtyList || !state.drawClear || !state.atlas || !state.binStats) {
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
    state.markPages = std::min(words[0], kVSMPageCount);
    for (u32 L = 0; L < kVSMLevels; ++L) {
        u32 n = 0;
        const u32* level = words + 4 + L * kVSMPagesPerLevel;
        for (u32 i = 0; i < kVSMPagesPerLevel; ++i)
            n += level[i] != 0 ? 1 : 0;
        state.levelPages[L] = n;
    }
    state.dirtyPages = std::min(words[kReadbackResidOffset + 1], kVSMStaticSlots);
    state.wrongPages = std::min(words[kReadbackResidOffset + 2], kVSMStaticSlots);
    const u32* bin = words + kReadbackBinOffset;
    state.binDraws = bin[0];
    state.binInstances = bin[1];
    state.binMaxPages = bin[2];
    state.binLodCulled = bin[3];
    state.binDrops = bin[7];
    nvDevice->unmapBuffer(oldest);
}

void ScheduleReadback(nvrhi::ICommandList* cmdList, VSMState& state)
{
    nvrhi::IBuffer* slot = state.readback[state.readbackWrite];
    if (!slot)
        return;
    cmdList->copyBuffer(slot, 0, state.counter, 0, sizeof(u32) * 4);
    cmdList->copyBuffer(slot, sizeof(u32) * 4, state.needed, 0, u64(kVSMPageCount) * sizeof(u32));
    cmdList->copyBuffer(slot, u64(kReadbackResidOffset) * sizeof(u32), state.drawClear, 0, sizeof(u32) * 2);
    cmdList->copyBuffer(slot, u64(kReadbackResidOffset + 2) * sizeof(u32), state.dirtyList, u64(kVSMStaticSlots) * sizeof(u32), sizeof(u32) * 2);
    cmdList->copyBuffer(slot, u64(kReadbackBinOffset) * sizeof(u32), state.binStats, 0, sizeof(u32) * 8);
    state.readbackWrite = (state.readbackWrite + 1) % VSMState::kReadbackSlots;
    if (state.readbackScheduled < VSMState::kReadbackSlots)
        ++state.readbackScheduled;
}

void LogTelemetry(VSMState& state)
{
    if (ps_r_vsm_debug < 1)
        return;
    if (Device.dwTimeGlobal - state.lastLogTime < 2000)
        return;
    state.lastLogTime = Device.dwTimeGlobal;
    Msg("[VSM] mark: pages=%u | L0=%u L1=%u L2=%u L3=%u L4=%u L5=%u | sun %s step max %.3f deg | window snap max %u pages | base %.1f m | zc %.0f | inval %u",
        state.markPages, state.levelPages[0], state.levelPages[1], state.levelPages[2],
        state.levelPages[3], state.levelPages[4], state.levelPages[5],
        state.sunMoving ? "moving" : "static", state.sunStepMax, state.snapMax,
        ps_r_vsm_base, state.zCentre, state.invalidations);
    Msg("[VSM] static: dirty=%u/%u rendered | wrong=%u | cache %s refresh %d",
        state.dirtyPages, state.markPages, state.wrongPages,
        ps_r_vsm_cache ? "on" : "off", ps_r_vsm_cache_refresh);
    Msg("[VSM] bin: draws=%u instances=%u maxPagesPerCaster=%u lodCulled=%u drops=%u | k=%.2f",
        state.binDraws, state.binInstances, state.binMaxPages, state.binLodCulled, state.binDrops, ps_r_vsm_cluster_lod);
    state.sunStepMax = 0.0f;
    state.snapMax = 0;
}

void ExecuteMark(fg::RenderContext* ctx, const FrameGraph& fg, const VSMMarkData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
    if (!cmdList || !nvDevice || !depth)
        return;
    VSMZone zone(data.gpuProfiler, cmdList, "VSM/Mark");
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
    mp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    mp.markStep = markStep;
    mp.lodBias = 0;
    auto markCB = cache.GetOrCreateVolatileCB("VSM", "MarkParams", sizeof(VsmMarkParams), data.device);
    cmdList->writeBuffer(markCB, &mp, sizeof(mp));

    const u32 clearDraw[4] = { 6u, 0u, 0u, 0u };
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.counter, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::CopyDest);
    cmdList->clearBufferUInt(state.needed, 0);
    cmdList->clearBufferUInt(state.counter, 0);
    cmdList->clearBufferUInt(state.slotDirty, 0);
    cmdList->clearBufferUInt(state.dirtyList, 0);
    cmdList->writeBuffer(state.drawClear, clearDraw, sizeof(clearDraw));
    if (!state.physInit) {
        cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::CopyDest);
        cmdList->clearBufferUInt(state.physTile, 0xFFFFFFFFu);
        cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::UnorderedAccess);
        state.physInit = true;
    }
    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.counter, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::UnorderedAccess);

    BindingSetBuilder bsb(*markRefl, nvDevice, "VSM.Mark");
    bsb.ConstantBuffer("VsmParams", vsmCB)
       .ConstantBuffer("VsmMarkParams", markCB)
       .Texture("g_Depth", depth)
       .BufferUAV("g_Needed", state.needed)
       .BufferUAV("g_Counter", state.counter);
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
    VSMZone zone(data.gpuProfiler, cmdList, "VSM/Resid");

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_resid", ".cs");
    if (!refl)
        return;

    VsmResidParams rp = {};
    for (u32 L = 0; L < kVSMLevels; ++L) {
        rp.pageBase[2 * L] = state.pageBase[L][0];
        rp.pageBase[2 * L + 1] = state.pageBase[L][1];
    }
    rp.frame = state.frame;
    rp.refreshN = u32(std::max(ps_r_vsm_cache_refresh, 0));
    rp.sunMoving = state.sunMoving ? 1u : 0u;
    rp.forceDirty = ps_r_vsm_cache ? 0u : 1u;
    rp.inval[3] = ps_r_vsm_base / float(kVSMPagesAxis);
    rp.inval[7] = 0.0f;
    rp.inval[11] = 0.0f;
    rp.inval[15] = 4096.0f;
    auto residCB = cache.GetOrCreateVolatileCB("VSM", "ResidParams", sizeof(VsmResidParams), data.device);
    cmdList->writeBuffer(residCB, &rp, sizeof(rp));

    cmdList->setBufferState(state.pageTable, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.pageList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.physTile, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.slotDirty, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::UnorderedAccess);

    BindingSetBuilder bsb(*refl, nvDevice, "VSM.Resid");
    bsb.ConstantBuffer("VsmResidParams", residCB)
       .BufferSRV("g_Needed", state.needed)
       .BufferUAV("g_PageTable", state.pageTable)
       .BufferUAV("g_PageList", state.pageList)
       .BufferUAV("g_PhysTile", state.physTile)
       .BufferUAV("g_SlotDirty", state.slotDirty)
       .BufferUAV("g_DirtyList", state.dirtyList)
       .BufferUAV("g_DrawClear", state.drawClear);
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
}

void ExecuteBin(fg::RenderContext* ctx, const VSMBinData& data)
{
    VSMState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.binPipeline || !state.argsPipeline)
        return;
    VSMZone zone(data.gpuProfiler, cmdList, "VSM/Bins");

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* binRefl = shaderLoader->GetCachedReflection("vsm_bin_cluster", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("vsm_draw_args", ".cs");
    if (!binRefl || !argsRefl)
        return;

    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::CopyDest);
    cmdList->clearBufferUInt(state.binStats, 0);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::UnorderedAccess);
    for (u32 i = 0; i < kVSMStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::UnorderedAccess);

    const bool haveEntries = data.config.entryBuffer && data.config.entryCount > 0;
    if (haveEntries) {
        auto vsmCB = VsmParamsCB(data.device);

        VsmBinParams bp = {};
        bp.entryCount = data.config.entryCount;
        bp.includeAT = 0;
        bp.capOpaque = kVSMPairCapOpaque;
        bp.capTerrain = kVSMPairCapTerrain;
        bp.capAT = kVSMPairCapAT;
        bp.errK = std::max(0.1f, ps_r_vsm_cluster_lod);
        auto binCB = cache.GetOrCreateVolatileCB("VSM", "BinParams", sizeof(VsmBinParams), data.device);
        cmdList->writeBuffer(binCB, &bp, sizeof(bp));

        BindingSetBuilder bsb(*binRefl, nvDevice, "VSM.Bin");
        bsb.ConstantBuffer("VsmParams", vsmCB)
           .ConstantBuffer("VsmBinParams", binCB)
           .BufferSRV("g_Entries", data.config.entryBuffer)
           .BufferSRV("g_PageTable", state.pageTable)
           .BufferSRV("g_SlotDirty", state.slotDirty)
           .BufferUAV("g_Stats", state.binStats)
           .BufferUAV("g_PairsOpaque", state.pairs[0])
           .BufferUAV("g_PairsTerrain", state.pairs[1])
           .BufferUAV("g_PairsAT", state.pairs[2]);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.binLayout, nvDevice);
        if (bindingSet) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binPipeline;
            cs.bindings = { bindingSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.config.entryCount + 63) / 64, 1, 1);
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

    cmdList->setBufferState(state.needed, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.counter, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::CopySource);
    cmdList->setBufferState(state.binStats, nvrhi::ResourceStates::CopySource);
    ScheduleReadback(cmdList, state);
    cmdList->setBufferState(state.drawClear, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
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
    VSMZone zone(data.gpuProfiler, cmdList, "VSM/Static");

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
    VSMZone zone(data.gpuProfiler, cmdList, "VSM/Resolve");

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("vsm_resolve", ".cs");
    if (!refl)
        return;

    auto vsmCB = VsmParamsCB(data.device);

    VsmResolveParams rp = {};
    rp.invViewProj = Device.mInvFullTransform;
    rp.prevViewProj = state.prevViewProj;
    rp.prevCamPos.set(state.prevCamPos.x, state.prevCamPos.y, state.prevCamPos.z, 0.0f);
    rp.curCamPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.0f);
    rp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    rp.params.set(0.0f, 0.05f, 0.0f, 0.0f);
    rp.params2.set(0.0f, 0.0f, 0.0f, 0.0f);
    rp.params3.set(0.0f, 0.0f, 0.0f, 0.0f);
    rp.params4.set(float(state.resolveCount & 63u), 0.0f, 0.0f, 0.0f);
    auto resolveCB = cache.GetOrCreateVolatileCB("VSM", "ResolveParams", sizeof(VsmResolveParams), data.device);
    cmdList->writeBuffer(resolveCB, &rp, sizeof(rp));

    BindingSetBuilder bsb(*refl, nvDevice, "VSM.Resolve");
    bsb.ConstantBuffer("VsmParams", vsmCB)
       .ConstantBuffer("VsmResolveParams", resolveCB)
       .Texture("g_Depth", depth)
       .Texture("g_Atlas", atlas)
       .BufferSRV("g_PageTable", state.pageTable)
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
    dp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
    dp.mode = u32(ps_r_vsm_debug);
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

void InvalidateVSMCache(VSMState& state)
{
    state.pageBasePrevValid = false;
    state.prevSunValid = false;
    state.zCentreValid = false;
    state.physInit = false;
    state.invalidations++;
}

void VSMBeginFrame(VSMState& state, const Fvector& camPos, const Fvector& sunDirIn)
{
    state.frame++;

    Fvector sd = sunDirIn;
    if (sd.magnitude() < 1e-4f)
        sd.set(0.0f, -1.0f, 0.0f);
    sd.normalize();
    Fvector up;
    up.set(0.0f, 1.0f, 0.0f);
    if (_abs(sd.y) > 0.99f)
        up.set(0.0f, 0.0f, 1.0f);
    Fvector eye;
    eye.set(0.0f, 0.0f, 0.0f);
    Fmatrix view;
    view.build_camera_dir(eye, sd, up);
    state.sunView = view;

    Fvector camL;
    view.transform_tiny(camL, camPos);

    state.sunMoving = !state.prevSunValid || sd.x != state.prevSunDir.x || sd.y != state.prevSunDir.y || sd.z != state.prevSunDir.z;
    if (state.prevSunValid) {
        float dp = sd.dotproduct(state.prevSunDir);
        dp = dp > 1.0f ? 1.0f : (dp < -1.0f ? -1.0f : dp);
        const float stepDeg = acosf(dp) * 57.29578f;
        if (stepDeg > state.sunStepMax)
            state.sunStepMax = stepDeg;
    }
    state.prevSunDir = sd;
    state.prevSunValid = true;

    VsmParams& params = state.params;
    params.view = view;
    for (u32 L = 0; L < kVSMLevels; ++L) {
        const float ext = ps_r_vsm_base * float(1u << L);
        const float texel = ext / float(kVSMVirtualRes);
        const float pageTexel = texel * float(kVSMPageSize);
        const float baseX = camL.x - 0.5f * ext;
        const float baseY = camL.y - 0.5f * ext;
        const s32 pbx = (s32)floorf(baseX / pageTexel);
        const s32 pby = (s32)floorf(baseY / pageTexel);
        state.pageBase[L][0] = pbx;
        state.pageBase[L][1] = pby;
        params.level[L].set(float(pbx) * pageTexel, float(pby) * pageTexel, ext, 0.0f);
    }
    if (state.pageBasePrevValid) {
        u32 mx = 0;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            const u32 d = (u32)(_abs(state.pageBase[L][0] - state.pageBasePrev[L][0]) + _abs(state.pageBase[L][1] - state.pageBasePrev[L][1]));
            if (d > mx)
                mx = d;
        }
        if (mx > state.snapMax)
            state.snapMax = mx;
    }
    for (u32 L = 0; L < kVSMLevels; ++L) {
        state.pageBasePrev[L][0] = state.pageBase[L][0];
        state.pageBasePrev[L][1] = state.pageBase[L][1];
    }
    state.pageBasePrevValid = true;

    const float zCentre = floorf(camL.z / kVSMZSnap) * kVSMZSnap;
    if (!state.zCentreValid || zCentre != state.zCentre) {
        if (state.zCentreValid)
            InvalidateVSMCache(state);
        state.zCentre = zCentre;
        state.zCentreValid = true;
    }
    params.zparams.set(zCentre + kVSMZNear, 1.0f / (kVSMZFar - kVSMZNear), ps_r_vsm_bias, ps_r_vsm_bias_dyn);
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
        if (live)
            InvalidateVSMCache(*state);
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
    VirtualResourceHandle dirtyHandle = fg.ImportBuffer("vsm_dirty_list", state->dirtyList, bufferDesc("vsm_dirty_list", u64(kVSMStaticSlots + 2) * sizeof(u32), sizeof(u32)));
    VirtualResourceHandle clearHandle = fg.ImportBuffer("vsm_draw_clear", state->drawClear, bufferDesc("vsm_draw_clear", sizeof(u32) * 4, 0));
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
            data.atlas = passBuilder.read(atlasData.atlas, ResourceState::ShaderResource);
            data.mask = passBuilder.write(maskHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMResolveData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteResolve(ctx, fg, data);
        });
    fg.GetRTRegistry().RegisterRT("rt_VSMMask", resolveData.mask);
    out.mask = resolveData.mask;

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
                data.dirtyList = passBuilder.read(residData.dirtyList, ResourceState::ShaderResource);
                data.mask = passBuilder.read(resolveData.mask, ResourceState::ShaderResource);
                data.output = passBuilder.write(viewHandle, ResourceState::UnorderedAccess);
            },
            [](const VSMDebugData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                ExecuteDebugView(ctx, fg, data);
            });
        out.debugView = debugData.output;
    }

    return out;
}

}
