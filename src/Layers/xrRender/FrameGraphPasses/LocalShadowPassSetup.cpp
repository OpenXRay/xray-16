#include "stdafx.h"
#include "LocalShadowPassSetup.h"
#include "PassCommon.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "xrCDB/Frustum.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

const Fvector kFaceDir[6] = { { 1.f, 0.f, 0.f }, { -1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, -1.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f, -1.f } };
const Fvector kFaceUp[6] = { { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f } };

constexpr u32 kLocalPairCaps[kLocalStreamCount] = { kLocalPairCapOpaque, kLocalPairCapTerrain, kLocalPairCapAT,
    kLocalPairCapDynOpaque, kLocalPairCapDynAT, kLocalPairCapSkinned };
constexpr const char* kLocalStreamNames[kLocalStreamCount] = { "Opaque", "Terrain", "AT", "DynOpaque", "DynAT", "Skinned" };
constexpr u32 kLocalArgsStride = sizeof(u32) * 4;

struct LocalShadowBinParams {
    u32 candCount;
    u32 nodeCount;
    u32 includeAT;
    float errK;
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    u32 budget;
    u32 frame;
    u32 pad[3];
};
static_assert(sizeof(LocalShadowBinParams) == 48, "LocalShadowBinParams is shader-visible");

struct LocalShadowDynBinParams {
    u32 entryBase;
    u32 entryCount;
    u32 statsBase;
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    u32 includeAT;
    u32 pad;
};
static_assert(sizeof(LocalShadowDynBinParams) == 32, "LocalShadowDynBinParams is shader-visible");

struct LocalShadowRouteParams {
    u32 pancake;
    u32 pad[3];
};

struct LocalShadowArgsParams {
    u32 caps[8];
};
static_assert(sizeof(LocalShadowArgsParams) == 32, "LocalShadowArgsParams is shader-visible");

