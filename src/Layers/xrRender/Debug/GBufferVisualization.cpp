// xrRender/Debug/GBufferVisualization.cpp
#include "stdafx.h"
#include "GBufferVisualization.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::debug {

using namespace framegraph;

GBufferVisualizer::GBufferVisualizer() {
    Msg("* [GBufferVisualizer] Created");

    // TODO: Load and compile visualization shaders
    // LoadShaders();
}

GBufferVisualizer::~GBufferVisualizer() {
    Msg("* [GBufferVisualizer] Destroyed");
}

void GBufferVisualizer::Setup(
    FrameGraph& fg,
    const passes::GBufferOutputs& gbuffer,
    VirtualResourceHandle backbuffer
) {
    // Only setup visualization pass if mode is not Off
    if (m_mode == GBufferVisMode::Off) {
        return;
    }

    Msg("~ [GBufferVisualizer] Setting up visualization pass (mode: %s)",
        GetModeName(m_mode));

    // Create visualization pass
    PassHandle visPass = fg.AddPass("GBufferVisualization");

    // Read from G-Buffer
    fg.PassRead(visPass, gbuffer.albedo, ResourceState::ShaderResource);
    fg.PassRead(visPass, gbuffer.normal, ResourceState::ShaderResource);
    fg.PassRead(visPass, gbuffer.material, ResourceState::ShaderResource);
    fg.PassRead(visPass, gbuffer.depth, ResourceState::ShaderResource);

    // Write to backbuffer
    fg.PassWrite(visPass, backbuffer, ResourceState::RenderTarget);

    // Set execution callback
    fg.SetPassCallback(visPass, [this, gbuffer, backbuffer](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, gbuffer, backbuffer);
    });

    Msg("  ✓ G-Buffer visualization pass configured");
}

void GBufferVisualizer::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    const passes::GBufferOutputs& gbuffer,
    VirtualResourceHandle backbuffer
) {
    Msg("~ [GBufferVisualizer] Executing (mode: %s)", GetModeName(m_mode));

    // Get physical resources
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(gbuffer.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(gbuffer.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(gbuffer.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(gbuffer.depth);
    nvrhi::ITexture* output = fg.GetPhysicalTexture(backbuffer);

    // Setup render pass
    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = output;
    passDesc.numRenderTargets = 1;
    passDesc.clearColor = false;  // Don't clear, we're drawing fullscreen

    ctx.BeginRenderPass(passDesc);

    // Set viewport (fullscreen)
    auto desc = output->getDesc();
    ctx.SetViewport(0, 0, (float)desc.width, (float)desc.height);

    // Bind G-Buffer textures
    // TODO: Bind textures properly through RenderContext
    // ctx.SetTexture(0, albedo);
    // ctx.SetTexture(1, normal);
    // ctx.SetTexture(2, material);
    // ctx.SetTexture(3, depth);

    // Set pipeline
    // TODO: Set visualization pipeline
    // ctx.SetPipeline(m_pipeline);

    // Update constants (visualization mode)
    // TODO: Update constant buffer with mode
    // ctx.UpdateConstantBuffer(0, &m_mode, sizeof(m_mode));

    // Draw fullscreen triangle
    ctx.Draw(3, 0);

    ctx.EndRenderPass();

    Msg("  ✓ G-Buffer visualization complete");
}

void GBufferVisualizer::CycleMode() {
    u32 nextMode = (static_cast<u32>(m_mode) + 1) % 7;
    m_mode = static_cast<GBufferVisMode>(nextMode);

    Msg("* G-Buffer visualization mode: %s", GetModeName(m_mode));
}

const char* GBufferVisualizer::GetModeName(GBufferVisMode mode) {
    switch (mode) {
        case GBufferVisMode::Off: return "Off";
        case GBufferVisMode::Albedo: return "Albedo";
        case GBufferVisMode::Normal: return "Normal";
        case GBufferVisMode::Depth: return "Depth";
        case GBufferVisMode::Metallic: return "Metallic";
        case GBufferVisMode::Roughness: return "Roughness";
        case GBufferVisMode::MaterialID: return "Material ID";
        default: return "Unknown";
    }
}

} // namespace xray::render::debug
