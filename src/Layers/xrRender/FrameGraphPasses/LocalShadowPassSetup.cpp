#include "stdafx.h"
#include "LocalShadowPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
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

nvrhi::TextureHandle MakeAtlas(nvrhi::IDevice* nvDevice, const char* name, u32 layers = 1)
{
    nvrhi::TextureDesc desc;
    desc.dimension = nvrhi::TextureDimension::Texture2DArray;
    desc.arraySize = layers;
    desc.width = kLocalShadowAtlas;
    desc.height = kLocalShadowAtlas;
    desc.format = nvrhi::Format::D32;
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

bool EnsureAtlasLayers(nvrhi::IDevice* nvDevice, LocalShadowState& state, u32 layers)
{
    if (state.atlasLayers >= layers && state.receiverTiles)
        return true;
    auto statics = MakeAtlas(nvDevice, "LocalShadow_Static", layers);
    auto dynamics = MakeAtlas(nvDevice, "LocalShadow_Dyn", layers);
    nvrhi::BufferDesc desc;
    desc.debugName = "LocalShadow_Receivers";
    desc.byteSize = u64(layers) * kLocalTileCount * sizeof(LocalShadowViewGPU);
    desc.structStride = sizeof(LocalShadowViewGPU);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    auto receivers = nvDevice->createBuffer(desc);
    if (!statics || !dynamics || !receivers)
        return false;
    state.staticAtlas = statics;
    state.dynAtlas = dynamics;
    state.receiverTiles = receivers;
    state.atlasLayers = layers;
    auto bind = [&](LocalShadowState& page) {
        page.staticAtlas = statics;
        page.dynAtlas = dynamics;
        page.staticAtlasFirst = page.dynAtlasFirst = true;
        page.stateReset = true;
    };
    bind(state);
    for (auto& page : state.overflowPages)
        bind(*page);
    return true;
}

nvrhi::IBuffer* FallbackTiles(nvrhi::IDevice* nvDevice)
{
    static const nvrhi::BufferDesc desc = [] {
        nvrhi::BufferDesc desc;
        desc.debugName = "LocalShadow_TilesFallback";
        desc.byteSize = u64(kLocalTileCount) * sizeof(LocalShadowViewGPU);
        desc.structStride = sizeof(LocalShadowViewGPU);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        return desc;
    }();
    return GetPassResourceCache().GetOrCreateStaticBuffer("LocalShadow", "TilesFallback", desc, nvDevice);
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
        desc.byteSize = u64(kLocalStatWords) * sizeof(u32) + u64(kLocalTileCount) * sizeof(u32) * 4;
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        state.readback[i] = nvDevice->createBuffer(desc);
    }
    state.readbackWrite = 0;
    state.readbackScheduled = 0;
    state.stateReset = true;
    R_ASSERT(state.staticAtlas && state.dynAtlas);

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
    Msg("* [LocalShadow] atlases %ux%u D32, %u view records, tiles %u..%u", kLocalShadowAtlas, kLocalShadowAtlas,
        kLocalTileCount, kLocalPointFaceMin, kLocalSpotTileMax);
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
    state.pageATPS = pageATPsResult.handle;

    auto& cache = GetPassResourceCache();
    state.binCountLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinCount", *countResult.reflection, nvDevice);
    state.binReserveLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinReserve", *reserveResult.reflection, nvDevice);
    state.binEmitLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinEmit", *emitResult.reflection, nvDevice);
    state.binDynLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBinDyn", *binResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowArgs", *argsResult.reflection, nvDevice);
    state.clearLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowClear", *clearVsResult.reflection, *clearPsResult.reflection, nvDevice);
    state.pageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPage", *pageVsResult.reflection, *pagePsResult.reflection, nvDevice);
    state.pageATLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPageAT", *pageVsResult.reflection, *pageATPsResult.reflection, nvDevice);
    state.skinPageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowSkinPage", *skinVsResult.reflection, *pageATPsResult.reflection, nvDevice);
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
    fbInfo.depthFormat = nvrhi::Format::D32;

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
        // NVRHI defaults to disabling depth clipping on backends that support it.
        // Local lights use virtual_size to clip their emitter's near geometry.
        desc.renderState.rasterState.depthClipEnable = true;
        return desc;
    };
    state.pagePipeline = cache.GetOrCreatePipeline("LocalShadowPage", makePageDesc(state.pageVS, pagePsResult.handle, state.pageLayout, false), fbInfo, nvDevice);
    state.pageATPipeline = cache.GetOrCreatePipeline("LocalShadowPageAT", makePageDesc(state.pageVS, pageATPsResult.handle, state.pageATLayout, true), fbInfo, nvDevice);
    state.skinPagePipeline = cache.GetOrCreatePipeline("LocalShadowSkinPage", makePageDesc(state.skinPageVS, pageATPsResult.handle, state.skinPageLayout, true), fbInfo, nvDevice);

    if (!state.binCountPipeline || !state.binReservePipeline || !state.binEmitPipeline || !state.binDynPipeline
        || !state.argsPipeline || !state.clearPipeline || !state.pagePipeline || !state.pageATPipeline || !state.skinPagePipeline) {
        Msg("! [LocalShadow] pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }
    Msg("* [LocalShadow] pipelines initialized");
    return true;
}

void BuildLocalShadowArgs(fg::RenderContext* ctx, fg::RenderDevice* device, LocalShadowState& state, u32 mode)
{
    auto* cmdList = ctx->GetCommandList();
    auto* nvDevice = device->GetNVRHIDevice();
    auto& cache = GetPassResourceCache();
    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("local_shadow_args", ".cs");
    R_ASSERT(reflection);
    LocalShadowArgsParams ap = {};
    std::copy_n(state.pairCapacity, kLocalStreamCount, ap.caps);
    ap.caps[6] = mode;
    auto cb = cache.GetOrCreateVolatileCB("LocalShadow", "ArgsParams", sizeof(ap), device, 1024);
    cmdList->writeBuffer(cb, &ap, sizeof(ap));
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::UnorderedAccess);
    BindingSetBuilder bsb(*reflection, nvDevice, "LocalShadow.Args");
    bsb.ConstantBuffer("LocalShadowArgsParams", cb).BufferSRV("g_Stats", state.stats).BufferUAV("g_Args", state.args);
    auto bindings = cache.GetOrCreateBindingSet(bsb.Build(), state.argsLayout, nvDevice);
    R_ASSERT(bindings);
    nvrhi::ComputeState cs;
    cs.pipeline = state.argsPipeline;
    cs.bindings = { bindings };
    cmdList->setComputeState(cs);
    cmdList->dispatch(1, 1, 1);
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::IndirectArgument);
}

