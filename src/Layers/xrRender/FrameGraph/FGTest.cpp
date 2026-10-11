#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

namespace fg = xray::render::fg;

void TestSimpleTriangle(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Triangle Test ===");

    FrameGraph fg(renderDevice);
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
    backbufferHandle = fg.PassWrite(clearPass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(clearPass, [backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        fg::RenderPassDesc passDesc;
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
    backbufferHandle = fg.PassWrite(trianglePass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(trianglePass, [backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        fg::RenderPassDesc passDesc;
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

void TestResourceAliasing(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Resource Aliasing Test ===");

    FrameGraph fg(renderDevice);
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

    ResourceDesc temp1Desc;
    temp1Desc.type = ResourceDesc::Type::Texture2D;
    temp1Desc.width = bbDesc.width;
    temp1Desc.height = bbDesc.height;
    temp1Desc.format = nvrhi::Format::RGBA16_FLOAT;
    temp1Desc.isRenderTarget = false;
    temp1Desc.isUAV = true;
    temp1Desc.isTransient = true;
    temp1Desc.debugName = "Temp1_BlurH";

    auto temp1 = fg.CreateTexture("Temp1", temp1Desc);

    ResourceDesc temp2Desc;
    temp2Desc.type = ResourceDesc::Type::Texture2D;
    temp2Desc.width = bbDesc.width;
    temp2Desc.height = bbDesc.height;
    temp2Desc.format = nvrhi::Format::RGBA16_FLOAT;
    temp2Desc.isRenderTarget = false;
    temp2Desc.isUAV = true;
    temp2Desc.isTransient = true;
    temp2Desc.debugName = "Temp2_BlurV";

    auto temp2 = fg.CreateTexture("Temp2", temp2Desc);

    ResourceDesc temp3Desc;
    temp3Desc.type = ResourceDesc::Type::Texture2D;
    temp3Desc.width = bbDesc.width;
    temp3Desc.height = bbDesc.height;
    temp3Desc.format = nvrhi::Format::RGBA16_FLOAT;
    temp3Desc.isRenderTarget = false;
    temp3Desc.isUAV = true;
    temp3Desc.isTransient = true;
    temp3Desc.debugName = "Temp3_Bloom";

    auto temp3 = fg.CreateTexture("Temp3", temp3Desc);

    auto pass1 = fg.AddPass("HorizontalBlur");
    fg.PassRead(pass1, backbufferHandle, ResourceState::ShaderResource);
    temp1 = fg.PassWrite(pass1, temp1, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass1, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 1: Horizontal Blur (compute shader would go here)");
    });

    auto pass2 = fg.AddPass("VerticalBlur");
    fg.PassRead(pass2, temp1, ResourceState::ShaderResource);
    temp2 = fg.PassWrite(pass2, temp2, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass2, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 2: Vertical Blur");
    });

    auto pass3 = fg.AddPass("BloomThreshold");
    fg.PassRead(pass3, temp2, ResourceState::ShaderResource);
    temp3 = fg.PassWrite(pass3, temp3, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass3, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 3: Bloom Threshold");
    });

    auto pass4 = fg.AddPass("Composite");
    fg.PassRead(pass4, temp3, ResourceState::ShaderResource);
    fg.PassRead(pass4, backbufferHandle, ResourceState::RenderTarget);
    backbufferHandle = fg.PassWrite(pass4, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(pass4, [backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        fg::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = false;

        ctx.BeginRenderPass(passDesc);
        Msg("  ~ Pass 4: Composite (final output)");
        ctx.EndRenderPass();
    });

    Msg("~ Compiling FrameGraph with aliasing enabled...");
    fg.Compile();

    fg.PrintExecutionOrder();

    Msg("~ Executing FrameGraph...");
    fg.Execute();

    fg.PrintStatistics();

    Msg("=== FrameGraph Resource Aliasing Test Complete ===");
    Msg("! Expected: temp1, temp2, temp3 should alias (reuse same memory)");
    Msg("! Memory saved = ~2x texture size (3 textures using 1 allocation)");
}

}