struct LocalShadowBinData {
    VirtualResourceHandle tiles;
    VirtualResourceHandle args;
    VirtualResourceHandle clearArgs;
    VirtualResourceHandle dirtyList;
    VirtualResourceHandle refreshDyn;
    VirtualResourceHandle order;
    LocalShadowState* state;
    fg::RenderDevice* device;
    LocalShadowConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct LocalShadowStaticData {
    VirtualResourceHandle atlas;
    VirtualResourceHandle tiles;
    VirtualResourceHandle args;
    VirtualResourceHandle clearArgs;
    VirtualResourceHandle dirtyList;
    LocalShadowState* state;
    fg::RenderDevice* device;
    LocalShadowConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct LocalShadowDynData {
    VirtualResourceHandle atlas;
    VirtualResourceHandle tiles;
    VirtualResourceHandle args;
    VirtualResourceHandle clearArgs;
    VirtualResourceHandle refreshDyn;
    LocalShadowState* state;
    fg::RenderDevice* device;
    LocalShadowConfig config;
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

nvrhi::BufferHandle MakeArgsBuffer(nvrhi::IDevice* nvDevice, const char* name, u32 count)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = u64(count) * kLocalArgsStride;
    desc.canHaveUAVs = true;
    desc.canHaveRawViews = true;
    desc.isDrawIndirectArgs = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return nvDevice->createBuffer(desc);
}

nvrhi::TextureHandle MakeAtlas(nvrhi::IDevice* nvDevice, const char* name)
{
    nvrhi::TextureDesc desc;
    desc.width = kLocalShadowAtlas;
    desc.height = kLocalShadowAtlas;
    desc.format = nvrhi::Format::D16;
    desc.debugName = name;
    desc.isShaderResource = true;
    desc.isRenderTarget = true;
    desc.isTypeless = true;
    desc.useClearValue = true;
    desc.clearValue = nvrhi::Color(0.0f);
    desc.initialState = nvrhi::ResourceStates::DepthWrite;
    desc.keepInitialState = true;
    return nvDevice->createTexture(desc);
}

nvrhi::IBuffer* FallbackTiles(nvrhi::IDevice* nvDevice)
{
    static nvrhi::BufferHandle s_tiles;
    if (!s_tiles) {
        nvrhi::BufferDesc desc;
        desc.debugName = "LocalShadow_TilesFallback";
        desc.byteSize = u64(kLocalTileCount) * sizeof(LocalShadowViewGPU);
        desc.structStride = sizeof(LocalShadowViewGPU);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_tiles = nvDevice->createBuffer(desc);
    }
    return s_tiles;
}

bool EnsureResources(nvrhi::IDevice* nvDevice, LocalShadowState& state)
{
    if (state.stateBuffer && state.staticAtlas)
        return true;
    if (state.resourcesFailed)
        return false;

    auto makePlainBuffer = [&](const char* name, u64 bytes, u32 stride) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = bytes;
        desc.structStride = stride;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };
    state.requestBuffer = makePlainBuffer("LocalShadow_Request",
        u64(kLocalTileCount) * sizeof(LocalShadowViewGPU), sizeof(LocalShadowViewGPU));
    state.candListBuffer = makePlainBuffer("LocalShadow_CandList",
        u64(kLocalTileCount) * sizeof(u32) * 4, sizeof(u32) * 4);
    state.stateBuffer = MakeUAVBuffer(nvDevice, "LocalShadow_State",
        u64(kLocalTileCount) * sizeof(LocalShadowViewGPU), sizeof(LocalShadowViewGPU), false);
    state.tileCount = MakeUAVBuffer(nvDevice, "LocalShadow_TileCount", u64(kLocalTileCount) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.schedule = MakeUAVBuffer(nvDevice, "LocalShadow_Schedule", u64(kLocalTileCount) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.dirtyList = MakeUAVBuffer(nvDevice, "LocalShadow_DirtyList", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), false);
    state.refreshDynBuffer = MakeUAVBuffer(nvDevice, "LocalShadow_RefreshDyn", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), false);
    state.pairBase = MakeUAVBuffer(nvDevice, "LocalShadow_PairBase", u64(kLocalTileCount) * sizeof(u32) * 4, sizeof(u32) * 4, false);
    state.emitArgs = MakeArgsBuffer(nvDevice, "LocalShadow_EmitArgs", 1);
    state.stats = MakeUAVBuffer(nvDevice, "LocalShadow_Stats", u64(kLocalStatWords) * sizeof(u32), sizeof(u32), false);
    for (u32 i = 0; i < kLocalStreamCount; ++i) {
        string64 nm;
        xr_sprintf(nm, "LocalShadow_Pairs%s", kLocalStreamNames[i]);
        state.pairs[i] = MakeUAVBuffer(nvDevice, nm, u64(kLocalPairCaps[i]) * sizeof(u32) * 2, sizeof(u32) * 2, false);
        state.pairCapacity[i] = kLocalPairCaps[i];
    }
    state.args = MakeArgsBuffer(nvDevice, "LocalShadow_Args", kLocalStreamCount);
    state.clearArgs = MakeArgsBuffer(nvDevice, "LocalShadow_ClearArgs", 2);
    for (u32 i = 0; i < LocalShadowState::kReadbackSlots; ++i) {
        nvrhi::BufferDesc desc;
        desc.debugName = "LocalShadow_Readback";
        desc.byteSize = u64(kLocalStatWords) * sizeof(u32);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        state.readback[i] = nvDevice->createBuffer(desc);
    }
    state.readbackWrite = 0;
    state.readbackScheduled = 0;
    state.stateReset = true;
    state.staticAtlas = MakeAtlas(nvDevice, "LocalShadow_Static");
    state.dynAtlas = MakeAtlas(nvDevice, "LocalShadow_Dyn");

    bool ok = state.requestBuffer && state.candListBuffer && state.stateBuffer && state.tileCount
        && state.schedule && state.dirtyList && state.refreshDynBuffer && state.pairBase && state.emitArgs
        && state.stats && state.args && state.clearArgs && state.staticAtlas && state.dynAtlas;
    for (u32 i = 0; i < kLocalStreamCount; ++i)
        ok = ok && state.pairs[i];
    if (!ok) {
        Msg("! [LocalShadow] resource creation failed");
        state.stateBuffer = nullptr;
        state.staticAtlas = nullptr;
        state.resourcesFailed = true;
        return false;
    }
    Msg("* [LocalShadow] atlases %ux%u D16, %u spot tiles + %u point faces", kLocalShadowAtlas, kLocalShadowAtlas, kLocalSpotSlots, kLocalPointSlots);
    return true;
}

bool EnsurePipelines(fg::RenderDevice* device, LocalShadowState& state)
{
    if (state.binCountPipeline && state.binReservePipeline && state.binEmitPipeline && state.binDynPipeline
        && state.argsPipeline && state.clearPipeline && state.pagePipeline && state.pageATPipeline && state.skinPagePipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto countResult = shaderLoader->LoadComputeShader("local_shadow_bin_count");
    auto reserveResult = shaderLoader->LoadComputeShader("local_shadow_bin_reserve");
    auto emitResult = shaderLoader->LoadComputeShader("local_shadow_bin_emit");
    auto binResult = shaderLoader->LoadComputeShader("local_shadow_bin_dyn");
    auto argsResult = shaderLoader->LoadComputeShader("local_shadow_args");
    auto clearVsResult = shaderLoader->LoadVertexShader("local_shadow_clear", "main");
    auto clearPsResult = shaderLoader->LoadPixelShader("vsm_clear", "main");
    auto pageVsResult = shaderLoader->LoadVertexShader("local_shadow_pull", "main");
    auto skinVsResult = shaderLoader->LoadVertexShader("local_shadow_pull_skinned", "main");
    auto pagePsResult = shaderLoader->LoadPixelShader("vsm_page", "main");
    auto pageATPsResult = shaderLoader->LoadPixelShader("vsm_page_at", "main");
    if (!countResult.handle || !countResult.reflection || !reserveResult.handle || !reserveResult.reflection
        || !emitResult.handle || !emitResult.reflection
        || !binResult.handle || !binResult.reflection || !argsResult.handle || !argsResult.reflection
        || !clearVsResult.handle || !clearVsResult.reflection || !clearPsResult.handle || !clearPsResult.reflection
        || !pageVsResult.handle || !pageVsResult.reflection || !pagePsResult.handle || !pagePsResult.reflection
        || !skinVsResult.handle || !skinVsResult.reflection
        || !pageATPsResult.handle || !pageATPsResult.reflection) {
        Msg("! [LocalShadow] shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }
    state.pageVS = pageVsResult.handle;
    state.skinPageVS = skinVsResult.handle;

    auto& cache = GetPassResourceCache();
    state.binCountLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinCount", *countResult.reflection, nvDevice);
    state.binReserveLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinReserve", *reserveResult.reflection, nvDevice);
    state.binEmitLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinEmit", *emitResult.reflection, nvDevice);
    state.binDynLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinDyn", *binResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowArgs", *argsResult.reflection, nvDevice);
    state.clearLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowClear", *clearVsResult.reflection, *clearPsResult.reflection, nvDevice);
    state.pageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPage", *pageVsResult.reflection, *pagePsResult.reflection, nvDevice);
    state.pageATLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPageAT", *pageVsResult.reflection, *pageATPsResult.reflection, nvDevice);
    state.skinPageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowSkinPage", *skinVsResult.reflection, *pagePsResult.reflection, nvDevice);
    if (!state.binCountLayout || !state.binReserveLayout || !state.binEmitLayout || !state.binDynLayout
        || !state.argsLayout || !state.clearLayout || !state.pageLayout || !state.pageATLayout || !state.skinPageLayout) {
        state.pipelinesFailed = true;
        return false;
    }

    auto makeComputePipeline = [&](const char* name, nvrhi::IShader* cs, nvrhi::IBindingLayout* layout) {
        nvrhi::ComputePipelineDesc desc;
        desc.CS = cs;
        desc.bindingLayouts = { layout };
        return cache.GetOrCreateComputePipeline(name, desc, nvDevice);
    };
    state.binCountPipeline = makeComputePipeline("LocalShadowBinCount", countResult.handle, state.binCountLayout);
    state.binReservePipeline = makeComputePipeline("LocalShadowBinReserve", reserveResult.handle, state.binReserveLayout);
    state.binEmitPipeline = makeComputePipeline("LocalShadowBinEmit", emitResult.handle, state.binEmitLayout);
    state.binDynPipeline = makeComputePipeline("LocalShadowBinDyn", binResult.handle, state.binDynLayout);

    nvrhi::ComputePipelineDesc argsDesc;
    argsDesc.CS = argsResult.handle;
    argsDesc.bindingLayouts = { state.argsLayout };
    state.argsPipeline = cache.GetOrCreateComputePipeline("LocalShadowArgs", argsDesc, nvDevice);

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
    state.clearPipeline = cache.GetOrCreatePipeline("LocalShadowClear", clearDesc, fbInfo, nvDevice);

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto makePageDesc = [&](nvrhi::IShader* vs, nvrhi::IShader* ps, nvrhi::IBindingLayout* layout, bool withBindless) {
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
        return desc;
    };
    state.pagePipeline = cache.GetOrCreatePipeline("LocalShadowPage", makePageDesc(state.pageVS, pagePsResult.handle, state.pageLayout, false), fbInfo, nvDevice);
    state.pageATPipeline = cache.GetOrCreatePipeline("LocalShadowPageAT", makePageDesc(state.pageVS, pageATPsResult.handle, state.pageATLayout, true), fbInfo, nvDevice);
    state.skinPagePipeline = cache.GetOrCreatePipeline("LocalShadowSkinPage", makePageDesc(state.skinPageVS, pagePsResult.handle, state.skinPageLayout, false), fbInfo, nvDevice);

    if (!state.binCountPipeline || !state.binReservePipeline || !state.binEmitPipeline || !state.binDynPipeline
        || !state.argsPipeline || !state.clearPipeline || !state.pagePipeline || !state.pageATPipeline || !state.skinPagePipeline) {
        Msg("! [LocalShadow] pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }
    Msg("* [LocalShadow] pipelines initialized");
    return true;
}

void ExecuteBin(fg::RenderContext* ctx, const FrameGraph& fg, const LocalShadowBinData& data)
{
    LocalShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice)
        return;
    if (!EnsurePipelines(data.device, state))
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* countRefl = shaderLoader->GetCachedReflection("local_shadow_bin_count", ".cs");
    auto* reserveRefl = shaderLoader->GetCachedReflection("local_shadow_bin_reserve", ".cs");
    auto* emitRefl = shaderLoader->GetCachedReflection("local_shadow_bin_emit", ".cs");
    auto* binRefl = shaderLoader->GetCachedReflection("local_shadow_bin_dyn", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("local_shadow_args", ".cs");
    if (!countRefl || !reserveRefl || !emitRefl || !binRefl || !argsRefl)
        return;

    if (data.gpuProfiler)
        data.gpuProfiler->BeginPass(cmdList, "Local Shadow.Bin");

    const LocalShadowConfig& cfg = data.config;
    GPUCullingManager* gpuCulling = cfg.gpuCulling;
    const bool haveEntries = gpuCulling && cfg.entryBuffer && gpuCulling->GetClusterEntryCount() > 0
        && cfg.bvhNodeBuffer && cfg.bvhIndexBuffer && cfg.bvhNodeCount > 0;
    if (haveEntries && (cfg.entryBuffer != state.lastEntryBuffer || cfg.bvhNodeBuffer != state.lastBvhNodeBuffer))
        state.stateReset = true;
    state.lastEntryBuffer = cfg.entryBuffer;
    state.lastBvhNodeBuffer = cfg.bvhNodeBuffer;

    if (state.stateReset) {
        cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::CopyDest);
        cmdList->setBufferState(state.schedule, nvrhi::ResourceStates::CopyDest);
        cmdList->clearBufferUInt(state.stateBuffer, 0);
        cmdList->clearBufferUInt(state.schedule, 0);
        state.stateReset = false;
    }

    cmdList->setBufferState(state.requestBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.candListBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(state.requestBuffer, state.request, sizeof(state.request));
    cmdList->writeBuffer(state.candListBuffer, state.candList, sizeof(state.candList));
    cmdList->clearBufferUInt(state.stats, 0);
    cmdList->setBufferState(state.requestBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.candListBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::UnorderedAccess);
    for (u32 i = 0; i < kLocalStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::UnorderedAccess);

    LocalShadowBinParams bp = {};
    bp.candCount = state.candCount;
    bp.nodeCount = cfg.bvhNodeCount;
    bp.includeAT = ps_r_vsm_at ? 1u : 0u;
    bp.errK = std::max(0.1f, ps_r_vsm_cluster_lod);
    bp.capOpaque = state.pairCapacity[0];
    bp.capTerrain = state.pairCapacity[1];
    bp.capAT = state.pairCapacity[2];
    bp.budget = u32(std::max(1, ps_r_local_shadow_pairs_budget));
    bp.frame = state.frame;
    auto binCB = cache.GetOrCreateVolatileCB("LocalShadow", "BinParams", sizeof(LocalShadowBinParams), data.device, 64);
    cmdList->writeBuffer(binCB, &bp, sizeof(bp));

    bool counted = false;
    if (haveEntries && state.candCount > 0) {
        cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.tileCount, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(cfg.entryBuffer, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(cfg.bvhNodeBuffer, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(cfg.bvhIndexBuffer, nvrhi::ResourceStates::ShaderResource);
        BindingSetBuilder cbs(*countRefl, nvDevice, "LocalShadow.BinCount");
        cbs.ConstantBuffer("LocalShadowBinParams", binCB)
           .BufferSRV("g_Entries", cfg.entryBuffer)
           .BufferSRV("g_Request", state.requestBuffer)
           .BufferSRV("g_CandList", state.candListBuffer)
           .BufferSRV("g_TileState", state.stateBuffer)
           .BufferSRV("g_BvhNodes", cfg.bvhNodeBuffer)
           .BufferSRV("g_BvhIndex", cfg.bvhIndexBuffer)
           .BufferUAV("g_TileCount", state.tileCount);
        if (auto countSet = cache.GetOrCreateBindingSet(cbs.Build(), state.binCountLayout, nvDevice)) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binCountPipeline;
            cs.bindings = { countSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch(state.candCount, 1, 1);
            counted = true;
        }
    }
    if (!counted) {
        cmdList->setBufferState(state.tileCount, nvrhi::ResourceStates::CopyDest);
        cmdList->clearBufferUInt(state.tileCount, 0);
    }

    cmdList->setBufferState(state.tileCount, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.schedule, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.pairBase, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.emitArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.clearArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::UnorderedAccess);
    {
        BindingSetBuilder rbs(*reserveRefl, nvDevice, "LocalShadow.BinReserve");
        rbs.ConstantBuffer("LocalShadowBinParams", binCB)
           .BufferSRV("g_Request", state.requestBuffer)
           .BufferSRV("g_CandList", state.candListBuffer)
           .BufferSRV("g_TileCount", state.tileCount)
           .BufferUAV("g_TileState", state.stateBuffer)
           .BufferUAV("g_Schedule", state.schedule)
           .BufferUAV("g_DirtyList", state.dirtyList)
           .BufferUAV("g_RefreshDyn", state.refreshDynBuffer)
           .BufferUAV("g_PairBase", state.pairBase)
           .BufferUAV("g_EmitArgs", state.emitArgs)
           .BufferUAV("g_ClearArgs", state.clearArgs)
           .BufferUAV("g_Stats", state.stats);
        if (auto reserveSet = cache.GetOrCreateBindingSet(rbs.Build(), state.binReserveLayout, nvDevice)) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binReservePipeline;
            cs.bindings = { reserveSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch(1, 1, 1);
        }
    }

    if (haveEntries) {
        cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.pairBase, nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(state.emitArgs, nvrhi::ResourceStates::IndirectArgument);
        BindingSetBuilder ebs(*emitRefl, nvDevice, "LocalShadow.BinEmit");
        ebs.ConstantBuffer("LocalShadowBinParams", binCB)
           .BufferSRV("g_Entries", cfg.entryBuffer)
           .BufferSRV("g_TileState", state.stateBuffer)
           .BufferSRV("g_DirtyList", state.dirtyList)
           .BufferSRV("g_PairBase", state.pairBase)
           .BufferSRV("g_BvhNodes", cfg.bvhNodeBuffer)
           .BufferSRV("g_BvhIndex", cfg.bvhIndexBuffer)
           .BufferUAV("g_Stats", state.stats)
           .BufferUAV("g_PairsOpaque", state.pairs[0])
           .BufferUAV("g_PairsTerrain", state.pairs[1])
           .BufferUAV("g_PairsAT", state.pairs[2]);
        if (auto emitSet = cache.GetOrCreateBindingSet(ebs.Build(), state.binEmitLayout, nvDevice)) {
            nvrhi::ComputeState cs;
            cs.pipeline = state.binEmitPipeline;
            cs.bindings = { emitSet };
            cs.indirectParams = state.emitArgs;
            cmdList->setComputeState(cs);
            cmdList->dispatchIndirect(0);
        }
    }

    cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::UnorderedAccess);

    auto dispatchDyn = [&](nvrhi::IBuffer* entries, u32 entryBase, u32 entryCount, u32 statsBase,
                           u32 streamOpaque, u32 streamTerrain, u32 streamAT,
                           u32 capOpaque, u32 capTerrain, u32 capAT, const char* label) {
        if (!entries || entryCount == 0)
            return;
        LocalShadowDynBinParams dp = {};
        dp.entryBase = entryBase;
        dp.entryCount = entryCount;
        dp.statsBase = statsBase;
        dp.capOpaque = capOpaque;
        dp.capTerrain = capTerrain;
        dp.capAT = capAT;
        dp.includeAT = ps_r_vsm_at ? 1u : 0u;
        auto dynCB = cache.GetOrCreateVolatileCB("LocalShadow", "DynBinParams", sizeof(LocalShadowDynBinParams), data.device, 64);
        cmdList->writeBuffer(dynCB, &dp, sizeof(dp));
        cmdList->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);

        BindingSetBuilder bsb(*binRefl, nvDevice, label);
        bsb.ConstantBuffer("LocalShadowDynBinParams", dynCB)
           .BufferSRV("g_Entries", entries)
           .BufferSRV("g_Tiles", state.stateBuffer)
           .BufferSRV("g_Refresh", state.refreshDynBuffer)
           .BufferUAV("g_Stats", state.stats)
           .BufferUAV("g_PairsOpaque", state.pairs[streamOpaque])
           .BufferUAV("g_PairsTerrain", state.pairs[streamTerrain])
           .BufferUAV("g_PairsAT", state.pairs[streamAT]);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.binDynLayout, nvDevice);
        if (!bindingSet)
            return;
        nvrhi::ComputeState cs;
        cs.pipeline = state.binDynPipeline;
        cs.bindings = { bindingSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch((entryCount + 63) / 64, 1, 1);
    };

    if (gpuCulling) {
        if (cfg.entryBuffer)
            dispatchDyn(cfg.entryBuffer, gpuCulling->GetClusterEntryCount(), gpuCulling->GetDynamicClusterEntryCount(), 16u,
                3, 5, 4, state.pairCapacity[3], 0u, state.pairCapacity[4], "LocalShadow.BinDyn");
        dispatchDyn(gpuCulling->GetSkinnedEntryBuffer(), 0, gpuCulling->GetSkinnedEntryCount(), 24u,
            5, 1, 2, state.pairCapacity[5], 0u, 0u, "LocalShadow.BinSkinned");
    }

    LocalShadowArgsParams ap = {};
    ap.caps[0] = state.pairCapacity[0];
    ap.caps[1] = state.pairCapacity[1];
    ap.caps[2] = state.pairCapacity[2];
    ap.caps[3] = state.pairCapacity[3];
    ap.caps[4] = state.pairCapacity[4];
    ap.caps[5] = state.pairCapacity[5];
    auto argsCB = cache.GetOrCreateVolatileCB("LocalShadow", "ArgsParams", sizeof(LocalShadowArgsParams), data.device, 64);
    cmdList->writeBuffer(argsCB, &ap, sizeof(ap));
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::ShaderResource);

    BindingSetBuilder abs(*argsRefl, nvDevice, "LocalShadow.Args");
    abs.ConstantBuffer("LocalShadowArgsParams", argsCB)
       .BufferSRV("g_Stats", state.stats)
       .BufferUAV("g_Args", state.args);
    if (auto argsSet = cache.GetOrCreateBindingSet(abs.Build(), state.argsLayout, nvDevice)) {
        nvrhi::ComputeState cs;
        cs.pipeline = state.argsPipeline;
        cs.bindings = { argsSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch(1, 1, 1);
    }

    for (u32 i = 0; i < kLocalStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.clearArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::ShaderResource);

    if (nvrhi::IBuffer* slot = state.readback[state.readbackWrite]) {
        cmdList->setBufferState(state.stats, nvrhi::ResourceStates::CopySource);
        cmdList->copyBuffer(slot, 0, state.stats, 0, u64(kLocalStatWords) * sizeof(u32));
        state.readbackWrite = (state.readbackWrite + 1) % LocalShadowState::kReadbackSlots;
        if (state.readbackScheduled < LocalShadowState::kReadbackSlots)
            ++state.readbackScheduled;
    }

    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Bin");
}

struct LocalDrawContext {
    nvrhi::ICommandList* cmdList;
    nvrhi::IDevice* nvDevice;
    nvrhi::IFramebuffer* framebuffer;
    nvrhi::IBindingSet* bindlessTable;
    nvrhi::IBuffer* routeCB;
    nvrhi::Viewport viewport;
    nvrhi::Rect scissor;
};

bool BeginAtlasPass(fg::RenderContext* ctx, const LocalShadowConfig& cfg, LocalShadowState& state,
                    nvrhi::ITexture* atlas, fg::RenderDevice* device, const char* fbName, u32 clearIndex,
                    bool pancake, LocalDrawContext& out)
{
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* clearVsRefl = shaderLoader->GetCachedReflection("local_shadow_clear", ".vs");
    auto* clearPsRefl = shaderLoader->GetCachedReflection("vsm_clear", ".ps");
    if (!clearVsRefl || !clearPsRefl)
        return false;

    auto& matBuffer = bindless::MaterialBuffer::Instance();
    auto srv = [&](nvrhi::IBuffer* b) {
        if (b)
            cmdList->setBufferState(b, nvrhi::ResourceStates::ShaderResource);
    };
    srv(state.stateBuffer);
    srv(state.dirtyList);
    srv(state.refreshDynBuffer);
    srv(cfg.entryBuffer);
    srv(cfg.staticInstanceBuffer);
    srv(cfg.terrainInstanceBuffer);
    srv(cfg.dynamicInstanceBuffer);
    srv(cfg.megaVertexBuffer);
    srv(cfg.megaIndexBuffer);
    srv(matBuffer.GetBuffer());
    if (cfg.gpuCulling) {
        srv(cfg.gpuCulling->GetSkinnedEntryBuffer());
        srv(cfg.gpuCulling->GetSkinnedPreVertexBuffer());
        srv(cfg.gpuCulling->GetSkinnedPools().GetCombinedIndexBuffer());
    }
    for (u32 i = 0; i < kLocalStreamCount; ++i)
        srv(state.pairs[i]);
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.clearArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setTextureState(atlas, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthWrite);
    cmdList->commitBarriers();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(atlas);
    auto framebuffer = cache.GetOrCreateFramebuffer(fbName, fbDesc, nvDevice);
    if (!framebuffer)
        return false;

    LocalShadowRouteParams rp = {};
    rp.pancake = pancake ? 1u : 0u;
    auto routeCB = cache.GetOrCreateVolatileCB("LocalShadow", "RouteParams", sizeof(LocalShadowRouteParams), device, 8);
    cmdList->writeBuffer(routeCB, &rp, sizeof(rp));

    auto* backend = device->GetBackend();
    out.routeCB = routeCB;
    out.cmdList = cmdList;
    out.nvDevice = nvDevice;
    out.framebuffer = framebuffer;
    out.bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
    out.viewport = nvrhi::Viewport(0.0f, float(kLocalShadowAtlas), 0.0f, float(kLocalShadowAtlas), 0.0f, 1.0f);
    out.scissor = nvrhi::Rect(kLocalShadowAtlas, kLocalShadowAtlas);

    BindingSetBuilder bsb(*clearVsRefl, *clearPsRefl, nvDevice, "LocalShadow.Clear");
    bsb.BufferSRV("g_Refresh", clearIndex == 0 ? state.dirtyList : state.refreshDynBuffer);
    bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
    if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.clearLayout, nvDevice)) {
        nvrhi::GraphicsState gs;
        gs.pipeline = state.clearPipeline;
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        gs.indirectParams = state.clearArgs;
        gs.viewport.addViewport(out.viewport);
        gs.viewport.addScissorRect(out.scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(clearIndex * kLocalArgsStride, 1);
    }
    return true;
}

void ExecuteStatic(fg::RenderContext* ctx, const FrameGraph& fg, const LocalShadowStaticData& data)
{
    LocalShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.atlas);
    if (!cmdList || !nvDevice || !atlas || !state.clearPipeline || !state.pagePipeline)
        return;

    if (state.staticAtlasFirst) {
        cmdList->clearDepthStencilTexture(atlas, nvrhi::AllSubresources, true, 0.0f, false, 0);
        state.staticAtlasFirst = false;
    }

    const LocalShadowConfig& cfg = data.config;
    const bool haveCasters = cfg.entryBuffer && cfg.megaVertexBuffer && cfg.megaIndexBuffer;

    if (cfg.materialCache)
        cfg.materialCache->FinalizePendingMaterials(ctx);
    bindless::MaterialBuffer::Instance().Upload(ctx);

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("local_shadow_pull", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_page", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!vsRefl || !psRefl || !atRefl)
        return;

    if (data.gpuProfiler)
        data.gpuProfiler->BeginPass(cmdList, "Local Shadow.Static");

    LocalDrawContext dc;
    if (!BeginAtlasPass(ctx, cfg, state, atlas, data.device, "LocalShadowStatic", 0, false, dc)) {
        if (data.gpuProfiler)
            data.gpuProfiler->EndPass(cmdList, "Local Shadow.Static");
        return;
    }

    auto& matBuffer = bindless::MaterialBuffer::Instance();
    auto drawStream = [&](u32 stream, nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingLayout* layout,
                          const ExtractedReflection& ps, nvrhi::IBuffer* instanceBuffer, bool withBindless, const char* label) {
        if (!pipeline || !layout || !instanceBuffer)
            return;
        BindingSetBuilder bsb(*vsRefl, ps, nvDevice, label);
        bsb.ConstantBuffer("LocalShadowRouteParams", dc.routeCB);
        bsb.BufferSRV("g_InstanceData", instanceBuffer);
        bsb.BufferSRV("g_Pairs", state.pairs[stream]);
        bsb.BufferSRV("g_Entries", cfg.entryBuffer);
        bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
        bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        if (withBindless)
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), layout, nvDevice);
        if (!bindingSet)
            return;
        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = dc.framebuffer;
        gs.bindings = { bindingSet };
        if (withBindless && dc.bindlessTable)
            gs.addBindingSet(dc.bindlessTable);
        gs.indirectParams = state.args;
        gs.viewport.addViewport(dc.viewport);
        gs.viewport.addScissorRect(dc.scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(stream * kLocalArgsStride, 1);
    };

    if (haveCasters) {
        drawStream(0, state.pagePipeline, state.pageLayout, *psRefl, cfg.staticInstanceBuffer, false, "LocalShadow.PageOpaque");
        drawStream(1, state.pagePipeline, state.pageLayout, *psRefl, cfg.terrainInstanceBuffer, false, "LocalShadow.PageTerrain");
        drawStream(2, state.pageATPipeline, state.pageATLayout, *atRefl, cfg.staticInstanceBuffer, true, "LocalShadow.PageAT");
    }

    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Static");
}

void ExecuteDyn(fg::RenderContext* ctx, const FrameGraph& fg, const LocalShadowDynData& data)
{
    LocalShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.atlas);
    if (!cmdList || !nvDevice || !atlas || !state.clearPipeline || !state.pagePipeline)
        return;

    if (state.dynAtlasFirst) {
        cmdList->clearDepthStencilTexture(atlas, nvrhi::AllSubresources, true, 0.0f, false, 0);
        state.dynAtlasFirst = false;
    }

    const LocalShadowConfig& cfg = data.config;
    const bool haveCasters = cfg.gpuCulling && cfg.megaVertexBuffer && cfg.megaIndexBuffer;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("local_shadow_pull", ".vs");
    auto* skinVsRefl = shaderLoader->GetCachedReflection("local_shadow_pull_skinned", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("vsm_page", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!vsRefl || !skinVsRefl || !psRefl || !atRefl)
        return;

    if (data.gpuProfiler)
        data.gpuProfiler->BeginPass(cmdList, "Local Shadow.Dyn");

    LocalDrawContext dc;
    if (!BeginAtlasPass(ctx, cfg, state, atlas, data.device, "LocalShadowDyn", 1, true, dc)) {
        if (data.gpuProfiler)
            data.gpuProfiler->EndPass(cmdList, "Local Shadow.Dyn");
        return;
    }

    if (!haveCasters) {
        if (data.gpuProfiler)
            data.gpuProfiler->EndPass(cmdList, "Local Shadow.Dyn");
        return;
    }

    auto& matBuffer = bindless::MaterialBuffer::Instance();
    GPUCullingManager& gpuCulling = *cfg.gpuCulling;

    auto draw = [&](nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingSet* bindingSet, u32 stream, bool withBindless) {
        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = dc.framebuffer;
        gs.bindings = { bindingSet };
        if (withBindless && dc.bindlessTable)
            gs.addBindingSet(dc.bindlessTable);
        gs.indirectParams = state.args;
        gs.viewport.addViewport(dc.viewport);
        gs.viewport.addScissorRect(dc.scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(stream * kLocalArgsStride, 1);
    };

    if (gpuCulling.GetDynamicClusterEntryCount() > 0 && cfg.entryBuffer && cfg.dynamicInstanceBuffer) {
        BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "LocalShadow.DynOpaque");
        bsb.ConstantBuffer("LocalShadowRouteParams", dc.routeCB);
        bsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
        bsb.BufferSRV("g_Pairs", state.pairs[3]);
        bsb.BufferSRV("g_Entries", cfg.entryBuffer);
        bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
        bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.pageLayout, nvDevice))
            draw(state.pagePipeline, bindingSet, 3, false);

        BindingSetBuilder atBsb(*vsRefl, *atRefl, nvDevice, "LocalShadow.DynAT");
        atBsb.ConstantBuffer("LocalShadowRouteParams", dc.routeCB);
        atBsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
        atBsb.BufferSRV("g_Pairs", state.pairs[4]);
        atBsb.BufferSRV("g_Entries", cfg.entryBuffer);
        atBsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
        atBsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        atBsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        atBsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        if (auto bindingSet = cache.GetOrCreateBindingSet(atBsb.Build(), state.pageATLayout, nvDevice))
            draw(state.pageATPipeline, bindingSet, 4, true);
    }

    nvrhi::IBuffer* skinnedEntries = gpuCulling.GetSkinnedEntryBuffer();
    nvrhi::IBuffer* preVB = gpuCulling.GetSkinnedPreVertexBuffer();
    nvrhi::IBuffer* skinnedIB = gpuCulling.GetSkinnedPools().GetCombinedIndexBuffer();
    if (gpuCulling.GetSkinnedEntryCount() > 0 && skinnedEntries && preVB && skinnedIB) {
        BindingSetBuilder bsb(*skinVsRefl, *psRefl, nvDevice, "LocalShadow.DynSkin");
        bsb.ConstantBuffer("LocalShadowRouteParams", dc.routeCB);
        bsb.BufferSRV("g_Pairs", state.pairs[5]);
        bsb.BufferSRV("g_Entries", skinnedEntries);
        bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
        bsb.BufferSRV("g_SkinnedVB", preVB);
        bsb.BufferSRV("g_SkinnedIB", skinnedIB);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.skinPageLayout, nvDevice))
            draw(state.skinPagePipeline, bindingSet, 5, false);
    }

    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Dyn");
}