void BinCasterBatch(fg::RenderContext* ctx, fg::RenderDevice* device, LocalShadowState& state,
                    nvrhi::IBuffer* entries, u32 entryBase, u32 count, u32 mode)
{
    auto* cmd = ctx->GetCommandList();
    auto* nvDevice = device->GetNVRHIDevice();
    auto& cache = GetPassResourceCache();
    const bool statics = mode == 1u;
    const bool skinned = mode == 3u;
    const u32 opaque = statics ? 0u : (skinned ? 5u : 3u);
    const u32 terrain = statics ? 1u : 0u;
    const u32 at = statics ? 2u : 4u;
    LocalShadowDynBinParams dp = {};
    dp.entryBase = entryBase;
    dp.entryCount = count;
    dp.statsBase = statics ? 32u : (skinned ? 24u : 16u);
    dp.capOpaque = state.pairCapacity[opaque];
    dp.capTerrain = statics ? state.pairCapacity[terrain] : 0u;
    dp.capAT = skinned ? 0u : state.pairCapacity[at];
    dp.includeAT = 1u;
    dp.pad = statics ? 1u : 0u;
    // count * refreshCount <= every writable stream capacity. No pair truncation.
    const u32 zeros[3] = {};
    cmd->setBufferState(state.stats, nvrhi::ResourceStates::CopyDest);
    cmd->writeBuffer(state.stats, zeros, sizeof(zeros), u64(dp.statsBase + 4u) * sizeof(u32));
    auto cb = cache.GetOrCreateVolatileCB("LocalShadow", "DynBinParams", sizeof(dp), device, 1024);
    cmd->writeBuffer(cb, &dp, sizeof(dp));
    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("local_shadow_bin_dyn", ".cs");
    R_ASSERT(reflection);
    cmd->setBufferState(state.stats, nvrhi::ResourceStates::UnorderedAccess);
    cmd->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);
    cmd->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
    cmd->setBufferState(statics ? state.dirtyList : state.refreshDynBuffer, nvrhi::ResourceStates::ShaderResource);
    for (u32 stream : { opaque, terrain, at })
        cmd->setBufferState(state.pairs[stream], nvrhi::ResourceStates::UnorderedAccess);
    BindingSetBuilder bsb(*reflection, nvDevice, "LocalShadow.CasterBatch");
    bsb.ConstantBuffer("LocalShadowDynBinParams", cb)
       .BufferSRV("g_Entries", entries)
       .BufferSRV("g_Tiles", state.stateBuffer)
       .BufferSRV("g_Refresh", statics ? state.dirtyList : state.refreshDynBuffer)
       .BufferUAV("g_Stats", state.stats)
       .BufferUAV("g_PairsOpaque", state.pairs[opaque])
       .BufferUAV("g_PairsTerrain", state.pairs[terrain])
       .BufferUAV("g_PairsAT", state.pairs[at]);
    auto bindings = cache.GetOrCreateBindingSet(bsb.Build(), state.binDynLayout, nvDevice);
    R_ASSERT(bindings);
    nvrhi::ComputeState cs;
    cs.pipeline = state.binDynPipeline;
    cs.bindings = { bindings };
    cmd->setComputeState(cs);
    cmd->dispatch((count + 63u) / 64u, 1, 1);
    BuildLocalShadowArgs(ctx, device, state, mode);
    for (u32 stream : { opaque, terrain, at })
        cmd->setBufferState(state.pairs[stream], nvrhi::ResourceStates::ShaderResource);
    ++state.statCasterBatches;
}

void ScheduleLocalShadowStats(LocalShadowState& state, nvrhi::ICommandList* cmdList)
{
    if (nvrhi::IBuffer* slot = state.readback[state.readbackWrite]) {
        cmdList->setBufferState(state.stats, nvrhi::ResourceStates::CopySource);
        cmdList->copyBuffer(slot, 0, state.stats, 0, u64(kLocalStatWords) * sizeof(u32));
        cmdList->setBufferState(state.schedule, nvrhi::ResourceStates::CopySource);
        cmdList->copyBuffer(slot, u64(kLocalStatWords) * sizeof(u32), state.schedule, 0,
            u64(kLocalTileCount) * sizeof(u32) * 4);
        state.readbackWrite = (state.readbackWrite + 1) % LocalShadowState::kReadbackSlots;
        if (state.readbackScheduled < LocalShadowState::kReadbackSlots)
            ++state.readbackScheduled;
    }

}

