#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

namespace ng = xray::render::ng;

void TestSimpleTriangle(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Triangle Test ===");

    FrameGraph fg(device);
    fg.SetRenderContext(context);

    const nvrhi::TextureDesc& bbDesc = backbuffer->getDesc();

    ResourceDesc backbufferDesc;
    backbufferDesc.type = ResourceDesc::Type::Texture2D;
    backbufferDesc.width = bbDesc.width;
    backbufferDesc.height = bbDesc.height;
    backbufferDesc.format = bbDesc.format;
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.isImported = true;
    backbufferDesc.isTransient = false;
    backbufferDesc.debugName = "Backbuffer";

    auto backbufferHandle = fg.ImportTexture("Backbuffer", backbuffer, backbufferDesc);

    auto clearPass = fg.AddPass("Clear");
    fg.PassWrite(clearPass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(clearPass, [backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 0.1f;
        passDesc.clearValue.color[1] = 0.1f;
        passDesc.clearValue.color[2] = 0.2f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        ctx.EndRenderPass();
    });

    auto trianglePass = fg.AddPass("RenderTriangle");
    fg.PassRead(trianglePass, backbufferHandle, ResourceState::RenderTarget);
    fg.PassWrite(trianglePass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(trianglePass, [backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = false;

        ctx.BeginRenderPass(passDesc);

        Msg("  ~ Drawing triangle (TODO: implement actual draw)");

        ctx.EndRenderPass();
    });

    Msg("~ Compiling FrameGraph...");
    fg.Compile();

    fg.PrintExecutionOrder();

    Msg("~ Executing FrameGraph...");
    fg.Execute();

    fg.PrintStatistics();

    Msg("=== FrameGraph Triangle Test Complete ===");
}

void TestTwoPassRender(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Two-Pass Test ===");

    FrameGraph fg(device);
    fg.SetRenderContext(context);

    const nvrhi::TextureDesc& bbDesc = backbuffer->getDesc();

    ResourceDesc backbufferDesc;
    backbufferDesc.type = ResourceDesc::Type::Texture2D;
    backbufferDesc.width = bbDesc.width;
    backbufferDesc.height = bbDesc.height;
    backbufferDesc.format = bbDesc.format;
    backbufferDesc.isRenderTarget = true;
    backbufferDesc.isImported = true;
    backbufferDesc.isTransient = false;
    backbufferDesc.debugName = "Backbuffer";

    auto backbufferHandle = fg.ImportTexture("Backbuffer", backbuffer, backbufferDesc);

    ResourceDesc hdrDesc;
    hdrDesc.type = ResourceDesc::Type::Texture2D;
    hdrDesc.width = bbDesc.width;
    hdrDesc.height = bbDesc.height;
    hdrDesc.format = nvrhi::Format::RGBA16_FLOAT;
    hdrDesc.isRenderTarget = true;
    hdrDesc.isTransient = true;
    hdrDesc.debugName = "HDR Buffer";

    auto hdrBuffer = fg.CreateTexture("HDR", hdrDesc);

    auto renderPass = fg.AddPass("RenderHDR");
    fg.PassWrite(renderPass, hdrBuffer, ResourceState::RenderTarget);
    fg.SetPassCallback(renderPass, [hdrBuffer](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(hdrBuffer);

        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 1.0f;
        passDesc.clearValue.color[1] = 0.5f;
        passDesc.clearValue.color[2] = 0.2f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        Msg("  ~ Rendering to HDR buffer");

        ctx.EndRenderPass();
    });

    auto tonemapPass = fg.AddPass("Tonemap");
    fg.PassRead(tonemapPass, hdrBuffer, ResourceState::ShaderResource);
    fg.PassWrite(tonemapPass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(tonemapPass, [hdrBuffer, backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);
        nvrhi::ITexture* hdrTex = fg.GetPhysicalTexture(hdrBuffer);

        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 0.2f;
        passDesc.clearValue.color[1] = 0.1f;
        passDesc.clearValue.color[2] = 0.05f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        Msg("  ~ Tonemapping (TODO: sample HDR texture)");

        ctx.EndRenderPass();
    });

    Msg("~ Compiling FrameGraph...");
    fg.Compile();

    fg.PrintExecutionOrder();

    Msg("~ Executing FrameGraph...");
    fg.Execute();

    fg.PrintStatistics();

    Msg("=== FrameGraph Two-Pass Test Complete ===");
}

}
