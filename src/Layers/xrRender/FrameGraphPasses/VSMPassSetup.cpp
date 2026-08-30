#include "stdafx.h"
#include "VSMPassSetup.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct VsmMarkParams {
    Fmatrix invViewProj;
    Fvector4 screen;
    u32 markStep;
    u32 lodBias;
    u32 pad[2];
};

struct VsmDebugParams {
    Fmatrix invViewProj;
    Fvector4 screen;
    u32 mode;
    u32 pad[3];
};

struct VSMMarkData {
    VirtualResourceHandle depth;
    VirtualResourceHandle order;
    VirtualResourceHandle needed;
    VSMState* state;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
    xray::profiler::GPUProfiler* gpuProfiler;
};

struct VSMDebugData {
    VirtualResourceHandle depth;
    VirtualResourceHandle needed;
    VirtualResourceHandle output;
    VSMState* state;
    fg::RenderDevice* device;
    u32 width;
    u32 height;
};

struct VSMZone {
    xray::profiler::GPUProfiler* profiler;
    nvrhi::ICommandList* cmdList;
    const char* name;
    VSMZone(xray::profiler::GPUProfiler* p, nvrhi::ICommandList* c, const char* n) : profiler(p), cmdList(c), name(n)
    {
        if (profiler && cmdList)
            profiler->BeginPass(cmdList, name);
    }
    ~VSMZone()
    {
        if (profiler && cmdList)
            profiler->EndPass(cmdList, name);
    }
};

