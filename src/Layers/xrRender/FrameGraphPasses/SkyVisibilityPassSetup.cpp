#include "stdafx.h"
#include "SkyVisibilityPassSetup.h"
#include "DeferredLightPassSetup.h"
#include "ShaderConstants.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "xrEngine/device.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/RayTracing/SkyVisibilityGrid.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

class SkyVisibilityBakeCB
{
public:
    Fvector4 origin;
    u32 dims[4];
    u32 first;
    u32 count;
    u32 rays;
    u32 maxNullEvents;
    float rayDistance;
    float backfaceLimit;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 detailMeshBatchStart;
    u32 staticDetailBatchStart;
    u32 detailPbrIndex;
    u32 detailBumpIndex;
    u32 pad0;
};
static_assert(sizeof(SkyVisibilityBakeCB) == 96);

class SkyVisibilityDebugCB
{
public:
    Fvector4 origin;
    u32 dims[4];
    float radius;
    float range;
    float pad0;
    float pad1;
};
static_assert(sizeof(SkyVisibilityDebugCB) == 48);

class SkyVisibilityBakePassData
{
public:
    RenderDevice* device = nullptr;
    SkyVisibilityGrid* grid = nullptr;
    RTFrameResources scene;
    VirtualResourceHandle probes;
    SkyVisibilityBakeCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    bool bake = false;
};

class SkyVisibilityDebugPassData
{
public:
    RenderDevice* device = nullptr;
    VirtualResourceHandle target;
    SkyVisibilityGrid* grid = nullptr;
    const SkyVisibilityInspectPassData* capture = nullptr;
    VirtualResourceHandle inspection;
    VirtualResourceHandle depth;
    VirtualResourceHandle probes;
    SkyVisibilityDebugCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    u32 width = 0;
    u32 height = 0;
    u32 variant = 0;
};

static constexpr const char* s_bakeShader = "sky_visibility_bake";
static constexpr const char* s_debugVertexShader = "fullscreen";
static constexpr u32 kDebugVariants = 2;
static constexpr const char* s_debugPixelShaders[kDebugVariants] =
{
    "sky_visibility_debug", "sky_visibility_inspector"
};
static constexpr const char* s_debugCacheNames[kDebugVariants] =
{
    "SkyVisibility_Debug", "SkyVisibility_Inspector"
};
static constexpr u32 kInspectVariants = 2;
static constexpr const char* s_inspectShaders[kInspectVariants] =
{
    "sky_visibility_inspect", "sky_visibility_inspect_rq"
};
static constexpr const char* s_inspectCacheNames[kInspectVariants] =
{
    "SkyVisibility_Inspect", "SkyVisibility_InspectRQ"
};

static nvrhi::ComputePipelineHandle s_bakePipeline;
static nvrhi::BindingLayoutHandle s_bakeLayout;
static nvrhi::BufferHandle s_bakePlaceholder;
static bool s_bakeAttempted = false;
static nvrhi::GraphicsPipelineHandle s_debugPipeline[kDebugVariants];
static nvrhi::BindingLayoutHandle s_debugLayout[kDebugVariants];
static bool s_debugAttempted[kDebugVariants] = {};
static nvrhi::ComputePipelineHandle s_inspectPipeline[kInspectVariants];
static nvrhi::BindingLayoutHandle s_inspectLayout[kInspectVariants];
static bool s_inspectAttempted[kInspectVariants] = {};
static bool s_rebakeRequested = false;

static bool LoadBakePipeline(RenderDevice* device)
{
    if (s_bakeAttempted)
        return s_bakePipeline != nullptr;
    s_bakeAttempted = true;
    auto* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* backend = device ? device->GetBackend() : nullptr;
    if (!nvDevice || !shaderLoader || !backend || !backend->GetBindlessLayout())
        return false;

    if (!s_bakePlaceholder)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkyVisibility_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_bakePlaceholder = nvDevice->createBuffer(desc);
        if (!s_bakePlaceholder)
            return false;
    }

    auto result = shaderLoader->LoadComputeShader(s_bakeShader);
    if (!result.handle || !result.reflection)
        return false;
    s_bakeLayout = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("SkyVisibility_Bake",
        *result.reflection, nvDevice);
    if (!s_bakeLayout)
        return false;
    nvrhi::ComputePipelineDesc desc;
    desc.CS = result.handle;
    desc.bindingLayouts = { s_bakeLayout, backend->GetBindlessLayout() };
    s_bakePipeline = nvDevice->createComputePipeline(desc);
    return s_bakePipeline != nullptr;
}

