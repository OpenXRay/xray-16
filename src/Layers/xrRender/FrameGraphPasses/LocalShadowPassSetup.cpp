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
    u32 entryBase;
    u32 entryCount;
    u32 refreshCount;
    u32 statsBase;
    u32 capOpaque;
    u32 capTerrain;
    u32 capAT;
    u32 includeAT;
    float errK;
    u32 pad[3];
};

struct LocalShadowArgsParams {
    u32 caps[8];
    u32 refreshStaticCount;
    u32 refreshDynCount;
    u32 pad[2];
};

struct LocalShadowBinData {
    VirtualResourceHandle tiles;
    VirtualResourceHandle args;
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
    LocalShadowState* state;
    fg::RenderDevice* device;
    LocalShadowConfig config;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct LocalShadowDynData {
    VirtualResourceHandle atlas;
    VirtualResourceHandle tiles;
    VirtualResourceHandle args;
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
        desc.byteSize = u64(kLocalTileCount) * sizeof(LocalShadowTileGPU);
        desc.structStride = sizeof(LocalShadowTileGPU);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_tiles = nvDevice->createBuffer(desc);
    }
    return s_tiles;
}

bool EnsureResources(nvrhi::IDevice* nvDevice, LocalShadowState& state)
{
    if (state.tiles && state.staticAtlas)
        return true;
    if (state.resourcesFailed)
        return false;

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "LocalShadow_Tiles";
        desc.byteSize = u64(kLocalTileCount) * sizeof(LocalShadowTileGPU);
        desc.structStride = sizeof(LocalShadowTileGPU);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        state.tiles = nvDevice->createBuffer(desc);
    }
    state.refreshStaticBuffer = MakeUAVBuffer(nvDevice, "LocalShadow_RefreshStatic", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), false);
    state.refreshDynBuffer = MakeUAVBuffer(nvDevice, "LocalShadow_RefreshDyn", u64(kLocalTileCount) * sizeof(u32), sizeof(u32), false);
    state.stats = MakeUAVBuffer(nvDevice, "LocalShadow_Stats", u64(kLocalStatWords) * sizeof(u32), sizeof(u32), false);
    for (u32 i = 0; i < kLocalStreamCount; ++i) {
        string64 nm;
        xr_sprintf(nm, "LocalShadow_Pairs%s", kLocalStreamNames[i]);
        state.pairs[i] = MakeUAVBuffer(nvDevice, nm, u64(kLocalPairCaps[i]) * sizeof(u32) * 2, sizeof(u32) * 2, false);
    }
    state.args = MakeArgsBuffer(nvDevice, "LocalShadow_Args", kLocalStreamCount);
    state.clearArgs = MakeArgsBuffer(nvDevice, "LocalShadow_ClearArgs", 2);
    state.staticAtlas = MakeAtlas(nvDevice, "LocalShadow_Static");
    state.dynAtlas = MakeAtlas(nvDevice, "LocalShadow_Dyn");

    bool ok = state.tiles && state.refreshStaticBuffer && state.refreshDynBuffer && state.stats
        && state.args && state.clearArgs && state.staticAtlas && state.dynAtlas;
    for (u32 i = 0; i < kLocalStreamCount; ++i)
        ok = ok && state.pairs[i];
    if (!ok) {
        Msg("! [LocalShadow] resource creation failed");
        state.tiles = nullptr;
        state.staticAtlas = nullptr;
        state.resourcesFailed = true;
        return false;
    }
    Msg("* [LocalShadow] atlases %ux%u D16, %u spot tiles + %u point faces", kLocalShadowAtlas, kLocalShadowAtlas, kLocalSpotSlots, kLocalPointSlots);
    return true;
}

