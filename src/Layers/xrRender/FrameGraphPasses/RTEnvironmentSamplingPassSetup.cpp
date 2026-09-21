#include "stdafx.h"
#include "RTEnvironmentSamplingPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

static nvrhi::ShaderHandle s_envShader;
static nvrhi::ComputePipelineHandle s_envPipeline;
static nvrhi::BindingLayoutHandle s_envLayout;
static nvrhi::IBuffer* s_envParamsCB = nullptr;

static bool LoadEnvironmentSamplingPipeline(RenderDevice* device, nvrhi::IDevice* nvDevice)
{
    if (s_envShader && s_envLayout && s_envParamsCB && s_envPipeline)
        return true;

    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!shaderLoader)
        return false;

    if (!s_envShader)
    {
        auto csResult = shaderLoader->LoadComputeShader("rt_environment", "main");
        if (!csResult.handle)
        {
            Msg("! [EnvironmentSampling] Failed to load the rt_environment compute shader");
            return false;
        }
        s_envShader = csResult.handle;
    }

    auto* reflection = shaderLoader->GetCachedReflection("rt_environment", ".cs");
    if (!reflection)
    {
        Msg("! [EnvironmentSampling] Failed to get the rt_environment reflection");
        return false;
    }

    auto& cache = GetPassResourceCache();

    if (!s_envLayout)
    {
        s_envLayout = cache.GetOrCreateBindingLayoutFromReflection("RTEnvironmentSampling", *reflection, nvDevice);
        if (!s_envLayout)
        {
            Msg("! [EnvironmentSampling] Failed to create the binding layout");
            return false;
        }
    }

    if (!s_envParamsCB)
    {
        s_envParamsCB = cache.GetOrCreateVolatileCB(
            "RTEnvironmentSampling", "EnvironmentSamplingParams", sizeof(EnvironmentSamplingCB), device);
        if (!s_envParamsCB)
        {
            Msg("! [EnvironmentSampling] Failed to create the parameter constant buffer");
            return false;
        }
    }

    if (!s_envPipeline)
    {
        nvrhi::ComputePipelineDesc pipelineDesc;
        pipelineDesc.CS = s_envShader;
        pipelineDesc.bindingLayouts = { s_envLayout };
        s_envPipeline = nvDevice->createComputePipeline(pipelineDesc);
        if (!s_envPipeline)
        {
            Msg("! [EnvironmentSampling] Failed to create the compute pipeline");
            return false;
        }
    }

    Msg("* [EnvironmentSampling] Environment CDF pipeline initialized");
    return true;
}

RTEnvironmentSamplingOutput setupRTEnvironmentSamplingPass(
    FrameGraph& fg,
    RenderDevice* device,
    nvrhi::ITexture* sky0,
    nvrhi::ITexture* sky1,
    float blend)
{
    RTEnvironmentSamplingOutput output;

    if (!device || !sky0 || !sky1)
        return output;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return output;

    if (sky0->getDesc().dimension != nvrhi::TextureDimension::TextureCube ||
        sky1->getDesc().dimension != nvrhi::TextureDimension::TextureCube)
        return output;

    const bool pipelineReady = LoadEnvironmentSamplingPipeline(device, nvDevice);

    auto importSky = [&](const char* name, nvrhi::ITexture* texture) {
        const auto& native = texture->getDesc();
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::TextureCube;
        desc.width = native.width;
        desc.height = native.height;
        desc.depth = native.depth;
        desc.arraySize = native.arraySize;
        desc.mipLevels = native.mipLevels;
        desc.format = native.format;
        desc.sampleCount = native.sampleCount;
        desc.isTransient = false;
        desc.debugName = name;
        return fg.ImportTexture(name, texture, desc);
    };

    VirtualResourceHandle sky0Handle = importSky("env_sky0", sky0);
    VirtualResourceHandle sky1Handle = importSky("env_sky1", sky1);

    ResourceDesc cdfDesc;
    cdfDesc.type = ResourceDesc::Type::Buffer;
    cdfDesc.bufferSize = u64(kEnvironmentSamplingEntryCount) * sizeof(float);
    cdfDesc.structStride = sizeof(float);
    cdfDesc.isUAV = true;
    cdfDesc.allowUAV = true;
    cdfDesc.isTransient = true;
    cdfDesc.debugName = "env_cdf";
    VirtualResourceHandle cdfHandle = fg.CreateBuffer("env_cdf", cdfDesc);

    if (!sky0Handle.is_valid() || !sky1Handle.is_valid() || !cdfHandle.is_valid())
        return output;

    auto& passData = fg.addCallbackPass<EnvironmentSamplingPassData>(
        "EnvironmentSampling",
        [&, sky0Handle, sky1Handle, cdfHandle, blend](
            FrameGraph& builder, PassHandle passHandle, EnvironmentSamplingPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.sky0 = passBuilder.read(sky0Handle, ResourceState::ShaderResource);
            data.sky1 = passBuilder.read(sky1Handle, ResourceState::ShaderResource);
            data.distribution = passBuilder.write(cdfHandle, ResourceState::UnorderedAccess);
            data.blend = blend;
        },
        [](const EnvironmentSamplingPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IBuffer* cdfBuffer = fg.GetPhysicalBuffer(data.distribution);
            if (!cmdList || !cdfBuffer)
                return;

            auto clearDistribution = [cmdList, cdfBuffer]() {
                cmdList->clearBufferUInt(cdfBuffer, 0);
            };

            nvrhi::IDevice* nvDevice = data.device ? data.device->GetNVRHIDevice() : nullptr;
            if (!nvDevice || !s_envPipeline || !s_envLayout || !s_envParamsCB)
            {
                clearDistribution();
                return;
            }

            nvrhi::ITexture* sky0Tex = fg.GetPhysicalTexture(data.sky0);
            nvrhi::ITexture* sky1Tex = fg.GetPhysicalTexture(data.sky1);
            if (!sky0Tex || !sky1Tex)
            {
                clearDistribution();
                return;
            }

            auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
            auto* reflection = shaderLoader ? shaderLoader->GetCachedReflection("rt_environment", ".cs") : nullptr;
            if (!reflection)
            {
                clearDistribution();
                return;
            }

            EnvironmentSamplingCB params;
            params.skyBlend = std::isfinite(data.blend) ? data.blend : 0.0f;
            params.pad0 = 0.0f;
            params.pad1 = 0.0f;
            params.pad2 = 0.0f;
            cmdList->writeBuffer(s_envParamsCB, &params, sizeof(params));

            BindingSetBuilder bsb(*reflection, nvDevice, "RTEnvironmentSampling");
            bsb.ConstantBuffer("EnvironmentSamplingParams", s_envParamsCB)
               .Texture("g_Sky0", sky0Tex)
               .Texture("g_Sky1", sky1Tex)
               .BufferUAV("g_EnvironmentCDF", cdfBuffer);

            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), s_envLayout);
            if (!bindingSet)
            {
                clearDistribution();
                return;
            }

            nvrhi::ComputeState computeState;
            computeState.pipeline = s_envPipeline;
            computeState.bindings = { bindingSet };
            cmdList->setComputeState(computeState);
            cmdList->dispatch(1, 1, 1);
        });

    output.distribution = passData.distribution;
    output.active = pipelineReady;
    return output;
}

void ShutdownRTEnvironmentSampling()
{
    s_envShader = nullptr;
    s_envPipeline = nullptr;
    s_envLayout = nullptr;
    s_envParamsCB = nullptr;
}

}