void ExecuteBin(fg::RenderContext* ctx, const FrameGraph& fg, const LocalShadowBinData& data)
{
    LocalShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice)
        return;
    R_ASSERT2(EnsurePipelines(data.device, state), "Cannot render complete local light shadows without their pipelines");

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* countRefl = shaderLoader->GetCachedReflection("local_shadow_bin_count", ".cs");
    auto* reserveRefl = shaderLoader->GetCachedReflection("local_shadow_bin_reserve", ".cs");
    auto* emitRefl = shaderLoader->GetCachedReflection("local_shadow_bin_emit", ".cs");
    R_ASSERT(countRefl && reserveRefl && emitRefl);

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
        state.dirtyViews = state.candCount;
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
    bp.includeAT = 1u;
    bp.errK = 1.0f;
    bp.capOpaque = state.pairCapacity[0];
    bp.capTerrain = state.pairCapacity[1];
    bp.capAT = state.pairCapacity[2];
    bp.budget = u32(std::max(1, ps_r_local_shadow_pairs_budget));
    bp.frame = state.frame;
    bp.pad[0] = gpuCulling ? gpuCulling->GetClusterEntryCount() : 0u;
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
    R_ASSERT2(counted || !haveEntries, "Local shadow caster counting failed");
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
        auto reserveSet = cache.GetOrCreateBindingSet(rbs.Build(), state.binReserveLayout, nvDevice);
        R_ASSERT(reserveSet);
        {
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
        auto emitSet = cache.GetOrCreateBindingSet(ebs.Build(), state.binEmitLayout, nvDevice);
        R_ASSERT(emitSet);
        {
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

    BuildLocalShadowArgs(ctx, data.device, state, 0u);

    for (u32 i = 0; i < kLocalStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.clearArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.stateBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.dirtyList, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::ShaderResource);


    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Bin");
}

struct LocalDrawContext {
    nvrhi::ICommandList* cmdList;
    nvrhi::IDevice* nvDevice;
    nvrhi::IFramebuffer* framebuffer;
    nvrhi::IBindingSet* bindlessTable;
    nvrhi::Viewport viewport;
    nvrhi::Rect scissor;
};

bool BeginAtlasPass(fg::RenderContext* ctx, const LocalShadowConfig& cfg, LocalShadowState& state,
                    nvrhi::ITexture* atlas, fg::RenderDevice* device, const char* fbName, u32 clearIndex,
                    LocalDrawContext& out)
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
    fbDesc.setDepthAttachment(atlas, nvrhi::TextureSubresourceSet(0, 1, state.atlasLayer, 1));
    auto framebuffer = cache.GetOrCreateFramebuffer(fbName, fbDesc, nvDevice);
    if (!framebuffer)
        return false;

    auto* backend = device->GetBackend();
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
        cmdList->clearDepthStencilTexture(atlas, nvrhi::TextureSubresourceSet(0, 1, state.atlasLayer, 1), true, 0.0f, false, 0);
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
    if (!BeginAtlasPass(ctx, cfg, state, atlas, data.device, "LocalShadowStatic", 0, dc)) {
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

        if (cfg.gpuCulling && state.dirtyViews > 0) {
            const u32 count = cfg.gpuCulling->GetClusterEntryCount();
            const u32 capacity = std::min({ state.pairCapacity[0], state.pairCapacity[1], state.pairCapacity[2] });
            const u64 upperBound = u64(count) * state.dirtyViews;
            if (upperBound > capacity || upperBound > u32(std::max(1, ps_r_local_shadow_pairs_budget)) || !cfg.bvhNodeCount) {
                const u32 batchSize = std::max(1u, capacity / state.dirtyViews);
                for (u32 base = 0; base < count; base += batchSize) {
                    BinCasterBatch(ctx, data.device, state, cfg.entryBuffer, base, std::min(batchSize, count - base), 1u);
                    drawStream(0, state.pagePipeline, state.pageLayout, *psRefl, cfg.staticInstanceBuffer, false, "LocalShadow.PageOpaque");
                    drawStream(1, state.pagePipeline, state.pageLayout, *psRefl, cfg.terrainInstanceBuffer, false, "LocalShadow.PageTerrain");
                    drawStream(2, state.pageATPipeline, state.pageATLayout, *atRefl, cfg.staticInstanceBuffer, true, "LocalShadow.PageAT");
                }
            }
        }
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
        cmdList->clearDepthStencilTexture(atlas, nvrhi::TextureSubresourceSet(0, 1, state.atlasLayer, 1), true, 0.0f, false, 0);
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
    if (!BeginAtlasPass(ctx, cfg, state, atlas, data.device, "LocalShadowDyn", 1, dc)) {
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
        const u32 count = gpuCulling.GetDynamicClusterEntryCount();
        const u32 batchSize = std::max(1u, std::min(state.pairCapacity[3], state.pairCapacity[4]) / std::max(1u, state.candCount));
        for (u32 base = 0; base < count; base += batchSize) {
            BinCasterBatch(ctx, data.device, state, cfg.entryBuffer, gpuCulling.GetClusterEntryCount() + base,
                std::min(batchSize, count - base), 2u);
            BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "LocalShadow.DynOpaque");
            bsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
            bsb.BufferSRV("g_Pairs", state.pairs[3]);
            bsb.BufferSRV("g_Entries", cfg.entryBuffer);
            bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
            bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
            bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
            if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.pageLayout, nvDevice))
                draw(state.pagePipeline, bindingSet, 3, false);

            BindingSetBuilder atBsb(*vsRefl, *atRefl, nvDevice, "LocalShadow.DynAT");
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
    }

    nvrhi::IBuffer* skinnedEntries = gpuCulling.GetSkinnedEntryBuffer();
    nvrhi::IBuffer* preVB = gpuCulling.GetSkinnedPreVertexBuffer();
    nvrhi::IBuffer* skinnedIB = gpuCulling.GetSkinnedPools().GetCombinedIndexBuffer();
    if (gpuCulling.GetSkinnedEntryCount() > 0 && skinnedEntries && preVB && skinnedIB) {
        const u32 count = gpuCulling.GetSkinnedEntryCount();
        const u32 batchSize = std::max(1u, state.pairCapacity[5] / std::max(1u, state.candCount));
        for (u32 base = 0; base < count; base += batchSize) {
            BinCasterBatch(ctx, data.device, state, skinnedEntries, base, std::min(batchSize, count - base), 3u);
            BindingSetBuilder bsb(*skinVsRefl, *atRefl, nvDevice, "LocalShadow.DynSkin");
            bsb.BufferSRV("g_Pairs", state.pairs[5]);
            bsb.BufferSRV("g_Entries", skinnedEntries);
            bsb.BufferSRV("g_LocalShadowTiles", state.stateBuffer);
            bsb.BufferSRV("g_SkinnedVB", preVB);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
            if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.skinPageLayout, nvDevice))
                draw(state.skinPagePipeline, bindingSet, 5, true);
        }
    }

    ScheduleLocalShadowStats(state, cmdList);

    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Dyn");
}

struct LocalShadowHudParams {
    Fmatrix warp;
    u32 slots[kLocalHudViewsMax];
    u32 entryCount;
    u32 viewCount;
    u32 pad[2];
};
static_assert(sizeof(LocalShadowHudParams) == 144, "LocalShadowHudParams is shader-visible");

struct LocalShadowHudData {
    VirtualResourceHandle atlas;
    VirtualResourceHandle tiles;
    VirtualResourceHandle order;
    LocalShadowConfig config;
    LocalShadowState* state;
    fg::RenderDevice* device;
    xray::profiler::GPUProfiler* gpuProfiler;
};

bool EnsureHudAtlas(nvrhi::IDevice* nvDevice, LocalShadowState& state)
{
    if (state.hudAtlas)
        return true;
    nvrhi::TextureDesc desc;
    desc.width = kLocalShadowAtlas;
    desc.height = kLocalShadowAtlas;
    desc.format = nvrhi::Format::D32;
    desc.debugName = "LocalShadow_Hud";
    desc.isShaderResource = true;
    desc.isRenderTarget = true;
    desc.isTypeless = true;
    desc.useClearValue = true;
    desc.clearValue = nvrhi::Color(0.0f);
    desc.initialState = nvrhi::ResourceStates::DepthWrite;
    desc.keepInitialState = true;
    state.hudAtlas = nvDevice->createTexture(desc);
    return state.hudAtlas != nullptr;
}