bool EnsurePipelines(fg::RenderDevice* device, VSMState& state)
{
    if (state.markPipeline && state.debugPipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto markResult = shaderLoader->LoadComputeShader("vsm_mark");
    auto debugResult = shaderLoader->LoadComputeShader("vsm_debug_view");
    if (!markResult.handle || !markResult.reflection || !debugResult.handle || !debugResult.reflection) {
        Msg("! [VSM] mark shaders failed to load");
        state.pipelinesFailed = true;
        return false;
    }

    auto& cache = GetPassResourceCache();
    state.markLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMMark", *markResult.reflection, nvDevice);
    state.debugLayout = cache.GetOrCreateBindingLayoutFromReflection("VSMDebugView", *debugResult.reflection, nvDevice);
    if (!state.markLayout || !state.debugLayout) {
        state.pipelinesFailed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc markDesc;
    markDesc.CS = markResult.handle;
    markDesc.bindingLayouts = { state.markLayout };
    state.markPipeline = cache.GetOrCreateComputePipeline("VSMMark", markDesc, nvDevice);

    nvrhi::ComputePipelineDesc debugDesc;
    debugDesc.CS = debugResult.handle;
    debugDesc.bindingLayouts = { state.debugLayout };
    state.debugPipeline = cache.GetOrCreateComputePipeline("VSMDebugView", debugDesc, nvDevice);

    if (!state.markPipeline || !state.debugPipeline) {
        Msg("! [VSM] mark pipeline creation failed");
        state.pipelinesFailed = true;
        return false;
    }
    Msg("* [VSM] mark pipelines initialized");
    return true;
}

bool EnsureBuffers(nvrhi::IDevice* nvDevice, VSMState& state)
{
    if (state.needed && state.counter)
        return true;

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_Needed";
        desc.byteSize = u64(kVSMPageCount) * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.needed = nvDevice->createBuffer(desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_Counter";
        desc.byteSize = sizeof(u32) * 4;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.counter = nvDevice->createBuffer(desc);
    }
    for (u32 i = 0; i < VSMState::kReadbackSlots; ++i) {
        if (state.readback[i])
            continue;
        nvrhi::BufferDesc desc;
        desc.debugName = "VSM_Readback";
        desc.byteSize = u64(kVSMPageCount + 4) * sizeof(u32);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        state.readback[i] = nvDevice->createBuffer(desc);
    }
    state.readbackWrite = 0;
    state.readbackScheduled = 0;

    if (!state.needed || !state.counter) {
        Msg("! [VSM] mark buffer creation failed");
        state.needed = nullptr;
        state.counter = nullptr;
        return false;
    }
    return true;
}

void ProcessReadback(nvrhi::IDevice* nvDevice, VSMState& state)
{
    if (state.readbackScheduled < VSMState::kReadbackSlots)
        return;
    nvrhi::IBuffer* oldest = state.readback[state.readbackWrite];
    if (!oldest)
        return;
    void* mapped = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    const u32* words = static_cast<const u32*>(mapped);
    state.markPages = std::min(words[0], kVSMPageCount);
    for (u32 L = 0; L < kVSMLevels; ++L) {
        u32 n = 0;
        const u32* level = words + 4 + L * kVSMPagesPerLevel;
        for (u32 i = 0; i < kVSMPagesPerLevel; ++i)
            n += level[i] != 0 ? 1 : 0;
        state.levelPages[L] = n;
    }
    nvDevice->unmapBuffer(oldest);
}

void ScheduleReadback(nvrhi::ICommandList* cmdList, VSMState& state)
{
    nvrhi::IBuffer* slot = state.readback[state.readbackWrite];
    if (!slot)
        return;
    cmdList->copyBuffer(slot, 0, state.counter, 0, sizeof(u32) * 4);
    cmdList->copyBuffer(slot, sizeof(u32) * 4, state.needed, 0, u64(kVSMPageCount) * sizeof(u32));
    state.readbackWrite = (state.readbackWrite + 1) % VSMState::kReadbackSlots;
    if (state.readbackScheduled < VSMState::kReadbackSlots)
        ++state.readbackScheduled;
}

void LogTelemetry(VSMState& state)
{
    if (ps_r_vsm_debug < 1)
        return;
    if (Device.dwTimeGlobal - state.lastLogTime < 2000)
        return;
    state.lastLogTime = Device.dwTimeGlobal;
    Msg("[VSM] mark: pages=%u | L0=%u L1=%u L2=%u L3=%u L4=%u L5=%u | sun %s step max %.3f deg | window snap max %u pages | base %.1f m | zc %.0f | inval %u",
        state.markPages, state.levelPages[0], state.levelPages[1], state.levelPages[2],
        state.levelPages[3], state.levelPages[4], state.levelPages[5],
        state.sunMoving ? "moving" : "static", state.sunStepMax, state.snapMax,
        ps_r_vsm_base, state.zCentre, state.invalidations);
    state.sunStepMax = 0.0f;
    state.snapMax = 0;
}

}

void InvalidateVSMCache(VSMState& state)
{
    state.pageBasePrevValid = false;
    state.prevSunValid = false;
    state.zCentreValid = false;
    state.invalidations++;
}

void VSMBeginFrame(VSMState& state, const Fvector& camPos, const Fvector& sunDirIn)
{
    state.frame++;

    Fvector sd = sunDirIn;
    if (sd.magnitude() < 1e-4f)
        sd.set(0.0f, -1.0f, 0.0f);
    sd.normalize();
    Fvector up;
    up.set(0.0f, 1.0f, 0.0f);
    if (_abs(sd.y) > 0.99f)
        up.set(0.0f, 0.0f, 1.0f);
    Fvector eye;
    eye.set(0.0f, 0.0f, 0.0f);
    Fmatrix view;
    view.build_camera_dir(eye, sd, up);
    state.sunView = view;

    Fvector camL;
    view.transform_tiny(camL, camPos);

    state.sunMoving = !state.prevSunValid || sd.x != state.prevSunDir.x || sd.y != state.prevSunDir.y || sd.z != state.prevSunDir.z;
    if (state.prevSunValid) {
        float dp = sd.dotproduct(state.prevSunDir);
        dp = dp > 1.0f ? 1.0f : (dp < -1.0f ? -1.0f : dp);
        const float stepDeg = acosf(dp) * 57.29578f;
        if (stepDeg > state.sunStepMax)
            state.sunStepMax = stepDeg;
    }
    state.prevSunDir = sd;
    state.prevSunValid = true;

    VsmParams& params = state.params;
    params.view = view;
    for (u32 L = 0; L < kVSMLevels; ++L) {
        const float ext = ps_r_vsm_base * float(1u << L);
        const float texel = ext / float(kVSMVirtualRes);
        const float pageTexel = texel * float(kVSMPageSize);
        const float baseX = camL.x - 0.5f * ext;
        const float baseY = camL.y - 0.5f * ext;
        const s32 pbx = (s32)floorf(baseX / pageTexel);
        const s32 pby = (s32)floorf(baseY / pageTexel);
        state.pageBase[L][0] = pbx;
        state.pageBase[L][1] = pby;
        params.level[L].set(float(pbx) * pageTexel, float(pby) * pageTexel, ext, 0.0f);
    }
    if (state.pageBasePrevValid) {
        u32 mx = 0;
        for (u32 L = 0; L < kVSMLevels; ++L) {
            const u32 d = (u32)(_abs(state.pageBase[L][0] - state.pageBasePrev[L][0]) + _abs(state.pageBase[L][1] - state.pageBasePrev[L][1]));
            if (d > mx)
                mx = d;
        }
        if (mx > state.snapMax)
            state.snapMax = mx;
    }
    for (u32 L = 0; L < kVSMLevels; ++L) {
        state.pageBasePrev[L][0] = state.pageBase[L][0];
        state.pageBasePrev[L][1] = state.pageBase[L][1];
    }
    state.pageBasePrevValid = true;

    const float zCentre = floorf(camL.z / kVSMZSnap) * kVSMZSnap;
    if (!state.zCentreValid || zCentre != state.zCentre) {
        if (state.zCentreValid)
            InvalidateVSMCache(state);
        state.zCentre = zCentre;
        state.zCentreValid = true;
    }
    params.zparams.set(zCentre + kVSMZNear, 1.0f / (kVSMZFar - kVSMZNear), ps_r_vsm_bias, ps_r_vsm_bias_dyn);
}

VSMMarkOutput setupVSMMarkPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle orderAfter,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    VSMMarkOutput out;
    if (!state || !device || !depth.is_valid() || width == 0 || height == 0)
        return out;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return out;

    ProcessReadback(nvDevice, *state);
    if (!EnsureBuffers(nvDevice, *state))
        return out;
    LogTelemetry(*state);
    state->active = true;

    ResourceDesc neededDesc;
    neededDesc.type = ResourceDesc::Type::Buffer;
    neededDesc.bufferSize = u64(kVSMPageCount) * sizeof(u32);
    neededDesc.structStride = sizeof(u32);
    neededDesc.isUAV = true;
    neededDesc.allowUAV = true;
    neededDesc.isImported = true;
    neededDesc.isTransient = false;
    neededDesc.debugName = "vsm_needed";
    VirtualResourceHandle neededHandle = fg.ImportBuffer("vsm_needed", state->needed, neededDesc);

    auto& markData = fg.addCallbackPass<VSMMarkData>(
        "VSM Mark",
        [&, depth, orderAfter, neededHandle, width, height, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, VSMMarkData& data) {
            data.state = state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.gpuProfiler = gpuProfiler;
            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            if (orderAfter.is_valid())
                data.order = passBuilder.read(orderAfter, ResourceState::ShaderResource);
            data.needed = passBuilder.write(neededHandle, ResourceState::UnorderedAccess);
        },
        [](const VSMMarkData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            VSMState& state = *data.state;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
            if (!cmdList || !nvDevice || !depth)
                return;
            VSMZone zone(data.gpuProfiler, cmdList, "VSM/Mark");
            if (!EnsurePipelines(data.device, state))
                return;

            auto& cache = GetPassResourceCache();
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* markRefl = shaderLoader->GetCachedReflection("vsm_mark", ".cs");
            if (!markRefl)
                return;

            auto vsmCB = cache.GetOrCreateVolatileCB("VSM", "VsmParams", sizeof(VsmParams), data.device);
            cmdList->writeBuffer(vsmCB, &state.params, sizeof(VsmParams));

            const u32 markStep = ps_r_vsm_mark_half ? 2u : 1u;
            VsmMarkParams mp = {};
            mp.invViewProj = Device.mInvFullTransform;
            mp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
            mp.markStep = markStep;
            mp.lodBias = 0;
            auto markCB = cache.GetOrCreateVolatileCB("VSM", "MarkParams", sizeof(VsmMarkParams), data.device);
            cmdList->writeBuffer(markCB, &mp, sizeof(mp));

            cmdList->setBufferState(state.needed, nvrhi::ResourceStates::CopyDest);
            cmdList->setBufferState(state.counter, nvrhi::ResourceStates::CopyDest);
            cmdList->clearBufferUInt(state.needed, 0);
            cmdList->clearBufferUInt(state.counter, 0);
            cmdList->setBufferState(state.needed, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(state.counter, nvrhi::ResourceStates::UnorderedAccess);

            BindingSetBuilder bsb(*markRefl, nvDevice, "VSM.Mark");
            bsb.ConstantBuffer("VsmParams", vsmCB)
               .ConstantBuffer("VsmMarkParams", markCB)
               .Texture("g_Depth", depth)
               .BufferUAV("g_Needed", state.needed)
               .BufferUAV("g_Counter", state.counter);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.markLayout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::ComputeState cs;
            cs.pipeline = state.markPipeline;
            cs.bindings = { bindingSet };
            cmdList->setComputeState(cs);
            const u32 mw = (data.width + markStep - 1) / markStep;
            const u32 mh = (data.height + markStep - 1) / markStep;
            cmdList->dispatch((mw + 7) / 8, (mh + 7) / 8, 1);

            cmdList->setBufferState(state.needed, nvrhi::ResourceStates::CopySource);
            cmdList->setBufferState(state.counter, nvrhi::ResourceStates::CopySource);
            ScheduleReadback(cmdList, state);
            cmdList->setBufferState(state.needed, nvrhi::ResourceStates::ShaderResource);
            cmdList->setBufferState(state.counter, nvrhi::ResourceStates::ShaderResource);
        });

    out.needed = markData.needed;
    out.active = true;

    if (ps_r_vsm_debug >= 2) {
        ResourceDesc viewDesc;
        viewDesc.type = ResourceDesc::Type::Texture2D;
        viewDesc.width = width;
        viewDesc.height = height;
        viewDesc.format = nvrhi::Format::RGBA8_UNORM;
        viewDesc.isUAV = true;
        viewDesc.allowUAV = true;
        viewDesc.isTransient = true;
        viewDesc.debugName = "rt_VSMDebug";
        VirtualResourceHandle viewHandle = fg.CreateTexture("rt_VSMDebug", viewDesc);

        auto& debugData = fg.addCallbackPass<VSMDebugData>(
            "VSM Debug View",
            [&, depth, viewHandle, width, height, state](FrameGraph& builder, PassHandle passHandle, VSMDebugData& data) {
                data.state = state;
                data.device = device;
                data.width = width;
                data.height = height;
                RenderPassBuilder passBuilder(builder, passHandle);
                data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
                data.needed = passBuilder.read(markData.needed, ResourceState::ShaderResource);
                data.output = passBuilder.write(viewHandle, ResourceState::UnorderedAccess);
            },
            [](const VSMDebugData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                VSMState& state = *data.state;
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ITexture* depth = fg.GetPhysicalTexture(data.depth);
                nvrhi::ITexture* output = fg.GetPhysicalTexture(data.output);
                if (!cmdList || !nvDevice || !depth || !output || !state.debugPipeline)
                    return;

                auto& cache = GetPassResourceCache();
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* refl = shaderLoader->GetCachedReflection("vsm_debug_view", ".cs");
                if (!refl)
                    return;

                auto vsmCB = cache.GetOrCreateVolatileCB("VSM", "VsmParams", sizeof(VsmParams), data.device);
                cmdList->writeBuffer(vsmCB, &state.params, sizeof(VsmParams));

                VsmDebugParams dp = {};
                dp.invViewProj = Device.mInvFullTransform;
                dp.screen.set(float(data.width), float(data.height), 1.0f / float(data.width), 1.0f / float(data.height));
                dp.mode = u32(ps_r_vsm_debug);
                auto debugCB = cache.GetOrCreateVolatileCB("VSM", "DebugParams", sizeof(VsmDebugParams), data.device);
                cmdList->writeBuffer(debugCB, &dp, sizeof(dp));

                BindingSetBuilder bsb(*refl, nvDevice, "VSM.DebugView");
                bsb.ConstantBuffer("VsmParams", vsmCB)
                   .ConstantBuffer("VsmDebugParams", debugCB)
                   .Texture("g_Depth", depth)
                   .BufferSRV("g_Needed", state.needed)
                   .TextureUAV("g_Output", output);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), state.debugLayout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::ComputeState cs;
                cs.pipeline = state.debugPipeline;
                cs.bindings = { bindingSet };
                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            });
        out.debugView = debugData.output;
    }

    return out;
}

}
