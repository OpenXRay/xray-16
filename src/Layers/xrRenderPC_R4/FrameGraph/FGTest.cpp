// xrRender/FrameGraph/FGTest.cpp
// FrameGraph test cases
#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

// Forward declaration
namespace ng = xray::render::ng;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  SIMPLE TRIANGLE TEST
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void TestSimpleTriangle(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Triangle Test ===");

    // Create FrameGraph
    FrameGraph fg(device);
    fg.SetRenderContext(context);

    // Get backbuffer dimensions
    const nvrhi::TextureDesc& bbDesc = backbuffer->getDesc();

    // Import backbuffer as virtual resource
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

    // Add clear pass
    auto clearPass = fg.AddPass("Clear");
    fg.PassWrite(clearPass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(clearPass, [backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        // Begin render pass with clear
        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 0.1f;  // Dark blue
        passDesc.clearValue.color[1] = 0.1f;
        passDesc.clearValue.color[2] = 0.2f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        // Nothing to render in clear pass

        ctx.EndRenderPass();
    });

    // Add triangle render pass
    auto trianglePass = fg.AddPass("RenderTriangle");
    fg.PassRead(trianglePass, backbufferHandle, ResourceState::RenderTarget);
    fg.PassWrite(trianglePass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(trianglePass, [backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        // Begin render pass (no clear)
        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = false;

        ctx.BeginRenderPass(passDesc);

        // TODO: Draw triangle (need to get pipeline, vertex buffer, etc from somewhere)
        // For now, just a placeholder
        Msg("  ~ Drawing triangle (TODO: implement actual draw)");

        ctx.EndRenderPass();
    });

    // Compile
    Msg("~ Compiling FrameGraph...");
    fg.Compile();

    // Print execution order
    fg.PrintExecutionOrder();

    // Execute
    Msg("~ Executing FrameGraph...");
    fg.Execute();

    // Print statistics
    fg.PrintStatistics();

    Msg("=== FrameGraph Triangle Test Complete ===");
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  TWO-PASS TEST (SIMPLE G-BUFFER STYLE)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void TestTwoPassRender(nvrhi::IDevice* device, ng::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Two-Pass Test ===");

    // Create FrameGraph
    FrameGraph fg(device);
    fg.SetRenderContext(context);

    // Get backbuffer dimensions
    const nvrhi::TextureDesc& bbDesc = backbuffer->getDesc();

    // Import backbuffer
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

    // Create intermediate HDR buffer
    ResourceDesc hdrDesc;
    hdrDesc.type = ResourceDesc::Type::Texture2D;
    hdrDesc.width = bbDesc.width;
    hdrDesc.height = bbDesc.height;
    hdrDesc.format = nvrhi::Format::RGBA16_FLOAT;
    hdrDesc.isRenderTarget = true;
    hdrDesc.isTransient = true;
    hdrDesc.debugName = "HDR Buffer";

    auto hdrBuffer = fg.CreateTexture("HDR", hdrDesc);

    // Pass 1: Render to HDR buffer
    auto renderPass = fg.AddPass("RenderHDR");
    fg.PassWrite(renderPass, hdrBuffer, ResourceState::RenderTarget);
    fg.SetPassCallback(renderPass, [hdrBuffer](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(hdrBuffer);

        // Begin render pass with clear to bright orange
        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 1.0f;  // Bright orange
        passDesc.clearValue.color[1] = 0.5f;
        passDesc.clearValue.color[2] = 0.2f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        Msg("  ~ Rendering to HDR buffer");

        ctx.EndRenderPass();
    });

    // Pass 2: Tonemap HDR → Backbuffer
    auto tonemapPass = fg.AddPass("Tonemap");
    fg.PassRead(tonemapPass, hdrBuffer, ResourceState::ShaderResource);
    fg.PassWrite(tonemapPass, backbufferHandle, ResourceState::RenderTarget);
    fg.SetPassCallback(tonemapPass, [hdrBuffer, backbufferHandle](ng::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);
        nvrhi::ITexture* hdrTex = fg.GetPhysicalTexture(hdrBuffer);

        // Begin render pass with clear to dark brown (to distinguish from HDR pass)
        ng::RenderPassDesc passDesc;
        passDesc.renderTargets[0] = rt;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 0.2f;  // Dark brown
        passDesc.clearValue.color[1] = 0.1f;
        passDesc.clearValue.color[2] = 0.05f;
        passDesc.clearValue.color[3] = 1.0f;

        ctx.BeginRenderPass(passDesc);

        Msg("  ~ Tonemapping (TODO: sample HDR texture)");

        ctx.EndRenderPass();
    });

    // Compile
    Msg("~ Compiling FrameGraph...");
    fg.Compile();

    // Print execution order
    fg.PrintExecutionOrder();

    // Execute
    Msg("~ Executing FrameGraph...");
    fg.Execute();

    // Print statistics
    fg.PrintStatistics();

    Msg("=== FrameGraph Two-Pass Test Complete ===");
}

} // namespace xray::render::framegraph
