#include "stdafx.h"
#include "WorldCachePassSetup.h"
#include "ShaderConstants.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

static nvrhi::ComputePipelineHandle s_worldCacheDecayPipeline;
static nvrhi::BindingLayoutHandle s_worldCacheDecayLayout;
static nvrhi::ComputePipelineHandle s_worldCacheSelectPipeline;
static nvrhi::BindingLayoutHandle s_worldCacheSelectLayout;
static nvrhi::ComputePipelineHandle s_worldCacheUpdatePipeline;
static nvrhi::BindingLayoutHandle s_worldCacheUpdateLayout;
static nvrhi::BufferHandle s_worldCachePlaceholderBuffer;
static LightingFallback s_worldCacheReadiness = LightingFallback::ResourcesUnavailable;
static bool s_worldCacheAttempted = false;

static constexpr const char* s_worldCacheDecayShader = "rt_world_cache_decay";
static constexpr const char* s_worldCacheSelectShader = "rt_world_cache_select";
static constexpr const char* s_worldCacheUpdateShader = "rt_world_cache_update";

static bool HasUAV(const ExtractedReflection& reflection, const char* name)
{
    return std::any_of(reflection.rtBindings.uavBindings.begin(), reflection.rtBindings.uavBindings.end(),
        [name](const auto& binding) { return binding.name == name; });
}

static bool HasSRV(const ExtractedReflection& reflection, const char* name)
{
    return std::any_of(reflection.rtBindings.inputTextures.begin(), reflection.rtBindings.inputTextures.end(),
        [name](const auto& binding) { return binding.name == name; });
}

static bool HasConstantBuffer(const ExtractedReflection& reflection, const char* name)
{
    const auto& buffers = reflection.constantLayout.constantBuffers.buffers;
    return std::any_of(buffers.begin(), buffers.end(), [name](const auto& item) { return item.name == name; });
}

void BindWorldCacheResources(BindingSetBuilder& bsb, const ExtractedReflection& reflection,
    const WorldRadianceCacheBuffers& buffers, nvrhi::IBuffer* constants)
{
    const auto uav = [&](const char* name, nvrhi::IBuffer* buffer)
    {
        if (buffer && HasUAV(reflection, name))
            bsb.BufferUAV(name, buffer);
    };
    if (constants && HasConstantBuffer(reflection, "RTWorldCacheParams"))
        bsb.ConstantBuffer("RTWorldCacheParams", constants);
    uav("u_WorldCacheChecksum", buffers.checksum);
    uav("u_WorldCacheLife", buffers.life);
    uav("u_WorldCacheRadiance", buffers.radiance);
    uav("u_WorldCachePosition", buffers.position);
    uav("u_WorldCacheNormal", buffers.normal);
    uav("u_WorldCacheStats", buffers.stats);
    uav("u_WorldCacheUpdateList", buffers.updateList);
    uav("u_WorldCacheUpdateArgs", buffers.updateArgs);
    if (buffers.radianceInput && HasSRV(reflection, "t_WorldCacheRadianceInput"))
        bsb.BufferSRV("t_WorldCacheRadianceInput", buffers.radianceInput);
    if (buffers.updateList && HasSRV(reflection, "t_WorldCacheUpdateList"))
        bsb.BufferSRV("t_WorldCacheUpdateList", buffers.updateList);
}

