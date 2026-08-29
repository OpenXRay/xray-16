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
constexpr u32 kCullThreadGroup = 64;
constexpr int kShadowDepthBias = -2;
constexpr float kShadowSlopeBias = -2.5f;
constexpr float kFarRedrawDist = 10.0f;
constexpr float kFarSunDotRedraw = 0.99999847f;

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
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
};

struct SunShadowFarData {
    VirtualResourceHandle farMap;
    VirtualResourceHandle opaqueArgs;
    VirtualResourceHandle terrainArgs;
    VirtualResourceHandle atArgs;
    SunShadowState* state;
    fg::RenderDevice* device;
    SunShadowDrawConfig config;
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
    auto opaqueResult = shaderLoader->LoadPixelShader("bindless_depth_opaque", "main");
    auto atResult = shaderLoader->LoadPixelShader("shadow_depth_at", "main");
    if (!vsResult.handle || !opaqueResult.handle || !atResult.handle ||
        !vsResult.reflection || !opaqueResult.reflection || !atResult.reflection) {
        Msg("! [SunShadow] depth shaders failed to load");
        state.depthPipelinesFailed = true;
        return false;
    }

    state.clusterVS = vsResult.handle;
    state.depthOpaquePS = opaqueResult.handle;
    state.depthATPS = atResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    state.depthOpaqueLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_Opaque", *vsResult.reflection, *opaqueResult.reflection, nvDevice);
    state.depthATLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShadowDepth_AT", *vsResult.reflection, *atResult.reflection, nvDevice);
    if (!state.depthOpaqueLayout || !state.depthATLayout) {
        state.depthPipelinesFailed = true;
        return false;
    }

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    auto makeDesc = [&](nvrhi::IShader* pixelShader, nvrhi::IBindingLayout* layout, bool withBindless) {
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = state.clusterVS;
        desc.PS = pixelShader;
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
        desc.renderState.rasterState.depthBias = kShadowDepthBias;
        desc.renderState.rasterState.slopeScaledDepthBias = kShadowSlopeBias;
        desc.renderState.rasterState.depthBiasClamp = 0.0f;
        return desc;
    };

    state.depthOpaquePipeline = cache.GetOrCreatePipeline("SunShadowDepth_Opaque",
        makeDesc(state.depthOpaquePS, state.depthOpaqueLayout, false), fbInfo, nvDevice);
    state.depthATPipeline = cache.GetOrCreatePipeline("SunShadowDepth_AT",
        makeDesc(state.depthATPS, state.depthATLayout, true), fbInfo, nvDevice);

    if (!state.depthOpaquePipeline || !state.depthATPipeline) {
        Msg("! [SunShadow] depth pipeline creation failed");
        state.depthPipelinesFailed = true;
        return false;
    }

