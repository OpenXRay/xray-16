#include "stdafx.h"
#include "DeferredLightPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "VSMPassSetup.h"
#include "LocalShadowPassSetup.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct alignas(16) TileParams {
    u32 tilesX;
    u32 tilesY;
    u32 maxTiles;
    u32 listBase;
    u32 forceMixed;
    u32 pad[3];
};
static_assert(sizeof(TileParams) == 32, "TileParams must be 32 bytes");

constexpr const char* kTileShaderNames[kLightTileClasses] = {
    "deferred_light_tile_lit",
    "deferred_light_tile_mixed",
    "deferred_light_tile_lit_lights",
    "deferred_light_tile_mixed_lights",
};
constexpr const char* kTileProfilerNames[kLightTileClasses] = {
    "Deferred Light.Lit",
    "Deferred Light.Mixed",
    "Deferred Light.LitLights",
    "Deferred Light.MixedLights",
};
constexpr u32 kTileClassSunMixed = 1;
constexpr u32 kTileClassLights = 2;
constexpr u32 kTileArgsStride = sizeof(u32) * 3;
constexpr u32 kTileArgsBytes = kTileArgsStride * kLightTileClasses;

struct DeferredLightPassData {
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle material;
    VirtualResourceHandle color;
    VirtualResourceHandle sunMask;
    VirtualResourceHandle localTiles;
    VirtualResourceHandle localStatic;
    VirtualResourceHandle localDyn;
    VirtualResourceHandle localHud;
    LocalShadowOutput localShadow;
    fg::RenderDevice* device = nullptr;
    DeferredLightPassState* state = nullptr;
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
    u32 width = 0;
    u32 height = 0;
};

bool LoadComputePass(fg::RenderDevice* device, const char* shaderName, const char* cacheName,
    nvrhi::ShaderHandle& outShader, nvrhi::BindingLayoutHandle& outLayout, nvrhi::ComputePipelineHandle& outPipeline)
{
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!shaderLoader || !nvDevice)
        return false;
    auto csResult = shaderLoader->LoadComputeShader(shaderName, "main");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [DeferredLight] Failed to load %s", shaderName);
        return false;
    }
    outShader = csResult.handle;
    outLayout = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(cacheName, *csResult.reflection, nvDevice);
    if (!outLayout) {
        Msg("! [DeferredLight] Failed to create binding layout for %s", shaderName);
        return false;
    }
    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = outShader;
    pipeDesc.bindingLayouts = { outLayout };
    auto* backend = device->GetBackend();
    if (backend && backend->GetBindlessLayout())
        pipeDesc.addBindingLayout(backend->GetBindlessLayout());
    outPipeline = GetPassResourceCache().GetOrCreateComputePipeline(cacheName, pipeDesc, nvDevice);
    if (!outPipeline) {
        Msg("! [DeferredLight] Failed to create compute pipeline for %s", shaderName);
        return false;
    }
    return true;
}

void EnsurePipelines(fg::RenderDevice* device, DeferredLightPassState& state)
{
    if (state.initialized || state.failed)
        return;
    if (!LoadComputePass(device, "tile_classify", "DeferredLight_Classify", state.classifyShader, state.classifyLayout, state.classifyPipeline)) {
        state.failed = true;
        return;
    }
    for (u32 cls = 0; cls < kLightTileClasses; ++cls) {
        string64 cacheName;
        xr_sprintf(cacheName, "DeferredLight_Tile%u", cls);
        if (!LoadComputePass(device, kTileShaderNames[cls], cacheName, state.tileShaders[cls], state.tileLayouts[cls], state.tilePipelines[cls])) {
            state.failed = true;
            return;
        }
    }
    state.initialized = true;
    Msg("* [DeferredLight] Pipelines initialized");
}

bool EnsureTileBuffers(fg::RenderDevice* device, DeferredLightPassState& state, u32 width, u32 height)
{
    const u32 tilesX = (width + kLightTileSize - 1) / kLightTileSize;
    const u32 tilesY = (height + kLightTileSize - 1) / kLightTileSize;
    const u32 maxTiles = tilesX * tilesY;
    if (state.tileListBuffer && state.tileArgsBuffer && state.maxTiles == maxTiles && state.tilesX == tilesX) {
        state.tilesY = tilesY;
        return true;
    }
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice || maxTiles == 0)
        return false;

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = u64(maxTiles) * kLightTileClasses * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DeferredLight_TileLists";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        state.tileListBuffer = nvDevice->createBuffer(desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = kTileArgsBytes;
        desc.debugName = "DeferredLight_TileArgs";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        state.tileArgsBuffer = nvDevice->createBuffer(desc);
    }
    if (!state.tileListBuffer || !state.tileArgsBuffer) {
        state.tileListBuffer = nullptr;
        state.tileArgsBuffer = nullptr;
        return false;
    }
    state.tilesX = tilesX;
    state.tilesY = tilesY;
    state.maxTiles = maxTiles;
    return true;
}