static LightingFallback LoadWorldCachePipelines(RenderDevice* device)
{
    if (s_worldCacheAttempted)
        return s_worldCacheReadiness;
    s_worldCacheAttempted = true;
    s_worldCacheReadiness = LightingFallback::ResourcesUnavailable;

    auto* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* backend = device ? device->GetBackend() : nullptr;
    if (!nvDevice || !shaderLoader)
    {
        s_worldCacheReadiness = LightingFallback::ShaderUnavailable;
        return s_worldCacheReadiness;
    }
    if (!backend || !backend->GetBindlessLayout())
    {
        s_worldCacheReadiness = LightingFallback::BindingUnavailable;
        return s_worldCacheReadiness;
    }

    if (!s_worldCachePlaceholderBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "WorldCache_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_worldCachePlaceholderBuffer = nvDevice->createBuffer(desc);
        if (!s_worldCachePlaceholderBuffer)
            return s_worldCacheReadiness;
    }

    auto& cache = GetPassResourceCache();
    const auto createPipeline = [&](const char* shader, const char* name, bool rayTracing,
        nvrhi::BindingLayoutHandle& layout, nvrhi::ComputePipelineHandle& pipeline)
    {
        auto result = shaderLoader->LoadComputeShader(shader);
        if (!result.handle || !result.reflection)
            return LightingFallback::ShaderUnavailable;
        layout = cache.GetOrCreateBindingLayoutFromReflection(name, *result.reflection, nvDevice);
        if (!layout)
            return LightingFallback::PipelineUnavailable;
        nvrhi::ComputePipelineDesc desc;
        desc.CS = result.handle;
        desc.bindingLayouts = { layout };
        if (rayTracing)
            desc.bindingLayouts.push_back(backend->GetBindlessLayout());
        pipeline = nvDevice->createComputePipeline(desc);
        return pipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    };

    s_worldCacheReadiness = createPipeline(s_worldCacheDecayShader, "WorldCache_Decay", false, s_worldCacheDecayLayout, s_worldCacheDecayPipeline);
    if (s_worldCacheReadiness != LightingFallback::None)
        return s_worldCacheReadiness;
    s_worldCacheReadiness = createPipeline(s_worldCacheSelectShader, "WorldCache_Select", false, s_worldCacheSelectLayout, s_worldCacheSelectPipeline);
    if (s_worldCacheReadiness != LightingFallback::None)
        return s_worldCacheReadiness;
    s_worldCacheReadiness = createPipeline(s_worldCacheUpdateShader, "WorldCache_Update", true, s_worldCacheUpdateLayout, s_worldCacheUpdatePipeline);
    if (s_worldCacheReadiness == LightingFallback::None)
        Msg("* [WorldCache] Radiance cache pipelines initialized");
    return s_worldCacheReadiness;
}

WorldRadianceCacheConfig BuildWorldCacheConfig()
{
    WorldRadianceCacheConfig config;
    config.enabled = ps_r_rt_world_cache != 0;
    config.jitter = true;
    config.bounce = static_cast<u32>(std::clamp(ps_r_rt_world_cache_bounce, 0, 8));
    config.debug = static_cast<u32>(std::clamp(ps_r_rt_world_cache_debug, 0, 4));
    config.capacityLog2 = static_cast<u32>(std::clamp(ps_r_rt_world_cache_size, 14, 22));
    config.lifetime = static_cast<u32>(std::clamp(ps_r_rt_world_cache_lifetime, 2, 300));
    config.updateTarget = static_cast<u32>(std::clamp(ps_r_rt_world_cache_updates, 1024, 1 << 20));
    config.maxSamples = static_cast<u32>(std::clamp(ps_r_rt_world_cache_history, 1, 256));
    config.cellSize = std::clamp(ps_r_rt_world_cache_cell, 0.05f, 4.0f);
    config.lodScale = std::clamp(ps_r_rt_world_cache_lod, 1.0f, 64.0f);
    return config;
}

static void PublishCacheState(LightingFrameState& lighting, const WorldRadianceCacheConfig& config,
    const WorldRadianceCache* cache)
{
    lighting.worldCacheRequested = config.enabled;
    lighting.worldCacheBounce = config.bounce;
    lighting.worldCacheUpdates = config.updateTarget;
    lighting.worldCacheDebug = config.debug;
    lighting.worldCacheCellSize = config.cellSize;
    lighting.worldCacheLifetime = config.lifetime;
    if (!cache)
        return;
    const auto stats = cache->GetStats();
    lighting.worldCacheCapacity = config.enabled ? stats.capacity : 0;
    lighting.worldCacheLiveCells = stats.liveCells;
    lighting.worldCacheUpdated = stats.updatedCells;
    lighting.worldCacheLiveCellsKnown = stats.liveCellsKnown;
    lighting.worldCacheEventsKnown = stats.eventsKnown;
    lighting.worldCacheSubstituted = stats.events[0];
    lighting.worldCacheUnsampled = stats.events[1];
    lighting.worldCacheAbsent = stats.events[2];
    lighting.worldCacheBypassed = stats.events[3];
    lighting.worldCacheBytes = config.enabled ? stats.bytes : 0;
}

