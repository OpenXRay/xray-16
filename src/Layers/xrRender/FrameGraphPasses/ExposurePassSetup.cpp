#include "stdafx.h"
#include "ExposurePassSetup.h"
#include "PassVertexFormats.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/xrRender_console.h"
#include <cstring>

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {
constexpr u32 kExposurePipeVersion = 5;
}

ExposureConfig GetDefaultExposureConfig()
{
    ExposureConfig config;
    const bool tonemapOn = ps_r2_ls_flags.test(R2FLAG_TONEMAP);
    config.middleGray = ps_r2_tonemap_middlegray;
    config.amount = tonemapOn ? ps_r2_tonemap_amount : 0.0f;
    config.lowLum = ps_r2_tonemap_low_lum;
    config.adaptation = ps_r2_tonemap_adaptation;
    return config;
}

nvrhi::ITexture* GetExposureTexture(const ExposurePassState& state)
{
    return state.exposureTexture.Get();
}

void PollExposureHistogram(ExposurePassState& state, nvrhi::IDevice* device)
{
    if (!device)
        return;
    const u32 readSlot = (state.histWriteSlot + 1u) % 3u;
    if (!state.histReadback[readSlot])
        return;
    void* mapped = device->mapBuffer(state.histReadback[readSlot], nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;
    memcpy(state.histBins, mapped, sizeof(state.histBins));
    device->unmapBuffer(state.histReadback[readSlot]);
}

static void UpdateMiddleGray(const ExposureConfig& config, float deltaTime, ExposurePassState& state, AdaptCB& out)
{
    state.f_luminance_adapt =
        0.9f * state.f_luminance_adapt + 0.1f * deltaTime * config.adaptation;

    Fvector3 none, full, result;
    none.set(1.f, 0.f, 1.f);
    full.set(config.middleGray, 1.f, config.lowLum);
    result.lerp(none, full, config.amount);

    out.middleGrayX = result.x;
    out.middleGrayY = result.y;
    out.middleGrayZ = result.z;
    out.middleGrayW = state.f_luminance_adapt;
}

static float ComputeFallbackExposure(const ExposureConfig& config, float deltaTime, ExposurePassState& state)
{
    AdaptCB cb{};
    UpdateMiddleGray(config, deltaTime, state, cb);
    const float Lw = 1.0f;
    float scale = cb.middleGrayX / std::max(Lw * cb.middleGrayY + cb.middleGrayZ, 1e-6f);
    state.currentExposure = std::lerp(state.currentExposure, scale, std::clamp(cb.middleGrayW, 0.f, 1.f));
    state.currentExposure = std::clamp(state.currentExposure, 1.f / 128.f, 20.f);
    return state.currentExposure;
}

void InitializeExposureResources(fg::RenderDevice* device, ExposurePassState& state)
{
    if (state.initialized && state.pipeVersion == kExposurePipeVersion && state.adaptPipeline)
        return;

    state.initialized = false;
    state.pipeVersion = 0;
    state.histogramPipeline = nullptr;
    state.adaptPipeline = nullptr;
    state.histogramLayout = nullptr;
    state.adaptLayout = nullptr;
    state.computeEnabled = false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [ExposurePass] NVRHI device not available");
        state.initialized = true;
        return;
    }

    framegraph::BindingSetBuilder::InvalidateReflectionCache();
    auto histogramResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("luminance_histogram");
    auto adaptResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("exposure_adapt");

    bool histogramOK = histogramResult.handle != nullptr;
    bool adaptOK = adaptResult.handle != nullptr;

    if (histogramOK)
        Msg("* [ExposurePass] Loaded luminance_histogram compute shader: OK");
    else
        Msg("! [ExposurePass] luminance_histogram.cs not found - using fallback");

    if (adaptOK)
        Msg("* [ExposurePass] Loaded exposure_adapt compute shader: OK");
    else
        Msg("! [ExposurePass] exposure_adapt.cs not found - using fallback");

    state.computeEnabled = histogramOK && adaptOK;

    if (!state.histogramBuffer)
    {
        nvrhi::BufferDesc bufDesc;
        bufDesc.debugName = "ExposureHistogram";
        bufDesc.byteSize = 64 * sizeof(u32);
        bufDesc.structStride = sizeof(u32);
        bufDesc.canHaveUAVs = true;
        bufDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        bufDesc.keepInitialState = true;

        state.histogramBuffer = nvDevice->createBuffer(bufDesc);
        if (!state.histogramBuffer)
            Msg("! [ExposurePass] Failed to create histogram buffer");
    }

    for (u32 i = 0; i < 3; i++) {
        if (state.histReadback[i])
            continue;
        nvrhi::BufferDesc rd;
        rd.debugName = "ExposureHistogramReadback";
        rd.byteSize = 64 * sizeof(u32);
        rd.cpuAccess = nvrhi::CpuAccessMode::Read;
        state.histReadback[i] = nvDevice->createBuffer(rd);
    }

    if (!state.exposureTexture)
    {
        nvrhi::TextureDesc texDesc;
        texDesc.debugName = "ExposureValue";
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.format = nvrhi::Format::R32_FLOAT;
        texDesc.isUAV = true;
        texDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        texDesc.keepInitialState = true;

        state.exposureTexture = nvDevice->createTexture(texDesc);
        if (!state.exposureTexture)
            Msg("! [ExposurePass] Failed to create exposure texture");
        else
        {
            nvrhi::CommandListHandle cmd = nvDevice->createCommandList();
            if (cmd)
            {
                cmd->open();
                float seed = 1.0f;
                cmd->writeTexture(state.exposureTexture, 0, 0, &seed, sizeof(float));
                cmd->close();
                nvDevice->executeCommandList(cmd);
            }
            state.currentExposure = 1.0f;
        }
    }

    if (state.computeEnabled) {
        auto& cache = framegraph::GetPassResourceCache();

        {
            state.histogramLayout = cache.GetOrCreateBindingLayoutFromReflection("ExposurePass_Histogram", *histogramResult.reflection, nvDevice);

            if (state.histogramLayout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = histogramResult.handle;
                pipeDesc.bindingLayouts = { state.histogramLayout };
                state.histogramPipeline = cache.GetOrCreateComputePipeline("ExposurePass_Histogram", pipeDesc, nvDevice);
            }
        }

        {
            state.adaptLayout = cache.GetOrCreateBindingLayoutFromReflection("ExposurePass_Adapt_v3", *adaptResult.reflection, nvDevice);

            if (state.adaptLayout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = adaptResult.handle;
                pipeDesc.bindingLayouts = { state.adaptLayout };
                state.adaptPipeline = cache.GetOrCreateComputePipeline("ExposurePass_Adapt_v3", pipeDesc, nvDevice);
            }
        }

        if (state.histogramPipeline && state.adaptPipeline)
            Msg("* [ExposurePass] Compute pipelines created successfully");
        else {
            Msg("! [ExposurePass] Failed to create compute pipelines - using fallback");
            state.computeEnabled = false;
        }
    }

    state.initialized = true;
    state.pipeVersion = kExposurePipeVersion;
    Msg("* [ExposurePass] Initialized (compute=%s)", state.computeEnabled ? "enabled" : "fallback");
}

