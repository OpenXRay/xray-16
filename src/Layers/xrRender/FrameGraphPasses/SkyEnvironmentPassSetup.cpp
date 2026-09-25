#include "stdafx.h"
#include "SkyEnvironmentPassSetup.h"
#include "Layers/xrRender/ColorSpace.h"
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
constexpr u32 kSkyGroupSize = 8;
constexpr u32 kSkyCubeFaces = 6;
constexpr u32 kSkyConstantVersions = 32;
constexpr u32 kSkySpecularSamples = 64;

static_assert(sizeof(SkySourceConstants) == 32, "SkySourceParams layout");
static_assert(sizeof(SkyDownsampleConstants) == 16, "SkyDownsampleParams layout");
static_assert(sizeof(SkyIrradianceConstants) == 16, "SkyIrradianceParams layout");
static_assert(sizeof(SkySpecularConstants) == 32, "SkySpecularParams layout");

u32 GroupCount(u32 size)
{
    return (size + kSkyGroupSize - 1) / kSkyGroupSize;
}

nvrhi::TextureSubresourceSet CubeLevel(u32 level)
{
    return nvrhi::TextureSubresourceSet(level, 1, 0, kSkyCubeFaces);
}

const ExtractedReflection* StageReflection(const SkyComputePipeline& stage)
{
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    return loader && stage.shader ? loader->GetCachedReflection(stage.shader, ".cs") : nullptr;
}

nvrhi::IBuffer* StageConstants(const SkyEnvironment& environment, const char* name, u32 size)
{
    return GetPassResourceCache().GetOrCreateVolatileCB("SkyEnvironment", name, size, environment.GetDevice(),
        kSkyConstantVersions);
}

bool DispatchStage(nvrhi::ICommandList* cmdList, const SkyComputePipeline& stage, BindingSetBuilder& bindings,
    u32 groupsX, u32 groupsY, u32 groupsZ)
{
    nvrhi::BindingSetHandle bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(), stage.layout,
        cmdList->getDevice());
    if (!bindingSet)
        return false;

    nvrhi::ComputeState computeState;
    computeState.pipeline = stage.pipeline;
    computeState.bindings = { bindingSet };
    cmdList->setComputeState(computeState);
    cmdList->dispatch(groupsX, groupsY, groupsZ);
    return true;
}

bool RecordSkySource(const SkySourcePassData& data, nvrhi::ICommandList* cmdList, nvrhi::ITexture* cube)
{
    const SkyEnvironment& environment = *data.environment;
    const SkyComputePipeline& source = environment.GetPipeline(SkyComputeStage::Source);
    const SkyComputePipeline& downsample = environment.GetPipeline(SkyComputeStage::Downsample);
    const ExtractedReflection* sourceReflection = StageReflection(source);
    const ExtractedReflection* downsampleReflection = StageReflection(downsample);
    nvrhi::IBuffer* sourceConstants = StageConstants(environment, "SkySourceParams", sizeof(SkySourceConstants));
    nvrhi::IBuffer* downsampleConstants = StageConstants(environment, "SkyDownsampleParams", sizeof(SkyDownsampleConstants));
    if (!source.pipeline || !downsample.pipeline || !sourceReflection || !downsampleReflection || !sourceConstants
        || !downsampleConstants || !data.params.sky0 || !data.params.sky1)
        return false;

    nvrhi::IDevice* nvDevice = cmdList->getDevice();
    const nvrhi::TextureDesc& desc = cube->getDesc();

    SkySourceConstants constants;
    constants.tint = SrgbToLinear(data.params.tint);
    constants.faceSize = desc.width;
    constants.blend = data.params.blend;
    constants.rotation = data.params.rotation;
    constants.energy = data.params.energy;
    constants.groundAlbedo = data.params.groundAlbedo;
    cmdList->writeBuffer(sourceConstants, &constants, sizeof(constants));

    BindingSetBuilder sourceBindings(*sourceReflection, nvDevice, "SkySource");
    sourceBindings.ConstantBuffer("SkySourceParams", sourceConstants)
        .Texture("g_Sky0", data.params.sky0)
        .Texture("g_Sky1", data.params.sky1)
        .TextureUAV("g_SkyCube", cube, nvrhi::Format::UNKNOWN, CubeLevel(0), nvrhi::TextureDimension::Texture2DArray);
    if (!DispatchStage(cmdList, source, sourceBindings, GroupCount(desc.width), GroupCount(desc.width), kSkyCubeFaces))
        return false;

    for (u32 level = 1; level < desc.mipLevels; ++level)
    {
        SkyDownsampleConstants params;
        params.targetSize = std::max(1u, desc.width >> level);
        cmdList->writeBuffer(downsampleConstants, &params, sizeof(params));

        BindingSetBuilder bindings(*downsampleReflection, nvDevice, "SkyDownsample");
        bindings.ConstantBuffer("SkyDownsampleParams", downsampleConstants)
            .Texture("g_Source", cube, nvrhi::Format::UNKNOWN, CubeLevel(level - 1))
            .TextureUAV("g_Target", cube, nvrhi::Format::UNKNOWN, CubeLevel(level), nvrhi::TextureDimension::Texture2DArray);
        if (!DispatchStage(cmdList, downsample, bindings, GroupCount(params.targetSize), GroupCount(params.targetSize),
                kSkyCubeFaces))
            return false;
    }
    return true;
}

