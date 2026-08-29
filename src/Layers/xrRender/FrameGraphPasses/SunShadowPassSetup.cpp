#include "stdafx.h"
#include "SunShadowPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrCDB/Frustum.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

constexpr float kFarEyeDist = 350.0f;
constexpr float kFarZNear = 1.0f;
constexpr float kFarZBeyond = 400.0f;
constexpr u32 kCullThreadGroup = 64;

struct alignas(16) SunShadowCullParams {
    Fvector4 frustumPlanes[6];
    float errBudget;
    u32 entryCount;
    u32 includeAT;
    u32 pad;
};

struct SunShadowCullData {
    VirtualResourceHandle order;
    VirtualResourceHandle opaqueArgs;
    VirtualResourceHandle terrainArgs;
    VirtualResourceHandle atArgs;
    SunShadowState* state;
    fg::RenderDevice* device;
    nvrhi::IBuffer* entryBuffer;
    u32 entryCount;
};

bool EnsureCullPipelines(fg::RenderDevice* device, SunShadowState& state)
{
    if (state.cullPipeline && state.argsPipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto cullResult = shaderLoader->LoadComputeShader("shadow_cluster_cull");
    auto argsResult = shaderLoader->LoadComputeShader("shadow_draw_args");
    if (!cullResult.handle || !argsResult.handle || !cullResult.reflection || !argsResult.reflection) {
        Msg("! [SunShadow] cull shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    state.cullLayout = cache.GetOrCreateBindingLayoutFromReflection("SunShadowCull", *cullResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("SunShadowArgs", *argsResult.reflection, nvDevice);
    if (!state.cullLayout || !state.argsLayout) {
        state.pipelinesFailed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc cullDesc;
    cullDesc.CS = cullResult.handle;
    cullDesc.bindingLayouts = { state.cullLayout };
    state.cullPipeline = cache.GetOrCreateComputePipeline("SunShadowCull", cullDesc, nvDevice);

    nvrhi::ComputePipelineDesc argsDesc;
    argsDesc.CS = argsResult.handle;
    argsDesc.bindingLayouts = { state.argsLayout };
    state.argsPipeline = cache.GetOrCreateComputePipeline("SunShadowArgs", argsDesc, nvDevice);

    if (!state.cullPipeline || !state.argsPipeline) {
        Msg("! [SunShadow] cull pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }

    Msg("* [SunShadow] cull pipelines initialized");
    return true;
}

nvrhi::BufferHandle MakeStreamBuffer(nvrhi::IDevice* nvDevice, const char* name, u32 elems)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = u64(std::max(elems, 1u)) * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.canHaveUAVs = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return nvDevice->createBuffer(desc);
}

nvrhi::BufferHandle MakeArgsBuffer(nvrhi::IDevice* nvDevice, const char* name)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = sizeof(u32) * 4;
    desc.canHaveUAVs = true;
    desc.canHaveRawViews = true;
    desc.isDrawIndirectArgs = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return nvDevice->createBuffer(desc);
}

bool EnsureCullBuffers(nvrhi::IDevice* nvDevice, SunShadowState& state, u32 entryCount)
{
    if (state.countBuffer && state.streamCapacity == entryCount)
        return true;

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "SunShadow_Count";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.countBuffer = nvDevice->createBuffer(desc);
    }
    state.opaqueStream = MakeStreamBuffer(nvDevice, "SunShadow_Opaque", entryCount);
    state.terrainStream = MakeStreamBuffer(nvDevice, "SunShadow_Terrain", entryCount);
    state.atStream = MakeStreamBuffer(nvDevice, "SunShadow_AT", entryCount);
    state.opaqueArgs = MakeArgsBuffer(nvDevice, "SunShadow_ArgsOpaque");
    state.terrainArgs = MakeArgsBuffer(nvDevice, "SunShadow_ArgsTerrain");
    state.atArgs = MakeArgsBuffer(nvDevice, "SunShadow_ArgsAT");

    for (u32 i = 0; i < SunShadowState::kReadbackSlots; ++i) {
        if (state.readback[i])
            continue;
        nvrhi::BufferDesc desc;
        desc.debugName = "SunShadow_Readback";
        desc.byteSize = sizeof(u32) * 4;
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        state.readback[i] = nvDevice->createBuffer(desc);
    }

    state.readbackWrite = 0;
    state.readbackScheduled = 0;
    state.castersOpaque = state.castersTerrain = state.castersAT = 0;

    if (!state.countBuffer || !state.opaqueStream || !state.terrainStream || !state.atStream ||
        !state.opaqueArgs || !state.terrainArgs || !state.atArgs) {
        Msg("! [SunShadow] cull buffer creation failed");
        state.countBuffer = nullptr;
        state.streamCapacity = 0;
        return false;
    }

    state.streamCapacity = entryCount;
    return true;
}

void ProcessReadback(nvrhi::IDevice* nvDevice, SunShadowState& state)
{
    if (state.readbackScheduled < SunShadowState::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    void* mapped = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* counts = static_cast<const u32*>(mapped);
    state.castersOpaque = std::min(counts[0], state.streamCapacity);
    state.castersTerrain = std::min(counts[1], state.streamCapacity);
    state.castersAT = std::min(counts[2], state.streamCapacity);
    nvDevice->unmapBuffer(oldest);
}

void ScheduleReadback(nvrhi::ICommandList* cmdList, SunShadowState& state)
{
    nvrhi::IBuffer* slot = state.readback[state.readbackWrite];
    if (!slot)
        return;
    cmdList->copyBuffer(slot, 0, state.countBuffer, 0, sizeof(u32) * 4);
    state.readbackWrite = (state.readbackWrite + 1) % SunShadowState::kReadbackSlots;
    if (state.readbackScheduled < SunShadowState::kReadbackSlots)
        ++state.readbackScheduled;
}

void ExecuteSunShadowCull(fg::RenderContext* ctx, const SunShadowCullData& data)
{
    SunShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice || !state.countBuffer || !state.farValid)
        return;
    if (!EnsureCullPipelines(data.device, state))
        return;

    auto& cache = framegraph::GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();

    SunShadowCullParams cb = {};
    {
        Fmatrix vp = state.farVP;
        CFrustum frustum;
        frustum.CreateFromMatrix(vp, FRUSTUM_P_ALL);
        for (u32 i = 0; i < frustum.p_count && i < 6; ++i)
            cb.frustumPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y, frustum.planes[i].n.z, frustum.planes[i].d);
    }
    cb.errBudget = state.farTexel * ps_r_shadow_cluster_lod;
    cb.entryCount = data.entryCount;
    cb.includeAT = ps_r_shadow_at ? 1u : 0u;

    auto paramsCB = cache.GetOrCreateVolatileCB("SunShadow", "CullParams", sizeof(SunShadowCullParams), data.device);
    cmdList->writeBuffer(paramsCB, &cb, sizeof(cb));

    const u32 zero[4] = { 0, 0, 0, 0 };
    cmdList->setBufferState(state.countBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(state.countBuffer, zero, sizeof(zero));

    cmdList->setBufferState(state.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.opaqueStream, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.terrainStream, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.atStream, nvrhi::ResourceStates::UnorderedAccess);

    auto* cullRefl = shaderLoader->GetCachedReflection("shadow_cluster_cull", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("shadow_draw_args", ".cs");
    if (!cullRefl || !argsRefl)
        return;

    framegraph::BindingSetBuilder bsb(*cullRefl, nvDevice, "SunShadow.Cull");
    bsb.ConstantBuffer("ShadowCullParams", paramsCB)
       .BufferSRV("g_Entries", data.entryBuffer)
       .BufferUAV("g_OutCount", state.countBuffer)
       .BufferUAV("g_OutOpaque", state.opaqueStream)
       .BufferUAV("g_OutTerrain", state.terrainStream)
       .BufferUAV("g_OutAT", state.atStream);
    auto cullSet = cache.GetOrCreateBindingSet(bsb.Build(), state.cullLayout, nvDevice);
    if (!cullSet)
        return;

    nvrhi::ComputeState cullState;
    cullState.pipeline = state.cullPipeline;
    cullState.bindings = { cullSet };
    cmdList->setComputeState(cullState);
    cmdList->dispatch((data.entryCount + kCullThreadGroup - 1) / kCullThreadGroup, 1, 1);

    cmdList->setBufferState(state.countBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.opaqueArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.terrainArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(state.atArgs, nvrhi::ResourceStates::UnorderedAccess);

    framegraph::BindingSetBuilder argsBsb(*argsRefl, nvDevice, "SunShadow.Args");
    argsBsb.BufferSRV("g_Count", state.countBuffer)
           .BufferUAV("g_ArgsOpaque", state.opaqueArgs)
           .BufferUAV("g_ArgsTerrain", state.terrainArgs)
           .BufferUAV("g_ArgsAT", state.atArgs);
    auto argsSet = cache.GetOrCreateBindingSet(argsBsb.Build(), state.argsLayout, nvDevice);
    if (!argsSet)
        return;

    nvrhi::ComputeState argsState;
    argsState.pipeline = state.argsPipeline;
    argsState.bindings = { argsSet };
    cmdList->setComputeState(argsState);
    cmdList->dispatch(1, 1, 1);

    cmdList->setBufferState(state.opaqueArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.terrainArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.atArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(state.opaqueStream, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.terrainStream, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(state.atStream, nvrhi::ResourceStates::ShaderResource);

    ScheduleReadback(cmdList, state);
}

} // namespace

void ComputeSunFarVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDirIn, float boxSize, u32 mapSize)
{
    Fvector sunDir = sunDirIn;
    if (sunDir.magnitude() < 1e-4f)
        sunDir.set(0.0f, -1.0f, 0.0f);
    sunDir.normalize();

    Fvector eye;
    eye.mad(Device.vCameraPosition, sunDir, -kFarEyeDist);
    Fvector up;
    up.set(0.0f, 1.0f, 0.0f);
    if (_abs(sunDir.y) > 0.99f)
        up.set(0.0f, 0.0f, 1.0f);

    Fmatrix view;
    view.build_camera_dir(eye, sunDir, up);

    const float texel = boxSize / float(std::max(mapSize, 1u));
    view.c.x = floorf(view.c.x / texel) * texel;
    view.c.y = floorf(view.c.y / texel) * texel;

    Fmatrix proj;
    proj.build_projection_ortho(boxSize, boxSize, kFarZNear, kFarEyeDist + boxSize + kFarZBeyond);
    outVP.mul(proj, view);
    outTexel = texel;
}

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCount,
    SunShadowState* state)
{
    SunShadowCullOutput out;
    if (!state || !device || !entryBuffer || entryCount == 0)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;

    ProcessReadback(nvDevice, *state);
    if (!EnsureCullBuffers(nvDevice, *state, entryCount))
        return out;

    Fvector sunDir;
    sunDir.set(0.0f, -1.0f, 0.0f);
    if (g_pGamePersistent)
        sunDir = g_pGamePersistent->Environment().CurrentEnv.sun_dir;
    ComputeSunFarVP(state->farVP, state->farTexel, sunDir, ps_r_sun_shadow_far_box, u32(ps_r_sun_shadow_far_size));
    state->farValid = true;
    state->candidates = entryCount;

    ResourceDesc argsDesc;
    argsDesc.type = ResourceDesc::Type::Buffer;
    argsDesc.bufferSize = sizeof(u32) * 4;
    argsDesc.isUAV = true;
    argsDesc.allowUAV = true;
    argsDesc.isImported = true;
    argsDesc.isTransient = false;
    argsDesc.debugName = "sun_shadow_args";
    VirtualResourceHandle opaqueArgs = fg.ImportBuffer("sun_shadow_args_opaque", state->opaqueArgs, argsDesc);
    VirtualResourceHandle terrainArgs = fg.ImportBuffer("sun_shadow_args_terrain", state->terrainArgs, argsDesc);
    VirtualResourceHandle atArgs = fg.ImportBuffer("sun_shadow_args_at", state->atArgs, argsDesc);

    auto& passData = fg.addCallbackPass<SunShadowCullData>(
        "Sun Shadow Cull",
        [&, orderAfter, opaqueArgs, terrainArgs, atArgs, entryBuffer, entryCount, state](FrameGraph& builder, PassHandle passHandle, SunShadowCullData& data) {
            data.state = state;
            data.device = device;
            data.entryBuffer = entryBuffer;
            data.entryCount = entryCount;

            RenderPassBuilder passBuilder(builder, passHandle);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::IndirectArgument);
            data.opaqueArgs = passBuilder.write(opaqueArgs, ResourceState::UnorderedAccess);
            data.terrainArgs = passBuilder.write(terrainArgs, ResourceState::UnorderedAccess);
            data.atArgs = passBuilder.write(atArgs, ResourceState::UnorderedAccess);
        },
        [](const SunShadowCullData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteSunShadowCull(ctx, data);
        });

    out.opaqueArgs = passData.opaqueArgs;
    out.terrainArgs = passData.terrainArgs;
    out.atArgs = passData.atArgs;
    out.active = true;
    return out;
}

} // namespace xray::render::fg::passes