void SpotBasis(const light* L, Fvector& outDir, Fvector& outUp)
{
    Fvector L_dir, L_up, L_right;
    L_dir.set(L->direction);
    float l_dir_m = L_dir.magnitude();
    if (_valid(l_dir_m) && l_dir_m > EPS_S)
        L_dir.div(l_dir_m);
    else
        L_dir.set(0, 0, 1);

    if (L->right.square_magnitude() > EPS) {
        L_right.set(L->right);
        L_right.normalize();
        L_up.crossproduct(L_dir, L_right);
        L_up.normalize();
        L_right.crossproduct(L_up, L_dir);
        L_right.normalize();
    } else {
        L_up.set(0, 1, 0);
        if (_abs(L_up.dotproduct(L_dir)) > .99f)
            L_up.set(0, 0, 1);
        L_right.crossproduct(L_up, L_dir);
        L_right.normalize();
        L_up.crossproduct(L_dir, L_right);
        L_up.normalize();
    }
    outDir = L_dir;
    outUp = L_up;
}

void FillRecord(LocalShadowViewGPU& rec, const Fmatrix& vp, float rectX, float rectY, float tileSize,
                float nearZ, float farZ, float texelPerMetre, const Fvector& pos, float range)
{
    rec.viewProj = vp;
    rec.rect.set(rectX, rectY, tileSize, ps_r_local_shadow_bias);
    rec.zparams.set(nearZ, farZ, texelPerMetre, 0.0f);
    rec.lightPos.set(pos.x, pos.y, pos.z, range);
    rec.shape.set(0.0f, 0.0f, 0.0f, 0.0f);
    CFrustum fr;
    Fmatrix m = vp;
    fr.CreateFromMatrix(m, FRUSTUM_P_ALL);
    for (u32 i = 0; i < 6; ++i) {
        const size_t src = std::min<size_t>(i, fr.p_count ? fr.p_count - 1 : 0);
        rec.planes[i].set(fr.planes[src].n.x, fr.planes[src].n.y, fr.planes[src].n.z, fr.planes[src].d);
    }
}

} // namespace