ExposureOutput setupExposurePass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle hdrSceneColor,
    const ExposureConfig& config,
    float deltaTime,
    u32 width,
    u32 height,
    ExposurePassState& state)
{
    InitializeExposureResources(device, state);

    if (!state.exposureTexture)
    {
        ExposureOutput empty{};
        return empty;
    }

    ResourceDesc exposureDesc;
    exposureDesc.type = ResourceDesc::Type::Texture2D;
    exposureDesc.debugName = "Exposure";
    exposureDesc.width = 1;
    exposureDesc.height = 1;
    exposureDesc.format = nvrhi::Format::R32_FLOAT;
    exposureDesc.isUAV = true;
    exposureDesc.allowUAV = true;
    exposureDesc.isImported = true;

    VirtualResourceHandle exposureHandle = fg.ImportTexture(
        "Exposure", state.exposureTexture.Get(), exposureDesc);

    ResourceDesc histogramDesc;
    histogramDesc.type = ResourceDesc::Type::Buffer;
    histogramDesc.debugName = "LuminanceHistogram";
    histogramDesc.bufferSize = 64 * sizeof(u32);
    histogramDesc.structStride = sizeof(u32);
    histogramDesc.isUAV = true;

    VirtualResourceHandle histogramHandle = fg.CreateBuffer("luminance_rt", histogramDesc);

    auto& passData = fg.addCallbackPass<ExposurePassData>(
        "Exposure",

        [&, width, height, deltaTime, config](FrameGraph& builder, PassHandle passHandle, ExposurePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.config = config;
            data.deltaTime = deltaTime;
            data.width = width;
            data.height = height;
            data.passState = &state;

            data.sceneColor = passBuilder.read(hdrSceneColor);
            data.exposureTexture = passBuilder.write(exposureHandle, ResourceState::UnorderedAccess);
            data.histogramBuffer = passBuilder.write(histogramHandle, ResourceState::UnorderedAccess);
        },

        [](const ExposurePassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* ps = data.passState;

            if (ps->computeEnabled && ps->histogramPipeline && ps->adaptPipeline) {
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);

                if (sceneTexture) {
                    auto& cache = framegraph::GetPassResourceCache();
                    auto histogramCB = cache.GetOrCreateVolatileCB("ExposurePass", "HistogramCB", sizeof(HistogramCB), data.device);
                    auto adaptCBHandle = cache.GetOrCreateVolatileCB("ExposurePass", "AdaptCB", sizeof(AdaptCB), data.device);

                    ctx->ClearBufferUint(ps->histogramBuffer.Get(), 0);

                    {
                        HistogramCB histCB;
                        histCB.minLogLum = -10.0f;
                        histCB.logLumRange = 14.0f;
                        histCB.width = data.width;
                        histCB.height = data.height;

                        cmdList->writeBuffer(histogramCB, &histCB, sizeof(histCB));

                        auto* histRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("luminance_histogram", ".cs");
                        BindingSetBuilder bsb(*histRefl, nvDevice, "Exposure.Histogram");
                        bsb.ConstantBuffer("ExposureParams", histogramCB)
                           .Texture("g_scene_color", sceneTexture)
                           .BufferUAV("g_histogram", ps->histogramBuffer);
                        nvrhi::BindingSetHandle histBindings = cache.GetOrCreateBindingSet(bsb.Build(), ps->histogramLayout, nvDevice);

                        if (histBindings) {
                            ctx->SetComputePipeline(ps->histogramPipeline.Get());
                            ctx->SetComputeBindingSet(0, histBindings.Get());

                            u32 groupsX = (data.width + 15) / 16;
                            u32 groupsY = (data.height + 15) / 16;
                            ctx->Dispatch(groupsX, groupsY, 1);
                            if (ps->histReadback[ps->histWriteSlot]) {
                                cmdList->copyBuffer(ps->histReadback[ps->histWriteSlot], 0, ps->histogramBuffer, 0, 64 * sizeof(u32));
                                ps->histWriteSlot = (ps->histWriteSlot + 1u) % 3u;
                            }
                        }
                    }

                    {
                        AdaptCB adaptCB{};
                        UpdateMiddleGray(data.config, data.deltaTime, *ps, adaptCB);

                        cmdList->writeBuffer(adaptCBHandle, &adaptCB, sizeof(adaptCB));

                        auto* adaptRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("exposure_adapt", ".cs");
                        BindingSetBuilder bsb(*adaptRefl, nvDevice, "Exposure.Adapt");
                        bsb.ConstantBuffer("ExposureAdaptParams", adaptCBHandle)
                           .BufferSRV("g_histogram", ps->histogramBuffer)
                           .TextureUAV("g_exposure", ps->exposureTexture);
                        auto bindDesc = bsb.Build();
                        nvrhi::BindingSetHandle adaptBindings = cache.GetOrCreateBindingSet(bindDesc, ps->adaptLayout, nvDevice);

                        if (adaptBindings) {
                            ctx->SetComputePipeline(ps->adaptPipeline.Get());
                            ctx->SetComputeBindingSet(0, adaptBindings.Get());
                            ctx->Dispatch(1, 1, 1);
                        }
                    }
                } else {
                    float exposure = ComputeFallbackExposure(data.config, data.deltaTime, *ps);

                    if (ps->exposureTexture) {
                        cmdList->writeTexture(ps->exposureTexture, 0, 0, &exposure, sizeof(float));
                    }
                }
            } else {
                float exposure = ComputeFallbackExposure(data.config, data.deltaTime, *ps);

                if (ps->exposureTexture) {
                    cmdList->writeTexture(ps->exposureTexture, 0, 0, &exposure, sizeof(float));
                }
            }
        }
    );

    ExposureOutput output;
    output.exposureTexture = passData.exposureTexture;
    output.histogramBuffer = passData.histogramBuffer;
    return output;
}

} // namespace xray::render::fg::passes
