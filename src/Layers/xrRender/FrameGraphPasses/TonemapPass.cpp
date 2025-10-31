// xrRender/FrameGraphPasses/TonemapPass.cpp
#include "stdafx.h"
#include "TonemapPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

TonemapPass::TonemapPass(ng::RenderDevice* device, const TonemapPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    Msg("* [TonemapPass] Created (exposure: %.2f, gamma: %.2f)",
        config.exposure, config.gamma);

    // Load shaders
    if (!LoadShaders())
    {
        Msg("! [TonemapPass] Failed to load shaders");
    }
}

TonemapPass::~TonemapPass() {
    Msg("* [TonemapPass] Destroyed");
}

bool TonemapPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    // Load fullscreen vertex shader
    m_vertexShader = loader.LoadVertexShader("fullscreen");
    if (!m_vertexShader)
    {
        Msg("! [TonemapPass] Failed to load fullscreen vertex shader");
        return false;
    }

    // Load tonemap pixel shader
    m_pixelShader = loader.LoadPixelShader("tonemap");
    if (!m_pixelShader)
    {
        Msg("! [TonemapPass] Failed to load tonemap pixel shader");
        return false;
    }

    Msg("  ✓ Tonemap shaders loaded successfully");
    return true;
}

bool TonemapPass::CreatePipeline(nvrhi::ITexture* backbufferTexture)
{
    VERIFY(m_vertexShader != nullptr);
    VERIFY(m_pixelShader != nullptr);
    VERIFY(backbufferTexture != nullptr);

    // Create graphics pipeline descriptor
    nvrhi::GraphicsPipelineDesc pipelineDesc;

    // Shaders
    pipelineDesc.VS = m_vertexShader;
    pipelineDesc.PS = m_pixelShader;

    // Vertex input - none (fullscreen triangle uses SV_VertexID)
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // Render state
    nvrhi::RasterState& rasterState = pipelineDesc.renderState.rasterState;
    rasterState.cullMode = nvrhi::RasterCullMode::None;
    rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    rasterState.frontCounterClockwise = false;

    // Blend state - no blending (opaque)
    nvrhi::BlendState& blendState = pipelineDesc.renderState.blendState;
    blendState.targets[0].enableBlend = false;
    blendState.targets[0].colorWriteMask = nvrhi::ColorMask::All;

    // Depth/stencil state - no depth (fullscreen)
    nvrhi::DepthStencilState& depthState = pipelineDesc.renderState.depthStencilState;
    depthState.depthTestEnable = false;
    depthState.depthWriteEnable = false;
    depthState.stencilEnable = false;

    // Framebuffer info
    auto backbufferDesc = backbufferTexture->getDesc();
    pipelineDesc.renderState.targetCount = 1;
    pipelineDesc.renderState.renderTargetFormats[0] = backbufferDesc.format;

    // Create pipeline
    m_pipeline = m_device->GetNVRHIDevice()->createGraphicsPipeline(pipelineDesc, nullptr);

    if (!m_pipeline)
    {
        Msg("! [TonemapPass] Failed to create graphics pipeline");
        return false;
    }

    Msg("  ✓ TonemapPass pipeline created successfully");
    return true;
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

    // Create pipeline if needed (once per frame, using actual backbuffer format)
    if (!m_pipeline)
    {
        if (!CreatePipeline(output))
        {
            Msg("! [TonemapPass] Failed to create pipeline");
            ctx.EndRenderPass();
            return;
        }
    }

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
    //  BIND PIPELINE & DRAW
    // ═══════════════════════════════════════════════════════

    // Set pipeline
    ctx.SetPipeline(m_pipeline.Get());

    // TODO: Bind HDR texture as shader resource
    // ctx.SetTexture(0, hdr);

    // TODO: Update constants (exposure, gamma)
    // ctx.UpdateConstantBuffer(0, &m_config, sizeof(m_config));

    // Draw fullscreen triangle
    ctx.Draw(3, 0);

    Msg("  ✓ Tonemap draw complete (3 vertices)");

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