WorldCachePassOutput setupWorldCachePass(FrameGraph& fg, RenderDevice* device, const WorldCachePassInputs& inputs)
{
    WorldCachePassOutput output;
    WorldRadianceCache* cache = inputs.cache;
    LightingFrameState* lighting = inputs.lighting;
    const WorldRadianceCacheConfig config = BuildWorldCacheConfig();
    if (lighting)
        PublishCacheState(*lighting, config, cache);

    const auto fail = [&](LightingFallback reason)
    {
        output.fallback = reason;
        output.scheduled = false;
        if (lighting)
            lighting->worldCacheFallback = reason;
        if (cache)
            output.constants = cache->BuildConstants(inputs.cameraPos, inputs.frame, false);
        return output;
    };

    if (!cache || !device || !lighting)
        return fail(LightingFallback::ResourcesUnavailable);
    if (!cache->Prepare(config))
        return fail(LightingFallback::ResourcesUnavailable);
    PublishCacheState(*lighting, config, cache);

    auto& resourceCache = GetPassResourceCache();
    output.constantBuffer = resourceCache.GetOrCreateVolatileCB("WorldCache", "RTWorldCache_CB",
        sizeof(WorldRadianceCacheCB), device);
    if (!output.constantBuffer)
        return fail(LightingFallback::ResourcesUnavailable);
    if (!config.enabled)
    {
        output.constants = cache->BuildConstants(inputs.cameraPos, inputs.frame, false);
        return output;
    }

    const LightingFallback readiness = LoadWorldCachePipelines(device);
    if (readiness != LightingFallback::None)
        return fail(readiness);
    if (!inputs.accelMgr || !inputs.accelMgr->IsReady())
        return fail(LightingFallback::SceneUnavailable);
    nvrhi::IBuffer* sceneConstantBuffer = resourceCache.GetOrCreateVolatileCB("WorldCache", "RTWorldCache_SceneCB",
        sizeof(RTGIRawCB), device);
    if (!sceneConstantBuffer)
        return fail(LightingFallback::ResourcesUnavailable);
    if (!inputs.lightData.is_valid() || !inputs.environmentDistribution.is_valid() || !inputs.sky.is_valid())
        return fail(LightingFallback::InputsUnavailable);

    output.constants = cache->BuildConstants(inputs.cameraPos, inputs.frame, true);
    const u32 capacity = cache->GetCapacity();
    const WorldRadianceCacheCB constants = output.constants;
    nvrhi::IBuffer* constantBuffer = output.constantBuffer;

    fg.addCallbackPass<WorldCacheDecayPassData>(
        "WorldCache Decay",
        [&, cache, device, lighting, constants, constantBuffer, capacity](
            FrameGraph& builder, PassHandle passHandle, WorldCacheDecayPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.resources = cache->Use(builder, pb, WorldRadianceCacheAccess::Maintain);
            pb.sideEffects();
            data.device = device;
            data.cache = cache;
            data.lighting = lighting;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.capacity = capacity;
        },
        [](const WorldCacheDecayPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (data.lighting->effective != LightingMode::RTGI)
                return;
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            const auto buffers = WorldRadianceCache::Resolve(fg, data.resources);
            if (!cmdList || !nvDevice || !buffers.Valid() || !s_worldCacheDecayPipeline || !s_worldCacheDecayLayout)
                return;
            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            auto* reflection = shaderLoader ? shaderLoader->GetCachedReflection(s_worldCacheDecayShader, ".cs") : nullptr;
            if (!reflection)
                return;

            BindingSetBuilder bsb(*reflection, nvDevice, "WorldCache_Decay");
            BindWorldCacheResources(bsb, *reflection, buffers, data.constantBuffer);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_worldCacheDecayLayout, nvDevice);
            if (!bindingSet)
                return;

            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "WorldCache Decay");
            data.cache->TrackRecordedWork();
            data.cache->RecordPendingClear(cmdList, buffers);
            cmdList->clearBufferUInt(buffers.stats, 0);
            cmdList->writeBuffer(data.constantBuffer, &data.constants, sizeof(data.constants));
            nvrhi::ComputeState cs;
            cs.pipeline = s_worldCacheDecayPipeline;
            cs.bindings = { bindingSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.capacity + 255) / 256, 1, 1);
        });

    fg.addCallbackPass<WorldCacheSelectPassData>(
        "WorldCache Select",
        [&, cache, device, lighting, constants, constantBuffer, capacity](
            FrameGraph& builder, PassHandle passHandle, WorldCacheSelectPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.resources = cache->Use(builder, pb, WorldRadianceCacheAccess::Select);
            pb.sideEffects();
            data.device = device;
            data.cache = cache;
            data.lighting = lighting;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.capacity = capacity;
        },
        [](const WorldCacheSelectPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (data.lighting->effective != LightingMode::RTGI || !data.cache->IsContentReady())
                return;
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            const auto buffers = WorldRadianceCache::Resolve(fg, data.resources);
            if (!cmdList || !nvDevice || !buffers.Valid() || !buffers.updateList || !buffers.updateArgs ||
                !s_worldCacheSelectPipeline || !s_worldCacheSelectLayout)
                return;
            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            auto* reflection = shaderLoader ? shaderLoader->GetCachedReflection(s_worldCacheSelectShader, ".cs") : nullptr;
            if (!reflection)
                return;

            BindingSetBuilder bsb(*reflection, nvDevice, "WorldCache_Select");
            BindWorldCacheResources(bsb, *reflection, buffers, data.constantBuffer);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_worldCacheSelectLayout, nvDevice);
            if (!bindingSet)
                return;

            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "WorldCache Select");
            const u32 args[4] = { 0u, 1u, 1u, 0u };
            cmdList->writeBuffer(buffers.updateArgs, args, sizeof(args));
            cmdList->writeBuffer(data.constantBuffer, &data.constants, sizeof(data.constants));
            nvrhi::ComputeState cs;
            cs.pipeline = s_worldCacheSelectPipeline;
            cs.bindings = { bindingSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.capacity + 255) / 256, 1, 1);
            data.lighting->worldCacheSelectRecorded = true;
        });

    fg.addCallbackPass<WorldCacheUpdatePassData>(
        "WorldCache Update",
        [&, cache, device, lighting, constants, constantBuffer, sceneConstantBuffer](
            FrameGraph& builder, PassHandle passHandle, WorldCacheUpdatePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.resources = cache->Use(builder, pb, WorldRadianceCacheAccess::Update);
            data.lightData = pb.read(inputs.lightData, ResourceState::ShaderResource);
            data.clusterGrid = pb.read(inputs.clusterGrid, ResourceState::ShaderResource);
            data.lightIndexList = pb.read(inputs.lightIndexList, ResourceState::ShaderResource);
            data.environmentDistribution = pb.read(inputs.environmentDistribution, ResourceState::ShaderResource);
            data.sky = pb.read(inputs.sky, ResourceState::ShaderResource);
            data.scene = inputs.accelMgr->UseScene(builder, pb);
            pb.sideEffects();
            data.device = device;
            data.cache = cache;
            data.lighting = lighting;
            data.sceneConstants = inputs.sceneConstants;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.sceneConstantBuffer = sceneConstantBuffer;
        },
        [](const WorldCacheUpdatePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (data.lighting->effective != LightingMode::RTGI || !data.cache->IsContentReady() ||
                !data.lighting->worldCacheSelectRecorded)
                return;
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            const auto buffers = WorldRadianceCache::Resolve(fg, data.resources);
            if (!cmdList || !nvDevice || !buffers.Valid() || !buffers.snapshot || !buffers.updateList ||
                !buffers.updateArgs || !s_worldCacheUpdatePipeline || !s_worldCacheUpdateLayout)
                return;

            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            auto* lightData = fg.GetPhysicalBuffer(data.lightData);
            auto* clusterGrid = fg.GetPhysicalBuffer(data.clusterGrid);
            auto* lightIndexList = fg.GetPhysicalBuffer(data.lightIndexList);
            auto* environmentDistribution = fg.GetPhysicalBuffer(data.environmentDistribution);
            auto* skyTexture = fg.GetPhysicalTexture(data.sky);
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals",
                sizeof(StaticGlobals), data.device);
            if (!scene.tlas || !scene.batchInfo || !scene.vertices || !scene.indices || !scene.materials ||
                !scene.terrainMaterials || !scene.grassMaterials || !scene.variants || !scene.textures ||
                !scene.emissiveTriangles || !scene.batchTransforms || !scene.emissiveBatchOffsets ||
                !lightData || !environmentDistribution || !staticGlobals || !skyTexture)
            {
                Msg("! [WorldCache] Update skipped: scene bindings unavailable");
                return;
            }

            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            auto* reflection = shaderLoader ? shaderLoader->GetCachedReflection(s_worldCacheUpdateShader, ".cs") : nullptr;
            if (!reflection)
                return;

            nvrhi::IBuffer* skinnedVB = scene.skinnedVertices ? scene.skinnedVertices : s_worldCachePlaceholderBuffer.Get();
            nvrhi::IBuffer* skinnedIB = scene.skinnedIndices ? scene.skinnedIndices : s_worldCachePlaceholderBuffer.Get();
            nvrhi::IBuffer* grassVB = scene.grassVertices ? scene.grassVertices : s_worldCachePlaceholderBuffer.Get();
            nvrhi::IBuffer* grassIB = scene.grassIndices ? scene.grassIndices : s_worldCachePlaceholderBuffer.Get();

            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "WorldCache Update");

            BindingSetBuilder bsb(*reflection, nvDevice, "WorldCache_Update");
            bsb.ConstantBuffer("RTGIRawParams", data.sceneConstantBuffer)
               .ConstantBuffer("static_globals", staticGlobals)
               .BufferSRV("g_LightData", lightData)
               .BufferSRV("g_ClusterGrid", clusterGrid ? clusterGrid : lightData)
               .BufferSRV("g_LightIndexList", lightIndexList ? lightIndexList : lightData)
               .AccelStruct("g_SceneTLAS", scene.tlas)
               .BufferSRV("g_BatchInfo", scene.batchInfo)
               .BufferSRV("g_MegaVB", scene.vertices)
               .BufferSRV("g_MegaIB", scene.indices)
               .Texture("g_Sky", skyTexture)
               .BufferSRV("g_SkinnedVB", skinnedVB)
               .BufferSRV("g_Materials", scene.materials)
               .BufferSRV("g_GrassMaterials", scene.grassMaterials)
               .BufferSRV("g_TerrainMaterials", scene.terrainMaterials)
               .BufferSRV("g_Variants", scene.variants)
               .BufferSRV("g_SkinnedIB", skinnedIB)
               .BufferSRV("g_GrassVB", grassVB)
               .BufferSRV("g_GrassIB", grassIB)
               .BufferSRV("g_EmissiveTriangles", scene.emissiveTriangles)
               .BufferSRV("g_RTBatchTransforms", scene.batchTransforms)
               .BufferSRV("g_EmissiveBatchOffsets", scene.emissiveBatchOffsets)
               .BufferSRV("g_EnvironmentCDF", environmentDistribution);
            BindWorldCacheResources(bsb, *reflection, buffers, data.constantBuffer);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_worldCacheUpdateLayout, nvDevice);
            if (!bindingSet)
                return;

            data.cache->TrackRecordedWork();
            cmdList->copyBuffer(buffers.snapshot, 0, buffers.radiance, 0, buffers.radiance->getDesc().byteSize);
            cmdList->writeBuffer(data.sceneConstantBuffer, &data.sceneConstants, sizeof(RTGIRawCB));
            cmdList->writeBuffer(data.constantBuffer, &data.constants, sizeof(data.constants));

            nvrhi::ComputeState cs;
            cs.pipeline = s_worldCacheUpdatePipeline;
            cs.bindings = { bindingSet };
            cs.addBindingSet(scene.textures);
            cs.indirectParams = buffers.updateArgs;
            cmdList->setComputeState(cs);
            cmdList->dispatchIndirect(0);
            data.lighting->worldCacheUpdateRecorded = true;
        });

    output.scheduled = true;
    lighting->worldCacheScheduled = true;
    lighting->worldCacheMaxBounces = std::max(std::max(inputs.sceneConstants.maxBounces, 1u), config.bounce + 1u);
    lighting->worldCacheFallback = LightingFallback::None;
    return output;
}

void ShutdownWorldCache()
{
    s_worldCacheDecayPipeline = nullptr;
    s_worldCacheDecayLayout = nullptr;
    s_worldCacheSelectPipeline = nullptr;
    s_worldCacheSelectLayout = nullptr;
    s_worldCacheUpdatePipeline = nullptr;
    s_worldCacheUpdateLayout = nullptr;
    s_worldCachePlaceholderBuffer = nullptr;
    s_worldCacheReadiness = LightingFallback::ResourcesUnavailable;
    s_worldCacheAttempted = false;
}
}
