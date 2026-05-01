#include "stdafx.h"
#include "StatGraphPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgStatGraphRender.h"

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupStatGraphPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    FGStatGraphRender* renderer,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<StatGraphPassData>(
        "StatGraph",
        [inputTarget, renderer, width, height](FrameGraph& builder, PassHandle passHandle, StatGraphPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.width = width;
            data.height = height;
            data.output = passBuilder.write(inputTarget, ResourceState::RenderTarget);
        },
        [](const StatGraphPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {
            if (!data.renderer || !data.renderer->HasWork())
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* outputRT = fg.GetPhysicalTexture(data.output);
            if (!cmdList || !outputRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            data.renderer->Draw(cmdList, framebuffer, data.width, data.height);
        });

    return passData.output;
}

}