void ResetLocalShadowPool(LocalShadowState& state)
{
    for (u32 i = 0; i < kLocalSpotSlots; ++i)
        state.spots[i] = LocalTile();
    state.statPairs = state.statDynPairs = state.statSkinnedPairs = state.statDrops = 0;
    for (u32 i = 0; i < kLocalPointLights; ++i)
        state.points[i] = LocalTile();
    for (u32 i = 0; i < kLocalTileCount; ++i)
        state.request[i] = LocalShadowViewGPU();
    state.candCount = 0;
    state.stateReset = true;
    state.pooledSpots = 0;
    state.pooledPoints = 0;
    state.slotOfLight.clear();
}

void ProcessLocalShadowStats(LocalShadowState& state, nvrhi::IDevice* device)
{
    if (state.readbackScheduled < LocalShadowState::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    void* mapped = device->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* words = static_cast<const u32*>(mapped);
    state.statAccepted = words[0];
    state.statDeferred = words[1];
    state.statSkipped = words[2];
    state.statUpToDate = words[3];
    state.statPairs = words[4] + words[5] + words[6];
    state.statDynPairs = words[20] + words[22];
    state.statSkinnedPairs = words[28];
    state.statDrops = words[7];
    state.statDynDrops = words[23] + words[31];
    state.statMaxVisited = words[8];
    state.statMaxPendingAge = words[12];
    state.statDynRefresh = words[13];
    u32 demand[3] = { words[9], words[10], words[11] };
    device->unmapBuffer(oldest);

    for (u32 s = 0; s < 3; ++s) {
        if (demand[s] <= state.pairCapacity[s])
            continue;
        const u64 grownCap = std::max<u64>(u64(demand[s]) + 1u, u64(demand[s]) * 5u / 4u);
        const u32 newCap = u32(std::min<u64>(grownCap, u64(1u) << 26));
        if (newCap <= state.pairCapacity[s])
            continue;
        string64 name;
        xr_sprintf(name, "LocalShadow_Pairs%s", kLocalStreamNames[s]);
        nvrhi::BufferHandle grown = MakeUAVBuffer(device, name, u64(newCap) * sizeof(u32) * 2, sizeof(u32) * 2, false);
        if (!grown) {
            Msg("! [LocalShadow] cannot allocate %u %s pairs", newCap, kLocalStreamNames[s]);
            continue;
        }
        Msg("* [LocalShadow] %s pair capacity: %u -> %u", kLocalStreamNames[s], state.pairCapacity[s], newCap);
        state.pairs[s] = grown;
        state.pairCapacity[s] = newCap;
    }

    if (ps_r_local_shadow_debug && Device.dwTimeGlobal - state.lastLogTime > 2000) {
        state.lastLogTime = Device.dwTimeGlobal;
        Msg("[LocalShadow] spots=%u points=%u accepted=%u deferred=%u skipped=%u upToDate=%u pairs=%u drops=%u dynDrops=%u maxVisited=%u maxPendingAge=%u dynRefresh=%u",
            state.pooledSpots, state.pooledPoints, state.statAccepted, state.statDeferred, state.statSkipped, state.statUpToDate,
            state.statPairs + state.statDynPairs + state.statSkinnedPairs, state.statDrops, state.statDynDrops,
            state.statMaxVisited, state.statMaxPendingAge, state.statDynRefresh);
    }
}

void SelectLocalShadowLights(
    LocalShadowState& state,
    const xr_vector<const light*>& lights,
    const Fvector& camPos,
    const Fmatrix& camViewProj)
{
    if (state.lastVsmAT != ps_r_vsm_at || state.lastClusterLod != ps_r_vsm_cluster_lod) {
        ResetLocalShadowPool(state);
        state.lastVsmAT = ps_r_vsm_at;
        state.lastClusterLod = ps_r_vsm_cluster_lod;
    }
    ++state.frame;
    const u32 frame = state.frame;
    state.slotOfLight.assign(lights.size(), 0u);
    if (state.resourcesFailed)
        return;
    state.candCount = 0;
    state.pooledSpots = 0;
    state.pooledPoints = 0;

    CFrustum camFrustum;
    Fmatrix camMatrix = camViewProj;
    camFrustum.CreateFromMatrix(camMatrix, FRUSTUM_P_ALL);

    struct Candidate { u32 index; float eff; };
    xr_vector<Candidate> spotCandidates;
    xr_vector<Candidate> pointCandidates;

    auto owns = [](const LocalTile* pool, u32 poolSize, const light* L) {
        for (u32 t = 0; t < poolSize; ++t)
            if (pool[t].owner == L)
                return true;
        return false;
    };

    for (u32 i = 0; i < lights.size(); ++i) {
        const light* L = lights[i];
        if (!L || !L->flags.bActive || !L->flags.bShadow || L->flags.bHudMode)
            continue;
        const float d2 = camPos.distance_to_sqr(L->position);
        if (L->flags.type == IRender_Light::SPOT) {
            float eff = d2 * (L->cone < deg2rad(60.f) ? 0.25f : 1.0f);
            if (owns(state.spots, kLocalSpotSlots, L))
                eff *= 0.64f;
            spotCandidates.push_back({ i, eff });
        } else if (L->flags.type == IRender_Light::POINT) {
            if (L->range < 3.0f || d2 > 30.0f * 30.0f)
                continue;
            float eff = d2;
            if (owns(state.points, kLocalPointLights, L))
                eff *= 0.64f;
            pointCandidates.push_back({ i, eff });
        }
    }

    auto byEff = [](const Candidate& a, const Candidate& b) { return a.eff < b.eff; };
    std::sort(spotCandidates.begin(), spotCandidates.end(), byEff);
    std::sort(pointCandidates.begin(), pointCandidates.end(), byEff);

    const u32 spotCap = std::min<u32>(kLocalSpotSlots, u32(std::max(1, ps_r_local_shadow_spots)));
    const u32 pointCap = std::min<u32>(kLocalPointLights, u32(std::max(1, ps_r_local_shadow_points)));
    if (spotCandidates.size() > spotCap)
        spotCandidates.resize(spotCap);
    if (pointCandidates.size() > pointCap)
        pointCandidates.resize(pointCap);

    auto assign = [&](xr_vector<Candidate>& candidates, LocalTile* pool, u32 poolSize, xr_vector<u32>& outTile) {
        outTile.assign(candidates.size(), u32(-1));
        for (u32 c = 0; c < candidates.size(); ++c) {
            const light* L = lights[candidates[c].index];
            for (u32 t = 0; t < poolSize; ++t) {
                if (pool[t].owner == L && pool[t].lastSeen != frame) {
                    pool[t].lastSeen = frame;
                    outTile[c] = t;
                    break;
                }
            }
        }
        for (u32 c = 0; c < candidates.size(); ++c) {
            if (outTile[c] != u32(-1))
                continue;
            u32 best = u32(-1);
            for (u32 t = 0; t < poolSize; ++t) {
                if (pool[t].lastSeen == frame)
                    continue;
                if (best == u32(-1) || pool[t].lastSeen < pool[best].lastSeen)
                    best = t;
            }
            if (best == u32(-1))
                continue;
            pool[best].owner = nullptr;
            pool[best].lastSeen = frame;
            outTile[c] = best;
        }
    };

    xr_vector<u32> spotTile;
    xr_vector<u32> pointTile;
    assign(spotCandidates, state.spots, kLocalSpotSlots, spotTile);
    assign(pointCandidates, state.points, kLocalPointLights, pointTile);

    auto cadenceFor = [](float dist) -> u32 {
        return dist < 30.0f ? 1u : (dist < 70.0f ? 4u : 12u);
    };

    struct Selected { bool point; u32 candidate; u32 tile; float eff; };
    xr_vector<Selected> selected;
    selected.reserve(spotCandidates.size() + pointCandidates.size());
    for (u32 c = 0; c < spotCandidates.size(); ++c)
        if (spotTile[c] != u32(-1))
            selected.push_back({ false, c, spotTile[c], spotCandidates[c].eff });
    for (u32 c = 0; c < pointCandidates.size(); ++c)
        if (pointTile[c] != u32(-1))
            selected.push_back({ true, c, pointTile[c], pointCandidates[c].eff });
    std::stable_sort(selected.begin(), selected.end(),
        [](const Selected& a, const Selected& b) { return a.eff < b.eff; });

    auto pushCandidate = [&](u32 slot, u32 stamp, u32 serial, bool inView, bool dynDue) {
        if (state.candCount >= kLocalTileCount)
            return;
        const u32 rank = state.candCount;
        u32* entry = state.candList[state.candCount++];
        entry[0] = slot;
        entry[1] = stamp;
        entry[2] = serial;
        entry[3] = rank | (dynDue ? 1u << 30 : 0u) | (inView ? 1u << 31 : 0u);
    };

    for (const Selected& sel : selected) {
        const u32 t = sel.tile;
        if (!sel.point) {
            const light* L = lights[spotCandidates[sel.candidate].index];
            LocalTile& tile = state.spots[t];
            const bool newOwner = tile.owner != L;
            Fvector dir, up;
            SpotBasis(L, dir, up);
            const bool moved = newOwner
                || tile.pos.distance_to_sqr(L->position) > 0.002f * 0.002f
                || tile.dir.dotproduct(dir) < 0.999999f
                || _abs(tile.range - L->range) > 0.01f
                || _abs(tile.cone - L->cone) > 0.001f;
            tile.owner = L;
            tile.pos = L->position;
            tile.dir = dir;
            tile.range = L->range;
            tile.cone = L->cone;
            if (newOwner) {
                tile.serial = ++state.nextSerial;
                tile.stamp = 1;
            } else if (moved) {
                if (++tile.stamp == 0)
                    tile.stamp = 1;
            }

            const float fov = L->cone + deg2rad(3.5f);
            const float nearZ = 0.5f;
            const float farZ = std::max(L->range, 1.0f);
            Fmatrix view, proj, vp;
            view.build_camera_dir(L->position, dir, up);
            proj.build_projection(fov, 1.f, nearZ, farZ);
            vp.mul(proj, view);

            const u32 slot = t;
            LocalShadowViewGPU& rec = state.request[slot];
            FillRecord(rec, vp, float((t & 3) * kLocalSpotTile), float((t >> 2) * kLocalSpotTile), float(kLocalSpotTile),
                nearZ, farZ, 2.0f * tanf(fov * 0.5f) / float(kLocalSpotTile), L->position, L->range);
            rec.meta[0] = tile.stamp;
            rec.meta[1] = tile.serial;
            rec.meta[2] = 0;
            rec.meta[3] = 0;

            tile.inView = camFrustum.testSphere_dirty(L->position, L->range);
            const u32 cadence = cadenceFor(camPos.distance_to(L->position));
            const bool dynDue = tile.inView && ((frame + slot) % cadence) == 0;
            pushCandidate(slot, tile.stamp, tile.serial, tile.inView, dynDue);
            state.slotOfLight[spotCandidates[sel.candidate].index] = slot + 1;
            ++state.pooledSpots;
            continue;
        }

        const light* L = lights[pointCandidates[sel.candidate].index];
        LocalTile& tile = state.points[t];
        const bool newOwner = tile.owner != L;
        const bool moved = newOwner
            || tile.pos.distance_to_sqr(L->position) > 0.002f * 0.002f
            || _abs(tile.range - L->range) > 0.01f;
        tile.owner = L;
        tile.pos = L->position;
        tile.range = L->range;
        if (newOwner) {
            tile.serial = ++state.nextSerial;
            tile.stamp = 1;
        } else if (moved) {
            if (++tile.stamp == 0)
                tile.stamp = 1;
        }

        const float nearZ = 0.25f;
        const float farZ = std::max(L->range, 1.0f);
        tile.inView = camFrustum.testSphere_dirty(L->position, L->range);
        const u32 cadence = cadenceFor(camPos.distance_to(L->position));

        for (u32 f = 0; f < 6; ++f) {
            const u32 k = t * 6 + f;
            const u32 slot = kLocalSpotSlots + k;
            Fmatrix view, proj, vp;
            Fvector fd = kFaceDir[f];
            Fvector fu = kFaceUp[f];
            view.build_camera_dir(L->position, fd, fu);
            proj.build_projection(PI_DIV_2, 1.f, nearZ, farZ);
            vp.mul(proj, view);
            LocalShadowViewGPU& rec = state.request[slot];
            FillRecord(rec, vp, float((k & 7) * kLocalPointFace), float(2048 + (k >> 3) * kLocalPointFace),
                float(kLocalPointFace), nearZ, farZ, 2.0f / float(kLocalPointFace), L->position, L->range);
            rec.meta[0] = tile.stamp;
            rec.meta[1] = tile.serial;
            rec.meta[2] = 1;
            rec.meta[3] = f;
            const bool dynDue = tile.inView && ((frame + slot) % cadence) == 0;
            pushCandidate(slot, tile.stamp, tile.serial, tile.inView, dynDue);
        }
        state.slotOfLight[pointCandidates[sel.candidate].index] = kLocalSpotSlots + t * 6 + 1;
        ++state.pooledPoints;
    }
}

LocalShadowOutput setupLocalShadowPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    const LocalShadowConfig& config,
    LocalShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    LocalShadowOutput out;
    if (!state || !device)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;
    if (!EnsureResources(nvDevice, *state))
        return out;
    ProcessLocalShadowStats(*state, nvDevice);
    out.state = state;
    if (state->pooledSpots == 0 && state->pooledPoints == 0)
        return out;

    auto bufferDesc = [](const char* name, u64 bytes, u32 stride, bool uav) {
        ResourceDesc d;
        d.type = ResourceDesc::Type::Buffer;
        d.bufferSize = bytes;
        d.structStride = stride;
        d.isUAV = uav;
        d.allowUAV = uav;
        d.isImported = true;
        d.isTransient = false;
        d.debugName = name;
        return d;
    };
    VirtualResourceHandle tilesHandle = fg.ImportBuffer("local_shadow_tiles", state->stateBuffer,
        bufferDesc("local_shadow_tiles", u64(kLocalTileCount) * sizeof(LocalShadowViewGPU), sizeof(LocalShadowViewGPU), true));
    VirtualResourceHandle argsHandle = fg.ImportBuffer("local_shadow_args", state->args,
        bufferDesc("local_shadow_args", u64(kLocalStreamCount) * kLocalArgsStride, 0, true));
    VirtualResourceHandle clearArgsHandle = fg.ImportBuffer("local_shadow_clear_args", state->clearArgs,
        bufferDesc("local_shadow_clear_args", u64(2) * kLocalArgsStride, 0, true));
    VirtualResourceHandle dirtyListHandle = fg.ImportBuffer("local_shadow_dirty_list", state->dirtyList,
        bufferDesc("local_shadow_dirty_list", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), true));
    VirtualResourceHandle refreshDynHandle = fg.ImportBuffer("local_shadow_refresh_dyn", state->refreshDynBuffer,
        bufferDesc("local_shadow_refresh_dyn", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), true));

    auto atlasDesc = [](const char* name) {
        ResourceDesc d;
        d.type = ResourceDesc::Type::Texture2D;
        d.width = kLocalShadowAtlas;
        d.height = kLocalShadowAtlas;
        d.format = nvrhi::Format::D16;
        d.isDepthStencil = true;
        d.isImported = true;
        d.isTransient = false;
        d.debugName = name;
        return d;
    };
    VirtualResourceHandle staticHandle = fg.ImportTexture("rt_LocalShadowStatic", state->staticAtlas, atlasDesc("rt_LocalShadowStatic"));
    VirtualResourceHandle dynHandle = fg.ImportTexture("rt_LocalShadowDyn", state->dynAtlas, atlasDesc("rt_LocalShadowDyn"));

    auto& binData = fg.addCallbackPass<LocalShadowBinData>(
        "Local Shadow Bin",
        [&, tilesHandle, argsHandle, clearArgsHandle, dirtyListHandle, refreshDynHandle, orderAfter, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, LocalShadowBinData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.tiles = passBuilder.write(tilesHandle, ResourceState::UnorderedAccess);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::ShaderResource);
            data.args = passBuilder.write(argsHandle, ResourceState::UnorderedAccess);
            data.clearArgs = passBuilder.write(clearArgsHandle, ResourceState::UnorderedAccess);
            data.dirtyList = passBuilder.write(dirtyListHandle, ResourceState::UnorderedAccess);
            data.refreshDyn = passBuilder.write(refreshDynHandle, ResourceState::UnorderedAccess);
        },
        [](const LocalShadowBinData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteBin(ctx, fg, data);
        });

    auto& staticData = fg.addCallbackPass<LocalShadowStaticData>(
        "Local Shadow Static",
        [&, staticHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, LocalShadowStaticData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.atlas = passBuilder.write(staticHandle, ResourceState::DepthStencilWrite);
            data.tiles = passBuilder.read(binData.tiles, ResourceState::ShaderResource);
            data.args = passBuilder.read(binData.args, ResourceState::IndirectArgument);
            data.clearArgs = passBuilder.read(binData.clearArgs, ResourceState::IndirectArgument);
            data.dirtyList = passBuilder.read(binData.dirtyList, ResourceState::ShaderResource);
        },
        [](const LocalShadowStaticData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteStatic(ctx, fg, data);
        });

    auto& dynData = fg.addCallbackPass<LocalShadowDynData>(
        "Local Shadow Dyn",
        [&, dynHandle, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, LocalShadowDynData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.atlas = passBuilder.write(dynHandle, ResourceState::DepthStencilWrite);
            data.tiles = passBuilder.read(binData.tiles, ResourceState::ShaderResource);
            data.args = passBuilder.read(binData.args, ResourceState::IndirectArgument);
            data.clearArgs = passBuilder.read(binData.clearArgs, ResourceState::IndirectArgument);
            data.refreshDyn = passBuilder.read(binData.refreshDyn, ResourceState::ShaderResource);
        },
        [](const LocalShadowDynData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteDyn(ctx, fg, data);
        });

    fg.GetRTRegistry().RegisterRT("rt_LocalShadowStatic", staticData.atlas);
    fg.GetRTRegistry().RegisterRT("rt_LocalShadowDyn", dynData.atlas);
    out.tiles = dynData.tiles;
    out.staticAtlas = staticData.atlas;
    out.dynAtlas = dynData.atlas;
    out.active = true;
    return out;
}

void ResolveLocalShadowBindings(
    const framegraph::FrameGraph& fg,
    const LocalShadowOutput& out,
    nvrhi::IDevice* device,
    nvrhi::IBuffer*& tiles,
    nvrhi::ITexture*& staticAtlas,
    nvrhi::ITexture*& dynAtlas)
{
    tiles = out.state ? out.state->stateBuffer.Get() : nullptr;
    if (!tiles)
        tiles = FallbackTiles(device);
    staticAtlas = nullptr;
    dynAtlas = nullptr;
    if (out.active) {
        staticAtlas = fg.GetPhysicalTexture(out.staticAtlas);
        dynAtlas = fg.GetPhysicalTexture(out.dynAtlas);
    }
    if (!staticAtlas)
        staticAtlas = GetPassResourceCache().GetDummyShadowMap2D(device);
    if (!dynAtlas)
        dynAtlas = GetPassResourceCache().GetDummyShadowMap2D(device);
}

}
