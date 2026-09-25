#include "stdafx.h"
#include "SkyEnvironmentPassSetup.h"
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
constexpr u32 kSkySourceGroupSize = 8;
constexpr u32 kSkyCubeFaces = 6;

static_assert(sizeof(SkySourceConstants) == 32, "SkySourceParams layout");

bool DispatchSkySource(const SkySourcePassData& data, nvrhi::ICommandList* cmdList, nvrhi::ITexture* cube)
{
    SkyEnvironment& environment = *data.environment;
    nvrhi::IComputePipeline* pipeline = environment.GetSourcePipeline();
    nvrhi::IBindingLayout* layout = environment.GetSourceLayout();
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* reflection = loader ? loader->GetCachedReflection("sky_source", ".cs") : nullptr;
    if (!pipeline || !layout || !reflection || !data.params.sky0 || !data.params.sky1)
        return false;

    nvrhi::IBuffer* constantBuffer = GetPassResourceCache().GetOrCreateVolatileCB(
        "SkySource", "SkySourceParams", sizeof(SkySourceConstants), environment.GetDevice());
    if (!constantBuffer)
        return false;

    SkySourceConstants constants;
    constants.blend = data.params.blend;
    constants.rotation = data.params.rotation;
    constants.energy = data.params.energy;
    constants.groundAlbedo = data.params.groundAlbedo;
    constants.faceSize = cube->getDesc().width;
    cmdList->writeBuffer(constantBuffer, &constants, sizeof(constants));

    nvrhi::IDevice* nvDevice = cmdList->getDevice();
    BindingSetBuilder bindings(*reflection, nvDevice, "SkySource");
    bindings.ConstantBuffer("SkySourceParams", constantBuffer)
        .Texture("g_Sky0", data.params.sky0)
        .Texture("g_Sky1", data.params.sky1)
        .TextureUAV("g_SkyCube", cube, nvrhi::Format::UNKNOWN, nvrhi::AllSubresources,
            nvrhi::TextureDimension::Texture2DArray);
    nvrhi::BindingSetHandle bindingSet = GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(), layout, nvDevice);
    if (!bindingSet)
        return false;

    nvrhi::ComputeState computeState;
    computeState.pipeline = pipeline;
    computeState.bindings = { bindingSet };
    cmdList->setComputeState(computeState);
    const u32 groups = (constants.faceSize + kSkySourceGroupSize - 1) / kSkySourceGroupSize;
    cmdList->dispatch(groups, groups, kSkyCubeFaces);
    return true;
}

void RecordSkySource(const SkySourcePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
{
    nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
    nvrhi::ITexture* cube = fg.GetPhysicalTexture(data.cube);
    if (!data.environment || !cmdList || !cube)
    {
        if (data.environment)
            data.environment->DiscardSource();
        return;
    }

    if (DispatchSkySource(data, cmdList, cube))
        return;

    cmdList->clearTextureFloat(cube, nvrhi::AllSubresources, nvrhi::Color(0.0f));
    data.environment->DiscardSource();
}
}

SkyEnvironmentFrame setupSkyEnvironmentPass(FrameGraph& fg, SkyEnvironment& environment)
{
    SkyEnvironmentParams params;
    if (!environment.Prepare(params))
        return SkyEnvironmentFrame();

    const bool source = environment.NeedsSource(params);
    if (source)
        environment.CommitSource(params);

    SkyEnvironmentFrame frame = environment.Use(fg);
    if (!source || !frame.Valid())
        return frame;

    const VirtualResourceHandle cube = frame.cube;
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
            RecordSkySource(data, fg, ctx);
        });

    frame.cube = passData.cube;
    return frame;
}
}