bool EnsureHudPipeline(fg::RenderDevice* device, LocalShadowState& state)
{
    if (state.hudPagePipeline)
        return true;
    if (state.hudPipelineFailed || !state.pageATPS)
        return false;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto& cache = GetPassResourceCache();
    if (!state.hudPageVS) {
        auto vsResult = shaderLoader->LoadVertexShader("local_shadow_pull_hud", "main");
        if (vsResult.handle && vsResult.reflection)
            state.hudPageVS = vsResult.handle;
    }
    auto* vsRefl = shaderLoader->GetCachedReflection("local_shadow_pull_hud", ".vs");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!state.hudPageVS || !vsRefl || !atRefl) {
        Msg("! [LocalShadow] hud pull shader failed to load");
        state.hudPipelineFailed = true;
        return false;
    }
    state.hudPageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowHudPage", *vsRefl, *atRefl, nvDevice);
    if (!state.hudPageLayout) {
        state.hudPipelineFailed = true;
        return false;
    }
    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;
    nvrhi::GraphicsPipelineDesc desc;
    desc.VS = state.hudPageVS;
    desc.PS = state.pageATPS;
    desc.inputLayout = nullptr;
    if (bindlessLayout)
        desc.bindingLayouts = { state.hudPageLayout, bindlessLayout };
    else
        desc.bindingLayouts = { state.hudPageLayout };
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.renderState.depthStencilState.depthTestEnable = true;
    desc.renderState.depthStencilState.depthWriteEnable = true;
    desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    desc.renderState.rasterState.frontCounterClockwise = false;
    desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    desc.renderState.rasterState.depthClipEnable = true;
    state.hudPagePipeline = cache.GetOrCreatePipeline("LocalShadowHudPage", desc, fbInfo, nvDevice);
    if (!state.hudPagePipeline) {
        Msg("! [LocalShadow] hud pipeline failed");
        state.hudPipelineFailed = true;
        return false;
    }
    Msg("* [LocalShadow] hud pipeline initialized");
    return true;
}

