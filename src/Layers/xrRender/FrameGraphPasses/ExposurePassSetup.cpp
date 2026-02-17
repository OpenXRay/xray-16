// xrRender/FrameGraphPasses/ExposurePassSetup.cpp
#include "stdafx.h"
#include "ExposurePassSetup.h"
#include "PassVertexFormats.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11HW.h"
#endif

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

// ═══════════════════════════════════════════════════════
//  EXPOSURE CONFIG
// ═══════════════════════════════════════════════════════

ExposureConfig GetDefaultExposureConfig()
{
    ExposureConfig config;
    config.minLogLuminance = -10.0f;
    config.maxLogLuminance = 4.0f;
    config.lowPercentile = 0.5f;
    config.highPercentile = 0.98f;
    config.adaptSpeedUp = 3.0f;
    config.adaptSpeedDown = 1.0f;
    config.minExposure = 0.001f;
    config.maxExposure = 64.0f;
    config.exposureCompensation = 0.0f;
    config.calibrationConstant = 12.5f;
    return config;
}

nvrhi::ITexture* GetExposureTexture(const ExposurePassState& state)
{
    return state.exposureTexture.Get();
}

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

static void InitializeExposureResources(ng::RenderDevice* device, ExposurePassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [ExposurePass] NVRHI device not available");
        state.initialized = true;
        return;
    }

    ref_cs histogram_cs;
    ref_cs adapt_cs;
    histogram_cs.create("luminance_histogram");
    adapt_cs.create("exposure_adapt");

    bool histogramOK = histogram_cs && histogram_cs->nvrhiShader;
    bool adaptOK = adapt_cs && adapt_cs->nvrhiShader;

    if (histogramOK)
        Msg("* [ExposurePass] Loaded luminance_histogram compute shader: OK");
    else
        Msg("! [ExposurePass] luminance_histogram.cs not found - using fallback");

    if (adaptOK)
        Msg("* [ExposurePass] Loaded exposure_adapt compute shader: OK");
    else
        Msg("! [ExposurePass] exposure_adapt.cs not found - using fallback");

    state.computeEnabled = histogramOK && adaptOK;

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
    }

    if (state.computeEnabled) {
        auto& cache = framegraph::GetPassResourceCache();

        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0)
            };
            state.histogramLayout = cache.GetOrCreateBindingLayout("ExposurePass_Histogram", layoutDesc, nvDevice);

            if (state.histogramLayout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = histogram_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { state.histogramLayout };
                state.histogramPipeline = cache.GetOrCreateComputePipeline("ExposurePass_Histogram", pipeDesc, nvDevice);
            }
        }

        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
                nvrhi::BindingLayoutItem::Texture_UAV(0)
            };
            state.adaptLayout = cache.GetOrCreateBindingLayout("ExposurePass_Adapt", layoutDesc, nvDevice);

            if (state.adaptLayout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = adapt_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { state.adaptLayout };
                state.adaptPipeline = cache.GetOrCreateComputePipeline("ExposurePass_Adapt", pipeDesc, nvDevice);
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
    Msg("* [ExposurePass] Initialized (compute=%s)", state.computeEnabled ? "enabled" : "fallback");
}

// ═══════════════════════════════════════════════════════
//  FALLBACK: Fixed exposure calculation
// ═══════════════════════════════════════════════════════

static float ComputeFallbackExposure(const ExposureConfig& config, float deltaTime, ExposurePassState& state)
{
    float targetExposure = 1.0f;

    targetExposure *= std::exp2(config.exposureCompensation);

    targetExposure = std::clamp(targetExposure, config.minExposure, config.maxExposure);

    float adaptSpeed = (targetExposure > state.currentExposure)
        ? config.adaptSpeedUp
        : config.adaptSpeedDown;

    float adaptFactor = 1.0f - std::exp(-deltaTime * adaptSpeed);
    state.currentExposure = std::lerp(state.currentExposure, targetExposure, adaptFactor);

    return state.currentExposure;
}

// ═══════════════════════════════════════════════════════
//  SETUP EXPOSURE PASS
// ═══════════════════════════════════════════════════════