static bool LoadDebugPipeline(RenderDevice* device, u32 variant)
{
    if (s_debugAttempted[variant])
        return s_debugPipeline[variant] != nullptr;
    s_debugAttempted[variant] = true;
    auto* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !shaderLoader)
        return false;
    auto vs = shaderLoader->LoadVertexShader(s_debugVertexShader);
    auto ps = shaderLoader->LoadPixelShader(s_debugPixelShaders[variant]);
    if (!vs.handle || !ps.handle || !vs.reflection || !ps.reflection)
        return false;
    auto& cache = GetPassResourceCache();
    s_debugLayout[variant] = cache.GetOrCreateBindingLayoutFromReflection(s_debugCacheNames[variant], *vs.reflection,
        *ps.reflection, nvDevice);
    if (!s_debugLayout[variant])
        return false;
    nvrhi::GraphicsPipelineDesc desc;
    desc.setVertexShader(vs.handle);
    desc.setPixelShader(ps.handle);
    desc.addBindingLayout(s_debugLayout[variant]);
    desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    desc.renderState.blendState.targets[0].setBlendEnable(false);
    desc.renderState.depthStencilState.setDepthTestEnable(false);
    desc.renderState.depthStencilState.setDepthWriteEnable(false);
    desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);
    s_debugPipeline[variant] = cache.GetOrCreatePipeline(s_debugCacheNames[variant], desc, fbInfo, nvDevice);
    return s_debugPipeline[variant] != nullptr;
}

static bool LoadInspectPipeline(RenderDevice* device, u32 variant)
{
    if (s_inspectAttempted[variant])
        return s_inspectPipeline[variant] != nullptr;
    s_inspectAttempted[variant] = true;
    auto* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !shaderLoader)
        return false;
    auto shader = shaderLoader->LoadComputeShader(s_inspectShaders[variant]);
    if (!shader.handle || !shader.reflection)
        return false;
    s_inspectLayout[variant] = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
        s_inspectCacheNames[variant], *shader.reflection, nvDevice);
    if (!s_inspectLayout[variant])
        return false;
    nvrhi::ComputePipelineDesc desc;
    desc.CS = shader.handle;
    desc.bindingLayouts = { s_inspectLayout[variant] };
    s_inspectPipeline[variant] = nvDevice->createComputePipeline(desc);
    return s_inspectPipeline[variant] != nullptr;
}

static VirtualResourceHandle ImportInspectionBuffer(FrameGraph& fg, nvrhi::IBuffer* buffer, bool readback)
{
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Buffer;
    desc.bufferSize = sizeof(SkyProbeDebugSnapshot);
    desc.structStride = readback ? 0 : SkyProbeDebugSnapshot::kBufferStride;
    desc.isUAV = !readback;
    desc.allowUAV = !readback;
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = readback ? "sky_visibility_inspection_readback" : "sky_visibility_inspection";
    return fg.ImportBuffer(desc.debugName.c_str(), buffer, desc);
}

static VirtualResourceHandle ImportProbes(FrameGraph& fg, nvrhi::IBuffer* buffer)
{
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Buffer;
    desc.bufferSize = buffer->getDesc().byteSize;
    desc.structStride = SkyVisibilityGrid::kProbeBytes;
    desc.isUAV = true;
    desc.allowUAV = true;
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = "sky_visibility_probes";
    return fg.ImportBuffer("sky_visibility_probes", buffer, desc);
}

static SkyVisibilityBakeCB BuildBakeConstants(const SkyVisibilityLayout& layout, const RTSceneGeneration& scene,
    u32 first, u32 count)
{
    const auto& counts = scene.counts;
    const u32 staticEnd = counts.identityStatic + counts.terrain + counts.transparent + counts.instancedTotal;
    SkyVisibilityBakeCB cb = {};
    cb.origin.set(layout.origin.x, layout.origin.y, layout.origin.z, layout.spacing);
    cb.dims[0] = layout.dims[0];
    cb.dims[1] = layout.dims[1];
    cb.dims[2] = layout.dims[2];
    cb.dims[3] = layout.count;
    cb.first = first;
    cb.count = count;
    cb.rays = layout.rays;
    cb.maxNullEvents = static_cast<u32>(std::max(ps_r_rt_max_null_events, 1));
    cb.rayDistance = layout.rayDistance;
    cb.backfaceLimit = layout.backfaceLimit;
    cb.identityStaticCount = counts.identityStatic;
    cb.terrainBatchCount = counts.terrain;
    cb.skinnedBatchStart = counts.skinned > 0 ? staticEnd : UINT32_MAX;
    cb.grassBatchStart = counts.grass > 0 ? staticEnd + counts.skinned : UINT32_MAX;
    cb.detailAtlasIndex = scene.detailAtlasIndex;
    cb.detailMeshBatchStart = scene.detailMeshBatchStart;
    cb.staticDetailBatchStart = scene.staticDetailBatchStart;
    cb.detailPbrIndex = scene.detailPbrIndex;
    cb.detailBumpIndex = scene.detailBumpIndex;
    return cb;
}

