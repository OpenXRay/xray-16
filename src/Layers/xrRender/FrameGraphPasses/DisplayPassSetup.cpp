#include "stdafx.h"
#include "DisplayPassSetup.h"
#include "BloomPassSetup.h"
#include "Layers/xrRender/DisplayCalibration.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/PostProcessEffects.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
constexpr u32 kExposureHistogramBins = 256;
constexpr u32 kExposureHistogramGroupSize = 16;
constexpr u32 kExposureHistogramStride = 2;
constexpr float kExposureMiddleGrayLog2 = -2.4739312f;
constexpr float kExposureMaxDeltaTime = 0.25f;

static_assert(sizeof(ExposureHistogramConstants) == 16, "ExposureHistogramParams layout");
static_assert(sizeof(ExposureAdaptConstants) == 32, "ExposureAdaptParams layout");
static_assert(sizeof(DisplayOutputConstants) == 144, "DisplayOutputParams layout");

constexpr float kMinSdrWhiteLevel = 0.1f;
constexpr float kMaxDisplayPeak = 64.0f;
constexpr float kMinPaperWhite = 0.25f;
constexpr float kMaxPaperWhite = 4.0f;

void ApplyDisplayOutput(const IRenderBackend::DisplayOutput& displayOutput, nvrhi::Format outputFormat,
    DisplayOutputConstants& constants)
{
    if (!displayOutput.hdr || nvrhi::getFormatInfo(outputFormat).kind != nvrhi::FormatKind::Float)
        return;

    const float paperWhite = std::clamp(ps_r_hdr_paper_white, kMinPaperWhite, kMaxPaperWhite);
    float peak = displayOutput.headroom / paperWhite;
    if (ps_r_hdr_peak > 0.0f)
        peak = std::min(peak, ps_r_hdr_peak);

    constants.outputHdr = 1u;
    constants.sdrWhiteLevel = std::max(displayOutput.sdrWhiteLevel * paperWhite, kMinSdrWhiteLevel);
    constants.displayPeak = std::clamp(peak, 1.0f, kMaxDisplayPeak);
}

void ApplyPostProcess(const PostProcessFrame& postProcess, DisplayOutputConstants& constants)
{
    if (!postProcess.active)
        return;

    constants.effectsEnabled = 1u;
    constants.blurShift.set(
        0.5f * postProcess.blur * constants.invSceneSize.x,
        0.5f * postProcess.blur * constants.invSceneSize.y);
    constants.dualityShift = postProcess.dualityShift;
    constants.noiseOffset = postProcess.noiseOffset;
    constants.noiseScale = postProcess.noiseScale;
    constants.gray = postProcess.gray;
    constants.noiseIntensity = postProcess.noiseIntensity;
    constants.colorGray = postProcess.colorGray;
    constants.colorMapInfluence = postProcess.colorMapInfluence;
    constants.colorBase = postProcess.colorBase;
    constants.colorMapInterpolate = postProcess.colorMapInterpolate;
    constants.colorAdd = postProcess.colorAdd;
}

VirtualResourceHandle ImportDisplayBuffer(FrameGraph& fg, const char* name, nvrhi::IBuffer* buffer)
{
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Buffer;
    desc.bufferSize = buffer->getDesc().byteSize;
    desc.structStride = buffer->getDesc().structStride;
    desc.isUAV = buffer->getDesc().canHaveUAVs;
    desc.allowUAV = desc.isUAV;
    desc.isImported = true;
    desc.isTransient = false;
    desc.debugName = name;
    return fg.ImportBuffer(name, buffer, desc);
}