void ExecuteHud(fg::RenderContext* ctx, const FrameGraph& fg, const LocalShadowHudData& data)
{
    LocalShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* atlas = fg.GetPhysicalTexture(data.atlas);
    if (!cmdList || !nvDevice || !atlas)
        return;
    cmdList->setTextureState(atlas, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthWrite);
    cmdList->commitBarriers();
    cmdList->clearDepthStencilTexture(atlas, nvrhi::AllSubresources, true, 0.0f, false, 0);
    if (state.hudViews == 0 || !data.config.gpuCulling)
        return;
    GPUCullingManager& gc = *data.config.gpuCulling;
    const u32 count = gc.GetSkinnedHudEntryCount();
    nvrhi::IBuffer* hudEntries = gc.GetSkinnedHudEntryBuffer();
    nvrhi::IBuffer* entries = gc.GetSkinnedEntryBuffer();
    nvrhi::IBuffer* preVB = gc.GetSkinnedPreVertexBuffer();
    nvrhi::IBuffer* ib = gc.GetSkinnedPools().GetCombinedIndexBuffer();
    if (count == 0 || !hudEntries || !entries || !preVB || !ib)
        return;
    if (!EnsureHudPipeline(data.device, state))
        return;

    auto& cache = GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("local_shadow_pull_hud", ".vs");
    auto* atRefl = shaderLoader->GetCachedReflection("vsm_page_at", ".ps");
    if (!vsRefl || !atRefl)
        return;

    if (data.gpuProfiler)
        data.gpuProfiler->BeginPass(cmdList, "Local Shadow.Hud");

    LocalShadowHudParams hp = {};
    hp.warp = HudFovWarp();
    std::copy_n(state.hudSlots, kLocalHudViewsMax, hp.slots);
    hp.entryCount = count;
    hp.viewCount = state.hudViews;
    auto hudCB = cache.GetOrCreateVolatileCB("LocalShadow", "HudParams", sizeof(LocalShadowHudParams), data.device);
    cmdList->writeBuffer(hudCB, &hp, sizeof(hp));

    auto& matBuffer = bindless::MaterialBuffer::Instance();
    cmdList->setBufferState(hudEntries, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(preVB, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(ib, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.receiverTiles, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(matBuffer.GetBuffer(), nvrhi::ResourceStates::ShaderResource);
    cmdList->commitBarriers();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(atlas);
    auto framebuffer = cache.GetOrCreateFramebuffer("LocalShadowHud", fbDesc, nvDevice);
    if (!framebuffer)
        return;

    BindingSetBuilder bsb(*vsRefl, *atRefl, nvDevice, "LocalShadow.Hud");
    bsb.ConstantBuffer("LocalShadowHudParams", hudCB)
       .BufferSRV("g_HudEntries", hudEntries)
       .BufferSRV("g_Entries", entries)
       .BufferSRV("g_LocalShadowTiles", state.receiverTiles)
       .BufferSRV("g_SkinnedVB", preVB)
       .BufferSRV("g_SkinnedIB", ib)
       .BufferSRV("g_Materials", matBuffer.GetBuffer());
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.hudPageLayout, nvDevice);
    if (!bindingSet)
        return;

    auto* backend = data.device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
    nvrhi::GraphicsState gs;
    gs.pipeline = state.hudPagePipeline;
    gs.framebuffer = framebuffer;
    gs.bindings = { bindingSet };
    if (bindlessTable)
        gs.addBindingSet(bindlessTable);
    gs.viewport.addViewport(nvrhi::Viewport(0.0f, float(kLocalShadowAtlas), 0.0f, float(kLocalShadowAtlas), 0.0f, 1.0f));
    gs.viewport.addScissorRect(nvrhi::Rect(kLocalShadowAtlas, kLocalShadowAtlas));
    cmdList->setGraphicsState(gs);
    cmdList->draw(nvrhi::DrawArguments().setVertexCount(GPUCullingManager::SKINNED_ENTRY_INDICES).setInstanceCount(count * state.hudViews));

    if (data.gpuProfiler)
        data.gpuProfiler->EndPass(cmdList, "Local Shadow.Hud");
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

void FillRecord(LocalShadowViewGPU& rec, const Fmatrix& vp, const LocalAtlasAllocator& atlas, u32 node,
                float nearZ, float farZ, float halfFovTan, const Fvector& pos, float range)
{
    u32 rectX = 0, rectY = 0, tileSize = 1;
    atlas.Rect(node, rectX, rectY, tileSize);
    rec.viewProj = vp;
    rec.rect.set(float(rectX), float(rectY), float(tileSize), ps_r_local_shadow_bias);
    rec.zparams.set(nearZ, farZ, 2.0f * halfFovTan / float(tileSize), 0.0f);
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

u32 LocalAtlasAllocator::LevelOf(u32 size)
{
    u32 level = 0;
    u32 s = kLocalShadowAtlas;
    while (s > size && level + 1 < kLocalAtlasLevels) {
        s >>= 1;
        ++level;
    }
    return level;
}

void LocalAtlasAllocator::Build()
{
    nodeX[0] = 0;
    nodeY[0] = 0;
    nodeLevel[0] = 0;
    u32 base = 0;
    u32 count = 1;
    for (u32 level = 1; level < kLocalAtlasLevels; ++level) {
        const u32 childBase = base + count;
        const u32 size = SizeOf(level);
        for (u32 p = 0; p < count; ++p) {
            const u32 parent = base + p;
            for (u32 k = 0; k < 4; ++k) {
                const u32 child = childBase + p * 4 + k;
                nodeX[child] = u16(nodeX[parent] + (k & 1u) * size);
                nodeY[child] = u16(nodeY[parent] + (k >> 1) * size);
                nodeLevel[child] = u8(level);
            }
        }
        base = childBase;
        count *= 4;
    }
    built = true;
}

void LocalAtlasAllocator::ListPush(u32 node)
{
    xr_vector<u32>& list = freeList[nodeLevel[node]];
    nodeSlot[node] = u16(list.size());
    list.push_back(node);
}

void LocalAtlasAllocator::ListRemove(u32 node)
{
    xr_vector<u32>& list = freeList[nodeLevel[node]];
    const u32 pos = nodeSlot[node];
    const u32 last = list.back();
    list[pos] = last;
    nodeSlot[last] = u16(pos);
    list.pop_back();
}

void LocalAtlasAllocator::Reset()
{
    if (!built)
        Build();
    for (u32 i = 0; i < kLocalAtlasNodes; ++i) {
        nodeState[i] = 3;
        nodeSlot[i] = 0;
    }
    for (u32 l = 0; l < kLocalAtlasLevels; ++l)
        freeList[l].clear();
    usedTexels = 0;
    nodeState[0] = 0;
    ListPush(0);
}

u32 LocalAtlasAllocator::Take(u32 level)
{
    if (!freeList[level].empty()) {
        const u32 node = freeList[level].back();
        ListRemove(node);
        nodeState[node] = 3;
        return node;
    }
    if (level == 0)
        return u32(-1);
    const u32 parent = Take(level - 1);
    if (parent == u32(-1))
        return u32(-1);
    nodeState[parent] = 1;
    const u32 parentBase = ((1u << (2u * (level - 1))) - 1u) / 3u;
    const u32 base = ((1u << (2u * level)) - 1u) / 3u + (parent - parentBase) * 4u;
    for (u32 k = 1; k < 4; ++k) {
        nodeState[base + k] = 0;
        ListPush(base + k);
    }
    nodeState[base] = 3;
    return base;
}

u32 LocalAtlasAllocator::Alloc(u32 level)
{
    if (!built)
        Reset();
    if (level >= kLocalAtlasLevels)
        return u32(-1);
    const u32 node = Take(level);
    if (node == u32(-1))
        return u32(-1);
    nodeState[node] = 2;
    const u32 size = SizeOf(level);
    usedTexels += size * size;
    return node;
}

void LocalAtlasAllocator::Free(u32 node)
{
    VERIFY(node < kLocalAtlasNodes && nodeState[node] == 2);
    u32 level = nodeLevel[node];
    const u32 size = SizeOf(level);
    usedTexels -= size * size;
    nodeState[node] = 0;
    ListPush(node);
    while (level > 0) {
        const u32 levelBase = ((1u << (2u * level)) - 1u) / 3u;
        const u32 sibBase = levelBase + (((node - levelBase) >> 2) << 2);
        bool all = true;
        for (u32 k = 0; k < 4; ++k)
            if (nodeState[sibBase + k] != 0)
                all = false;
        if (!all)
            break;
        for (u32 k = 0; k < 4; ++k) {
            ListRemove(sibBase + k);
            nodeState[sibBase + k] = 3;
        }
        const u32 parentBase = ((1u << (2u * (level - 1))) - 1u) / 3u;
        node = parentBase + ((node - levelBase) >> 2);
        --level;
        nodeState[node] = 0;
        ListPush(node);
    }
}

void LocalAtlasAllocator::Rect(u32 node, u32& x, u32& y, u32& size) const
{
    x = nodeX[node];
    y = nodeY[node];
    size = SizeOf(nodeLevel[node]);
}

void ResetLocalShadowPool(LocalShadowState& state)
{
    state.overflowPages.clear();
    state.activePages = state.atlasLayers = 0;
    state.receiverTiles = nullptr;
    state.staticAtlas = state.dynAtlas = state.hudAtlas = nullptr;
    state.hudViews = 0;
    std::fill_n(state.owners, kLocalTileCount, nullptr);
    std::fill_n(state.request, kLocalTileCount, LocalShadowViewGPU{});
    state.candCount = state.dirtyViews = 0;
    state.stateReset = state.staticAtlasFirst = state.dynAtlasFirst = true;
    state.pooledSpots = state.pooledPoints = 0;
    state.statAccepted = state.statDeferred = state.statDynRefresh = state.statOverflowViews = 0;
    state.statPairs = state.statDynPairs = state.statSkinnedPairs = state.statDrops = state.statDynDrops = 0;
    state.statAtlasPercent = state.statCasterBatches = 0;
    state.slotOfLight.clear();
}

void WarmLocalShadowPool(fg::RenderDevice* device, LocalShadowState& state)
{
    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    if (!nvDevice)
        return;
    R_ASSERT2(EnsureAtlasLayers(nvDevice, state, 1), "Cannot allocate the local shadow atlas");
    R_ASSERT2(EnsureResources(nvDevice, state), "Cannot allocate a complete local shadow page");
    R_ASSERT2(EnsurePipelines(device, state), "Cannot render complete local light shadows without their pipelines");
}

void ProcessLocalShadowStats(LocalShadowState& state, nvrhi::IDevice* device)
{
    if (state.readbackScheduled < LocalShadowState::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    const u32* words = static_cast<const u32*>(device->mapBuffer(oldest, nvrhi::CpuAccessMode::Read));
    if (!words)
        return;
    state.statAccepted = words[0];
    state.statDeferred = words[1];
    state.statOverflowViews = words[2];
    state.statUpToDate = words[3];
    state.statPairs = words[4] + words[5] + words[6] + words[32] + words[33] + words[34];
    state.statDynPairs = words[16] + words[18];
    state.statSkinnedPairs = words[24];
    state.statDrops = words[7] + words[39];
    state.statDynDrops = words[23] + words[31];
    state.statMaxVisited = words[8];
    state.statDynRefresh = words[13];
    device->unmapBuffer(oldest);
}

namespace {
struct ShadowCandidate {
    const light* source;
    u32 lightIndex;
    float desiredSize;
};

void SelectLocalShadowPage(LocalShadowState& state, const xr_vector<ShadowCandidate>& candidates)
{
    ++state.frame;
    state.candCount = 0;
    state.dirtyViews = 0;
    state.statCasterBatches = 0;
    state.pooledSpots = state.pooledPoints = 0;
    xr_vector<u32> sizes, nodes;
    for (const auto& candidate : candidates) {
        const bool point = candidate.source->flags.type == IRender_Light::POINT;
        const u32 lo = point ? kLocalPointFaceMin : kLocalSpotTileMin;
        const u32 hi = point ? kLocalPointFaceMax : kLocalSpotTileMax;
        u32 size = lo;
        while (size < hi && float(size) < candidate.desiredSize)
            size <<= 1;
        sizes.push_back(size);
    }
    // Pack complete lights. A full page fits at minimum sizes, so resolution can
    // decrease under pressure without rejecting a light or any of its six faces.
    for (;;) {
        state.atlas.Reset();
        nodes.clear();
        bool fit = true;
        for (u32 i = 0; i < candidates.size() && fit; ++i) {
            const u32 faces = candidates[i].source->flags.type == IRender_Light::POINT ? 6u : 1u;
            for (u32 f = 0; f < faces; ++f) {
                const u32 node = state.atlas.Alloc(LocalAtlasAllocator::LevelOf(sizes[i]));
                if (node == ~0u) {
                    fit = false;
                    break;
                }
                nodes.push_back(node);
            }
        }
        if (fit)
            break;
        bool reduced = false;
        for (u32 i = 0; i < candidates.size(); ++i) {
            const u32 lo = candidates[i].source->flags.type == IRender_Light::POINT ? kLocalPointFaceMin : kLocalSpotTileMin;
            if (sizes[i] > lo) {
                sizes[i] >>= 1;
                reduced = true;
            }
        }
        R_ASSERT2(reduced, "A local shadow page must fit all of its minimum-size views");
    }

    state.slotOfLight.clear();
    for (const auto& candidate : candidates) {
        const light* L = candidate.source;
        const bool point = L->flags.type == IRender_Light::POINT;
        const u32 base = state.candCount;
        const u32 faces = point ? 6u : 1u;
        const bool newOwner = state.owners[base] != L || state.request[base].meta[2] != u32(point);
        const u32 serial = newOwner ? ++state.nextSerial : state.request[base].meta[1];
        state.slotOfLight.push_back(base + 1u);
        const float farZ = std::max(L->range + EPS_S, 0.002f);
        const float nearZ = clampr(L->virtual_size, 0.001f, farZ * 0.5f);
        const float fov = point ? PI_DIV_2 + deg2rad(11.5f) : L->cone + deg2rad(3.5f);
        for (u32 f = 0; f < faces; ++f) {
            const u32 slot = state.candCount++;
            Fvector dir, up;
            if (point) {
                dir = kFaceDir[f];
                up = kFaceUp[f];
            } else {
                SpotBasis(L, dir, up);
            }
            Fmatrix view, proj, vp;
            view.build_camera_dir(L->position, dir, up);
            proj.build_projection(fov, 1.f, nearZ, farZ);
            vp.mul(proj, view);
            LocalShadowViewGPU rec = {};
            FillRecord(rec, vp, state.atlas, nodes[slot], nearZ, farZ, tanf(fov * 0.5f), L->position, L->range);
            rec.shape.y = float(state.atlasLayer);
            const auto& previous = state.request[slot];
            // Exact transform/rectangle comparison includes virtual size and roll.
            // The baseline is a requested revision that is rendered this frame;
            // sub-threshold movement can no longer accumulate against an old map.
            const bool changed = newOwner || state.owners[slot] != L
                || memcmp(&rec, &previous, offsetof(LocalShadowViewGPU, meta)) != 0;
            u32 stamp = previous.meta[0] + u32(changed);
            if (stamp == 0u)
                stamp = 1u;
            rec.meta[0] = stamp;
            rec.meta[1] = serial;
            rec.meta[2] = point ? 1u : 0u;
            rec.meta[3] = f;
            state.dirtyViews += u32(changed);
            state.request[slot] = rec;
            state.owners[slot] = L;
            state.candList[slot][0] = slot;
            state.candList[slot][1] = stamp;
            state.candList[slot][2] = serial;
            state.candList[slot][3] = slot | (3u << 30); // Visible; refresh dynamic depth every frame.
        }
        if (point)
            ++state.pooledPoints;
        else
            ++state.pooledSpots;
    }
    state.statAtlasPercent = u32(u64(state.atlas.UsedTexels()) * 100u / (u64(kLocalShadowAtlas) * kLocalShadowAtlas));
}
} // namespace

static bool ViewTouchesSphere(const LocalShadowViewGPU& rec, const Fvector4& sphere, bool planes)
{
    Fvector c;
    c.set(sphere.x, sphere.y, sphere.z);
    Fvector lp;
    lp.set(rec.lightPos.x, rec.lightPos.y, rec.lightPos.z);
    const float reach = rec.lightPos.w + sphere.w;
    if (lp.distance_to_sqr(c) > reach * reach)
        return false;
    if (!planes)
        return true;
    for (u32 i = 0; i < 6; ++i) {
        const Fvector4& p = rec.planes[i];
        if (p.x * c.x + p.y * c.y + p.z * c.z + p.w > sphere.w)
            return false;
    }
    return true;
}

static bool AllocHudRect(LocalShadowState& state, u32 pageSlot, Fvector4& outRect)
{
    if (state.hudViews >= kLocalHudViewsMax)
        return false;
    const u32 node = state.hudAlloc.Alloc(LocalAtlasAllocator::LevelOf(kLocalHudTileSize));
    if (node == ~0u)
        return false;
    u32 x = 0, y = 0, size = 1;
    state.hudAlloc.Rect(node, x, y, size);
    outRect.set(0.0f, float(x), float(y), float(size));
    state.hudSlots[state.hudViews++] = pageSlot;
    return true;
}

static void SelectLocalShadowHudViews(LocalShadowState& state, const HudShadowFit* fit)
{
    state.hudViews = 0;
    state.hudAlloc.Reset();
    for (u32 page = 0; page < state.activePages; ++page) {
        LocalShadowState& current = page == 0 ? state : *state.overflowPages[page - 1];
        for (u32 slot = 0; slot < current.candCount;) {
            LocalShadowViewGPU& rec = current.request[slot];
            const bool point = rec.meta[2] != 0u;
            const u32 faces = point ? 6u : 1u;
            for (u32 f = 0; f < faces; ++f)
                current.request[slot + f].hud.set(0.0f, 0.0f, 0.0f, 0.0f);
            if (!fit || !ViewTouchesSphere(rec, fit->trueSphere, !point)) {
                slot += faces;
                continue;
            }
            const light* L = current.owners[slot];
            const float flags = float(kLocalHudCasters | (L->flags.bCastHudToWorld ? kLocalHudToWorld : 0u));
            Fvector center;
            center.set(fit->shownSphere.x, fit->shownSphere.y, fit->shownSphere.z);
            Fvector lightPos;
            lightPos.set(rec.lightPos.x, rec.lightPos.y, rec.lightPos.z);
            fit->warp.transform_tiny(lightPos);
            Fvector toCenter;
            toCenter.sub(center, lightPos);
            const float dist = toCenter.magnitude();
            const float r = fit->shownSphere.w;
            if (dist > r + 0.01f) {
                Fvector4 rect;
                if (!AllocHudRect(state, page * kLocalTileCount + slot, rect)) {
                    slot += faces;
                    continue;
                }
                rect.x = flags;
                Fvector dir;
                dir.div(toCenter, dist);
                Fvector up, right;
                Fvector::generate_orthonormal_basis_normalized(dir, up, right);
                const float halfFov = asinf(std::min(r / dist, 0.999f)) + deg2rad(1.0f);
                const float nearZ = std::max(dist - r, 0.001f);
                const float farZ = std::max(rec.zparams.y, dist + r);
                Fmatrix view, proj, vp;
                view.build_camera_dir(lightPos, dir, up);
                proj.build_projection(2.0f * halfFov, 1.f, nearZ, farZ);
                vp.mul(proj, view);
                for (u32 f = 0; f < faces; ++f) {
                    LocalShadowViewGPU& face = current.request[slot + f];
                    face.hud = rect;
                    face.hudZ.set(nearZ, farZ, 2.0f * tanf(halfFov) / float(kLocalHudTileSize), ps_r_local_shadow_hud_bias);
                    face.hudViewProj = vp;
                }
            } else {
                for (u32 f = 0; f < faces; ++f) {
                    LocalShadowViewGPU& face = current.request[slot + f];
                    Fvector4 rect;
                    if (!AllocHudRect(state, page * kLocalTileCount + slot + f, rect))
                        break;
                    rect.x = flags;
                    Fvector dir, up, right;
                    if (point) {
                        dir = kFaceDir[f];
                    } else {
                        SpotBasis(L, dir, up);
                        fit->warp.transform_dir(dir);
                        dir.normalize_safe();
                    }
                    Fvector::generate_orthonormal_basis_normalized(dir, up, right);
                    const float tanHalf = 0.5f * face.zparams.z * face.rect.z;
                    Fmatrix view, proj;
                    view.build_camera_dir(lightPos, dir, up);
                    proj.build_projection(2.0f * atanf(tanHalf), 1.f, face.zparams.x, face.zparams.y);
                    face.hud = rect;
                    face.hudZ.set(face.zparams.x, face.zparams.y, 2.0f * tanHalf / float(kLocalHudTileSize), ps_r_local_shadow_hud_bias);
                    face.hudViewProj.mul(proj, view);
                }
            }
            slot += faces;
        }
    }
}

void SelectLocalShadowLights(
    LocalShadowState& state,
    const xr_vector<const light*>& lights,
    const Fvector& camPos,
    float projScale,
    const HudShadowFit* hudFit)
{
    xr_vector<ShadowCandidate> spots, points;
    for (u32 i = 0; i < lights.size(); ++i) {
        const light* L = lights[i];
        if (!L || !L->flags.bActive || !L->flags.bShadow)
            continue;
        const bool point = L->flags.type == IRender_Light::POINT;
        if (!point && L->flags.type != IRender_Light::SPOT)
            continue;
        const float dist = camPos.distance_to(L->position);
        const float radius = dist <= L->range ? 1e9f : projScale * L->range / std::max(dist, 1e-3f);
        float desired = 2.0f * radius * ps_r_local_shadow_texel_ratio;
        if (dist - L->range > ps_r_local_shadow_range || radius < ps_r_local_shadow_min_px)
            desired = 0.0f;
        (point ? points : spots).push_back({ L, i, desired });
    }
    auto nearest = [&](const ShadowCandidate& a, const ShadowCandidate& b) {
        const float da = camPos.distance_to_sqr(a.source->position);
        const float db = camPos.distance_to_sqr(b.source->position);
        return da != db ? da < db : std::less<const light*>()(a.source, b.source);
    };
    std::sort(spots.begin(), spots.end(), nearest);
    std::sort(points.begin(), points.end(), nearest);
    // Legacy pool controls now choose which lights receive the larger tiles.
    // Every remaining eligible light still gets a complete minimum-resolution map.
    for (u32 i = u32(std::max(0, ps_r_local_shadow_spots)); i < spots.size(); ++i)
        spots[i].desiredSize = 0.0f;
    for (u32 i = u32(std::max(0, ps_r_local_shadow_points)); i < points.size(); ++i)
        points[i].desiredSize = 0.0f;

    xr_vector<u32> slots(lights.size(), 0u);
    u32 si = 0, pi = 0, page = 0;
    while (si < spots.size() || pi < points.size()) {
        xr_vector<ShadowCandidate> batch;
        for (u32 count = 0; si < spots.size() && count < kLocalSpotSlotsMax; ++count)
            batch.push_back(spots[si++]);
        const u32 pointRoom = std::min(kLocalPointLightsMax, (kLocalTileCount - u32(batch.size())) / 6u);
        for (u32 count = 0; pi < points.size() && count < pointRoom; ++count)
            batch.push_back(points[pi++]);
        if (page > state.overflowPages.size())
            state.overflowPages.emplace_back(xr_new<LocalShadowState>());
        LocalShadowState& current = page == 0 ? state : *state.overflowPages[page - 1];
        current.atlasLayer = page;
        SelectLocalShadowPage(current, batch);
        for (u32 i = 0; i < batch.size(); ++i)
            slots[batch[i].lightIndex] = page * kLocalTileCount + current.slotOfLight[i];
        ++page;
    }
    state.slotOfLight = std::move(slots);
    state.activePages = page;
    state.pooledSpots = u32(spots.size());
    state.pooledPoints = u32(points.size());
    SelectLocalShadowHudViews(state, hudFit);
}

static LocalShadowOutput SetupLocalShadowPage(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    const LocalShadowConfig& config,
    LocalShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler,
    VirtualResourceHandle staticHandle, VirtualResourceHandle dynHandle)
{
    LocalShadowOutput out;
    if (!state || !device)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;
    R_ASSERT2(EnsureResources(nvDevice, *state), "Cannot allocate a complete local shadow page");
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
            data.args = passBuilder.write(binData.args, ResourceState::IndirectArgument);
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
            data.args = passBuilder.write(staticData.args, ResourceState::IndirectArgument);
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

LocalShadowOutput setupLocalShadowPasses(
    FrameGraph& fg, fg::RenderDevice* device, VirtualResourceHandle orderAfter,
    const LocalShadowConfig& config, LocalShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    LocalShadowOutput out;
    if (!state || !device || state->activePages == 0)
        return out;
    auto* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;
    R_ASSERT2(EnsureAtlasLayers(nvDevice, *state, state->activePages), "Cannot allocate local shadow overflow pages");
    auto textureDesc = [&](const char* name) {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2DArray;
        desc.width = desc.height = kLocalShadowAtlas;
        desc.arraySize = state->atlasLayers;
        desc.format = nvrhi::Format::D32;
        desc.isDepthStencil = true;
        desc.isImported = true;
        desc.isTransient = false;
        desc.debugName = name;
        return desc;
    };
    auto staticHandle = fg.ImportTexture("rt_LocalShadowStatic", state->staticAtlas, textureDesc("rt_LocalShadowStatic"));
    auto dynHandle = fg.ImportTexture("rt_LocalShadowDyn", state->dynAtlas, textureDesc("rt_LocalShadowDyn"));
    xr_vector<VirtualResourceHandle> pageTiles;
    for (u32 i = 0; i < state->activePages; ++i) {
        auto& page = i == 0 ? *state : *state->overflowPages[i - 1];
        page.staticAtlas = state->staticAtlas;
        page.dynAtlas = state->dynAtlas;
        auto rendered = SetupLocalShadowPage(fg, device, orderAfter, config, &page, gpuProfiler, staticHandle, dynHandle);
        R_ASSERT(rendered.active);
        staticHandle = rendered.staticAtlas;
        dynHandle = rendered.dynAtlas;
        pageTiles.push_back(rendered.tiles);
    }
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Buffer;
    desc.bufferSize = state->receiverTiles->getDesc().byteSize;
    desc.structStride = sizeof(LocalShadowViewGPU);
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = "local_shadow_receivers";
    auto receivers = fg.ImportBuffer("local_shadow_receivers", state->receiverTiles, desc);
    struct PublishData { VirtualResourceHandle tiles; LocalShadowState* state; };
    auto& published = fg.addCallbackPass<PublishData>("Local Shadow Publish",
        [=](FrameGraph& builder, PassHandle handle, PublishData& data) {
            RenderPassBuilder pass(builder, handle);
            pass.read(staticHandle, ResourceState::ShaderResource);
            pass.read(dynHandle, ResourceState::ShaderResource);
            for (auto tiles : pageTiles)
                pass.read(tiles, ResourceState::CopySource);
            data.tiles = pass.write(receivers, ResourceState::CopyDest);
            data.state = state;
        },
        [](const PublishData& data, const FrameGraph&, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            const u64 bytes = u64(kLocalTileCount) * sizeof(LocalShadowViewGPU);
            cmd->setBufferState(data.state->receiverTiles, nvrhi::ResourceStates::CopyDest);
            for (u32 i = 0; i < data.state->activePages; ++i) {
                auto& page = i == 0 ? *data.state : *data.state->overflowPages[i - 1];
                cmd->setBufferState(page.stateBuffer, nvrhi::ResourceStates::CopySource);
                cmd->copyBuffer(data.state->receiverTiles, u64(i) * bytes, page.stateBuffer, 0, bytes);
            }
            cmd->setBufferState(data.state->receiverTiles, nvrhi::ResourceStates::ShaderResource);
        });
    R_ASSERT2(EnsureHudAtlas(nvDevice, *state), "Cannot allocate the local shadow HUD atlas");
    ResourceDesc hudDesc;
    hudDesc.type = ResourceDesc::Type::Texture2D;
    hudDesc.width = hudDesc.height = kLocalShadowAtlas;
    hudDesc.format = nvrhi::Format::D32;
    hudDesc.isDepthStencil = true;
    hudDesc.isImported = true;
    hudDesc.isTransient = false;
    hudDesc.debugName = "rt_LocalShadowHud";
    auto hudHandle = fg.ImportTexture("rt_LocalShadowHud", state->hudAtlas, hudDesc);
    auto& hudData = fg.addCallbackPass<LocalShadowHudData>("Local Shadow HUD",
        [&, hudHandle, orderAfter, config, state, gpuProfiler](FrameGraph& builder, PassHandle handle, LocalShadowHudData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder pass(builder, handle);
            data.atlas = pass.write(hudHandle, ResourceState::DepthStencilWrite);
            data.tiles = pass.read(published.tiles, ResourceState::ShaderResource);
            if (orderAfter.is_valid())
                data.order = pass.read(orderAfter, ResourceState::ShaderResource);
        },
        [](const LocalShadowHudData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteHud(ctx, fg, data);
        });
    fg.GetRTRegistry().RegisterRT("rt_LocalShadowHud", hudData.atlas);
    out.tiles = published.tiles;
    out.staticAtlas = staticHandle;
    out.dynAtlas = dynHandle;
    out.hudAtlas = hudData.atlas;
    out.state = state;
    out.active = true;
    return out;
}

void ResolveLocalShadowBindings(
    const framegraph::FrameGraph& fg,
    const LocalShadowOutput& out,
    nvrhi::IDevice* device,
    nvrhi::IBuffer*& tiles,
    nvrhi::ITexture*& staticAtlas,
    nvrhi::ITexture*& dynAtlas,
    nvrhi::ITexture*& hudAtlas)
{
    tiles = out.state ? out.state->receiverTiles.Get() : nullptr;
    if (!tiles)
        tiles = FallbackTiles(device);
    staticAtlas = nullptr;
    dynAtlas = nullptr;
    hudAtlas = nullptr;
    if (out.active) {
        staticAtlas = fg.GetPhysicalTexture(out.staticAtlas);
        dynAtlas = fg.GetPhysicalTexture(out.dynAtlas);
        hudAtlas = fg.GetPhysicalTexture(out.hudAtlas);
    }
    if (!staticAtlas)
        staticAtlas = GetPassResourceCache().GetDummyShadowMap(device);
    if (!dynAtlas)
        dynAtlas = GetPassResourceCache().GetDummyShadowMap(device);
    if (!hudAtlas)
        hudAtlas = GetPassResourceCache().GetDummyShadowMap2D(device);
}

}
