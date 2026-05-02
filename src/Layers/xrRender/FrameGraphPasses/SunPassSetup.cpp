#include "stdafx.h"
#include "SunPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgEnvironmentRender.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorInput,
    FGEnvironmentRender* renderer,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<SunPassData>(
        "Sun",
        [colorInput, renderer, width, height](FrameGraph& builder, PassHandle passHandle, SunPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.width = width;
            data.height = height;
            data.colorOutput = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
        },
        [](const SunPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (!data.renderer) return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            if (!cmdList || !colorRT) return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            CEnvironment* environment = g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr;
            if (!environment) return;

            data.renderer->DrawSun(cmdList, framebuffer, environment, data.width, data.height);
        });

    return passData.colorOutput;
}

} // namespace xray::render::fg::passes
