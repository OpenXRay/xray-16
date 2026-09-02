#include "stdafx.h"
#include "DeferredLightPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct DeferredLightPassData {
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle color;
    SunShadowMaps sunShadowMaps;
    fg::RenderDevice* device = nullptr;
    DeferredLightPassState* state = nullptr;
    xray::profiler::GPUProfiler* gpuProfiler = nullptr;
    u32 width = 0;
    u32 height = 0;
};

void EnsureDeferredLightPipeline(fg::RenderDevice* device, DeferredLightPassState& state)
{
    if (state.initialized || state.failed)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader) {
        state.failed = true;
        return;
    }

    auto csResult = shaderLoader->LoadComputeShader("deferred_light", "main");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [DeferredLight] Failed to load deferred_light compute shader");
        state.failed = true;
        return;
    }
    state.shader = csResult.handle;

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("DeferredLight", *csResult.reflection, nvDevice);
    if (!state.layout) {
        Msg("! [DeferredLight] Failed to create binding layout");
        state.failed = true;
        return;
    }

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.shader;
    pipeDesc.bindingLayouts = { state.layout };
    auto* backend = device->GetBackend();
    if (backend && backend->GetBindlessLayout())
        pipeDesc.addBindingLayout(backend->GetBindlessLayout());

    state.pipeline = cache.GetOrCreateComputePipeline("DeferredLight", pipeDesc, nvDevice);
    if (!state.pipeline) {
        Msg("! [DeferredLight] Failed to create compute pipeline");
        state.failed = true;
        return;
    }

    state.initialized = true;
    Msg("* [DeferredLight] Pipeline initialized");
}

}

DefaultOutputLayout setupDeferredLightPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    SunShadowMaps sunShadowMaps,
    xray::profiler::GPUProfiler* gpuProfiler,
    DeferredLightPassState* state)
{
    if (!state || !inputs.albedo.is_valid() || !inputs.depth.is_valid() || !inputs.normal.is_valid() || !inputs.baseColor.is_valid())
        return inputs;

    EnsureDeferredLightPipeline(device, *state);
    if (!state->initialized)
        return inputs;

    auto& passData = fg.addCallbackPass<DeferredLightPassData>(
        "Deferred Light",
        [&, width, height, sunShadowMaps, state, gpuProfiler](FrameGraph& builder, PassHandle passHandle, DeferredLightPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.state = state;
            data.gpuProfiler = gpuProfiler;
            data.width = width;
            data.height = height;
            data.depth = passBuilder.read(inputs.depth, ResourceState::ShaderResource);
            data.normal = passBuilder.read(inputs.normal, ResourceState::ShaderResource);
            data.baseColor = passBuilder.read(inputs.baseColor, ResourceState::ShaderResource);
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::UnorderedAccess);
            ReadSunShadowMaps(passBuilder, sunShadowMaps, data.sunShadowMaps);
        },
        [](const DeferredLightPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ZoneScoped;
            ZoneName("DeferredLightPass", 17);

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = fg.GetPhysicalTexture(data.baseColor);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!depthRT || !normalRT || !baseColorRT || !colorRT || !cmdList)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("deferred_light", ".cs");
            if (!refl)
                return;

            nvrhi::ITexture* sunMaps[kSunMapSlots];
            ResolveSunShadowMaps(fg, data.sunShadowMaps, nvDevice, sunMaps);

            auto& clm = ClusteredLightManager::Instance();
            BindingSetBuilder bsb(*refl, nvDevice, "DeferredLight");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.Texture("g_GBufferDepth", depthRT);
            bsb.Texture("g_GBufferNormal", normalRT);
            bsb.Texture("g_GBufferBaseColor", baseColorRT);
            bsb.TextureUAV("g_SceneColor", colorRT);
            bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
            BindSunShadowMaps(bsb, sunMaps);

            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->layout, nvDevice);
            if (!bindingSet)
                return;

            if (data.gpuProfiler)
                data.gpuProfiler->BeginPass(cmdList, "Lighting.Deferred");

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->pipeline;
            cs.bindings = { bindingSet };
            if (auto* backend = data.device->GetBackend()) {
                if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                    cs.addBindingSet(bindlessTable);
            }
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);

            if (data.gpuProfiler)
                data.gpuProfiler->EndPass(cmdList, "Lighting.Deferred");
        });

    DefaultOutputLayout outputs = inputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    return outputs;
}

}