VirtualResourceHandle setupSkyVisibilityBakePass(FrameGraph& fg, RenderDevice* device,
    RTAccelStructManager* accelMgr, SkyVisibilityGrid& grid)
{
    nvrhi::IBuffer* buffer = grid.GetBuffer();
    if (!device || !buffer)
        return {};
    const VirtualResourceHandle probes = ImportProbes(fg, buffer);

    u32 first = 0;
    u32 count = 0;
    const u32 budget = static_cast<u32>(std::clamp(ps_r_sky_probe_budget, 256, 1 << 20));
    bool bake = grid.TakeBakeSlice(budget, first, count) && accelMgr && accelMgr->IsSupported() &&
        accelMgr->IsReady() && LoadBakePipeline(device);
    if (!bake && !grid.HasPendingWork())
        return probes;

    SkyVisibilityBakeCB constants = {};
    nvrhi::IBuffer* constantBuffer = nullptr;
    if (bake)
    {
        const auto scene = accelMgr->GetScene();
        constantBuffer = GetPassResourceCache().GetOrCreateVolatileCB("SkyVisibility", "SkyVisibilityBake_CB",
            sizeof(SkyVisibilityBakeCB), device);
        bake = scene && constantBuffer;
        if (bake)
            constants = BuildBakeConstants(grid.GetLayout(), *scene, first, count);
    }

    auto& passData = fg.addCallbackPass<SkyVisibilityBakePassData>(
        "SkyVisibility Bake",
        [&, probes, bake, constants, constantBuffer](FrameGraph& builder, PassHandle passHandle,
            SkyVisibilityBakePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.probes = pb.readWrite(probes, ResourceState::UnorderedAccess);
            if (bake)
                data.scene = accelMgr->UseScene(builder, pb);
            pb.sideEffects();
            data.device = device;
            data.grid = &grid;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.bake = bake;
        },
        [](const SkyVisibilityBakePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            if (!cmdList || !nvDevice)
                return;
            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "SkyVisibility Bake");
            data.grid->RecordPending(cmdList);
            if (!data.bake || !s_bakePipeline || !s_bakeLayout)
                return;

            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            auto* probes = fg.GetPhysicalBuffer(data.probes);
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals",
                sizeof(StaticGlobals), data.device);
            if (!scene.tlas || !scene.batchInfo || !scene.vertices || !scene.indices || !scene.materials ||
                !scene.terrainMaterials || !scene.grassMaterials || !scene.variants || !scene.textures ||
                !probes || !staticGlobals)
            {
                Msg("! [SkyVisibility] bake skipped: scene bindings unavailable");
                return;
            }
            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            auto* reflection = shaderLoader ? shaderLoader->GetCachedReflection(s_bakeShader, ".cs") : nullptr;
            if (!reflection)
                return;

            nvrhi::IBuffer* placeholder = s_bakePlaceholder.Get();
            BindingSetBuilder bsb(*reflection, nvDevice, "SkyVisibility_Bake");
            bsb.ConstantBuffer("SkyVisibilityBakeParams", data.constantBuffer)
               .ConstantBuffer("static_globals", staticGlobals)
               .AccelStruct("g_SceneTLAS", scene.tlas)
               .BufferSRV("g_BatchInfo", scene.batchInfo)
               .BufferSRV("g_MegaVB", scene.vertices)
               .BufferSRV("g_MegaIB", scene.indices)
               .BufferSRV("g_SkinnedVB", scene.skinnedVertices ? scene.skinnedVertices : placeholder)
               .BufferSRV("g_SkinnedIB", scene.skinnedIndices ? scene.skinnedIndices : placeholder)
               .BufferSRV("g_GrassVB", scene.grassVertices ? scene.grassVertices : placeholder)
               .BufferSRV("g_GrassIB", scene.grassIndices ? scene.grassIndices : placeholder)
               .BufferSRV("g_Materials", scene.materials)
               .BufferSRV("g_GrassMaterials", scene.grassMaterials)
               .BufferSRV("g_TerrainMaterials", scene.terrainMaterials)
               .BufferSRV("g_Variants", scene.variants)
               .BufferUAV("u_SkyProbes", probes);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_bakeLayout, nvDevice);
            if (!bindingSet)
                return;

            cmdList->writeBuffer(data.constantBuffer, &data.constants, sizeof(data.constants));
            nvrhi::ComputeState cs;
            cs.pipeline = s_bakePipeline;
            cs.bindings = { bindingSet };
            cs.addBindingSet(scene.textures);
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.constants.count + 63) / 64, 1, 1);
            data.grid->CommitBakeSlice(cmdList, data.constants.first, data.constants.count);
        });
    return passData.probes;
}

