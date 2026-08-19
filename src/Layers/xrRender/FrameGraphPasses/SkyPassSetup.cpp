#include "stdafx.h"
#include "SkyPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgEnvironmentRender.h"

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    FGEnvironmentRender* renderer,
    u32 width,
    u32 height,
    bool composite)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<SkyPassData>(
        composite ? "SkyComposite" : "Sky",
        [colorInput, depthInput, renderer, width, height, composite](FrameGraph& builder, PassHandle passHandle, SkyPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.renderer = renderer;
            data.width = width;
            data.height = height;
            data.composite = composite;
            data.colorOutput = passBuilder.write(colorInput, ResourceState::RenderTarget);
            data.depthOutput = passBuilder.read(depthInput, ResourceState::ShaderResource);
        },
        [](const SkyPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (!data.renderer) return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            if (!cmdList || !colorRT) return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            CEnvironment* environment = g_pGamePersistent ? &g_pGamePersistent->Environment() : nullptr;
            if (!environment) return;

            nvrhi::ITexture* depthRT = nullptr;
            if (data.depthOutput.is_valid())
                depthRT = fg.GetPhysicalTexture(data.depthOutput);

            data.renderer->DrawSky(cmdList, framebuffer, environment, data.width, data.height, depthRT, data.composite);
            data.renderer->DrawClouds(cmdList, framebuffer, environment, data.width, data.height);
        });

    return passData.colorOutput;
}

} // namespace xray::render::fg::passes
