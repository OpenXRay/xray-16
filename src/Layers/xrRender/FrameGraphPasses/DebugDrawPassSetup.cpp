#include "stdafx.h"
#include "DebugDrawPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FGDebugDraw.h"

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupDebugDrawPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<DebugDrawPassData>(
        "DebugDraw",
        [inputTarget, width, height](FrameGraph& builder, PassHandle passHandle, DebugDrawPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.output = passBuilder.readWrite(inputTarget, ResourceState::RenderTarget);
        },
        [](const DebugDrawPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {
            if (!g_debug_draw.HasWork())
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* outputRT = fg.GetPhysicalTexture(data.output);
            if (!cmdList || !outputRT)
            {
                g_debug_draw.Clear();
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            Fmatrix vp;
            vp.mul(Device.mProject, Device.mView);
            g_debug_draw.Render(cmdList, framebuffer, vp);
        });

    return passData.output;
}

}