    Msg("* [SunShadow] depth pipelines initialized");
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

bool EnsureFarMap(nvrhi::IDevice* nvDevice, SunShadowState& state, u32 size)
{
    if (state.farMap && state.farMapSize == size)
        return true;

    nvrhi::TextureDesc desc;
    desc.width = size;
    desc.height = size;
    desc.format = nvrhi::Format::D32;
    desc.debugName = "SunShadow_Far";
    desc.isShaderResource = true;
    desc.isRenderTarget = true;
    desc.isTypeless = true;
    desc.useClearValue = true;
    desc.clearValue = nvrhi::Color(0.0f);
    desc.initialState = nvrhi::ResourceStates::DepthWrite;
    desc.keepInitialState = true;
    state.farMap = nvDevice->createTexture(desc);
    state.farMapSize = state.farMap ? size : 0;
    if (!state.farMap)
        Msg("! [SunShadow] far map creation failed (%u)", size);
    return state.farMap != nullptr;
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
    if (!cmdList || !nvDevice)
        return;
    GPUZone zone(data.gpuProfiler, cmdList, "Shadow/Cull");
    if (!state.farRedraw || !state.countBuffer || !state.farValid)
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

void ExecuteSunShadowFar(fg::RenderContext* ctx, const FrameGraph& fg, const SunShadowFarData& data)
{
    SunShadowState& state = *data.state;
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
    nvrhi::ITexture* farMap = fg.GetPhysicalTexture(data.farMap);
    if (!cmdList || !nvDevice || !farMap)
        return;
    GPUZone zone(data.gpuProfiler, cmdList, "Shadow/Far");
    if (!state.farRedraw)
        return;

    cmdList->clearDepthStencilTexture(farMap, nvrhi::AllSubresources, true, 0.0f, false, 0);

    if (!state.farValid || !state.countBuffer)
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
    fbDesc.setDepthAttachment(farMap);
    auto framebuffer = cache.GetOrCreateFramebuffer("SunShadowFar", fbDesc, nvDevice);
    if (!framebuffer)
        return;

    StaticGlobals globals = BuildStaticGlobals();
    globals.m_VP = state.farVP;
    auto lightCB = cache.GetOrCreateVolatileCB("SunShadow", "StaticGlobals", sizeof(StaticGlobals), data.device);
    cmdList->writeBuffer(lightCB, &globals, sizeof(globals));

    auto* backend = data.device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

    const auto& rtDesc = farMap->getDesc();
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
        cfg.staticInstanceBuffer, state.opaqueStream, state.opaqueArgs, false, "SunShadow.Opaque");
    drawStream(state.depthOpaquePipeline, state.depthOpaqueLayout, *opaqueRefl,
        cfg.terrainInstanceBuffer, state.terrainStream, state.terrainArgs, false, "SunShadow.Terrain");
    drawStream(state.depthATPipeline, state.depthATLayout, *atRefl,
        cfg.staticInstanceBuffer, state.atStream, state.atArgs, true, "SunShadow.AT");
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

void InvalidateSunShadowCache(SunShadowState& state)
{
    state.farValid = false;
    state.farRedraw = false;
}

SunShadowCullOutput setupSunShadowCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle orderAfter,
    nvrhi::IBuffer* entryBuffer,
    u32 entryCount,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
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
    if (sunDir.magnitude() < 1e-4f)
        sunDir.set(0.0f, -1.0f, 0.0f);
    sunDir.normalize();

    const Fvector camPos = Device.vCameraPosition;
    const float box = ps_r_sun_shadow_far_box;
    const u32 size = u32(std::max(ps_r_sun_shadow_far_size, 64));
    const float lod = ps_r_shadow_cluster_lod;
    const int at = ps_r_shadow_at ? 1 : 0;
    const bool cacheValid = state->farValid
        && state->farEntryCount == entryCount
        && state->farBox == box
        && state->farMapSize == size
        && state->farLod == lod
        && state->farAT == at
        && camPos.distance_to_sqr(state->farCamPos) < kFarRedrawDist * kFarRedrawDist
        && state->farSunDir.dotproduct(sunDir) > kFarSunDotRedraw;
    state->farRedraw = !cacheValid;
    if (state->farRedraw) {
        ComputeSunFarVP(state->farVP, state->farTexel, sunDir, box, size);
        state->farValid = true;
        state->farCamPos = camPos;
        state->farSunDir = sunDir;
        state->farEntryCount = entryCount;
        state->farBox = box;
        state->farLod = lod;
        state->farAT = at;
        ++state->farRedraws;
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
    VirtualResourceHandle opaqueArgs = fg.ImportBuffer("sun_shadow_args_opaque", state->opaqueArgs, argsDesc);
    VirtualResourceHandle terrainArgs = fg.ImportBuffer("sun_shadow_args_terrain", state->terrainArgs, argsDesc);
    VirtualResourceHandle atArgs = fg.ImportBuffer("sun_shadow_args_at", state->atArgs, argsDesc);

    auto& passData = fg.addCallbackPass<SunShadowCullData>(
        "Sun Shadow Cull",
        [&, orderAfter, opaqueArgs, terrainArgs, atArgs, entryBuffer, entryCount, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, SunShadowCullData& data) {
            data.state = state;
            data.device = device;
            data.entryBuffer = entryBuffer;
            data.entryCount = entryCount;
            data.gpuProfiler = gpuProfiler;

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

framegraph::VirtualResourceHandle setupSunShadowFarPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const SunShadowCullOutput& cull,
    const SunShadowDrawConfig& config,
    SunShadowState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    if (!state || !device || !cull.active)
        return VirtualResourceHandle();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return VirtualResourceHandle();

    const u32 size = u32(std::max(ps_r_sun_shadow_far_size, 64));
    if (!EnsureFarMap(nvDevice, *state, size))
        return VirtualResourceHandle();

    ResourceDesc mapDesc;
    mapDesc.type = ResourceDesc::Type::Texture2D;
    mapDesc.width = size;
    mapDesc.height = size;
    mapDesc.format = nvrhi::Format::D32;
    mapDesc.isDepthStencil = true;
    mapDesc.isImported = true;
    mapDesc.isTransient = false;
    mapDesc.debugName = "rt_SunShadowFar";
    VirtualResourceHandle farHandle = fg.ImportTexture("rt_SunShadowFar", state->farMap, mapDesc);

    auto& passData = fg.addCallbackPass<SunShadowFarData>(
        "Sun Shadow Far",
        [&, farHandle, cull, config, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, SunShadowFarData& data) {
            data.state = state;
            data.device = device;
            data.config = config;
            data.gpuProfiler = gpuProfiler;

            RenderPassBuilder passBuilder(builder, passHandle);
            data.farMap = passBuilder.write(farHandle, ResourceState::DepthStencilWrite);
            data.opaqueArgs = passBuilder.read(cull.opaqueArgs, ResourceState::IndirectArgument);
            data.terrainArgs = passBuilder.read(cull.terrainArgs, ResourceState::IndirectArgument);
            data.atArgs = passBuilder.read(cull.atArgs, ResourceState::IndirectArgument);
        },
        [](const SunShadowFarData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ExecuteSunShadowFar(ctx, fg, data);
        });

    return passData.farMap;
}

} // namespace xray::render::fg::passes