bool EnsureExposureBuffers(fg::RenderDevice* device, DisplayPassState& state)
{
    if (state.histogram && state.exposure)
        return true;
    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    if (!nvDevice)
        return false;

    nvrhi::BufferDesc histogramDesc;
    histogramDesc.byteSize = kExposureHistogramBins * sizeof(u32);
    histogramDesc.canHaveUAVs = true;
    histogramDesc.canHaveRawViews = true;
    histogramDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    histogramDesc.keepInitialState = true;
    histogramDesc.debugName = "ExposureHistogram";

    nvrhi::BufferDesc exposureDesc;
    exposureDesc.byteSize = sizeof(Fvector4);
    exposureDesc.structStride = sizeof(Fvector4);
    exposureDesc.canHaveUAVs = true;
    exposureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    exposureDesc.keepInitialState = true;
    exposureDesc.debugName = "ExposureState";

    state.histogram = nvDevice->createBuffer(histogramDesc);
    state.exposure = nvDevice->createBuffer(exposureDesc);
    state.exposureInitialized = false;
    state.meteredPreviousFrame = false;
    if (state.histogram && state.exposure)
        return true;

    Msg("! [Display] exposure buffers could not be created");
    state.histogram = nullptr;
    state.exposure = nullptr;
    return false;
}

bool EnsureExposurePipelines(fg::RenderDevice* device, DisplayPassState& state)
{
    if (state.histogramPipeline && state.adaptPipeline)
        return true;
    if (state.exposurePipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !loader)
        return false;

    auto histogramShader = loader->LoadComputeShader("exposure_histogram");
    auto adaptShader = loader->LoadComputeShader("exposure_adapt");
    if (!histogramShader.handle || !histogramShader.reflection || !adaptShader.handle || !adaptShader.reflection)
    {
        Msg("! [Display] exposure shaders unavailable, exposure stays at its last value");
        state.exposurePipelinesFailed = true;
        return false;
    }

    auto& cache = GetPassResourceCache();
    state.histogramLayout = cache.GetOrCreateBindingLayoutFromReflection("ExposureHistogram", *histogramShader.reflection, nvDevice);
    state.adaptLayout = cache.GetOrCreateBindingLayoutFromReflection("ExposureAdapt", *adaptShader.reflection, nvDevice);
    if (state.histogramLayout && state.adaptLayout)
    {
        nvrhi::ComputePipelineDesc histogramDesc;
        histogramDesc.CS = histogramShader.handle;
        histogramDesc.bindingLayouts = { state.histogramLayout };
        state.histogramPipeline = cache.GetOrCreateComputePipeline("ExposureHistogram", histogramDesc, nvDevice);

        nvrhi::ComputePipelineDesc adaptDesc;
        adaptDesc.CS = adaptShader.handle;
        adaptDesc.bindingLayouts = { state.adaptLayout };
        state.adaptPipeline = cache.GetOrCreateComputePipeline("ExposureAdapt", adaptDesc, nvDevice);
    }

    if (state.histogramPipeline && state.adaptPipeline)
        return true;

    Msg("! [Display] exposure pipelines could not be created, exposure stays at its last value");
    state.exposurePipelinesFailed = true;
    return false;
}

bool EnsureOutputPipeline(fg::RenderDevice* device, nvrhi::Format outputFormat, DisplayPassState& state)
{
    if (state.outputPipeline && state.outputFormat == outputFormat)
        return true;
    if (state.outputPipelineFailed && state.outputFormat == outputFormat)
        return false;

    state.outputPipeline = nullptr;
    state.outputFormat = outputFormat;
    state.outputPipelineFailed = true;

    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !loader)
        return false;

    auto vertexShader = loader->LoadVertexShader("fullscreen");
    auto pixelShader = loader->LoadPixelShader("display_output");
    if (!vertexShader.handle || !vertexShader.reflection || !pixelShader.handle || !pixelShader.reflection)
    {
        Msg("! [Display] display output shaders unavailable, the frame cannot be presented");
        return false;
    }

    auto& cache = GetPassResourceCache();
    state.outputLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "DisplayOutput", *vertexShader.reflection, *pixelShader.reflection, nvDevice);
    if (!state.outputLayout)
        return false;

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.setVertexShader(vertexShader.handle);
    pipelineDesc.setPixelShader(pixelShader.handle);
    pipelineDesc.addBindingLayout(state.outputLayout);
    pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    pipelineDesc.renderState.blendState.targets[0].setBlendEnable(false);
    pipelineDesc.renderState.depthStencilState.setDepthTestEnable(false);
    pipelineDesc.renderState.depthStencilState.setDepthWriteEnable(false);
    pipelineDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::FramebufferInfoEx framebufferInfo;
    framebufferInfo.addColorFormat(outputFormat);
    state.outputPipeline = cache.GetOrCreatePipeline("DisplayOutput", pipelineDesc, framebufferInfo, nvDevice);
    if (!state.outputPipeline)
    {
        Msg("! [Display] display output pipeline could not be created, the frame cannot be presented");
        return false;
    }

    state.outputPipelineFailed = false;
    return true;
}
}