ExposureOutput setupExposurePass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle hdrSceneColor,
    const ExposureConfig& config,
    float deltaTime,
    u32 width,
    u32 height,
    ExposurePassState& state)
{
    InitializeExposureResources(device, state);

    // Create exposure texture resource in framegraph
    ResourceDesc exposureDesc;
    exposureDesc.type = ResourceDesc::Type::Texture2D;
    exposureDesc.debugName = "Exposure";
    exposureDesc.width = 1;
    exposureDesc.height = 1;
    exposureDesc.format = nvrhi::Format::R32_FLOAT;
    exposureDesc.isRenderTarget = false;
    exposureDesc.isUAV = true;

    VirtualResourceHandle exposureHandle = fg.CreateTexture("exposure_rt", exposureDesc);

    // Create histogram buffer resource
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

            // Read HDR scene for histogram
            data.sceneColor = passBuilder.read(hdrSceneColor);

            // Write exposure output
            data.exposureTexture = passBuilder.write(exposureHandle, ResourceState::UnorderedAccess);

            // Write histogram (intermediate)
            data.histogramBuffer = passBuilder.write(histogramHandle, ResourceState::UnorderedAccess);
        },

        [](const ExposurePassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* ps = data.passState;

            if (ps->computeEnabled && ps->histogramPipeline && ps->adaptPipeline) {
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);

                if (sceneTexture) {
                    auto& cache = framegraph::GetPassResourceCache();
                    auto histogramCB = cache.GetOrCreateVolatileCB("ExposurePass", "HistogramCB", sizeof(HistogramCB), 16, nvDevice);
                    auto adaptCBHandle = cache.GetOrCreateVolatileCB("ExposurePass", "AdaptCB", sizeof(AdaptCB), 16, nvDevice);

                    ctx->ClearBufferUint(ps->histogramBuffer.Get(), 0);

                    {
                        HistogramCB histCB;
                        histCB.minLogLum = data.config.minLogLuminance;
                        histCB.logLumRange = data.config.maxLogLuminance - data.config.minLogLuminance;
                        histCB.width = data.width;
                        histCB.height = data.height;

                        cmdList->writeBuffer(histogramCB, &histCB, sizeof(histCB));

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, histogramCB),
                            nvrhi::BindingSetItem::Texture_SRV(0, sceneTexture),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, ps->histogramBuffer)
                        };
                        nvrhi::BindingSetHandle histBindings = nvDevice->createBindingSet(bindDesc, ps->histogramLayout);

                        if (histBindings) {
                            ctx->SetComputePipeline(ps->histogramPipeline.Get());
                            ctx->SetComputeBindingSet(0, histBindings.Get());

                            u32 groupsX = (data.width + 15) / 16;
                            u32 groupsY = (data.height + 15) / 16;
                            ctx->Dispatch(groupsX, groupsY, 1);
                        }
                    }

                    {
                        AdaptCB adaptCB;
                        adaptCB.minLogLum = data.config.minLogLuminance;
                        adaptCB.logLumRange = data.config.maxLogLuminance - data.config.minLogLuminance;
                        adaptCB.lowPercentile = data.config.lowPercentile;
                        adaptCB.highPercentile = data.config.highPercentile;
                        adaptCB.adaptSpeedUp = data.config.adaptSpeedUp;
                        adaptCB.adaptSpeedDown = data.config.adaptSpeedDown;
                        adaptCB.deltaTime = data.deltaTime;
                        adaptCB.exposureCompensation = data.config.exposureCompensation;
                        adaptCB.minExposure = data.config.minExposure;
                        adaptCB.maxExposure = data.config.maxExposure;
                        adaptCB.calibrationConstant = data.config.calibrationConstant;
                        adaptCB.padding = 0.0f;

                        cmdList->writeBuffer(adaptCBHandle, &adaptCB, sizeof(adaptCB));

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, adaptCBHandle),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, ps->histogramBuffer),
                            nvrhi::BindingSetItem::Texture_UAV(0, ps->exposureTexture)
                        };
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

} // namespace xray::render::RENDER_NAMESPACE::passes
