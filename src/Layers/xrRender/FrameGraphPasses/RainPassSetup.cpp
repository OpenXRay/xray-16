#include "stdafx.h"
#include "RainPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgRainRender.h"

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupRainPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle worldPosTarget,
    framegraph::VirtualResourceHandle rainSMTarget,
    FGRainRender* renderer,
    const Fmatrix& rainSampleVP,
    bool rainSMValid,
    const RainHeightmapInfo& heightmap)
{
    using namespace framegraph;

    if (!depthTarget.is_valid() || !worldPosTarget.is_valid())
    {
        Msg("! [RainPass] Missing depth/worldPos — skipping rain (would be unoccluded)");
        return inputTarget;
    }

    auto& passData = fg.addCallbackPass<RainPassData>(
        "Rain",
        [&](FrameGraph& builder, PassHandle passHandle, RainPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.heightmap = heightmap;
            data.rainSampleVP = rainSampleVP;
            data.rainSMValid = rainSMValid;
            data.depth = passBuilder.readWrite(depthTarget, ResourceState::DepthStencilWrite);
            data.worldPos = passBuilder.read(worldPosTarget, ResourceState::ShaderResource);
            data.output = passBuilder.readWrite(inputTarget, ResourceState::RenderTarget);
            if (rainSMTarget.is_valid())
                data.rainSM = passBuilder.read(rainSMTarget, ResourceState::ShaderResource);
        },
        [](const RainPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (!data.renderer) return;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmdList ? cmdList->getDevice() : nullptr;
            auto* outputRT = fg.GetPhysicalTexture(data.output);
            auto* depth    = fg.GetPhysicalTexture(data.depth);
            auto* worldPos = fg.GetPhysicalTexture(data.worldPos);
            auto* rainSM   = data.rainSM.is_valid() ? fg.GetPhysicalTexture(data.rainSM) : nullptr;
            if (!cmdList || !nv || !outputRT || !depth || !worldPos) return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);
            fbDesc.setDepthAttachment(depth);
            auto framebuffer = GetPassResourceCache().GetOrCreateFramebuffer("RainPass_Soft", fbDesc, nv);
            if (!framebuffer) return;

            data.renderer->SetRainShadowInputs(rainSM, data.rainSampleVP, data.rainSMValid && rainSM);
            data.renderer->Draw(cmdList, framebuffer, worldPos, data.heightmap);
        });

    return passData.output;
}

}
