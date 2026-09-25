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

static bool LoadEnvironmentSamplingPipeline(nvrhi::IDevice* nvDevice)
{
    if (s_envShader && s_envLayout && s_envPipeline)
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
    VirtualResourceHandle sky)
{
    RTEnvironmentSamplingOutput output;

    if (!device || !sky.is_valid() || fg.GetResourceDesc(sky).type != ResourceDesc::Type::TextureCube)
        return output;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return output;

    const bool pipelineReady = LoadEnvironmentSamplingPipeline(nvDevice);

    ResourceDesc cdfDesc;
    cdfDesc.type = ResourceDesc::Type::Buffer;
    cdfDesc.bufferSize = u64(kEnvironmentSamplingEntryCount) * sizeof(float);
    cdfDesc.structStride = sizeof(float);
    cdfDesc.isUAV = true;
    cdfDesc.allowUAV = true;
    cdfDesc.isTransient = true;
    cdfDesc.debugName = "env_cdf";
    VirtualResourceHandle cdfHandle = fg.CreateBuffer("env_cdf", cdfDesc);

    if (!cdfHandle.is_valid())
        return output;

    auto& passData = fg.addCallbackPass<EnvironmentSamplingPassData>(
        "EnvironmentSampling",
        [&, sky, cdfHandle](
            FrameGraph& builder, PassHandle passHandle, EnvironmentSamplingPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.sky = passBuilder.read(sky, ResourceState::ShaderResource);
            data.distribution = passBuilder.write(cdfHandle, ResourceState::UnorderedAccess);
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
            if (!nvDevice || !s_envPipeline || !s_envLayout)
            {
                clearDistribution();
                return;
            }

            nvrhi::ITexture* skyTex = fg.GetPhysicalTexture(data.sky);
            if (!skyTex)
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

            BindingSetBuilder bsb(*reflection, nvDevice, "RTEnvironmentSampling");
            bsb.Texture("g_Sky", skyTex)
               .BufferUAV("g_EnvironmentCDF", cdfBuffer);

            auto bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), s_envLayout, nvDevice);
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
}

}
