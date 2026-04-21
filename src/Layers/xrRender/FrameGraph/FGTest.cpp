// xrRender/FrameGraph/FGTest.cpp
// FrameGraph test cases
#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

// Forward declaration
namespace fg = xray::render::fg;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  SIMPLE TRIANGLE TEST
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void TestSimpleTriangle(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Triangle Test ===");

    // Create FrameGraph
    FrameGraph fg(renderDevice);
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
    fg.SetPassCallback(clearPass, [backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        // Begin render pass with clear
        fg::RenderPassDesc passDesc;
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
    fg.SetPassCallback(trianglePass, [backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);

        // Begin render pass (no clear)
        fg::RenderPassDesc passDesc;
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

void TestTwoPassRender(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Two-Pass Test ===");

    FrameGraph fg(renderDevice);
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
    fg.SetPassCallback(renderPass, [hdrBuffer](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(hdrBuffer);

        // Begin render pass with clear to bright orange
        fg::RenderPassDesc passDesc;
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
    fg.SetPassCallback(tonemapPass, [hdrBuffer, backbufferHandle](fg::RenderContext& ctx, const FrameGraph& fg) {
        nvrhi::ITexture* rt = fg.GetPhysicalTexture(backbufferHandle);
        nvrhi::ITexture* hdrTex = fg.GetPhysicalTexture(hdrBuffer);

        // Begin render pass with clear to dark brown (to distinguish from HDR pass)
        fg::RenderPassDesc passDesc;
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

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  ALIASING TEST (Multiple Transient Resources)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void TestResourceAliasing(fg::RenderDevice* renderDevice, fg::RenderContext* context, nvrhi::ITexture* backbuffer) {
    Msg("=== FrameGraph Resource Aliasing Test ===");

    FrameGraph fg(renderDevice);
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

    // Create multiple transient intermediate textures
    // These will have non-overlapping lifetimes, enabling aliasing

    // Intermediate texture 1: Used in Pass 1 only (NOT a render target to allow aliasing)
    ResourceDesc temp1Desc;
    temp1Desc.type = ResourceDesc::Type::Texture2D;
    temp1Desc.width = bbDesc.width;
    temp1Desc.height = bbDesc.height;
    temp1Desc.format = nvrhi::Format::RGBA16_FLOAT;
    temp1Desc.isRenderTarget = false;  // Mark as non-RT to allow aliasing
    temp1Desc.isUAV = true;            // Use UAV instead
    temp1Desc.isTransient = true;
    temp1Desc.debugName = "Temp1_BlurH";

    auto temp1 = fg.CreateTexture("Temp1", temp1Desc);

    // Intermediate texture 2: Used in Pass 2 only
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

    // Intermediate texture 3: Used in Pass 3 only
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

    // Pass 1: Read backbuffer, write temp1 (horizontal blur)
    auto pass1 = fg.AddPass("HorizontalBlur");
    fg.PassRead(pass1, backbufferHandle, ResourceState::ShaderResource);
    fg.PassWrite(pass1, temp1, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass1, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 1: Horizontal Blur (compute shader would go here)");
    });

    // Pass 2: Read temp1, write temp2 (vertical blur)
    // temp1 lifetime ends here, so its memory can be reused
    auto pass2 = fg.AddPass("VerticalBlur");
    fg.PassRead(pass2, temp1, ResourceState::ShaderResource);
    fg.PassWrite(pass2, temp2, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass2, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 2: Vertical Blur");
    });

    // Pass 3: Read temp2, write temp3 (bloom threshold)
    // temp2 lifetime ends here, so its memory can be reused
    auto pass3 = fg.AddPass("BloomThreshold");
    fg.PassRead(pass3, temp2, ResourceState::ShaderResource);
    fg.PassWrite(pass3, temp3, ResourceState::UnorderedAccess);
    fg.SetPassCallback(pass3, [](fg::RenderContext& ctx, const FrameGraph& fg) {
        Msg("  ~ Pass 3: Bloom Threshold");
    });

    // Pass 4: Read temp3 and backbuffer, write to backbuffer (final composite)
    // temp3 lifetime ends here
    auto pass4 = fg.AddPass("Composite");
    fg.PassRead(pass4, temp3, ResourceState::ShaderResource);
    fg.PassRead(pass4, backbufferHandle, ResourceState::RenderTarget);
    fg.PassWrite(pass4, backbufferHandle, ResourceState::RenderTarget);
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

    // Compile
    Msg("~ Compiling FrameGraph with aliasing enabled...");
    fg.Compile();

    // Print execution order and resource lifetimes
    fg.PrintExecutionOrder();

    // Execute
    Msg("~ Executing FrameGraph...");
    fg.Execute();

    // Print statistics - should show aliasing savings!
    fg.PrintStatistics();

    Msg("=== FrameGraph Resource Aliasing Test Complete ===");
    Msg("! Expected: temp1, temp2, temp3 should alias (reuse same memory)");
    Msg("! Memory saved = ~2x texture size (3 textures using 1 allocation)");
}

} // namespace xray::render::framegraph
