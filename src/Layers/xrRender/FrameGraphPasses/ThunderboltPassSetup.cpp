#include "stdafx.h"

#include "ThunderboltPassSetup.h"

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgThunderboltRender.h"

namespace xray::render::fg::passes
{
framegraph::VirtualResourceHandle setupThunderboltPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle worldPosTarget,
    FGThunderboltRender* renderer)
{
    using namespace framegraph;

    if (!depthTarget.is_valid() || !worldPosTarget.is_valid())
    {
        Msg("! [ThunderboltPass] Missing depth/worldPos — skipping bolt (would be unoccluded)");
        return inputTarget;
    }

    auto& passData = fg.addCallbackPass<ThunderboltPassData>(
        "Thunderbolt",
        [inputTarget, depthTarget, worldPosTarget, renderer](FrameGraph& builder, PassHandle passHandle, ThunderboltPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.depth = passBuilder.readWrite(depthTarget, ResourceState::DepthStencilWrite);
            data.worldPos = passBuilder.read(worldPosTarget, ResourceState::ShaderResource);
            data.output = passBuilder.readWrite(inputTarget, ResourceState::RenderTarget);
        },
        [](const ThunderboltPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.renderer || !data.renderer->HasWork())
                return;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmdList ? cmdList->getDevice() : nullptr;
            auto* outputRT = fg.GetPhysicalTexture(data.output);
            auto* depth = fg.GetPhysicalTexture(data.depth);
            auto* worldPos = fg.GetPhysicalTexture(data.worldPos);
            if (!cmdList || !nv || !outputRT || !depth || !worldPos)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);
            fbDesc.setDepthAttachment(depth);
            auto framebuffer = GetPassResourceCache().GetOrCreateFramebuffer("ThunderboltPass_Soft", fbDesc, nv);
            if (!framebuffer)
                return;

            data.renderer->Draw(cmdList, framebuffer, worldPos);
        });

    return passData.output;
}
} // namespace xray::render::fg::passes
