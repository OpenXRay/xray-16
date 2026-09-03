#include "stdafx.h"
#include "SunShadowPassSetup.h"
#include "ShaderConstants.h"
#include "PassCommon.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
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
#include "xrCDB/Frustum.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

constexpr float kFarEyeDist = 350.0f;
constexpr float kFarZNear = 1.0f;
constexpr float kFarZBeyond = 400.0f;
constexpr float kFarRedrawDist = 10.0f;
constexpr float kFarSunDotRedraw = 0.99999847f;
constexpr float kCascSunTau = 1.0f;
constexpr float kCascSnapDot = 0.99939f;
constexpr float kCascadeBox0 = 25.0f;
constexpr u32 kCascadeSize0 = 4096;
constexpr float kCascadeBox1 = 60.0f;
constexpr u32 kCascadeSize1 = 2048;
constexpr float kCascadeZBehind = -160.0f;
constexpr float kCascadeZAhead = 460.0f;
constexpr float kAnchorGrid = 4.0f;
constexpr u32 kCullThreadGroup = 64;

struct TargetNames {
    const char* suffix;
    const char* passName;
    const char* zoneName;
    const char* rtName;
    const char* framebufferName;
    const char* argsOpaqueName;
    const char* argsTerrainName;
    const char* argsATName;
    const char* srvName;
};

constexpr TargetNames kTargetNames[kSunTargetCount] = {
    { "Far", "Sun Shadow Far", "Shadow/Far", "rt_SunShadowFar", "SunShadowFar",
      "sun_shadow_args_opaque_far", "sun_shadow_args_terrain_far", "sun_shadow_args_at_far", "g_SunShadowFar" },
    { "Casc0", "Sun Shadow Casc0", "Shadow/Casc0", "rt_SunShadowCasc0", "SunShadowCasc0",
      "sun_shadow_args_opaque_casc0", "sun_shadow_args_terrain_casc0", "sun_shadow_args_at_casc0", "g_SunShadowCasc0" },
    { "Casc1", "Sun Shadow Casc1", "Shadow/Casc1", "rt_SunShadowCasc1", "SunShadowCasc1",
      "sun_shadow_args_opaque_casc1", "sun_shadow_args_terrain_casc1", "sun_shadow_args_at_casc1", "g_SunShadowCasc1" },
};

struct alignas(16) SunShadowCullParams {
    Fvector4 frustumPlanes[6];
    float errBudget;
    u32 entryCount;
    u32 includeAT;
    u32 pad;
};

struct SunShadowCullData {
    VirtualResourceHandle order;
    VirtualResourceHandle skinnedOrder;
    GPUCullingManager* gpuCulling = nullptr;
    SunShadowCullOutput::Target targets[kSunTargetCount];
    SunShadowState* state;
    fg::RenderDevice* device;
    nvrhi::IBuffer* entryBuffer;
    u32 entryCount;
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
};

struct SunShadowMapData {
    VirtualResourceHandle map;
    VirtualResourceHandle opaqueArgs;
    VirtualResourceHandle terrainArgs;
    VirtualResourceHandle atArgs;
    VirtualResourceHandle dynamicArgs;
    VirtualResourceHandle skinnedArgs;
    VirtualResourceHandle skinnedCullArgs;
    SunShadowState* state;
    fg::RenderDevice* device;
    SunShadowDrawConfig config;
    u32 target = 0;
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
};

