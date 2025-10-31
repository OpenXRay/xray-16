// xrRender/FrameGraphPasses/LightingPass.cpp
#include "stdafx.h"
#include "LightingPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

LightingPass::LightingPass(ng::RenderDevice* device, const LightingPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    Msg("* [LightingPass] Created (%ux%u)", config.width, config.height);

    // Load shaders
    if (!LoadShaders())
    {
        Msg("! [LightingPass] Failed to load shaders");
    }
}

LightingPass::~LightingPass() {
    Msg("* [LightingPass] Destroyed");
}

bool LightingPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    // Load fullscreen vertex shader (same as tonemap)
    m_vertexShader = loader.LoadVertexShader("fullscreen");
    if (!m_vertexShader)
    {
        Msg("! [LightingPass] Failed to load fullscreen vertex shader");
        return false;
    }

    // Load lighting pixel shader
    m_pixelShader = loader.LoadPixelShader("lighting");
    if (!m_pixelShader)
    {
        Msg("! [LightingPass] Failed to load lighting pixel shader");
        return false;
    }

    Msg("  ✓ Lighting shaders loaded successfully");
    return true;
}

LightingPassOutput LightingPass::Setup(
    FrameGraph& fg,
    const GBufferOutputs& gbuffer
) {
    Msg("~ [LightingPass] Setting up in FrameGraph");

    LightingPassOutput output;

    // ═══════════════════════════════════════════════════════
    //  CREATE HDR BUFFER
    // ═══════════════════════════════════════════════════════

    ResourceDesc hdrDesc;
    hdrDesc.type = ResourceDesc::Type::Texture2D;
    hdrDesc.width = m_config.width;
    hdrDesc.height = m_config.height;
    hdrDesc.format = m_config.hdrFormat;
    hdrDesc.isRenderTarget = true;
    hdrDesc.isTransient = true;
    hdrDesc.debugName = "HDR";

    output.hdrColor = fg.CreateTexture("HDR", hdrDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE LIGHTING PASS
    // ═══════════════════════════════════════════════════════

    PassHandle lightingPass = fg.AddPass("Lighting");

    // Read from G-Buffer
    fg.PassRead(lightingPass, gbuffer.albedo, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.normal, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.material, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.depth, ResourceState::ShaderResource);

    // Write to HDR buffer
    fg.PassWrite(lightingPass, output.hdrColor, ResourceState::RenderTarget);

    // Set execution callback
    fg.SetPassCallback(lightingPass, [this, gbuffer, output](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, gbuffer, output);
    });

    Msg("  ✓ Lighting pass configured");

    return output;
}

void LightingPass::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    const GBufferOutputs& gbuffer,
    const LightingPassOutput& output
) {
    Msg("~ [LightingPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    // Reset statistics
    m_stats = Stats{};

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(gbuffer.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(gbuffer.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(gbuffer.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(gbuffer.depth);
    nvrhi::ITexture* hdr = fg.GetPhysicalTexture(output.hdrColor);

    // ═══════════════════════════════════════════════════════
    //  SETUP RENDER PASS
    // ═══════════════════════════════════════════════════════

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = hdr;
    passDesc.numRenderTargets = 1;
    passDesc.clearColor = true;
    passDesc.clearValue.color[0] = 0.0f;
    passDesc.clearValue.color[1] = 0.0f;
    passDesc.clearValue.color[2] = 0.0f;
    passDesc.clearValue.color[3] = 1.0f;

    ctx.BeginRenderPass(passDesc);

    // Set viewport
    ctx.SetViewport(0, 0,
        static_cast<float>(m_config.width),
        static_cast<float>(m_config.height));

    // Set scissor
    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  BIND G-BUFFER TEXTURES & DRAW
    // ═══════════════════════════════════════════════════════

    // TODO: Bind G-Buffer textures through RenderContext
    // ctx.SetTexture(0, albedo);
    // ctx.SetTexture(1, normal);
    // ctx.SetTexture(2, material);
    // ctx.SetTexture(3, depth);

    // TODO: Set pipeline
    // ctx.SetPipeline(m_pipeline);

    // TODO: Update per-frame constants (camera, lights)
    // ctx.UpdateConstantBuffer(0, &constants, sizeof(constants));

    // Draw fullscreen triangle
    // TODO: Need to set up pipeline, textures, and constants
    // For now, skip drawing until pipeline is created
    // ctx.Draw(3, 0);

    if (m_vertexShader && m_pixelShader)
    {
        Msg("  (Shaders loaded but pipeline not yet created)");
    }
    else
    {
        Msg("  (Skipping lighting draw - shaders not loaded)");
    }

    ctx.EndRenderPass();

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    m_stats.numLights = 1;  // Currently just sun

    Msg("  ✓ Lighting pass complete: %.2f ms", m_stats.cpuTimeMs);
}

} // namespace xray::render::passes
