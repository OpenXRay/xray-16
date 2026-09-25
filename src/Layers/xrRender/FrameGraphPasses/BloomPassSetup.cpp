#include "stdafx.h"
#include "BloomPassSetup.h"
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
constexpr u32 kBloomGroupSize = 8;
constexpr u32 kBloomMaxLevels = 8;
constexpr u32 kBloomSmallestExtent = 8;
constexpr u32 kBloomConstantVersions = 64;

static_assert(sizeof(BloomConstants) == 32, "BloomParams layout");

bool EnsureBloomPipelines(fg::RenderDevice* device, BloomPassState& state)
{
    if (state.downsamplePipeline && state.upsamplePipeline)
        return true;
    if (state.pipelinesFailed)
        return false;

    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !loader)
        return false;

    auto downsampleShader = loader->LoadComputeShader("bloom_downsample");
    auto upsampleShader = loader->LoadComputeShader("bloom_upsample");
    if (!downsampleShader.handle || !downsampleShader.reflection || !upsampleShader.handle || !upsampleShader.reflection)
    {
        Msg("! [Bloom] bloom shaders unavailable, bloom is disabled");
        state.pipelinesFailed = true;
        return false;
    }

    auto& cache = GetPassResourceCache();
    state.downsampleLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomDownsample", *downsampleShader.reflection, nvDevice);
    state.upsampleLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomUpsample", *upsampleShader.reflection, nvDevice);
    if (state.downsampleLayout && state.upsampleLayout)
    {
        nvrhi::ComputePipelineDesc downsampleDesc;
        downsampleDesc.CS = downsampleShader.handle;
        downsampleDesc.bindingLayouts = { state.downsampleLayout };
        state.downsamplePipeline = cache.GetOrCreateComputePipeline("BloomDownsample", downsampleDesc, nvDevice);

        nvrhi::ComputePipelineDesc upsampleDesc;
        upsampleDesc.CS = upsampleShader.handle;
        upsampleDesc.bindingLayouts = { state.upsampleLayout };
        state.upsamplePipeline = cache.GetOrCreateComputePipeline("BloomUpsample", upsampleDesc, nvDevice);
    }

    if (state.downsamplePipeline && state.upsamplePipeline)
        return true;

    Msg("! [Bloom] bloom pipelines could not be created, bloom is disabled");
    state.pipelinesFailed = true;
    return false;
}

nvrhi::TextureSubresourceSet BloomLevel(u32 level)
{
    return nvrhi::TextureSubresourceSet(level, 1, 0, 1);
}

u32 LevelExtent(u32 extent, u32 level)
{
    return std::max(1u, extent >> level);
}

bool DispatchBloomLevel(
    nvrhi::ICommandList* cmdList,
    BindingSetBuilder& bindings,
    nvrhi::IBindingLayout* layout,
    nvrhi::IComputePipeline* pipeline,
    nvrhi::IBuffer* constantBuffer,
    const BloomConstants& constants)
{
    cmdList->writeBuffer(constantBuffer, &constants, sizeof(constants));
    nvrhi::BindingSetHandle bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(), layout, cmdList->getDevice());
    if (!bindingSet)
        return false;

    nvrhi::ComputeState computeState;
    computeState.pipeline = pipeline;
    computeState.bindings = { bindingSet };
    cmdList->setComputeState(computeState);
    cmdList->dispatch(
        (constants.targetWidth + kBloomGroupSize - 1) / kBloomGroupSize,
        (constants.targetHeight + kBloomGroupSize - 1) / kBloomGroupSize,
        1);
    return true;
}

