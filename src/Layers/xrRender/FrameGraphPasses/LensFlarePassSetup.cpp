#include "stdafx.h"

#include "LensFlarePassSetup.h"

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgLensFlareRender.h"

namespace xray::render::fg::passes
{
framegraph::VirtualResourceHandle setupLensFlarePass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGLensFlareRender* renderer)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<LensFlarePassData>(
        "LensFlare",
        [inputTarget, depthTarget, renderer](FrameGraph& builder, PassHandle passHandle, LensFlarePassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.depth = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
            data.output = passBuilder.readWrite(inputTarget, ResourceState::RenderTarget);
        },
        [](const LensFlarePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
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

            data.renderer->DispatchVisibility(cmdList, depth);
            data.renderer->Draw(cmdList, framebuffer);
        });

    return passData.output;
}
} // namespace xray::render::fg::passes
