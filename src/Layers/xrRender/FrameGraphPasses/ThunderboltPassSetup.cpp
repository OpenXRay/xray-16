#include "stdafx.h"

#include "ThunderboltPassSetup.h"

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgThunderboltRender.h"

namespace xray::render::fg::passes
{
framegraph::VirtualResourceHandle setupThunderboltPass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle inputTarget,
    framegraph::VirtualResourceHandle depthTarget, FGThunderboltRender* renderer, const LightingFrameState* lighting)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<ThunderboltPassData>(
        "Thunderbolt",
        [inputTarget, depthTarget, renderer, lighting](FrameGraph& builder, PassHandle passHandle, ThunderboltPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.lighting = lighting;
            data.depth = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
            data.output = passBuilder.readWrite(inputTarget, ResourceState::RenderTarget);
        },
        [](const ThunderboltPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (data.lighting && data.lighting->effective == LightingMode::ReferencePT && data.lighting->recorded)
                return;
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
            auto framebuffer = GetPassResourceCache().GetOrCreateFramebuffer(fbDesc, cmdList->getDevice());

            data.renderer->Draw(cmdList, framebuffer);
        });

    return passData.output;
}
} // namespace xray::render::fg::passes