bool RecordSkyLighting(const SkyLightingPassData& data, nvrhi::ICommandList* cmdList, nvrhi::ITexture* cube,
    nvrhi::IBuffer* irradiance, nvrhi::ITexture* specular)
{
    const SkyEnvironment& environment = *data.environment;
    const SkyComputePipeline& irradianceStage = environment.GetPipeline(SkyComputeStage::Irradiance);
    const SkyComputePipeline& specularStage = environment.GetPipeline(SkyComputeStage::Specular);
    const ExtractedReflection* irradianceReflection = StageReflection(irradianceStage);
    const ExtractedReflection* specularReflection = StageReflection(specularStage);
    nvrhi::IBuffer* irradianceConstants = StageConstants(environment, "SkyIrradianceParams", sizeof(SkyIrradianceConstants));
    nvrhi::IBuffer* specularConstants = StageConstants(environment, "SkySpecularParams", sizeof(SkySpecularConstants));
    if (!irradianceStage.pipeline || !specularStage.pipeline || !irradianceReflection || !specularReflection
        || !irradianceConstants || !specularConstants)
        return false;

    nvrhi::IDevice* nvDevice = cmdList->getDevice();
    const u32 cubeSize = cube->getDesc().width;

    SkyIrradianceConstants irradianceParams;
    irradianceParams.faceSize = SkyEnvironment::kIrradianceFaceSize;
    irradianceParams.sourceLod = std::log2(float(cubeSize) / float(SkyEnvironment::kIrradianceFaceSize));
    cmdList->writeBuffer(irradianceConstants, &irradianceParams, sizeof(irradianceParams));

    BindingSetBuilder irradianceBindings(*irradianceReflection, nvDevice, "SkyIrradiance");
    irradianceBindings.ConstantBuffer("SkyIrradianceParams", irradianceConstants)
        .Texture("g_Sky", cube)
        .BufferUAV("g_SkyIrradiance", irradiance);
    if (!DispatchStage(cmdList, irradianceStage, irradianceBindings, 1, 1, 1))
        return false;

    const nvrhi::TextureDesc& specularDesc = specular->getDesc();
    const float mirrorLod = std::log2(float(cubeSize) / float(specularDesc.width));
    const float texelSolidAngle = 4.0f * PI / (6.0f * float(cubeSize) * float(cubeSize));
    for (u32 level = 0; level < specularDesc.mipLevels; ++level)
    {
        SkySpecularConstants params;
        params.roughness = specularDesc.mipLevels > 1 ? float(level) / float(specularDesc.mipLevels - 1) : 0.0f;
        params.faceSize = std::max(1u, specularDesc.width >> level);
        params.mirrorLod = mirrorLod;
        params.sourceTexelSolidAngle = texelSolidAngle;
        params.sampleCount = kSkySpecularSamples;
        cmdList->writeBuffer(specularConstants, &params, sizeof(params));

        BindingSetBuilder bindings(*specularReflection, nvDevice, "SkySpecular");
        bindings.ConstantBuffer("SkySpecularParams", specularConstants)
            .Texture("g_Sky", cube)
            .TextureUAV("g_Specular", specular, nvrhi::Format::UNKNOWN, CubeLevel(level), nvrhi::TextureDimension::Texture2DArray);
        if (!DispatchStage(cmdList, specularStage, bindings, GroupCount(params.faceSize), GroupCount(params.faceSize),
                kSkyCubeFaces))
            return false;
    }
    return true;
}

