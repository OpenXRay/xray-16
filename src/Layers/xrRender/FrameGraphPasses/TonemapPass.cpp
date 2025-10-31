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

    PassHandle tonemapPass = fg.AddPass("Tonemap");

    fg.PassRead(tonemapPass, hdrInput, ResourceState::ShaderResource);

    fg.PassWrite(tonemapPass, backbuffer, ResourceState::RenderTarget);

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

    m_stats = Stats{};

    nvrhi::ITexture* hdr = fg.GetPhysicalTexture(hdrInput);
    nvrhi::ITexture* output = fg.GetPhysicalTexture(backbuffer);

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = output;
    passDesc.numRenderTargets = 1;
    passDesc.clearColor = false;

    ctx.BeginRenderPass(passDesc);

    auto desc = output->getDesc();
    ctx.SetViewport(0, 0, (float)desc.width, (float)desc.height);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = desc.width;
    scissor.height = desc.height;
    ctx.SetScissor(scissor);

    Msg("  (Skipping tonemap draw - pipeline not yet implemented)");

    ctx.EndRenderPass();

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    Msg("  ✓ Tonemap pass complete: %.2f ms", m_stats.cpuTimeMs);
}

}