bool EnsurePipelines(fg::RenderDevice* device, LocalShadowState& state)
{
    if (state.binPipeline && state.argsPipeline && state.clearPipeline && state.pagePipeline && state.pageATPipeline && state.skinPagePipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto binResult = shaderLoader->LoadComputeShader("local_shadow_bin");
    auto argsResult = shaderLoader->LoadComputeShader("local_shadow_args");
    auto clearVsResult = shaderLoader->LoadVertexShader("local_shadow_clear", "main");
    auto clearPsResult = shaderLoader->LoadPixelShader("vsm_clear", "main");
    auto pageVsResult = shaderLoader->LoadVertexShader("local_shadow_pull", "main");
    auto skinVsResult = shaderLoader->LoadVertexShader("local_shadow_pull_skinned", "main");
    auto pagePsResult = shaderLoader->LoadPixelShader("vsm_page", "main");
    auto pageATPsResult = shaderLoader->LoadPixelShader("vsm_page_at", "main");
    if (!binResult.handle || !binResult.reflection || !argsResult.handle || !argsResult.reflection
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
    state.binLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowBin", *binResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowArgs", *argsResult.reflection, nvDevice);
    state.clearLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowClear", *clearVsResult.reflection, *clearPsResult.reflection, nvDevice);
    state.pageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPage", *pageVsResult.reflection, *pagePsResult.reflection, nvDevice);
    state.pageATLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowPageAT", *pageVsResult.reflection, *pageATPsResult.reflection, nvDevice);
    state.skinPageLayout = cache.GetOrCreateBindingLayoutFromReflection("LocalShadowSkinPage", *skinVsResult.reflection, *pagePsResult.reflection, nvDevice);
    if (!state.binLayout || !state.argsLayout || !state.clearLayout || !state.pageLayout || !state.pageATLayout || !state.skinPageLayout) {
        state.pipelinesFailed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc binDesc;
    binDesc.CS = binResult.handle;
    binDesc.bindingLayouts = { state.binLayout };
    state.binPipeline = cache.GetOrCreateComputePipeline("LocalShadowBin", binDesc, nvDevice);

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

    if (!state.binPipeline || !state.argsPipeline || !state.clearPipeline || !state.pagePipeline || !state.pageATPipeline || !state.skinPagePipeline) {
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
    auto* binRefl = shaderLoader->GetCachedReflection("local_shadow_bin", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("local_shadow_args", ".cs");
    if (!binRefl || !argsRefl)
        return;

    if (data.gpuProfiler)
        data.gpuProfiler->BeginPass(cmdList, "Local Shadow.Bin");

    cmdList->setBufferState(state.tiles, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.refreshStaticBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(state.tiles, state.records, sizeof(state.records));
    cmdList->writeBuffer(state.refreshStaticBuffer, state.refreshStatic, sizeof(state.refreshStatic));
    cmdList->writeBuffer(state.refreshDynBuffer, state.refreshDyn, sizeof(state.refreshDyn));
    cmdList->clearBufferUInt(state.stats, 0);
    cmdList->setBufferState(state.tiles, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.refreshStaticBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.refreshDynBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::UnorderedAccess);
    for (u32 i = 0; i < kLocalStreamCount; ++i)
        cmdList->setBufferState(state.pairs[i], nvrhi::ResourceStates::UnorderedAccess);

    const LocalShadowConfig& cfg = data.config;
    GPUCullingManager* gpuCulling = cfg.gpuCulling;

    auto dispatchSource = [&](nvrhi::IBuffer* entries, u32 entryBase, u32 entryCount, u32 statsBase,
                              nvrhi::IBuffer* refresh, u32 refreshCount, u32 streamOpaque, u32 streamTerrain, u32 streamAT,
                              u32 capOpaque, u32 capTerrain, u32 capAT, const char* label) {
        if (!entries || entryCount == 0 || refreshCount == 0)
            return;
        LocalShadowBinParams bp = {};
        bp.entryBase = entryBase;
        bp.entryCount = entryCount;
        bp.refreshCount = refreshCount;
        bp.statsBase = statsBase;
        bp.capOpaque = capOpaque;
        bp.capTerrain = capTerrain;
        bp.capAT = capAT;
        bp.includeAT = ps_r_vsm_at ? 1u : 0u;
        bp.errK = std::max(0.1f, ps_r_vsm_cluster_lod);
        auto binCB = cache.GetOrCreateVolatileCB("LocalShadow", "BinParams", sizeof(LocalShadowBinParams), data.device, 64);
        cmdList->writeBuffer(binCB, &bp, sizeof(bp));
        cmdList->setBufferState(entries, nvrhi::ResourceStates::ShaderResource);

        BindingSetBuilder bsb(*binRefl, nvDevice, label);
        bsb.ConstantBuffer("LocalShadowBinParams", binCB)
           .BufferSRV("g_Entries", entries)
           .BufferSRV("g_Tiles", state.tiles)
           .BufferSRV("g_Refresh", refresh)
           .BufferUAV("g_Stats", state.stats)
           .BufferUAV("g_PairsOpaque", state.pairs[streamOpaque])
           .BufferUAV("g_PairsTerrain", state.pairs[streamTerrain])
           .BufferUAV("g_PairsAT", state.pairs[streamAT]);
        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.binLayout, nvDevice);
        if (!bindingSet)
            return;
        nvrhi::ComputeState cs;
        cs.pipeline = state.binPipeline;
        cs.bindings = { bindingSet };
        cmdList->setComputeState(cs);
        cmdList->dispatch((entryCount + 63) / 64, 1, 1);
    };

    if (gpuCulling && cfg.entryBuffer) {
        dispatchSource(cfg.entryBuffer, 0, gpuCulling->GetClusterEntryCount(), 0u,
            state.refreshStaticBuffer, state.refreshStaticCount, 0, 1, 2,
            kLocalPairCapOpaque, kLocalPairCapTerrain, kLocalPairCapAT, "LocalShadow.Bin");
        dispatchSource(cfg.entryBuffer, gpuCulling->GetClusterEntryCount(), gpuCulling->GetDynamicClusterEntryCount(), 8u,
            state.refreshDynBuffer, state.refreshDynCount, 3, 5, 4,
            kLocalPairCapDynOpaque, 0u, kLocalPairCapDynAT, "LocalShadow.BinDyn");
        dispatchSource(gpuCulling->GetSkinnedEntryBuffer(), 0, gpuCulling->GetSkinnedEntryCount(), 16u,
            state.refreshDynBuffer, state.refreshDynCount, 5, 1, 2,
            kLocalPairCapSkinned, 0u, 0u, "LocalShadow.BinSkinned");
    }

    LocalShadowArgsParams ap = {};
    ap.caps[0] = kLocalPairCapOpaque;
    ap.caps[1] = kLocalPairCapTerrain;
    ap.caps[2] = kLocalPairCapAT;
    ap.caps[3] = kLocalPairCapDynOpaque;
    ap.caps[4] = kLocalPairCapDynAT;
    ap.caps[5] = kLocalPairCapSkinned;
    ap.refreshStaticCount = state.refreshStaticCount;
    ap.refreshDynCount = state.refreshDynCount;
    auto argsCB = cache.GetOrCreateVolatileCB("LocalShadow", "ArgsParams", sizeof(LocalShadowArgsParams), data.device, 64);
    cmdList->writeBuffer(argsCB, &ap, sizeof(ap));
    cmdList->setBufferState(state.args, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.clearArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.stats, nvrhi::ResourceStates::ShaderResource);

    BindingSetBuilder abs(*argsRefl, nvDevice, "LocalShadow.Args");
    abs.ConstantBuffer("LocalShadowArgsParams", argsCB)
       .BufferSRV("g_Stats", state.stats)
       .BufferUAV("g_Args", state.args)
       .BufferUAV("g_ClearArgs", state.clearArgs);
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
    srv(state.tiles);
    srv(state.refreshStaticBuffer);
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

    auto* backend = device->GetBackend();
    out.cmdList = cmdList;
    out.nvDevice = nvDevice;
    out.framebuffer = framebuffer;
    out.bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
    out.viewport = nvrhi::Viewport(0.0f, float(kLocalShadowAtlas), 0.0f, float(kLocalShadowAtlas), 0.0f, 1.0f);
    out.scissor = nvrhi::Rect(kLocalShadowAtlas, kLocalShadowAtlas);

    BindingSetBuilder bsb(*clearVsRefl, *clearPsRefl, nvDevice, "LocalShadow.Clear");
    bsb.BufferSRV("g_Refresh", clearIndex == 0 ? state.refreshStaticBuffer : state.refreshDynBuffer);
    bsb.BufferSRV("g_LocalShadowTiles", state.tiles);
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
    if (state.refreshStaticCount == 0)
        return;

    const LocalShadowConfig& cfg = data.config;
    if (!cfg.entryBuffer || !cfg.megaVertexBuffer || !cfg.megaIndexBuffer)
        return;

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
        bsb.BufferSRV("g_LocalShadowTiles", state.tiles);
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

    drawStream(0, state.pagePipeline, state.pageLayout, *psRefl, cfg.staticInstanceBuffer, false, "LocalShadow.PageOpaque");
    drawStream(1, state.pagePipeline, state.pageLayout, *psRefl, cfg.terrainInstanceBuffer, false, "LocalShadow.PageTerrain");
    drawStream(2, state.pageATPipeline, state.pageATLayout, *atRefl, cfg.staticInstanceBuffer, true, "LocalShadow.PageAT");

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
    if (state.refreshDynCount == 0)
        return;

    const LocalShadowConfig& cfg = data.config;
    if (!cfg.gpuCulling || !cfg.megaVertexBuffer || !cfg.megaIndexBuffer)
        return;

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
        bsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
        bsb.BufferSRV("g_Pairs", state.pairs[3]);
        bsb.BufferSRV("g_Entries", cfg.entryBuffer);
        bsb.BufferSRV("g_LocalShadowTiles", state.tiles);
        bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);
        if (auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.pageLayout, nvDevice))
            draw(state.pagePipeline, bindingSet, 3, false);

        BindingSetBuilder atBsb(*vsRefl, *atRefl, nvDevice, "LocalShadow.DynAT");
        atBsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
        atBsb.BufferSRV("g_Pairs", state.pairs[4]);
        atBsb.BufferSRV("g_Entries", cfg.entryBuffer);
        atBsb.BufferSRV("g_LocalShadowTiles", state.tiles);
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
        bsb.BufferSRV("g_Pairs", state.pairs[5]);
        bsb.BufferSRV("g_Entries", skinnedEntries);
        bsb.BufferSRV("g_LocalShadowTiles", state.tiles);
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

void FillRecord(LocalShadowTileGPU& rec, const Fmatrix& vp, float rectX, float rectY, float tileSize,
                float nearZ, float farZ, float texelPerMetre, const Fvector& pos, float range)
{
    rec.viewProj = vp;
    rec.rect.set(rectX, rectY, tileSize, ps_r_local_shadow_bias);
    rec.zparams.set(nearZ, farZ, texelPerMetre, 1.0f);
    rec.lightPos.set(pos.x, pos.y, pos.z, range);
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
    for (u32 i = 0; i < kLocalPointLights; ++i)
        state.points[i] = LocalTile();
    for (u32 i = 0; i < kLocalTileCount; ++i)
        state.records[i] = LocalShadowTileGPU();
    state.refreshStaticCount = 0;
    state.refreshDynCount = 0;
    state.pooledSpots = 0;
    state.pooledPoints = 0;
    state.slotOfLight.clear();
}

void SelectLocalShadowLights(
    LocalShadowState& state,
    const xr_vector<const light*>& lights,
    const Fvector& camPos,
    const Fmatrix& camViewProj)
{
    ++state.frame;
    const u32 frame = state.frame;
    state.slotOfLight.assign(lights.size(), 0u);
    if (state.resourcesFailed)
        return;
    state.refreshStaticCount = 0;
    state.refreshDynCount = 0;
    state.pooledSpots = 0;
    state.pooledPoints = 0;

    CFrustum camFrustum;
    Fmatrix camMatrix = camViewProj;
    camFrustum.CreateFromMatrix(camMatrix, FRUSTUM_P_ALL);

    struct Candidate { u32 index; float eff; };
    xr_vector<Candidate> spotCandidates;
    xr_vector<Candidate> pointCandidates;

    for (u32 i = 0; i < lights.size(); ++i) {
        const light* L = lights[i];
        if (!L || !L->flags.bActive || !L->flags.bShadow || L->flags.bHudMode)
            continue;
        const float d2 = camPos.distance_to_sqr(L->position);
        if (L->flags.type == IRender_Light::SPOT) {
            const float eff = d2 * (L->cone < deg2rad(60.f) ? 0.25f : 1.0f);
            spotCandidates.push_back({ i, eff });
        } else if (L->flags.type == IRender_Light::POINT) {
            if (L->range < 3.0f || d2 > 30.0f * 30.0f)
                continue;
            pointCandidates.push_back({ i, d2 });
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
            pool[best].staticValid = false;
            pool[best].lastSeen = frame;
            outTile[c] = best;
        }
    };

    xr_vector<u32> spotTile;
    xr_vector<u32> pointTile;
    assign(spotCandidates, state.spots, kLocalSpotSlots, spotTile);
    assign(pointCandidates, state.points, kLocalPointLights, pointTile);

    for (u32 i = 0; i < kLocalTileCount; ++i)
        state.records[i].zparams.w = 0.0f;

    auto pushRefresh = [&](u32 slot, bool needStatic, bool inView) {
        if (needStatic && state.refreshStaticCount < kLocalTileCount)
            state.refreshStatic[state.refreshStaticCount++] = slot;
        if (inView && state.refreshDynCount < kLocalTileCount)
            state.refreshDyn[state.refreshDynCount++] = slot;
    };

    for (u32 c = 0; c < spotCandidates.size(); ++c) {
        const u32 t = spotTile[c];
        if (t == u32(-1))
            continue;
        const light* L = lights[spotCandidates[c].index];
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

        const float fov = L->cone + deg2rad(3.5f);
        const float nearZ = 0.5f;
        const float farZ = std::max(L->range, 1.0f);
        Fmatrix view, proj, vp;
        view.build_camera_dir(L->position, dir, up);
        proj.build_projection(fov, 1.f, nearZ, farZ);
        vp.mul(proj, view);

        const u32 slot = t;
        LocalShadowTileGPU& rec = state.records[slot];
        FillRecord(rec, vp, float((t & 3) * kLocalSpotTile), float((t >> 2) * kLocalSpotTile), float(kLocalSpotTile),
            nearZ, farZ, 2.0f * tanf(fov * 0.5f) / float(kLocalSpotTile), L->position, L->range);

        const bool needStatic = !tile.staticValid || moved;
        tile.inView = camFrustum.testSphere_dirty(L->position, L->range);
        tile.staticValid = true;
        pushRefresh(slot, needStatic, tile.inView);
        state.slotOfLight[spotCandidates[c].index] = slot + 1;
        ++state.pooledSpots;
    }

    for (u32 c = 0; c < pointCandidates.size(); ++c) {
        const u32 t = pointTile[c];
        if (t == u32(-1))
            continue;
        const light* L = lights[pointCandidates[c].index];
        LocalTile& tile = state.points[t];
        const bool newOwner = tile.owner != L;
        const bool moved = newOwner
            || tile.pos.distance_to_sqr(L->position) > 0.002f * 0.002f
            || _abs(tile.range - L->range) > 0.01f;
        tile.owner = L;
        tile.pos = L->position;
        tile.range = L->range;

        const float nearZ = 0.25f;
        const float farZ = std::max(L->range, 1.0f);
        const bool needStatic = !tile.staticValid || moved;
        tile.inView = camFrustum.testSphere_dirty(L->position, L->range);
        tile.staticValid = true;

        for (u32 f = 0; f < 6; ++f) {
            const u32 k = t * 6 + f;
            const u32 slot = kLocalSpotSlots + k;
            Fmatrix view, proj, vp;
            Fvector fd = kFaceDir[f];
            Fvector fu = kFaceUp[f];
            view.build_camera_dir(L->position, fd, fu);
            proj.build_projection(PI_DIV_2, 1.f, nearZ, farZ);
            vp.mul(proj, view);
            FillRecord(state.records[slot], vp, float((k & 7) * kLocalPointFace), float(2048 + (k >> 3) * kLocalPointFace),
                float(kLocalPointFace), nearZ, farZ, 2.0f / float(kLocalPointFace), L->position, L->range);
            pushRefresh(slot, needStatic, tile.inView);
        }
        state.slotOfLight[pointCandidates[c].index] = kLocalSpotSlots + t * 6 + 1;
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
    VirtualResourceHandle tilesHandle = fg.ImportBuffer("local_shadow_tiles", state->tiles,
        bufferDesc("local_shadow_tiles", u64(kLocalTileCount) * sizeof(LocalShadowTileGPU), sizeof(LocalShadowTileGPU), false));
    VirtualResourceHandle argsHandle = fg.ImportBuffer("local_shadow_args", state->args,
        bufferDesc("local_shadow_args", u64(kLocalStreamCount) * kLocalArgsStride, 0, true));

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
        [&, tilesHandle, argsHandle, orderAfter, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, LocalShadowBinData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.tiles = passBuilder.write(tilesHandle, ResourceState::ShaderResource);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::ShaderResource);
            data.args = passBuilder.write(argsHandle, ResourceState::UnorderedAccess);
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
    tiles = out.state ? out.state->tiles.Get() : nullptr;
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