bool RecordSkyDFG(const SkyDFGPassData& data, nvrhi::ICommandList* cmdList, nvrhi::ITexture* dfg)
{
    const SkyComputePipeline& stage = data.environment->GetPipeline(SkyComputeStage::DFG);
    const ExtractedReflection* reflection = StageReflection(stage);
    if (!stage.pipeline || !reflection)
        return false;

    BindingSetBuilder bindings(*reflection, cmdList->getDevice(), "SkyDFG");
    bindings.TextureUAV("g_DFG", dfg);
    const u32 size = dfg->getDesc().width;
    return DispatchStage(cmdList, stage, bindings, GroupCount(size), GroupCount(size), 1);
}

VirtualResourceHandle AddSkyDFGPass(FrameGraph& fg, SkyEnvironment& environment, VirtualResourceHandle dfg)
{
    auto& passData = fg.addCallbackPass<SkyDFGPassData>(
        "Sky DFG",
        [dfg, &environment](FrameGraph& builder, PassHandle passHandle, SkyDFGPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.dfg = passBuilder.write(dfg, ResourceState::UnorderedAccess);
            passBuilder.sideEffects();
            data.environment = &environment;
        },
        [](const SkyDFGPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::ITexture* texture = fg.GetPhysicalTexture(data.dfg);
            data.environment->CompleteDFG(cmdList && texture && RecordSkyDFG(data, cmdList, texture));
        });
    return passData.dfg;
}

VirtualResourceHandle AddSkySourcePass(FrameGraph& fg, SkyEnvironment& environment, VirtualResourceHandle cube,
    const SkyEnvironmentParams& params)
{
    auto& passData = fg.addCallbackPass<SkySourcePassData>(
        "Sky Source",
        [cube, params, &environment](FrameGraph& builder, PassHandle passHandle, SkySourcePassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.cube = passBuilder.write(cube, ResourceState::UnorderedAccess);
            passBuilder.sideEffects();
            data.environment = &environment;
            data.params = params;
        },
        [](const SkySourcePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::ITexture* texture = fg.GetPhysicalTexture(data.cube);
            if (cmdList && texture && RecordSkySource(data, cmdList, texture))
                return;
            if (cmdList && texture)
                cmdList->clearTextureFloat(texture, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            data.environment->DiscardSource();
        });
    return passData.cube;
}

void AddSkyLightingPass(FrameGraph& fg, SkyEnvironment& environment, SkyEnvironmentFrame& frame)
{
    const VirtualResourceHandle cube = frame.cube;
    const VirtualResourceHandle irradiance = frame.irradiance;
    const VirtualResourceHandle specular = frame.specular;
    auto& passData = fg.addCallbackPass<SkyLightingPassData>(
        "Sky Lighting",
        [cube, irradiance, specular, &environment](FrameGraph& builder, PassHandle passHandle, SkyLightingPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.cube = passBuilder.read(cube, ResourceState::ShaderResource);
            data.irradiance = passBuilder.write(irradiance, ResourceState::UnorderedAccess);
            data.specular = passBuilder.write(specular, ResourceState::UnorderedAccess);
            passBuilder.sideEffects();
            data.environment = &environment;
        },
        [](const SkyLightingPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::ITexture* cube = fg.GetPhysicalTexture(data.cube);
            nvrhi::IBuffer* irradiance = fg.GetPhysicalBuffer(data.irradiance);
            nvrhi::ITexture* specular = fg.GetPhysicalTexture(data.specular);
            const bool ready = cmdList && cube && irradiance && specular && data.environment->IsSourceValid()
                && RecordSkyLighting(data, cmdList, cube, irradiance, specular);
            data.environment->CompleteLighting(ready);
            if (!ready)
                data.environment->DiscardSource();
        });
    frame.irradiance = passData.irradiance;
    frame.specular = passData.specular;
}
}

SkyEnvironmentFrame setupSkyEnvironmentPass(FrameGraph& fg, SkyEnvironment& environment)
{
    SkyEnvironmentParams params;
    const bool prepared = environment.Prepare(params);
    const bool source = prepared && environment.NeedsSource(params);
    if (source)
        environment.CommitSource(params);

    SkyEnvironmentFrame frame = environment.Use(fg, prepared);
    if (!prepared || !frame.cube.is_valid())
        return frame;

    if (environment.NeedsDFG())
        frame.dfg = AddSkyDFGPass(fg, environment, frame.dfg);

    if (!source)
        return frame;

    frame.cube = AddSkySourcePass(fg, environment, frame.cube, params);
    AddSkyLightingPass(fg, environment, frame);
    return frame;
}
}