void ScheduleTileStats(DeferredLightPassState& state, nvrhi::ICommandList* cmdList, nvrhi::IDevice* nvDevice)
{
    state.readbackFrame++;
    if ((state.readbackFrame % 30) != 0)
        return;
    nvrhi::BufferHandle& slot = state.readback[state.readbackWrite];
    if (!slot) {
        nvrhi::BufferDesc desc;
        desc.byteSize = kTileArgsBytes;
        desc.debugName = "DeferredLight_TileStatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = nvDevice->createBuffer(desc);
        if (!slot)
            return;
    }
    cmdList->copyBuffer(slot, 0, state.tileArgsBuffer, 0, kTileArgsBytes);
    state.readbackWrite = (state.readbackWrite + 1) % DeferredLightPassState::kReadbackSlots;
    if (state.readbackScheduled < DeferredLightPassState::kReadbackSlots)
        ++state.readbackScheduled;
}

}

void ProcessDeferredLightStats(DeferredLightPassState& state, nvrhi::IDevice* device)
{
    if (state.readbackScheduled < DeferredLightPassState::kReadbackSlots || !device)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    void* mapped = device->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* words = static_cast<const u32*>(mapped);
    for (u32 cls = 0; cls < kLightTileClasses; ++cls)
        state.tileCounts[cls] = words[cls * 3];
    device->unmapBuffer(oldest);
}

