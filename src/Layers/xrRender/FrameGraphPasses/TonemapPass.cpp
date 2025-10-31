// xrRender/FrameGraphPasses/TonemapPass.cpp
#include "stdafx.h"
#include "TonemapPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::passes {

using namespace framegraph;

TonemapPass::TonemapPass(const TonemapPassConfig& config)
    : m_config(config)
{
    Msg("* [TonemapPass] Created (exposure: %.2f, gamma: %.2f)",
        config.exposure, config.gamma);

    // TODO: Load and compile shaders
    // LoadShaders();
}

TonemapPass::~TonemapPass() {
    Msg("* [TonemapPass] Destroyed");
}

void TonemapPass::Setup(
    FrameGraph& fg,
    VirtualResourceHandle hdrInput,
    VirtualResourceHandle backbuffer
) {
    Msg("~ [TonemapPass] Setting up in FrameGraph");

    // ═══════════════════════════════════════════════════════
    //  CREATE TONEMAP PASS
    // ═══════════════════════════════════════════════════════

    PassHandle tonemapPass = fg.AddPass("Tonemap");

    // Read HDR input
    fg.PassRead(tonemapPass, hdrInput, ResourceState::ShaderResource);

    // Write to backbuffer
    fg.PassWrite(tonemapPass, backbuffer, ResourceState::RenderTarget);

    // Set execution callback
    fg.SetPassCallback(tonemapPass, [this, hdrInput, backbuffer](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, hdrInput, backbuffer);
    });

    Msg("  ✓ Tonemap pass configured");
}

void TonemapPass::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    VirtualResourceHandle hdrInput,
    VirtualResourceHandle backbuffer
) {
    Msg("~ [TonemapPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    // Reset statistics
    m_stats = Stats{};

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* hdr = fg.GetPhysicalTexture(hdrInput);
    nvrhi::ITexture* output = fg.GetPhysicalTexture(backbuffer);

    // ═══════════════════════════════════════════════════════
    //  SETUP RENDER PASS
    // ═══════════════════════════════════════════════════════

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = output;
    passDesc.numRenderTargets = 1;
    passDesc.clearColor = false;  // Don't clear, drawing fullscreen

    ctx.BeginRenderPass(passDesc);

    // Set viewport
    auto desc = output->getDesc();
    ctx.SetViewport(0, 0, (float)desc.width, (float)desc.height);

    // Set scissor
    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = desc.width;
    scissor.height = desc.height;
    ctx.SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  BIND TEXTURES & DRAW
    // ═══════════════════════════════════════════════════════

    // TODO: Bind HDR texture
    // ctx.SetTexture(0, hdr);

    // TODO: Set pipeline
    // ctx.SetPipeline(m_pipeline);

    // TODO: Update constants (exposure, gamma)
    // ctx.UpdateConstantBuffer(0, &m_config, sizeof(m_config));

    // Draw fullscreen triangle
    // TODO: Skip drawing until pipeline is set up
    // ctx.Draw(3, 0);

    Msg("  (Skipping tonemap draw - pipeline not yet implemented)");

    ctx.EndRenderPass();

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    Msg("  ✓ Tonemap pass complete: %.2f ms", m_stats.cpuTimeMs);
}

} // namespace xray::render::passes