void RecordBloom(const BloomPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
{
    nvrhi::ITexture* sceneTexture = fg.GetPhysicalTexture(data.sceneColor);
    nvrhi::IBuffer* exposureBuffer = fg.GetPhysicalBuffer(data.exposure);
    nvrhi::ITexture* bloomTexture = fg.GetPhysicalTexture(data.bloom);
    auto* loader = GEnv.Render->GetShaderLoader();
    auto* downsampleReflection = loader->GetCachedReflection("bloom_downsample", ".cs");
    auto* upsampleReflection = loader->GetCachedReflection("bloom_upsample", ".cs");
    if (!sceneTexture || !exposureBuffer || !bloomTexture || !downsampleReflection || !upsampleReflection)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = cmdList->getDevice();
    nvrhi::IBuffer* constantBuffer = GetPassResourceCache().GetOrCreateVolatileCB(
        "Bloom", "BloomParams", sizeof(BloomConstants), data.device, kBloomConstantVersions);
    if (!constantBuffer)
        return;

    const BloomPassState& state = *data.state;
    const nvrhi::TextureDesc& sceneDesc = sceneTexture->getDesc();
    for (u32 level = 0; level < data.levels; ++level)
    {
        const u32 sourceWidth = level == 0 ? sceneDesc.width : LevelExtent(data.width, level - 1);
        const u32 sourceHeight = level == 0 ? sceneDesc.height : LevelExtent(data.height, level - 1);

        BloomConstants constants;
        constants.sourceTexelSize.set(1.0f / float(sourceWidth), 1.0f / float(sourceHeight));
        constants.targetWidth = LevelExtent(data.width, level);
        constants.targetHeight = LevelExtent(data.height, level);
        constants.firstLevel = level == 0 ? 1u : 0u;

        BindingSetBuilder bindings(*downsampleReflection, nvDevice, "BloomDownsample");
        if (level == 0)
            bindings.Texture("t_Source", sceneTexture);
        else
            bindings.Texture("t_Source", bloomTexture, nvrhi::Format::UNKNOWN, BloomLevel(level - 1));
        bindings.BufferSRV("t_Exposure", exposureBuffer)
            .TextureUAV("u_Target", bloomTexture, nvrhi::Format::UNKNOWN, BloomLevel(level))
            .ConstantBuffer("BloomParams", constantBuffer);
        if (!DispatchBloomLevel(cmdList, bindings, state.downsampleLayout, state.downsamplePipeline, constantBuffer, constants))
            return;
    }

    for (u32 level = data.levels - 1; level > 0; --level)
    {
        BloomConstants constants;
        constants.sourceTexelSize.set(
            1.0f / float(LevelExtent(data.width, level)),
            1.0f / float(LevelExtent(data.height, level)));
        constants.targetWidth = LevelExtent(data.width, level - 1);
        constants.targetHeight = LevelExtent(data.height, level - 1);

        BindingSetBuilder bindings(*upsampleReflection, nvDevice, "BloomUpsample");
        bindings.Texture("t_Source", bloomTexture, nvrhi::Format::UNKNOWN, BloomLevel(level))
            .TextureUAV("u_Target", bloomTexture, nvrhi::Format::UNKNOWN, BloomLevel(level - 1))
            .ConstantBuffer("BloomParams", constantBuffer);
        if (!DispatchBloomLevel(cmdList, bindings, state.upsampleLayout, state.upsamplePipeline, constantBuffer, constants))
            return;
    }
}
}

u32 CalculateBloomLevels(u32 width, u32 height)
{
    const u32 extent = std::min(width, height);
    u32 levels = 1;
    while (levels < kBloomMaxLevels && (extent >> levels) >= kBloomSmallestExtent)
        ++levels;
    return levels;
}

BloomOutput setupBloomPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle exposure,
    u32 width,
    u32 height,
    BloomPassState& state)
{
    BloomOutput output;
    if (!sceneColor.is_valid() || !exposure.is_valid() || width < 2 || height < 2 || !EnsureBloomPipelines(device, state))
        return output;

    const u32 bloomWidth = width / 2;
    const u32 bloomHeight = height / 2;
    const u32 levels = CalculateBloomLevels(bloomWidth, bloomHeight);

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = bloomWidth;
    desc.height = bloomHeight;
    desc.mipLevels = levels;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isUAV = true;
    desc.isTransient = true;
    desc.debugName = "rt_Bloom";
    const VirtualResourceHandle bloom = fg.CreateTexture("rt_Bloom", desc);

    auto& passData = fg.addCallbackPass<BloomPassData>(
        "Bloom",
        [sceneColor, exposure, bloom, device, bloomWidth, bloomHeight, levels, &state](
            FrameGraph& builder, PassHandle passHandle, BloomPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
            data.exposure = passBuilder.read(exposure, ResourceState::ShaderResource);
            data.bloom = passBuilder.write(bloom, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.width = bloomWidth;
            data.height = bloomHeight;
            data.levels = levels;
        },
        [](const BloomPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            RecordBloom(data, fg, ctx);
        }
    );

    output.texture = passData.bloom;
    output.levels = levels;
    return output;
}
}