VirtualResourceHandle setupLightingFailurePass(
    FrameGraph& fg,
    VirtualResourceHandle sceneColor,
    LightingFrameState& lighting)
{
    if (!lighting.opaqueScheduled || lighting.scheduled == LightingMode::Raster || !sceneColor.is_valid())
        return sceneColor;

    auto& passData = fg.addCallbackPass<LightingFailurePassData>(
        "LightingFailure",
        [sceneColor, &lighting](FrameGraph& builder, PassHandle passHandle, LightingFailurePassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.sceneColor = passBuilder.readWrite(sceneColor, ResourceState::RenderTarget);
            passBuilder.sideEffects();
            data.lighting = &lighting;
        },
        [](const LightingFailurePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            auto* lightingState = data.lighting;
            if (!lightingState || (lightingState->recorded && !lightingState->frameFailed))
                return;

            if (!lightingState->frameFailed)
                lightingState->Fail(LightingFallback::RecordingUnavailable);

            Msg("! [LightingFailure] ray traced lighting recorded no world color (%s); clearing the world to black",
                LightingFallbackName(lightingState->fallback));

            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            auto* sceneTarget = fg.GetPhysicalTexture(data.sceneColor);
            if (!cmdList || !sceneTarget)
                return;

            cmdList->clearTextureFloat(sceneTarget, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            lightingState->failureCleared = true;
        }
    );

    return passData.sceneColor;
}

VirtualResourceHandle setupInterfaceLayer(FrameGraph& fg, u32 width, u32 height)
{
    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.isRenderTarget = true;
    desc.isTransient = true;
    desc.debugName = "rt_Interface";
    const VirtualResourceHandle layer = fg.CreateTexture("rt_Interface", desc);

    auto& passData = fg.addCallbackPass<InterfaceLayerPassData>(
        "Interface Clear",
        [layer](FrameGraph& builder, PassHandle passHandle, InterfaceLayerPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.layer = passBuilder.write(layer, ResourceState::RenderTarget);
        },
        [](const InterfaceLayerPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            auto* texture = fg.GetPhysicalTexture(data.layer);
            if (texture)
                ctx->GetCommandList()->clearTextureFloat(texture, nvrhi::AllSubresources, nvrhi::Color(0.0f));
        }
    );

    return passData.layer;
}

VirtualResourceHandle setupExposurePasses(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    u32 width,
    u32 height,
    bool meterScene,
    DisplayPassState& state)
{
    if (!EnsureExposureBuffers(device, state))
        return VirtualResourceHandle();

    VirtualResourceHandle histogram = ImportDisplayBuffer(fg, "ExposureHistogram", state.histogram);
    VirtualResourceHandle exposure = ImportDisplayBuffer(fg, "ExposureState", state.exposure);

    if (!state.exposureInitialized)
    {
        auto& initData = fg.addCallbackPass<ExposureInitPassData>(
            "Exposure Init",
            [histogram, exposure, &state](FrameGraph& builder, PassHandle passHandle, ExposureInitPassData& data)
            {
                RenderPassBuilder passBuilder(builder, passHandle);
                data.histogram = passBuilder.readWrite(histogram, ResourceState::UnorderedAccess);
                data.exposure = passBuilder.readWrite(exposure, ResourceState::CopyDest);
                passBuilder.sideEffects();
                data.state = &state;
            },
            [](const ExposureInitPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
            {
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                nvrhi::IBuffer* histogramBuffer = fg.GetPhysicalBuffer(data.histogram);
                nvrhi::IBuffer* exposureBuffer = fg.GetPhysicalBuffer(data.exposure);
                if (!histogramBuffer || !exposureBuffer)
                    return;
                const Fvector4 unitExposure = { 1.0f, 0.0f, 0.0f, kExposureMiddleGrayLog2 };
                cmdList->clearBufferUInt(histogramBuffer, 0u);
                cmdList->writeBuffer(exposureBuffer, &unitExposure, sizeof(unitExposure));
                data.state->exposureInitialized = true;
            }
        );
        histogram = initData.histogram;
        exposure = initData.exposure;
    }

    const bool meter = meterScene && sceneColor.is_valid() && EnsureExposurePipelines(device, state);
    const bool reset = !state.meteredPreviousFrame;
    state.meteredPreviousFrame = meter;
    if (!meter)
        return exposure;

    const bool autoExposure = ps_r_exposure_auto != 0;
    if (autoExposure)
    {
        auto& histogramData = fg.addCallbackPass<ExposureHistogramPassData>(
            "Exposure Histogram",
            [sceneColor, histogram, device, width, height, &state](FrameGraph& builder, PassHandle passHandle, ExposureHistogramPassData& data)
            {
                RenderPassBuilder passBuilder(builder, passHandle);
                data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
                data.histogram = passBuilder.readWrite(histogram, ResourceState::UnorderedAccess);
                data.device = device;
                data.state = &state;
                data.width = width;
                data.height = height;
            },
            [](const ExposureHistogramPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
            {
                nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);
                nvrhi::IBuffer* histogramBuffer = fg.GetPhysicalBuffer(data.histogram);
                auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("exposure_histogram", ".cs");
                if (!sceneTexture || !histogramBuffer || !reflection)
                    return;

                nvrhi::ICommandList* cmdList = ctx->GetCommandList();
                nvrhi::IDevice* nvDevice = cmdList->getDevice();
                auto& cache = GetPassResourceCache();
                nvrhi::IBuffer* constants = cache.GetOrCreateVolatileCB(
                    "ExposureHistogram", "ExposureHistogramParams", sizeof(ExposureHistogramConstants), data.device);
                if (!constants)
                    return;

                ExposureHistogramConstants cb;
                cb.sceneWidth = data.width;
                cb.sceneHeight = data.height;
                cmdList->writeBuffer(constants, &cb, sizeof(cb));

                BindingSetBuilder bindings(*reflection, nvDevice, "ExposureHistogram");
                bindings.Texture("t_Scene", sceneTexture)
                    .BufferUAV("u_Histogram", histogramBuffer)
                    .ConstantBuffer("ExposureHistogramParams", constants);
                nvrhi::BindingSetHandle bindingSet = cache.GetOrCreateBindingSet(bindings.Build(), data.state->histogramLayout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::ComputeState computeState;
                computeState.pipeline = data.state->histogramPipeline;
                computeState.bindings = { bindingSet };
                cmdList->setComputeState(computeState);

                const u32 sampleWidth = (data.width + kExposureHistogramStride - 1) / kExposureHistogramStride;
                const u32 sampleHeight = (data.height + kExposureHistogramStride - 1) / kExposureHistogramStride;
                cmdList->dispatch(
                    (sampleWidth + kExposureHistogramGroupSize - 1) / kExposureHistogramGroupSize,
                    (sampleHeight + kExposureHistogramGroupSize - 1) / kExposureHistogramGroupSize,
                    1);
            }
        );
        histogram = histogramData.histogram;
    }

    ExposureAdaptConstants adapt;
    adapt.compensation = ps_r_exposure_compensation;
    adapt.minEv = std::min(ps_r_exposure_min_ev, ps_r_exposure_max_ev);
    adapt.maxEv = std::max(ps_r_exposure_min_ev, ps_r_exposure_max_ev);
    adapt.rateUp = ps_r_exposure_speed_up;
    adapt.rateDown = ps_r_exposure_speed_down;
    adapt.deltaTime = std::clamp(Device.fTimeDeltaReal, 0.0f, kExposureMaxDeltaTime);
    adapt.autoExposure = autoExposure ? 1u : 0u;
    adapt.reset = reset ? 1u : 0u;

    auto& adaptData = fg.addCallbackPass<ExposureAdaptPassData>(
        "Exposure Adapt",
        [histogram, exposure, device, adapt, &state](FrameGraph& builder, PassHandle passHandle, ExposureAdaptPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.histogram = passBuilder.readWrite(histogram, ResourceState::UnorderedAccess);
            data.exposure = passBuilder.readWrite(exposure, ResourceState::UnorderedAccess);
            passBuilder.sideEffects();
            data.device = device;
            data.state = &state;
            data.constants = adapt;
        },
        [](const ExposureAdaptPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::IBuffer* histogramBuffer = fg.GetPhysicalBuffer(data.histogram);
            nvrhi::IBuffer* exposureBuffer = fg.GetPhysicalBuffer(data.exposure);
            auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("exposure_adapt", ".cs");
            if (!histogramBuffer || !exposureBuffer || !reflection)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();
            nvrhi::IBuffer* constants = cache.GetOrCreateVolatileCB(
                "ExposureAdapt", "ExposureAdaptParams", sizeof(ExposureAdaptConstants), data.device);
            if (!constants)
                return;
            cmdList->writeBuffer(constants, &data.constants, sizeof(data.constants));

            BindingSetBuilder bindings(*reflection, nvDevice, "ExposureAdapt");
            bindings.BufferUAV("u_Histogram", histogramBuffer)
                .BufferUAV("u_Exposure", exposureBuffer)
                .ConstantBuffer("ExposureAdaptParams", constants);
            nvrhi::BindingSetHandle bindingSet = cache.GetOrCreateBindingSet(bindings.Build(), data.state->adaptLayout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::ComputeState computeState;
            computeState.pipeline = data.state->adaptPipeline;
            computeState.bindings = { bindingSet };
            cmdList->setComputeState(computeState);
            cmdList->dispatch(1, 1, 1);
        }
    );

    return adaptData.exposure;
}

VirtualResourceHandle setupDisplayOutputPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle exposure,
    const BloomOutput& bloom,
    VirtualResourceHandle interfaceLayer,
    VirtualResourceHandle output,
    u32 width,
    u32 height,
    const DisplayCalibration& calibration,
    const PostProcessFrame& postProcess,
    const IRenderBackend::DisplayOutput& displayOutput,
    DisplayPassState& state,
    const LightingFrameState* lighting)
{
    VERIFY(interfaceLayer.is_valid());
    VERIFY(output.is_valid());

    const nvrhi::Format outputFormat = fg.GetResourceDesc(output).format;
    EnsureOutputPipeline(device, outputFormat, state);

    DisplayOutputConstants constants;
    constants.tonemapper = ps_r_tonemap;
    constants.gamma = calibration.GetGamma();
    constants.brightness = calibration.GetBrightness();
    constants.contrast = calibration.GetContrast();
    constants.invSceneSize.set(1.0f / float(std::max(width, 1u)), 1.0f / float(std::max(height, 1u)));
    const bool bloomValid = bloom.texture.is_valid() && bloom.levels > 0;
    if (bloomValid)
    {
        constants.bloomIntensity = std::clamp(ps_r_bloom_intensity, 0.0f, 1.0f);
        constants.bloomNormalization = 1.0f / float(bloom.levels);
    }
    ApplyPostProcess(postProcess, constants);
    ApplyDisplayOutput(displayOutput, outputFormat, constants);

    const VirtualResourceHandle bloomTexture = bloomValid ? bloom.texture : VirtualResourceHandle();
    auto& passData = fg.addCallbackPass<DisplayOutputPassData>(
        "Display Output",
        [sceneColor, exposure, bloomTexture, interfaceLayer, output, device, width, height, constants, &postProcess, &state, lighting](
            FrameGraph& builder, PassHandle passHandle, DisplayOutputPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            if (sceneColor.is_valid() && exposure.is_valid())
            {
                data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
                data.exposure = passBuilder.read(exposure, ResourceState::ShaderResource);
                if (bloomTexture.is_valid())
                    data.bloom = passBuilder.read(bloomTexture, ResourceState::ShaderResource);
            }
            data.interfaceLayer = passBuilder.read(interfaceLayer, ResourceState::ShaderResource);
            data.output = passBuilder.write(output, ResourceState::RenderTarget);
            if (constants.effectsEnabled != 0u)
            {
                data.noise = postProcess.noise;
                data.colorMap0 = postProcess.colorMap0;
                data.colorMap1 = postProcess.colorMap1;
            }
            data.device = device;
            data.state = &state;
            data.lighting = lighting;
            data.constants = constants;
            data.width = width;
            data.height = height;
        },
        [](const DisplayOutputPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            DisplayPassState& state = *data.state;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(data.output);
            nvrhi::ITexture* interfaceTexture = fg.GetPhysicalTexture(data.interfaceLayer);
            if (!outputTexture || !interfaceTexture)
                return;
            if (!state.outputPipeline || !state.outputLayout || !state.exposure)
            {
                cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
                return;
            }

            nvrhi::ITexture* sceneTexture = data.sceneColor.is_valid() ? fg.GetPhysicalTexture(data.sceneColor) : nullptr;
            nvrhi::ITexture* bloomTexture = data.bloom.is_valid() ? fg.GetPhysicalTexture(data.bloom) : nullptr;
            nvrhi::IBuffer* exposureBuffer = data.exposure.is_valid() ? fg.GetPhysicalBuffer(data.exposure) : state.exposure.Get();
            const bool lightingLost = data.lighting && data.lighting->frameFailed && !data.lighting->failureCleared;
            const bool sceneValid = sceneTexture && exposureBuffer && state.exposureInitialized && !lightingLost;

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vertexReflection = loader->GetCachedReflection("fullscreen", ".vs");
            auto* pixelReflection = loader->GetCachedReflection("display_output", ".ps");
            if (!vertexReflection || !pixelReflection || !exposureBuffer)
                return;

            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();
            nvrhi::IBuffer* constantBuffer = cache.GetOrCreateVolatileCB(
                "DisplayOutput", "DisplayOutputParams", sizeof(DisplayOutputConstants), data.device);
            if (!constantBuffer)
                return;

            DisplayOutputConstants constants = data.constants;
            constants.sceneValid = sceneValid ? 1u : 0u;
            constants.frameIndex = state.frameIndex++;
            if (!bloomTexture)
                constants.bloomIntensity = 0.0f;
            if (!data.noise)
                constants.noiseIntensity = 0.0f;
            if (!data.colorMap0 || !data.colorMap1)
                constants.colorMapInfluence = 0.0f;
            cmdList->writeBuffer(constantBuffer, &constants, sizeof(constants));

            BindingSetBuilder bindings(*vertexReflection, *pixelReflection, nvDevice, "DisplayOutput");
            bindings.Texture("t_Scene", sceneValid ? sceneTexture : interfaceTexture)
                .Texture("t_Interface", interfaceTexture)
                .BufferSRV("t_Exposure", exposureBuffer)
                .Texture("t_Bloom", bloomTexture ? bloomTexture : interfaceTexture)
                .Texture("t_Noise", data.noise ? data.noise.Get() : interfaceTexture)
                .Texture("t_ColorMap0", data.colorMap0 ? data.colorMap0.Get() : interfaceTexture)
                .Texture("t_ColorMap1", data.colorMap1 ? data.colorMap1.Get() : interfaceTexture)
                .ConstantBuffer("DisplayOutputParams", constantBuffer);
            nvrhi::BindingSetHandle bindingSet = cache.GetOrCreateBindingSet(bindings.Build(), state.outputLayout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc framebufferDesc;
            framebufferDesc.addColorAttachment(outputTexture);
            nvrhi::FramebufferHandle framebuffer = cache.GetOrCreateFramebuffer(framebufferDesc, nvDevice);

            nvrhi::Viewport viewport(float(data.width), float(data.height));
            nvrhi::GraphicsState graphicsState;
            graphicsState.pipeline = state.outputPipeline;
            graphicsState.framebuffer = framebuffer;
            graphicsState.viewport.addViewportAndScissorRect(viewport);
            graphicsState.addBindingSet(bindingSet);
            cmdList->setGraphicsState(graphicsState);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );

    return passData.output;
}
}
