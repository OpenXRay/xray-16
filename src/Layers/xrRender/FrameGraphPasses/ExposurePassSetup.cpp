#include "stdafx.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
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

static ref_cs s_histogram_cs;
static ref_cs s_adapt_cs;

static nvrhi::BufferHandle s_histogram_buffer;
static nvrhi::BufferHandle s_histogram_cb;
static nvrhi::BufferHandle s_adapt_cb;
static nvrhi::TextureHandle s_exposure_texture;

static nvrhi::ComputePipelineHandle s_histogram_pipeline;
static nvrhi::ComputePipelineHandle s_adapt_pipeline;

static nvrhi::BindingLayoutHandle s_histogram_layout;
static nvrhi::BindingLayoutHandle s_adapt_layout;

static bool s_initialized = false;
static bool s_compute_enabled = false;
static float s_current_exposure = 1.0f;

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

nvrhi::ITexture* GetExposureTexture()
{
    return s_exposure_texture.Get();
}

struct HistogramCB {
    float minLogLum;
    float logLumRange;
    u32 width;
    u32 height;
};

struct AdaptCB {
    float minLogLum;
    float logLumRange;
    float lowPercentile;
    float highPercentile;
    float adaptSpeedUp;
    float adaptSpeedDown;
    float deltaTime;
    float exposureCompensation;
    float minExposure;
    float maxExposure;
    float calibrationConstant;
    float padding;
};

static void InitializeExposureResources(ng::RenderDevice* device)
{
    if (s_initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [ExposurePass] NVRHI device not available");
        s_initialized = true;
        return;
    }

    s_histogram_cs.create("luminance_histogram");
    s_adapt_cs.create("exposure_adapt");

    bool histogramOK = s_histogram_cs && s_histogram_cs->nvrhiShader;
    bool adaptOK = s_adapt_cs && s_adapt_cs->nvrhiShader;

    if (histogramOK)
        Msg("* [ExposurePass] Loaded luminance_histogram compute shader: OK");
    else
        Msg("! [ExposurePass] luminance_histogram.cs not found - using fallback");

    if (adaptOK)
        Msg("* [ExposurePass] Loaded exposure_adapt compute shader: OK");
    else
        Msg("! [ExposurePass] exposure_adapt.cs not found - using fallback");

    s_compute_enabled = histogramOK && adaptOK;

    {
        nvrhi::BufferDesc bufDesc;
        bufDesc.debugName = "ExposureHistogram";
        bufDesc.byteSize = 64 * sizeof(u32);
        bufDesc.structStride = sizeof(u32);
        bufDesc.canHaveUAVs = true;
        bufDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        bufDesc.keepInitialState = true;

        s_histogram_buffer = nvDevice->createBuffer(bufDesc);
        if (!s_histogram_buffer)
            Msg("! [ExposurePass] Failed to create histogram buffer");
    }

    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "HistogramCB";
        cbDesc.byteSize = sizeof(HistogramCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;

        s_histogram_cb = nvDevice->createBuffer(cbDesc);
    }

    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "AdaptCB";
        cbDesc.byteSize = sizeof(AdaptCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;

        s_adapt_cb = nvDevice->createBuffer(cbDesc);
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

        s_exposure_texture = nvDevice->createTexture(texDesc);
        if (!s_exposure_texture)
            Msg("! [ExposurePass] Failed to create exposure texture");
    }

    if (s_compute_enabled) {
        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(5),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0)
            };
            s_histogram_layout = nvDevice->createBindingLayout(layoutDesc);

            if (s_histogram_layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = s_histogram_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { s_histogram_layout };
                s_histogram_pipeline = nvDevice->createComputePipeline(pipeDesc);
            }
        }

        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(5),
                nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
                nvrhi::BindingLayoutItem::Texture_UAV(0)
            };
            s_adapt_layout = nvDevice->createBindingLayout(layoutDesc);

            if (s_adapt_layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = s_adapt_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { s_adapt_layout };
                s_adapt_pipeline = nvDevice->createComputePipeline(pipeDesc);
            }
        }

        if (s_histogram_pipeline && s_adapt_pipeline)
            Msg("* [ExposurePass] Compute pipelines created successfully");
        else {
            Msg("! [ExposurePass] Failed to create compute pipelines - using fallback");
            s_compute_enabled = false;
        }
    }

    s_initialized = true;
    Msg("* [ExposurePass] Initialized (compute=%s)", s_compute_enabled ? "enabled" : "fallback");
}