SkyVisibilityInspection setupSkyVisibilityInspectPass(FrameGraph& fg, RenderDevice* device,
    RTAccelStructManager* accelMgr, SkyVisibilityGrid& grid, VirtualResourceHandle probes,
    VirtualResourceHandle depth, VirtualResourceHandle normal, VirtualResourceHandle material,
    VirtualResourceHandle lightingResult, const DeferredLightPassState* deferred, const LightingFrameState* lighting)
{
    SkyVisibilityInspection result;
    if (ps_r_sky_probe_debug != 3)
        return result;
    if (!device || !probes.is_valid() || !depth.is_valid() || !normal.is_valid() || !material.is_valid() ||
        grid.GetState() == SkyVisibilityState::Empty)
    {
        grid.ReportDebugFailure("diagnostic pass has no probe grid or matching opaque G-buffer inputs");
        return result;
    }
    if (!grid.PrepareDebugSnapshot())
        return result;
    result.buffer = ImportInspectionBuffer(fg, grid.GetDebugBuffer(), false);
    if (!grid.NeedsDebugSnapshot())
        return result;
    const bool rayQuery = accelMgr && accelMgr->IsSupported() && accelMgr->IsReady() && LoadInspectPipeline(device, 1);
    const u32 variant = rayQuery ? 1u : 0u;
    if (!rayQuery && !LoadInspectPipeline(device, 0))
    {
        grid.ReportDebugFailure("diagnostic compute shader or pipeline could not be loaded; see the shader compiler log");
        return result;
    }
    auto* constantBuffer = GetPassResourceCache().GetOrCreateVolatileCB("SkyVisibility",
        "SkyVisibilityInspect_CB", sizeof(SkyVisibilityInspectCB), device);
    if (!constantBuffer)
    {
        grid.ReportDebugFailure("diagnostic constant buffer could not be allocated");
        return result;
    }
    const auto& layout = grid.GetLayout();
    SkyVisibilityInspectCB constants;
    constants.capture[0] = Device.dwFrame;
    constants.capture[3] = layout.rays;
    constants.settings.set(ps_r_sky_probe_spacing, float(ps_r_sky_probe_rays), float(ps_r_sky_probe_max), 0.0f);
    constants.lighting.set(layout.rayDistance, layout.backfaceLimit, float(ps_r_sky_ibl), 0.0f);
    const u64 sceneRevision = rayQuery ? accelMgr->GetSceneRevision() : 0;
    constants.scene[0] = u32(sceneRevision);
    constants.scene[1] = u32(sceneRevision >> 32);
    constants.scene[2] = variant;
    auto& capture = fg.addCallbackPass<SkyVisibilityInspectPassData>(
        "SkyVisibility Inspect",
        [&, constants, constantBuffer, variant](FrameGraph& builder, PassHandle passHandle,
            SkyVisibilityInspectPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.material = pb.read(material, ResourceState::ShaderResource);
            data.probes = pb.read(probes, ResourceState::ShaderResource);
            if (lightingResult.is_valid())
                pb.read(lightingResult, ResourceState::ShaderResource);
            if (variant)
                data.tlas = accelMgr->UseScene(builder, pb).tlas;
            data.output = pb.write(result.buffer, ResourceState::UnorderedAccess);
            data.device = device;
            data.grid = &grid;
            data.deferred = deferred;
            data.lighting = lighting;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.variant = variant;
        },
        [](const SkyVisibilityInspectPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.grid->NeedsDebugSnapshot())
                return;
            auto* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            auto* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            auto* output = fg.GetPhysicalBuffer(data.output);
            auto* probes = fg.GetPhysicalBuffer(data.probes);
            auto* depth = fg.GetPhysicalTexture(data.depth);
            auto* normal = fg.GetPhysicalTexture(data.normal);
            auto* material = fg.GetPhysicalTexture(data.material);
            auto* tlas = data.variant ? fg.GetPhysicalAccelerationStructure(data.tlas) : nullptr;
            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            if (!cmdList || !nvDevice || !output || !probes || !depth || !normal || !material || !shaderLoader ||
                (data.variant && !tlas))
            {
                data.grid->ReportDebugFailure("diagnostic dispatch is missing its device, command list, or graph resources");
                return;
            }
            auto* reflection = shaderLoader->GetCachedReflection(s_inspectShaders[data.variant], ".cs");
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals",
                sizeof(StaticGlobals), data.device);
            if (!reflection || !staticGlobals)
            {
                data.grid->ReportDebugFailure("diagnostic shader reflection or frame constants are unavailable");
                return;
            }
            BindingSetBuilder bsb(*reflection, nvDevice, s_inspectCacheNames[data.variant]);
            bsb.ConstantBuffer("static_globals", staticGlobals)
               .ConstantBuffer("SkyVisibilityInspectParams", data.constantBuffer)
               .Texture("g_Depth", depth)
               .Texture("g_GBufferNormal", normal)
               .Texture("g_GBufferMaterial", material)
               .BufferSRV("g_SkyProbes", probes)
               .BufferUAV("u_SkyProbeInspection", output);
            if (tlas)
                bsb.AccelStruct("g_SkyProbeTLAS", tlas);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_inspectLayout[data.variant], nvDevice);
            if (!bindingSet)
            {
                data.grid->ReportDebugFailure("diagnostic shader resource bindings could not be created");
                return;
            }
            SkyVisibilityInspectCB constants = data.constants;
            if (data.deferred && data.deferred->skyVisibilityRecordedFrame == constants.capture[0])
                constants.capture[1] = data.deferred->skyVisibilityRecordedStatus;
            if (data.lighting)
                constants.lighting.w = float(data.lighting->effective);
            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "SkyVisibility Inspect");
            cmdList->writeBuffer(data.constantBuffer, &constants, sizeof(constants));
            nvrhi::ComputeState state;
            state.pipeline = s_inspectPipeline[data.variant];
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);
            cmdList->dispatch(1, 1, 1);
            data.recorded = true;
        });
    result.buffer = capture.output;
    result.capture = &capture;
    const auto readback = ImportInspectionBuffer(fg, grid.GetDebugReadbackBuffer(), true);
    fg.addCallbackPass<SkyVisibilityInspectCopyPassData>(
        "SkyVisibility Inspect Readback",
        [&](FrameGraph& builder, PassHandle passHandle, SkyVisibilityInspectCopyPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.source = pb.read(capture.output, ResourceState::CopySource);
            data.destination = pb.write(readback, ResourceState::CopyDest);
            pb.sideEffects();
            data.grid = &grid;
            data.capture = &capture;
        },
        [](const SkyVisibilityInspectCopyPassData& data, const FrameGraph&, fg::RenderContext* ctx)
        {
            if (!ctx || !ctx->GetCommandList())
                data.grid->ReportDebugFailure("diagnostic readback pass has no command list");
            else if (data.capture->recorded)
                data.grid->RecordDebugSnapshot(ctx->GetCommandList());
        });
    return result;
}

