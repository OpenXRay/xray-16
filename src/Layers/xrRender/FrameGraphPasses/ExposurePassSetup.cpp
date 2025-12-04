// xrRender/FrameGraphPasses/ExposurePassSetup.cpp
#include "stdafx.h"
#include "ExposurePassSetup.h"
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
//  STATIC STATE
// ═══════════════════════════════════════════════════════

// Compute shaders (X-Ray format)
static ref_cs s_histogram_cs;          // luminance_histogram.cs
static ref_cs s_adapt_cs;              // exposure_adapt.cs

// NVRHI Resources
static nvrhi::BufferHandle s_histogram_buffer;      // 64 u32 bins (structured buffer)
static nvrhi::BufferHandle s_histogram_cb;          // Constant buffer for histogram params
static nvrhi::BufferHandle s_adapt_cb;              // Constant buffer for adapt params
static nvrhi::TextureHandle s_exposure_texture;     // 1x1 R32_FLOAT output (UAV)

// Compute pipelines
static nvrhi::ComputePipelineHandle s_histogram_pipeline;
static nvrhi::ComputePipelineHandle s_adapt_pipeline;

// Binding layouts
static nvrhi::BindingLayoutHandle s_histogram_layout;
static nvrhi::BindingLayoutHandle s_adapt_layout;

static bool s_initialized = false;
static bool s_compute_enabled = false;  // True if compute shaders loaded OK
static float s_current_exposure = 1.0f; // Adapted exposure value (fallback)

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

// Get the physical exposure texture directly
nvrhi::ITexture* GetExposureTexture()
{
    return s_exposure_texture.Get();
}

// ═══════════════════════════════════════════════════════
//  CONSTANT BUFFER STRUCTURES (must match HLSL)
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

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

    // ─────────────────────────────────────────────────────
    // Load compute shaders
    // ─────────────────────────────────────────────────────
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

    // ─────────────────────────────────────────────────────
    // Create NVRHI buffers
    // ─────────────────────────────────────────────────────

    // Histogram buffer: 64 u32 bins (StructuredBuffer + UAV)
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

    // Histogram constant buffer
    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "HistogramCB";
        cbDesc.byteSize = sizeof(HistogramCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;
        cbDesc.keepInitialState = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        s_histogram_cb = nvDevice->createBuffer(cbDesc);
    }

    // Adapt constant buffer
    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "AdaptCB";
        cbDesc.byteSize = sizeof(AdaptCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;
        cbDesc.keepInitialState = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        s_adapt_cb = nvDevice->createBuffer(cbDesc);
    }

    // Exposure texture: 1x1 R32_FLOAT (UAV read/write)
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

    // ─────────────────────────────────────────────────────
    // Create binding layouts and pipelines (cached)
    // ─────────────────────────────────────────────────────
    if (s_compute_enabled) {
        auto& cache = framegraph::GetPassResourceCache();

        // Histogram binding layout:
        // b5: ConstantBuffer (HistogramCB) - b5 to avoid conflicts with common.h
        // t0: Texture2D (scene color SRV)
        // u0: RWStructuredBuffer (histogram UAV)
        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(5),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0)  // Structured buffer, not typed
            };
            s_histogram_layout = cache.GetOrCreateBindingLayout("ExposurePass_Histogram", layoutDesc, nvDevice);

            if (s_histogram_layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = s_histogram_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { s_histogram_layout };
                s_histogram_pipeline = cache.GetOrCreateComputePipeline("ExposurePass_Histogram", pipeDesc, nvDevice);
            }
        }

        // Adapt binding layout:
        // b5: ConstantBuffer (AdaptCB) - b5 to avoid conflicts with common.h
        // t0: StructuredBuffer (histogram SRV)
        // u0: RWTexture2D (exposure UAV)
        {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(5),
                nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
                nvrhi::BindingLayoutItem::Texture_UAV(0)
            };
            s_adapt_layout = cache.GetOrCreateBindingLayout("ExposurePass_Adapt", layoutDesc, nvDevice);

            if (s_adapt_layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = s_adapt_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { s_adapt_layout };
                s_adapt_pipeline = cache.GetOrCreateComputePipeline("ExposurePass_Adapt", pipeDesc, nvDevice);
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

// ═══════════════════════════════════════════════════════
//  FALLBACK: Fixed exposure calculation
// ═══════════════════════════════════════════════════════

static float ComputeFallbackExposure(const ExposureConfig& config, float deltaTime)
{
    // Simple temporal adaptation toward a fixed target
    float targetExposure = 1.0f;

    // Apply exposure compensation
    targetExposure *= std::exp2(config.exposureCompensation);

    // Clamp
    targetExposure = std::clamp(targetExposure, config.minExposure, config.maxExposure);

    // Temporal adaptation
    float adaptSpeed = (targetExposure > s_current_exposure)
        ? config.adaptSpeedUp
        : config.adaptSpeedDown;

    float adaptFactor = 1.0f - std::exp(-deltaTime * adaptSpeed);
    s_current_exposure = std::lerp(s_current_exposure, targetExposure, adaptFactor);

    return s_current_exposure;
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
    u32 height)
{
    // Initialize resources on first call
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

        // Setup lambda
        [&, width, height, deltaTime, config](FrameGraph& builder, PassHandle passHandle, ExposurePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.config = config;
            data.deltaTime = deltaTime;
            data.width = width;
            data.height = height;

            // Read HDR scene for histogram
            data.sceneColor = passBuilder.read(hdrSceneColor);

            // Write exposure output
            data.exposureTexture = passBuilder.write(exposureHandle, ResourceState::UnorderedAccess);

            // Write histogram (intermediate)
            data.histogramBuffer = passBuilder.write(histogramHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const ExposurePassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Exposure Pass");

            if (s_compute_enabled && s_histogram_pipeline && s_adapt_pipeline) {
                // ─────────────────────────────────────────────────────
                // NVRHI Compute Path
                // ─────────────────────────────────────────────────────
                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

                // Get scene texture from framegraph
                nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);

                if (sceneTexture) {
                    // Pass 1: Clear histogram
                    ctx->ClearBufferUint(s_histogram_buffer.Get(), 0);

                    // Pass 2: Build histogram from scene
                    {
                        HistogramCB histCB;
                        histCB.minLogLum = data.config.minLogLuminance;
                        histCB.logLumRange = data.config.maxLogLuminance - data.config.minLogLuminance;
                        histCB.width = data.width;
                        histCB.height = data.height;

                        cmdList->writeBuffer(s_histogram_cb, &histCB, sizeof(histCB));

                        // Create binding set for histogram pass
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

                    // Pass 3: Compute adapted exposure
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

                        // Create binding set for adapt pass
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
                    // Scene texture not available - use fallback
                    float exposure = ComputeFallbackExposure(data.config, data.deltaTime);

                    // Write to static exposure texture (used directly by tonemap pass)
                    if (s_exposure_texture) {
                        cmdList->writeTexture(s_exposure_texture, 0, 0, &exposure, sizeof(float));
                    }
                }
            } else {
                // ─────────────────────────────────────────────────────
                // Fallback Path (no compute shaders)
                // ─────────────────────────────────────────────────────
                float exposure = ComputeFallbackExposure(data.config, data.deltaTime);

                // Write to static exposure texture (used directly by tonemap pass)
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

} // namespace xray::render::RENDER_NAMESPACE::passes