DefaultOutputLayout setupDeferredLightPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    VirtualResourceHandle sunMask,
    const LocalShadowOutput& localShadow,
    xray::profiler::GPUProfiler* gpuProfiler,
    DeferredLightPassState* state)
{
    if (!state || !inputs.albedo.is_valid() || !inputs.depth.is_valid() || !inputs.normal.is_valid() || !inputs.baseColor.is_valid() || !inputs.material.is_valid())
        return inputs;

    EnsurePipelines(device, *state);
    if (!state->initialized || !EnsureTileBuffers(device, *state, width, height))
        return inputs;

    auto& passData = fg.addCallbackPass<DeferredLightPassData>(
        "Deferred Light",
        [&, width, height, sunMask, localShadow, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, DeferredLightPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.state = state;
            data.gpuProfiler = gpuProfiler;
            data.width = width;
            data.height = height;
            data.depth = passBuilder.read(inputs.depth, ResourceState::ShaderResource);
            data.normal = passBuilder.read(inputs.normal, ResourceState::ShaderResource);
            data.baseColor = passBuilder.read(inputs.baseColor, ResourceState::ShaderResource);
            data.material = passBuilder.read(inputs.material, ResourceState::ShaderResource);
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::UnorderedAccess);
            if (sunMask.is_valid())
                data.sunMask = passBuilder.read(sunMask, ResourceState::ShaderResource);
            data.localShadow = localShadow;
            if (localShadow.active) {
                data.localTiles = passBuilder.read(localShadow.tiles, ResourceState::ShaderResource);
                data.localStatic = passBuilder.read(localShadow.staticAtlas, ResourceState::ShaderResource);
                data.localDyn = passBuilder.read(localShadow.dynAtlas, ResourceState::ShaderResource);
                data.localHud = passBuilder.read(localShadow.hudAtlas, ResourceState::ShaderResource);
            }
        },
        [](const DeferredLightPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ZoneScoped;
            ZoneName("DeferredLightPass", 17);

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = fg.GetPhysicalTexture(data.baseColor);
            auto* materialRT = fg.GetPhysicalTexture(data.material);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!depthRT || !normalRT || !baseColorRT || !materialRT || !colorRT || !cmdList)
                return;

            auto& state = *data.state;
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* classifyRefl = shaderLoader->GetCachedReflection("tile_classify", ".cs");
            if (!classifyRefl)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            nvrhi::ITexture* sunMask = ResolveSunMask(fg, data.sunMask, nvDevice);
            nvrhi::IBuffer* localTiles = nullptr;
            nvrhi::ITexture* localStatic = nullptr;
            nvrhi::ITexture* localDyn = nullptr;
            nvrhi::ITexture* localHud = nullptr;
            ResolveLocalShadowBindings(fg, data.localShadow, nvDevice, localTiles, localStatic, localDyn, localHud);
            nvrhi::IBindingSet* bindlessTable = nullptr;
            if (auto* backend = data.device->GetBackend())
                bindlessTable = backend->GetBindlessDescriptorTable();

            const u32 argsInit[kLightTileClasses * 3] = { 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 };
            cmdList->writeBuffer(state.tileArgsBuffer, argsInit, sizeof(argsInit));

            auto tileCB = cache.GetOrCreateVolatileCB("DeferredLight", "TileParams", sizeof(TileParams), data.device, 256);
            TileParams tp = {};
            tp.tilesX = state.tilesX;
            tp.tilesY = state.tilesY;
            tp.maxTiles = state.maxTiles;
            tp.listBase = 0;
            tp.forceMixed = ps_r_sun_shadow_debug != 0 ? 1u : 0u;
            cmdList->writeBuffer(tileCB, &tp, sizeof(tp));

            auto& clm = ClusteredLightManager::Instance();
            {
                BindingSetBuilder bsb(*classifyRefl, nvDevice, "DeferredLight.Classify");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("TileParams", tileCB);
                bsb.Texture("g_GBufferDepth", depthRT);
                bsb.Texture("g_GBufferNormal", normalRT);
                bsb.Texture("g_SunShadowMask", sunMask);
                bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
                bsb.BufferUAV("g_TileLists", state.tileListBuffer);
                bsb.BufferUAV("g_TileArgs", state.tileArgsBuffer);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.classifyLayout, nvDevice);
                if (!bindingSet)
                    return;
                if (data.gpuProfiler)
                    data.gpuProfiler->BeginPass(cmdList, "Deferred Light.Classify");
                nvrhi::ComputeState cs;
                cs.pipeline = state.classifyPipeline;
                cs.bindings = { bindingSet };
                if (bindlessTable)
                    cs.addBindingSet(bindlessTable);
                cmdList->setComputeState(cs);
                cmdList->dispatch(state.tilesX, state.tilesY, 1);
                if (data.gpuProfiler)
                    data.gpuProfiler->EndPass(cmdList, "Deferred Light.Classify");
            }

            for (u32 cls = 0; cls < kLightTileClasses; ++cls) {
                auto* refl = shaderLoader->GetCachedReflection(kTileShaderNames[cls], ".cs");
                if (!refl)
                    continue;
                tp.listBase = cls * state.maxTiles;
                cmdList->writeBuffer(tileCB, &tp, sizeof(tp));

                BindingSetBuilder bsb(*refl, nvDevice, kTileProfilerNames[cls]);
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("TileParams", tileCB);
                bsb.Texture("g_GBufferDepth", depthRT);
                bsb.Texture("g_GBufferNormal", normalRT);
                bsb.Texture("g_GBufferBaseColor", baseColorRT);
                bsb.Texture("g_GBufferMaterial", materialRT);
                bsb.TextureUAV("g_SceneColor", colorRT);
                bsb.BufferSRV("g_TileList", state.tileListBuffer);
                if (cls & kTileClassLights) {
                    bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
                    bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
                    bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
                    bsb.BufferSRV("g_LocalShadowTiles", localTiles);
                    bsb.Texture("g_LocalShadowStatic", localStatic);
                    bsb.Texture("g_LocalShadowDyn", localDyn);
                    bsb.Texture("g_LocalShadowHud", localHud);
                }
                if (cls & kTileClassSunMixed)
                    bsb.Texture("g_SunShadowMask", sunMask);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.tileLayouts[cls], nvDevice);
                if (!bindingSet)
                    continue;

                if (data.gpuProfiler)
                    data.gpuProfiler->BeginPass(cmdList, kTileProfilerNames[cls]);
                nvrhi::ComputeState cs;
                cs.pipeline = state.tilePipelines[cls];
                cs.bindings = { bindingSet };
                if (bindlessTable)
                    cs.addBindingSet(bindlessTable);
                cs.indirectParams = state.tileArgsBuffer;
                cmdList->setComputeState(cs);
                cmdList->dispatchIndirect(cls * kTileArgsStride);
                if (data.gpuProfiler)
                    data.gpuProfiler->EndPass(cmdList, kTileProfilerNames[cls]);
            }

            ScheduleTileStats(state, cmdList, nvDevice);
        });

    DefaultOutputLayout outputs = inputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    return outputs;
}

}