VirtualResourceHandle setupSkyVisibilityDebugPass(FrameGraph& fg, RenderDevice* device, SkyVisibilityGrid& grid,
    VirtualResourceHandle probes, VirtualResourceHandle target, VirtualResourceHandle depth,
    const SkyVisibilityInspection& inspection, u32 width, u32 height)
{
    if (ps_r_sky_probe_debug == 0)
        return target;
    if (!device || !probes.is_valid() || !depth.is_valid() || grid.GetState() == SkyVisibilityState::Empty)
        return target;
    const bool inspect = ps_r_sky_probe_debug == 3;
    if (inspect && !inspection.buffer.is_valid())
        return target;
    const u32 variant = inspect ? 1u : 0u;
    if (!LoadDebugPipeline(device, variant))
        return target;
    nvrhi::IBuffer* constantBuffer = GetPassResourceCache().GetOrCreateVolatileCB("SkyVisibility",
        "SkyVisibilityDebug_CB", sizeof(SkyVisibilityDebugCB), device);
    if (!constantBuffer)
        return target;

    const SkyVisibilityLayout& layout = grid.GetLayout();
    SkyVisibilityDebugCB constants = {};
    constants.origin.set(layout.origin.x, layout.origin.y, layout.origin.z, layout.spacing);
    constants.dims[0] = layout.dims[0];
    constants.dims[1] = layout.dims[1];
    constants.dims[2] = layout.dims[2];
    constants.dims[3] = static_cast<u32>(ps_r_sky_probe_debug);
    constants.radius = layout.spacing * std::clamp(ps_r_sky_probe_debug_radius, 0.02f, 0.5f);
    constants.range = std::clamp(ps_r_sky_probe_debug_range, 4.0f, 500.0f);

    auto& passData = fg.addCallbackPass<SkyVisibilityDebugPassData>(
        "SkyVisibility Debug",
        [&, probes, target, depth, constants, constantBuffer, width, height, variant, inspection](FrameGraph& builder,
            PassHandle passHandle, SkyVisibilityDebugPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.probes = pb.read(probes, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.target = pb.readWrite(target, ResourceState::RenderTarget);
            if (inspection.buffer.is_valid())
                data.inspection = pb.read(inspection.buffer, ResourceState::ShaderResource);
            data.capture = inspection.capture;
            data.grid = &grid;
            data.variant = variant;
            data.device = device;
            data.constants = constants;
            data.constantBuffer = constantBuffer;
            data.width = width;
            data.height = height;
        },
        [](const SkyVisibilityDebugPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            auto* probes = fg.GetPhysicalBuffer(data.probes);
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* targetTex = fg.GetPhysicalTexture(data.target);
            auto* inspection = data.inspection.is_valid() ? fg.GetPhysicalBuffer(data.inspection) : nullptr;
            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            if (!cmdList || !nvDevice || !probes || !depthTex || !targetTex || !shaderLoader ||
                !s_debugPipeline[data.variant] || !s_debugLayout[data.variant])
                return;
            if (data.variant == 1 && (!inspection ||
                (!data.grid->HasDebugSnapshot() && (!data.capture || !data.capture->recorded))))
                return;
            auto* vsRefl = shaderLoader->GetCachedReflection(s_debugVertexShader, ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection(s_debugPixelShaders[data.variant], ".ps");
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals",
                sizeof(StaticGlobals), data.device);
            if (!vsRefl || !psRefl || !staticGlobals)
                return;

            BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "SkyVisibility_Debug");
            bsb.ConstantBuffer("static_globals", staticGlobals)
               .ConstantBuffer("SkyVisibilityDebugParams", data.constantBuffer)
               .Texture("g_Depth", depthTex);
            if (data.variant == 1)
                bsb.BufferSRV("g_SkyProbeInspection", inspection);
            else
                bsb.BufferSRV("g_SkyProbes", probes);
            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_debugLayout[data.variant], nvDevice);
            if (!bindingSet)
                return;

            xray::profiler::GPUPassScope scope(fg.GetGPUProfiler(), cmdList, "SkyVisibility Debug");
            cmdList->writeBuffer(data.constantBuffer, &data.constants, sizeof(data.constants));

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(targetTex);
            auto framebuffer = GetPassResourceCache().GetOrCreateFramebuffer(fbDesc, nvDevice);
            nvrhi::Viewport viewport(static_cast<float>(data.width), static_cast<float>(data.height));
            nvrhi::GraphicsState state;
            state.pipeline = s_debugPipeline[data.variant];
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);
            cmdList->setGraphicsState(state);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        });
    return passData.target;
}

void RequestSkyVisibilityRebake()
{
    s_rebakeRequested = true;
}

bool ConsumeSkyVisibilityRebake()
{
    const bool requested = s_rebakeRequested;
    s_rebakeRequested = false;
    return requested;
}

void ShutdownSkyVisibility()
{
    s_bakePipeline = nullptr;
    s_bakeLayout = nullptr;
    s_bakePlaceholder = nullptr;
    s_bakeAttempted = false;
    for (u32 variant = 0; variant < kDebugVariants; ++variant)
    {
        s_debugPipeline[variant] = nullptr;
        s_debugLayout[variant] = nullptr;
        s_debugAttempted[variant] = false;
    }
    for (u32 variant = 0; variant < kInspectVariants; ++variant)
    {
        s_inspectPipeline[variant] = nullptr;
        s_inspectLayout[variant] = nullptr;
        s_inspectAttempted[variant] = false;
    }
}
}
