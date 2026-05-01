#include "stdafx.h"

#include "ThunderboltPassSetup.h"

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgThunderboltRender.h"

namespace xray::render::fg::passes
{
framegraph::VirtualResourceHandle setupThunderboltPass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGThunderboltRender* renderer)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<ThunderboltPassData>(
        "Thunderbolt",
        [inputTarget, depthTarget, renderer](FrameGraph& builder, PassHandle passHandle, ThunderboltPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.depth = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
            data.output = passBuilder.write(inputTarget, ResourceState::RenderTarget);
        },
        [](const ThunderboltPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.renderer || !data.renderer->HasWork())
                return;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* outputRT = fg.GetPhysicalTexture(data.output);
            auto* depth = fg.GetPhysicalTexture(data.depth);
            if (!cmdList || !outputRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);
            if (depth)
                fbDesc.setDepthAttachment(depth);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            data.renderer->Draw(cmdList, framebuffer);
        });

    return passData.output;
}
} // namespace xray::render::fg::passes