static float ComputeFallbackExposure(const ExposureConfig& config, float deltaTime)
{
    float targetExposure = 1.0f;

    targetExposure *= std::exp2(config.exposureCompensation);

    targetExposure = std::clamp(targetExposure, config.minExposure, config.maxExposure);

    float adaptSpeed = (targetExposure > s_current_exposure)
        ? config.adaptSpeedUp
        : config.adaptSpeedDown;

    float adaptFactor = 1.0f - std::exp(-deltaTime * adaptSpeed);
    s_current_exposure = std::lerp(s_current_exposure, targetExposure, adaptFactor);

    return s_current_exposure;
}

ExposureOutput setupExposurePass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle hdrSceneColor,
    const ExposureConfig& config,
    float deltaTime,
    u32 width,
    u32 height)
{
    InitializeExposureResources(device);

    struct ExposurePassData {
        VirtualResourceHandle sceneColor;
        VirtualResourceHandle exposureTexture;
        VirtualResourceHandle histogramBuffer;

        ng::RenderDevice* device;
        ExposureConfig config;
        float deltaTime;
        u32 width;
        u32 height;
    };

    ResourceDesc exposureDesc;
    exposureDesc.type = ResourceDesc::Type::Texture2D;
    exposureDesc.debugName = "Exposure";
    exposureDesc.width = 1;
    exposureDesc.height = 1;
    exposureDesc.format = nvrhi::Format::R32_FLOAT;
    exposureDesc.isRenderTarget = false;
    exposureDesc.isUAV = true;

    VirtualResourceHandle exposureHandle = fg.CreateTexture("exposure_rt", exposureDesc);

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

            data.sceneColor = passBuilder.read(hdrSceneColor);

            data.exposureTexture = passBuilder.write(exposureHandle, ResourceState::UnorderedAccess);

            data.histogramBuffer = passBuilder.write(histogramHandle, ResourceState::UnorderedAccess);
        },

        [](const ExposurePassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Exposure Pass");

            if (s_compute_enabled && s_histogram_pipeline && s_adapt_pipeline) {
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);

                if (sceneTexture) {
                    ctx->ClearBufferUint(s_histogram_buffer.Get(), 0);

                    {
                        HistogramCB histCB;
                        histCB.minLogLum = data.config.minLogLuminance;
                        histCB.logLumRange = data.config.maxLogLuminance - data.config.minLogLuminance;
                        histCB.width = data.width;
                        histCB.height = data.height;

                        cmdList->writeBuffer(s_histogram_cb, &histCB, sizeof(histCB));

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, s_histogram_cb),
                            nvrhi::BindingSetItem::Texture_SRV(0, sceneTexture),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, s_histogram_buffer)
                        };
                        nvrhi::BindingSetHandle histBindings = nvDevice->createBindingSet(bindDesc, s_histogram_layout);

                        if (histBindings) {
                            ctx->SetComputePipeline(s_histogram_pipeline.Get());
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

                        cmdList->writeBuffer(s_adapt_cb, &adaptCB, sizeof(adaptCB));

                        nvrhi::BindingSetDesc bindDesc;
                        bindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, s_adapt_cb),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, s_histogram_buffer),
                            nvrhi::BindingSetItem::Texture_UAV(0, s_exposure_texture)
                        };
                        nvrhi::BindingSetHandle adaptBindings = nvDevice->createBindingSet(bindDesc, s_adapt_layout);

                        if (adaptBindings) {
                            ctx->SetComputePipeline(s_adapt_pipeline.Get());
                            ctx->SetComputeBindingSet(0, adaptBindings.Get());
                            ctx->Dispatch(1, 1, 1);
                        }
                    }
                } else {
                    float exposure = ComputeFallbackExposure(data.config, data.deltaTime);

                    if (s_exposure_texture) {
                        cmdList->writeTexture(s_exposure_texture, 0, 0, &exposure, sizeof(float));
                    }
                }
            } else {
                float exposure = ComputeFallbackExposure(data.config, data.deltaTime);

                if (s_exposure_texture) {
                    cmdList->writeTexture(s_exposure_texture, 0, 0, &exposure, sizeof(float));
                }
            }

            cmdList->endMarker();
        }
    );

    ExposureOutput output;
    output.exposureTexture = passData.exposureTexture;
    output.histogramBuffer = passData.histogramBuffer;
    return output;
}

}