struct GPUZone {
    xray::profiler::GPUProfiler* profiler;
    nvrhi::ICommandList* cmdList;
    const char* name;
    GPUZone(xray::profiler::GPUProfiler* p, nvrhi::ICommandList* c, const char* n) : profiler(p), cmdList(c), name(n)
    {
        if (profiler)
            profiler->BeginPass(cmdList, name);
    }
    ~GPUZone()
    {
        if (profiler)
            profiler->EndPass(cmdList, name);
    }
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
    auto skinnedArgsResult = shaderLoader->LoadComputeShader("shadow_skinned_draw_args");
    if (!cullResult.handle || !argsResult.handle || !cullResult.reflection || !argsResult.reflection
        || !skinnedArgsResult.handle || !skinnedArgsResult.reflection) {
        Msg("! [SunShadow] cull shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    state.cullLayout = cache.GetOrCreateBindingLayoutFromReflection("SunShadowCull", *cullResult.reflection, nvDevice);
    state.argsLayout = cache.GetOrCreateBindingLayoutFromReflection("SunShadowArgs", *argsResult.reflection, nvDevice);
    state.skinnedArgsLayout = cache.GetOrCreateBindingLayoutFromReflection("SunShadowSkinnedArgs", *skinnedArgsResult.reflection, nvDevice);
    if (!state.cullLayout || !state.argsLayout || !state.skinnedArgsLayout) {
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

    nvrhi::ComputePipelineDesc skinnedArgsDesc;
    skinnedArgsDesc.CS = skinnedArgsResult.handle;
    skinnedArgsDesc.bindingLayouts = { state.skinnedArgsLayout };
    state.skinnedArgsPipeline = cache.GetOrCreateComputePipeline("SunShadowSkinnedArgs", skinnedArgsDesc, nvDevice);

    if (!state.cullPipeline || !state.argsPipeline || !state.skinnedArgsPipeline) {
        Msg("! [SunShadow] cull pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }

    Msg("* [SunShadow] cull pipelines initialized");
    return true;
}

bool EnsureDepthPipelines(fg::RenderDevice* device, SunShadowState& state)
{
    if (state.depthOpaquePipeline && state.depthATPipeline)
        return true;
    if (state.depthPipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto vsResult = shaderLoader->LoadVertexShader("cluster_pull", "main");
    auto skinnedVsResult = shaderLoader->LoadVertexShader("cluster_pull_skinned", "main");
    auto opaqueResult = shaderLoader->LoadPixelShader("bindless_depth_opaque", "main");
    auto atResult = shaderLoader->LoadPixelShader("shadow_depth_at", "main");
    auto forwardVsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto dynamicResult = shaderLoader->LoadPixelShader("bindless_depth_at", "main");
    if (!vsResult.handle || !opaqueResult.handle || !atResult.handle || !forwardVsResult.handle || !dynamicResult.handle ||
        !skinnedVsResult.handle || !skinnedVsResult.reflection ||
        !vsResult.reflection || !opaqueResult.reflection || !atResult.reflection || !forwardVsResult.reflection || !dynamicResult.reflection) {
        Msg("! [SunShadow] depth shaders failed to load");
        state.depthPipelinesFailed = true;
        return false;
    }

    state.clusterVS = vsResult.handle;
    state.skinnedVS = skinnedVsResult.handle;
    state.depthOpaquePS = opaqueResult.handle;
    state.depthATPS = atResult.handle;
    state.forwardVS = forwardVsResult.handle;
    state.depthDynamicPS = dynamicResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    state.depthOpaqueLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_Opaque", *vsResult.reflection, *opaqueResult.reflection, nvDevice);
    state.depthATLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_AT", *vsResult.reflection, *atResult.reflection, nvDevice);
    state.depthDynamicLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_Dynamic", *forwardVsResult.reflection, *dynamicResult.reflection, nvDevice);
    state.depthSkinnedLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_Skinned", *skinnedVsResult.reflection, *opaqueResult.reflection, nvDevice);
    if (!state.depthOpaqueLayout || !state.depthATLayout || !state.depthDynamicLayout || !state.depthSkinnedLayout) {
        state.depthPipelinesFailed = true;
        return false;
    }

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.depthDynamicInputLayout = nvDevice->createInputLayout(attrs, attrCount, state.forwardVS);

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    auto makeDesc = [&](nvrhi::IShader* vs, nvrhi::IInputLayout* il, nvrhi::IShader* pixelShader, nvrhi::IBindingLayout* layout, bool withBindless) {
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = vs;
        desc.PS = pixelShader;
        desc.inputLayout = il;
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
        desc.renderState.rasterState.depthBias = -state.rasterBias;
        desc.renderState.rasterState.slopeScaledDepthBias = -state.rasterSlope;
        desc.renderState.rasterState.depthBiasClamp = 0.0f;
        return desc;
    };

    string128 name;
    xr_sprintf(name, "SunShadowDepth_Opaque_b%d_s%.2f", state.rasterBias, state.rasterSlope);
    state.depthOpaquePipeline = cache.GetOrCreatePipeline(name,
        makeDesc(state.clusterVS, nullptr, state.depthOpaquePS, state.depthOpaqueLayout, false), fbInfo, nvDevice);
    xr_sprintf(name, "SunShadowDepth_AT_b%d_s%.2f", state.rasterBias, state.rasterSlope);
    state.depthATPipeline = cache.GetOrCreatePipeline(name,
        makeDesc(state.clusterVS, nullptr, state.depthATPS, state.depthATLayout, true), fbInfo, nvDevice);
    xr_sprintf(name, "SunShadowDepth_Dynamic_b%d_s%.2f", state.rasterBias, state.rasterSlope);
    state.depthDynamicPipeline = cache.GetOrCreatePipeline(name,
        makeDesc(state.forwardVS, state.depthDynamicInputLayout, state.depthDynamicPS, state.depthDynamicLayout, true), fbInfo, nvDevice);
    xr_sprintf(name, "SunShadowDepth_Skinned_b%d_s%.2f", state.rasterBias, state.rasterSlope);
    state.depthSkinnedPipeline = cache.GetOrCreatePipeline(name,
        makeDesc(state.skinnedVS, nullptr, state.depthOpaquePS, state.depthSkinnedLayout, false), fbInfo, nvDevice);

    if (!state.depthOpaquePipeline || !state.depthATPipeline || !state.depthDynamicPipeline || !state.depthSkinnedPipeline) {
        Msg("! [SunShadow] depth pipeline creation failed");
        state.depthPipelinesFailed = true;
        return false;
    }

    Msg("* [SunShadow] depth pipelines initialized");
    return true;
}

nvrhi::BufferHandle MakeStreamBuffer(nvrhi::IDevice* nvDevice, const std::string& name, u32 elems)
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

nvrhi::BufferHandle MakeArgsBuffer(nvrhi::IDevice* nvDevice, const std::string& name)
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

bool EnsureCullBuffers(nvrhi::IDevice* nvDevice, SunShadowTarget& target, const char* suffix, u32 entryCount)
{
    if (target.countBuffer && target.streamCapacity == entryCount)
        return true;

    const std::string base = std::string("SunShadow_") + suffix;
    {
        nvrhi::BufferDesc desc;
        desc.debugName = base + "_Count";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        target.countBuffer = nvDevice->createBuffer(desc);
    }
    target.opaqueStream = MakeStreamBuffer(nvDevice, base + "_Opaque", entryCount);
    target.terrainStream = MakeStreamBuffer(nvDevice, base + "_Terrain", entryCount);
    target.atStream = MakeStreamBuffer(nvDevice, base + "_AT", entryCount);
    target.opaqueArgs = MakeArgsBuffer(nvDevice, base + "_ArgsOpaque");
    target.terrainArgs = MakeArgsBuffer(nvDevice, base + "_ArgsTerrain");
    target.atArgs = MakeArgsBuffer(nvDevice, base + "_ArgsAT");
    {
        nvrhi::BufferDesc desc;
        desc.debugName = base + "_SkinnedCount";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        target.skinnedCountBuffer = nvDevice->createBuffer(desc);
    }
    target.skinnedStream = MakeStreamBuffer(nvDevice, base + "_Skinned", kSunShadowSkinnedEntryCap);
    target.skinnedArgs = MakeArgsBuffer(nvDevice, base + "_ArgsSkinned");

    for (u32 i = 0; i < SunShadowTarget::kReadbackSlots; ++i) {
        if (target.readback[i])
            continue;
        nvrhi::BufferDesc desc;
        desc.debugName = base + "_Readback";
        desc.byteSize = sizeof(u32) * 4;
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        target.readback[i] = nvDevice->createBuffer(desc);
    }

    target.readbackWrite = 0;
    target.readbackScheduled = 0;
    target.castersOpaque = target.castersTerrain = target.castersAT = 0;

    if (!target.countBuffer || !target.opaqueStream || !target.terrainStream || !target.atStream ||
        !target.opaqueArgs || !target.terrainArgs || !target.atArgs ||
        !target.skinnedCountBuffer || !target.skinnedStream || !target.skinnedArgs) {
        Msg("! [SunShadow] %s cull buffer creation failed", suffix);
        target.countBuffer = nullptr;
        target.streamCapacity = 0;
        return false;
    }

    target.streamCapacity = entryCount;
    return true;
}

bool EnsureMap(nvrhi::IDevice* nvDevice, SunShadowTarget& target, const char* suffix, u32 size)
{
    if (target.map && target.mapSize == size)
        return true;

    nvrhi::TextureDesc desc;
    desc.width = size;
    desc.height = size;
    desc.format = nvrhi::Format::D32;
    desc.debugName = std::string("SunShadow_") + suffix;
    desc.isShaderResource = true;
    desc.isRenderTarget = true;
    desc.isTypeless = true;
    desc.useClearValue = true;
    desc.clearValue = nvrhi::Color(0.0f);
    desc.initialState = nvrhi::ResourceStates::DepthWrite;
    desc.keepInitialState = true;
    target.map = nvDevice->createTexture(desc);
    target.mapSize = target.map ? size : 0;
    if (!target.map)
        Msg("! [SunShadow] %s map creation failed (%u)", suffix, size);
    return target.map != nullptr;
}

void ProcessReadback(nvrhi::IDevice* nvDevice, SunShadowTarget& target)
{
    if (target.readbackScheduled < SunShadowTarget::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = target.readback[target.readbackWrite];
    if (!oldest)
        return;
    void* mapped = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* counts = static_cast<const u32*>(mapped);
    target.castersOpaque = std::min(counts[0], target.streamCapacity);
    target.castersTerrain = std::min(counts[1], target.streamCapacity);
    target.castersAT = std::min(counts[2], target.streamCapacity);
    nvDevice->unmapBuffer(oldest);
}

void ScheduleReadback(nvrhi::ICommandList* cmdList, SunShadowTarget& target)
{
    nvrhi::IBuffer* slot = target.readback[target.readbackWrite];
    if (!slot)
        return;
    cmdList->copyBuffer(slot, 0, target.countBuffer, 0, sizeof(u32) * 4);
    target.readbackWrite = (target.readbackWrite + 1) % SunShadowTarget::kReadbackSlots;
    if (target.readbackScheduled < SunShadowTarget::kReadbackSlots)
        ++target.readbackScheduled;
}

void CullTarget(fg::RenderContext* ctx, const SunShadowCullData& data, SunShadowTarget& target,
                const ExtractedReflection& cullRefl, const ExtractedReflection& argsRefl)
{
    SunShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();

    SunShadowCullParams cb = {};
    {
        Fmatrix vp = target.vp;
        CFrustum frustum;
        frustum.CreateFromMatrix(vp, FRUSTUM_P_ALL);
        for (u32 i = 0; i < frustum.p_count && i < 6; ++i)
            cb.frustumPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y, frustum.planes[i].n.z, frustum.planes[i].d);
    }
    cb.errBudget = target.texel * ps_r_shadow_cluster_lod;
    cb.entryCount = data.entryCount;
    cb.includeAT = ps_r_shadow_at ? 1u : 0u;

    auto paramsCB = cache.GetOrCreateVolatileCB("SunShadow", "CullParams", sizeof(SunShadowCullParams), data.device);
    cmdList->writeBuffer(paramsCB, &cb, sizeof(cb));

    const u32 zero[4] = { 0, 0, 0, 0 };
    cmdList->setBufferState(target.countBuffer, nvrhi::ResourceStates::CopyDest);
    cmdList->writeBuffer(target.countBuffer, zero, sizeof(zero));

    cmdList->setBufferState(target.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(target.opaqueStream, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(target.terrainStream, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(target.atStream, nvrhi::ResourceStates::UnorderedAccess);

    framegraph::BindingSetBuilder bsb(cullRefl, nvDevice, "SunShadow.Cull");
    bsb.ConstantBuffer("ShadowCullParams", paramsCB)
       .BufferSRV("g_Entries", data.entryBuffer)
       .BufferUAV("g_OutCount", target.countBuffer)
       .BufferUAV("g_OutOpaque", target.opaqueStream)
       .BufferUAV("g_OutTerrain", target.terrainStream)
       .BufferUAV("g_OutAT", target.atStream);
    auto cullSet = cache.GetOrCreateBindingSet(bsb.Build(), state.cullLayout, nvDevice);
    if (!cullSet)
        return;

    nvrhi::ComputeState cullState;
    cullState.pipeline = state.cullPipeline;
    cullState.bindings = { cullSet };
    cmdList->setComputeState(cullState);
    cmdList->dispatch((data.entryCount + kCullThreadGroup - 1) / kCullThreadGroup, 1, 1);

    cmdList->setBufferState(target.countBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(target.opaqueArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(target.terrainArgs, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(target.atArgs, nvrhi::ResourceStates::UnorderedAccess);

    framegraph::BindingSetBuilder argsBsb(argsRefl, nvDevice, "SunShadow.Args");
    argsBsb.BufferSRV("g_Count", target.countBuffer)
           .BufferUAV("g_ArgsOpaque", target.opaqueArgs)
           .BufferUAV("g_ArgsTerrain", target.terrainArgs)
           .BufferUAV("g_ArgsAT", target.atArgs);
    auto argsSet = cache.GetOrCreateBindingSet(argsBsb.Build(), state.argsLayout, nvDevice);
    if (!argsSet)
        return;

    nvrhi::ComputeState argsState;
    argsState.pipeline = state.argsPipeline;
    argsState.bindings = { argsSet };
    cmdList->setComputeState(argsState);
    cmdList->dispatch(1, 1, 1);

    cmdList->setBufferState(target.opaqueArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(target.terrainArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(target.atArgs, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(target.opaqueStream, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(target.terrainStream, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(target.atStream, nvrhi::ResourceStates::ShaderResource);

    nvrhi::IBuffer* skinnedEntries = data.gpuCulling ? data.gpuCulling->GetSkinnedEntryBuffer() : nullptr;
    const u32 skinnedCount = data.gpuCulling ? data.gpuCulling->GetSkinnedEntryCount() : 0u;
    cmdList->setBufferState(target.skinnedArgs, nvrhi::ResourceStates::CopyDest);
    cmdList->clearBufferUInt(target.skinnedArgs, 0);
    if (skinnedEntries && skinnedCount > 0 && state.skinnedArgsPipeline) {
        cb.entryCount = skinnedCount;
        cb.includeAT = 0;
        auto skinnedParamsCB = cache.GetOrCreateVolatileCB("SunShadow", "SkinnedCullParams", sizeof(SunShadowCullParams), data.device, 4);
        cmdList->writeBuffer(skinnedParamsCB, &cb, sizeof(cb));

        cmdList->setBufferState(target.skinnedCountBuffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(target.skinnedCountBuffer, zero, sizeof(zero));
        cmdList->setBufferState(target.skinnedCountBuffer, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(target.skinnedStream, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(skinnedEntries, nvrhi::ResourceStates::ShaderResource);

        framegraph::BindingSetBuilder sbsb(cullRefl, nvDevice, "SunShadow.CullSkinned");
        sbsb.ConstantBuffer("ShadowCullParams", skinnedParamsCB)
            .BufferSRV("g_Entries", skinnedEntries)
            .BufferUAV("g_OutCount", target.skinnedCountBuffer)
            .BufferUAV("g_OutOpaque", target.skinnedStream)
            .BufferUAV("g_OutTerrain", target.terrainStream)
            .BufferUAV("g_OutAT", target.atStream);
        auto skinnedCullSet = cache.GetOrCreateBindingSet(sbsb.Build(), state.cullLayout, nvDevice);
        auto* skinnedArgsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("shadow_skinned_draw_args", ".cs");
        if (skinnedCullSet && skinnedArgsRefl) {
            nvrhi::ComputeState skinnedCullState;
            skinnedCullState.pipeline = state.cullPipeline;
            skinnedCullState.bindings = { skinnedCullSet };
            cmdList->setComputeState(skinnedCullState);
            cmdList->dispatch((skinnedCount + kCullThreadGroup - 1) / kCullThreadGroup, 1, 1);

            cmdList->setBufferState(target.skinnedCountBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->setBufferState(target.skinnedArgs, nvrhi::ResourceStates::UnorderedAccess);
            framegraph::BindingSetBuilder sabsb(*skinnedArgsRefl, nvDevice, "SunShadow.SkinnedArgs");
            sabsb.BufferSRV("g_Count", target.skinnedCountBuffer)
                 .BufferUAV("g_ArgsSkinned", target.skinnedArgs);
            if (auto skinnedArgsSet = cache.GetOrCreateBindingSet(sabsb.Build(), state.skinnedArgsLayout, nvDevice)) {
                nvrhi::ComputeState skinnedArgsState;
                skinnedArgsState.pipeline = state.skinnedArgsPipeline;
                skinnedArgsState.bindings = { skinnedArgsSet };
                cmdList->setComputeState(skinnedArgsState);
                cmdList->dispatch(1, 1, 1);
            }
        }
        cmdList->setBufferState(target.skinnedStream, nvrhi::ResourceStates::ShaderResource);
    }
    cmdList->setBufferState(target.skinnedArgs, nvrhi::ResourceStates::IndirectArgument);

    ScheduleReadback(cmdList, target);
}

void ExecuteSunShadowCull(fg::RenderContext* ctx, const SunShadowCullData& data)
{
    SunShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    if (!cmdList || !nvDevice)
        return;
    GPUZone zone(data.gpuProfiler, cmdList, "Shadow/Cull");

    bool any = false;
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        const SunShadowTarget& target = state.targets[t];
        any |= target.redraw && target.valid && target.countBuffer;
    }
    if (!any)
        return;
    if (!EnsureCullPipelines(data.device, state))
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* cullRefl = shaderLoader->GetCachedReflection("shadow_cluster_cull", ".cs");
    auto* argsRefl = shaderLoader->GetCachedReflection("shadow_draw_args", ".cs");
    if (!cullRefl || !argsRefl)
        return;

    for (u32 t = 0; t < kSunTargetCount; ++t) {
        SunShadowTarget& target = state.targets[t];
        if (!target.redraw || !target.valid || !target.countBuffer)
            continue;
        CullTarget(ctx, data, target, *cullRefl, *argsRefl);
    }
}

struct MapDrawContext {
    nvrhi::ICommandList* cmdList = nullptr;
    nvrhi::IDevice* nvDevice = nullptr;
    fg::RenderDevice* device = nullptr;
    nvrhi::IFramebuffer* framebuffer = nullptr;
    nvrhi::IBuffer* lightCB = nullptr;
    nvrhi::IBindingSet* bindlessTable = nullptr;
    nvrhi::Viewport viewport;
    nvrhi::Rect scissor;
};

void DrawDynamicCasters(SunShadowState& state, const SunShadowDrawConfig& cfg, const MapDrawContext& draw)
{
    if (!cfg.dynamicCompactDrawArgs || !cfg.dynamicCompactMaterialIDs || !cfg.dynamicCompactBatchIndices ||
        !cfg.dynamicCompactCount || !cfg.dynamicInstanceBuffer || !cfg.dynamicFadeBuffer || cfg.dynamicObjectCount == 0 ||
        !state.depthDynamicPipeline || !state.depthDynamicLayout)
        return;

    auto& cache = framegraph::GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* forwardVsRefl = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
    auto* dynamicPsRefl = shaderLoader->GetCachedReflection("bindless_depth_at", ".ps");
    nvrhi::IBuffer* drawIndexBuffer = GetOrCreateDrawIndexBuffer("SunShadow", draw.nvDevice);
    if (!forwardVsRefl || !dynamicPsRefl || !drawIndexBuffer)
        return;

    framegraph::BindingSetBuilder dbsb(*forwardVsRefl, *dynamicPsRefl, draw.nvDevice, "SunShadow.Dynamic");
    dbsb.ConstantBuffer("static_globals", draw.lightCB);
    dbsb.BufferSRV("g_Materials", bindless::MaterialBuffer::Instance().GetBuffer());
    dbsb.BufferSRV("g_InstanceData", cfg.dynamicInstanceBuffer);
    dbsb.BufferSRV("g_CompactBatchIndices", cfg.dynamicCompactBatchIndices);
    dbsb.BufferSRV("g_CompactMaterialIDs", cfg.dynamicCompactMaterialIDs);
    dbsb.BufferSRV("g_DrawFades", cfg.dynamicFadeBuffer);
    auto dynamicSet = cache.GetOrCreateBindingSet(dbsb.Build(), state.depthDynamicLayout, draw.nvDevice);
    if (!dynamicSet)
        return;

    nvrhi::GraphicsState gs;
    gs.pipeline = state.depthDynamicPipeline;
    gs.framebuffer = draw.framebuffer;
    gs.bindings = { dynamicSet };
    if (draw.bindlessTable)
        gs.addBindingSet(draw.bindlessTable);
    gs.vertexBuffers = { { cfg.megaVertexBuffer, 0, 0 }, { drawIndexBuffer, 1, 0 } };
    gs.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    gs.indirectParams = cfg.dynamicCompactDrawArgs;
    gs.indirectCountBuffer = cfg.dynamicCompactCount;
    gs.viewport.addViewport(draw.viewport);
    gs.viewport.addScissorRect(draw.scissor);
    draw.cmdList->setGraphicsState(gs);
    DrawIndexedIndirectCountOrFallback(draw.cmdList, 0, 0, cfg.dynamicObjectCount);
}

}

namespace {

void DrawSkinnedCasters(SunShadowState& state, const SunShadowMapData& data, const MapDrawContext& draw)
{
    const SunShadowDrawConfig& cfg = data.config;
    SunShadowTarget& target = state.targets[data.target];
    if (!cfg.gpuCulling || !state.depthSkinnedPipeline || !state.depthSkinnedLayout || !target.skinnedStream || !target.skinnedArgs)
        return;

    GPUCullingManager& gpuCulling = *cfg.gpuCulling;
    nvrhi::IBuffer* entries = gpuCulling.GetSkinnedEntryBuffer();
    nvrhi::IBuffer* preVB = gpuCulling.GetSkinnedPreVertexBuffer();
    nvrhi::IBuffer* skinnedIB = gpuCulling.GetSkinnedPools().GetCombinedIndexBuffer();
    if (gpuCulling.GetSkinnedEntryCount() == 0 || !entries || !preVB || !skinnedIB)
        return;

    auto& cache = framegraph::GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("cluster_pull_skinned", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("bindless_depth_opaque", ".ps");
    if (!vsRefl || !psRefl)
        return;

    framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, draw.nvDevice, "SunShadow.Skinned");
    bsb.ConstantBuffer("static_globals", draw.lightCB)
       .BufferSRV("g_VisibleEntries", target.skinnedStream)
       .BufferSRV("g_Entries", entries)
       .BufferSRV("g_SkinnedVB", preVB)
       .BufferSRV("g_SkinnedIB", skinnedIB);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.depthSkinnedLayout, draw.nvDevice);
    if (!bindingSet)
        return;

    nvrhi::GraphicsState gs;
    gs.pipeline = state.depthSkinnedPipeline;
    gs.framebuffer = draw.framebuffer;
    gs.bindings = { bindingSet };
    gs.indirectParams = target.skinnedArgs;
    gs.viewport.addViewport(draw.viewport);
    gs.viewport.addScissorRect(draw.scissor);
    draw.cmdList->setGraphicsState(gs);
    draw.cmdList->drawIndirect(0, 1);
}

void ExecuteSunShadowMap(fg::RenderContext* ctx, const FrameGraph& fg, const SunShadowMapData& data)
{
    SunShadowState& state = *data.state;
    SunShadowTarget& target = state.targets[data.target];
    const TargetNames& names = kTargetNames[data.target];
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* map = fg.GetPhysicalTexture(data.map);
    if (!cmdList || !nvDevice || !map)
        return;
    GPUZone zone(data.gpuProfiler, cmdList, names.zoneName);
    if (!target.redraw)
        return;

    cmdList->clearDepthStencilTexture(map, nvrhi::AllSubresources, true, 0.0f, false, 0);

    if (!target.valid || !target.countBuffer)
        return;
    if (!EnsureDepthPipelines(data.device, state))
        return;

    const SunShadowDrawConfig& cfg = data.config;
    if (!cfg.entryBuffer || !cfg.megaVertexBuffer || !cfg.megaIndexBuffer)
        return;

    if (cfg.materialCache)
        cfg.materialCache->FinalizePendingMaterials(ctx);
    auto& matBuffer = bindless::MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    auto& cache = framegraph::GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("cluster_pull", ".vs");
    auto* opaqueRefl = shaderLoader->GetCachedReflection("bindless_depth_opaque", ".ps");
    auto* atRefl = shaderLoader->GetCachedReflection("shadow_depth_at", ".ps");
    if (!vsRefl || !opaqueRefl || !atRefl)
        return;

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(map);
    auto framebuffer = cache.GetOrCreateFramebuffer(names.framebufferName, fbDesc, nvDevice);
    if (!framebuffer)
        return;

    StaticGlobals globals = BuildStaticGlobals();
    globals.m_VP = target.vp;
    auto lightCB = cache.GetOrCreateVolatileCB("SunShadow", "StaticGlobals", sizeof(StaticGlobals), data.device);
    cmdList->writeBuffer(lightCB, &globals, sizeof(globals));

    auto* backend = data.device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

    const auto& rtDesc = map->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    auto drawStream = [&](nvrhi::IGraphicsPipeline* pipeline, nvrhi::IBindingLayout* layout,
                          const ExtractedReflection& psRefl, nvrhi::IBuffer* instanceBuffer,
                          nvrhi::IBuffer* stream, nvrhi::IBuffer* args, bool withBindless, const char* label) {
        if (!pipeline || !layout || !instanceBuffer || !stream || !args)
            return;

        framegraph::BindingSetBuilder bsb(*vsRefl, psRefl, nvDevice, label);
        bsb.ConstantBuffer("static_globals", lightCB);
        if (withBindless)
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        bsb.BufferSRV("g_InstanceData", instanceBuffer);
        bsb.BufferSRV("g_VisibleEntries", stream);
        bsb.BufferSRV("g_Entries", cfg.entryBuffer);
        bsb.BufferSRV("g_MegaVB", cfg.megaVertexBuffer);
        bsb.BufferSRV("g_MegaIB", cfg.megaIndexBuffer);

        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), layout, nvDevice);
        if (!bindingSet)
            return;

        nvrhi::GraphicsState gs;
        gs.pipeline = pipeline;
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        if (withBindless && bindlessTable)
            gs.addBindingSet(bindlessTable);
        gs.indirectParams = args;
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

        cmdList->setGraphicsState(gs);
        cmdList->drawIndirect(0, 1);
    };

    drawStream(state.depthOpaquePipeline, state.depthOpaqueLayout, *opaqueRefl,
        cfg.staticInstanceBuffer, target.opaqueStream, target.opaqueArgs, false, "SunShadow.Opaque");
    drawStream(state.depthOpaquePipeline, state.depthOpaqueLayout, *opaqueRefl,
        cfg.terrainInstanceBuffer, target.terrainStream, target.terrainArgs, false, "SunShadow.Terrain");
    drawStream(state.depthATPipeline, state.depthATLayout, *atRefl,
        cfg.staticInstanceBuffer, target.atStream, target.atArgs, true, "SunShadow.AT");

    if (data.target == kSunTargetFar)
        return;

    MapDrawContext draw;
    draw.cmdList = cmdList;
    draw.nvDevice = nvDevice;
    draw.device = data.device;
    draw.framebuffer = framebuffer;
    draw.lightCB = lightCB;
    draw.bindlessTable = bindlessTable;
    draw.viewport = viewport;
    draw.scissor = nvrhi::Rect(rtDesc.width, rtDesc.height);
    DrawDynamicCasters(state, cfg, draw);
    DrawSkinnedCasters(state, data, draw);
}

} // namespace

void InvalidateSunShadowCache(SunShadowState& state)
{
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        state.targets[t].valid = false;
        state.targets[t].redraw = false;
    }
    state.cascSunInit = false;
}

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

void ComputeSunCascadeVP(Fmatrix& outVP, float& outTexel, const Fvector& sunDirIn, float boxSize, u32 mapSize)
{
    Fvector sunDir = sunDirIn;
    if (sunDir.magnitude() < 1e-4f)
        sunDir.set(0.0f, -1.0f, 0.0f);
    sunDir.normalize();

    Fvector up;
    up.set(0.0f, 1.0f, 0.0f);
    if (_abs(sunDir.y) > 0.99f)
        up.set(0.0f, 0.0f, 1.0f);

    Fmatrix view;
    view.build_camera_dir(Device.vCameraPosition, sunDir, up);

    Fmatrix proj;
    proj.build_projection_ortho(boxSize, boxSize, kCascadeZBehind, kCascadeZAhead);
    Fmatrix vp;
    vp.mul(proj, view);

    Fvector anchor;
    anchor.set((floorf(Device.vCameraPosition.x / kAnchorGrid) + 0.5f) * kAnchorGrid,
               (floorf(Device.vCameraPosition.y / kAnchorGrid) + 0.5f) * kAnchorGrid,
               (floorf(Device.vCameraPosition.z / kAnchorGrid) + 0.5f) * kAnchorGrid);
    Fvector a;
    vp.transform_tiny(a, anchor);
    const float texNdc = 2.0f / float(std::max(mapSize, 1u));
    const float fx = a.x - floorf(a.x / texNdc) * texNdc;
    const float fy = a.y - floorf(a.y / texNdc) * texNdc;
    Fmatrix snap;
    snap.identity();
    snap.c.x = -fx;
    snap.c.y = -fy;
    outVP.mul(snap, vp);
    outTexel = boxSize / float(std::max(mapSize, 1u));
}

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    framegraph::VirtualResourceHandle skinnedOrder,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCapacity,
    u32 entryCount,
    GPUCullingManager* gpuCulling,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    SunShadowCullOutput out;
    if (!state || !device || !entryBuffer || entryCount == 0)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;

    SunShadowTarget& far = state->targets[kSunTargetFar];
    SunShadowTarget& casc0 = state->targets[kSunTargetCasc0];
    for (u32 t = 0; t < kSunTargetCount; ++t)
        ProcessReadback(nvDevice, state->targets[t]);
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        if (!EnsureCullBuffers(nvDevice, state->targets[t], kTargetNames[t].suffix, entryCapacity))
            return out;
    }

    const int rasterBias = ps_r_sun_shadow_bias;
    const float rasterSlope = ps_r_sun_shadow_slope;
    if (state->rasterBias != rasterBias || state->rasterSlope != rasterSlope) {
        state->rasterBias = rasterBias;
        state->rasterSlope = rasterSlope;
        state->depthOpaquePipeline = nullptr;
        state->depthATPipeline = nullptr;
        state->depthDynamicPipeline = nullptr;
        state->depthPipelinesFailed = false;
        state->depthSkinnedPipeline = nullptr;
        far.valid = false;
    }

    Fvector sunDir = SunDirVisual();
    if (sunDir.magnitude() < 1e-4f)
        sunDir.set(0.0f, -1.0f, 0.0f);
    sunDir.normalize();

    if (!state->cascSunInit) {
        state->cascSunDir = sunDir;
        state->cascSunInit = true;
    } else if (state->cascSunDir.dotproduct(sunDir) < kCascSnapDot) {
        state->cascSunDir = sunDir;
    } else {
        const float k = 1.0f - expf(-Device.fTimeDelta / kCascSunTau);
        state->cascSunDir.lerp(state->cascSunDir, sunDir, k);
        if (state->cascSunDir.magnitude() > 1e-4f)
            state->cascSunDir.normalize();
        else
            state->cascSunDir = sunDir;
    }
    const Fvector cascSunDir = state->cascSunDir;

    const Fvector camPos = Device.vCameraPosition;
    const float box = ps_r_sun_shadow_far_box;
    const u32 size = u32(std::max(ps_r_sun_shadow_far_size, 64));
    const float lod = ps_r_shadow_cluster_lod;
    const int at = ps_r_shadow_at ? 1 : 0;
    const bool cacheValid = far.valid
        && state->farEntryCount == entryCount
        && state->farBox == box
        && far.mapSize == size
        && state->farLod == lod
        && state->farAT == at
        && camPos.distance_to_sqr(state->farCamPos) < kFarRedrawDist * kFarRedrawDist
        && state->farSunDir.dotproduct(sunDir) > kFarSunDotRedraw;
    far.redraw = !cacheValid;
    if (far.redraw) {
        ComputeSunFarVP(far.vp, far.texel, sunDir, box, size);
        far.valid = true;
        state->farCamPos = camPos;
        state->farSunDir = sunDir;
        state->farEntryCount = entryCount;
        state->farBox = box;
        state->farLod = lod;
        state->farAT = at;
        ++state->farRedraws;
    }

    ComputeSunCascadeVP(casc0.vp, casc0.texel, cascSunDir, kCascadeBox0, kCascadeSize0);
    casc0.valid = true;
    casc0.redraw = true;

    SunShadowTarget& casc1 = state->targets[kSunTargetCasc1];
    casc1.redraw = !casc1.valid || (Device.dwFrame & 1) == 0;
    if (casc1.redraw) {
        ComputeSunCascadeVP(casc1.vp, casc1.texel, cascSunDir, kCascadeBox1, kCascadeSize1);
        casc1.valid = true;
    }

    state->candidates = entryCount;

    ResourceDesc argsDesc;
    argsDesc.type = ResourceDesc::Type::Buffer;
    argsDesc.bufferSize = sizeof(u32) * 4;
    argsDesc.isUAV = true;
    argsDesc.allowUAV = true;
    argsDesc.isImported = true;
    argsDesc.isTransient = false;
    argsDesc.debugName = "sun_shadow_args";
    static const char* kSkinnedArgsNames[kSunTargetCount] = { "sun_shadow_args_skinned_far", "sun_shadow_args_skinned_casc0", "sun_shadow_args_skinned_casc1" };
    SunShadowCullOutput::Target imported[kSunTargetCount];
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        imported[t].opaqueArgs = fg.ImportBuffer(kTargetNames[t].argsOpaqueName, state->targets[t].opaqueArgs, argsDesc);
        imported[t].terrainArgs = fg.ImportBuffer(kTargetNames[t].argsTerrainName, state->targets[t].terrainArgs, argsDesc);
        imported[t].atArgs = fg.ImportBuffer(kTargetNames[t].argsATName, state->targets[t].atArgs, argsDesc);
        imported[t].skinnedArgs = fg.ImportBuffer(kSkinnedArgsNames[t], state->targets[t].skinnedArgs, argsDesc);
    }

    auto& passData = fg.addCallbackPass<SunShadowCullData>(
        "Sun Shadow Cull",
        [&, orderAfter, skinnedOrder, entryBuffer, entryCount, gpuCulling, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, SunShadowCullData& data) {
            data.state = state;
            data.device = device;
            data.entryBuffer = entryBuffer;
            data.entryCount = entryCount;
            data.gpuCulling = gpuCulling;
            data.gpuProfiler = gpuProfiler;

            RenderPassBuilder passBuilder(builder, passHandle);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::IndirectArgument);
            if (skinnedOrder.is_valid())
                data.skinnedOrder = passBuilder.read(skinnedOrder, ResourceState::ShaderResource);
            for (u32 t = 0; t < kSunTargetCount; ++t) {
                data.targets[t].opaqueArgs = passBuilder.write(imported[t].opaqueArgs, ResourceState::UnorderedAccess);
                data.targets[t].terrainArgs = passBuilder.write(imported[t].terrainArgs, ResourceState::UnorderedAccess);
                data.targets[t].atArgs = passBuilder.write(imported[t].atArgs, ResourceState::UnorderedAccess);
                data.targets[t].skinnedArgs = passBuilder.write(imported[t].skinnedArgs, ResourceState::UnorderedAccess);
            }
        },
        [](const SunShadowCullData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteSunShadowCull(ctx, data);
        });

    for (u32 t = 0; t < kSunTargetCount; ++t)
        out.targets[t] = passData.targets[t];
    out.active = true;
    return out;
}

SunShadowMaps setupSunShadowMapPasses(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const SunShadowCullOutput& cull,
    const SunShadowDrawConfig& config,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    SunShadowMaps maps;
    if (!state || !device || !cull.active)
        return maps;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return maps;

    for (u32 t = 0; t < kSunTargetCount; ++t) {
        SunShadowTarget& target = state->targets[t];
        const TargetNames& names = kTargetNames[t];
        const u32 size = (t == kSunTargetFar) ? u32(std::max(ps_r_sun_shadow_far_size, 64))
            : (t == kSunTargetCasc0 ? kCascadeSize0 : kCascadeSize1);
        if (!EnsureMap(nvDevice, target, names.suffix, size))
            continue;

        ResourceDesc mapDesc;
        mapDesc.type = ResourceDesc::Type::Texture2D;
        mapDesc.width = size;
        mapDesc.height = size;
        mapDesc.format = nvrhi::Format::D32;
        mapDesc.isDepthStencil = true;
        mapDesc.isImported = true;
        mapDesc.isTransient = false;
        mapDesc.debugName = names.rtName;
        VirtualResourceHandle handle = fg.ImportTexture(names.rtName, target.map, mapDesc);
        const SunShadowCullOutput::Target cullTarget = cull.targets[t];

        auto& passData = fg.addCallbackPass<SunShadowMapData>(
            names.passName,
            [&, handle, cullTarget, config, state, t, gpuProfiler](FrameGraph& builder, PassHandle passHandle, SunShadowMapData& data) {
                data.state = state;
                data.device = device;
                data.config = config;
                data.target = t;
                data.gpuProfiler = gpuProfiler;

                RenderPassBuilder passBuilder(builder, passHandle);
                data.map = passBuilder.write(handle, ResourceState::DepthStencilWrite);
                data.opaqueArgs = passBuilder.read(cullTarget.opaqueArgs, ResourceState::IndirectArgument);
                data.terrainArgs = passBuilder.read(cullTarget.terrainArgs, ResourceState::IndirectArgument);
                data.atArgs = passBuilder.read(cullTarget.atArgs, ResourceState::IndirectArgument);
                if (t != kSunTargetFar && cullTarget.skinnedArgs.is_valid())
                    data.skinnedCullArgs = passBuilder.read(cullTarget.skinnedArgs, ResourceState::IndirectArgument);
                if (t != kSunTargetFar && config.dynamicArgs.is_valid())
                    data.dynamicArgs = passBuilder.read(config.dynamicArgs, ResourceState::IndirectArgument);
                if (t != kSunTargetFar && config.skinnedArgs.is_valid())
                    data.skinnedArgs = passBuilder.read(config.skinnedArgs, ResourceState::ShaderResource);
            },
            [](const SunShadowMapData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                ExecuteSunShadowMap(ctx, fg, data);
            });

        fg.GetRTRegistry().RegisterRT(names.rtName, passData.map);
        maps.maps[t] = passData.map;
    }
    return maps;
}

void ReadSunShadowMaps(RenderPassBuilder& builder, const SunShadowMaps& in, SunShadowMaps& out)
{
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        if (in.maps[t].is_valid())
            out.maps[t] = builder.read(in.maps[t], ResourceState::ShaderResource);
    }
    if (in.mask.is_valid())
        out.mask = builder.read(in.mask, ResourceState::ShaderResource);
}

void ResolveSunShadowMaps(const FrameGraph& fg, const SunShadowMaps& maps, nvrhi::IDevice* device, nvrhi::ITexture** out)
{
    nvrhi::ITexture* dummy = framegraph::GetPassResourceCache().GetDummyShadowMap2D(device);
    for (u32 t = 0; t < kSunTargetCount; ++t) {
        nvrhi::ITexture* tex = maps.maps[t].is_valid() ? fg.GetPhysicalTexture(maps.maps[t]) : nullptr;
        out[t] = tex ? tex : dummy;
    }
    nvrhi::ITexture* mask = maps.mask.is_valid() ? fg.GetPhysicalTexture(maps.mask) : nullptr;
    out[kSunTargetCount] = mask ? mask : dummy;
}

void BindSunShadowMaps(BindingSetBuilder& bsb, nvrhi::ITexture* const* maps)
{
    for (u32 t = 0; t < kSunTargetCount; ++t)
        bsb.Texture(kTargetNames[t].srvName, maps[t]);
    bsb.Texture("g_SunShadowMask", maps[kSunTargetCount]);
}

} // namespace xray::render::fg::passes
