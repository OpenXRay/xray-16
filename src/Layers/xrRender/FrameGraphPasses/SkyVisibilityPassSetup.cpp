#include "stdafx.h"
#include "SkyVisibilityPassSetup.h"
#include "ShaderConstants.h"
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

static constexpr const char* s_bakeShader = "sky_visibility_bake";

static nvrhi::ComputePipelineHandle s_bakePipeline;
static nvrhi::BindingLayoutHandle s_bakeLayout;
static nvrhi::BufferHandle s_bakePlaceholder;
static bool s_bakeAttempted = false;
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
}
}
