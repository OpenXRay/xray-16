// xrRender/FrameGraphPasses/GBufferPass.cpp
#include "stdafx.h"
#include "GBufferPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::passes {

using namespace framegraph;

GBufferPass::GBufferPass(const GBufferPassConfig& config)
    : m_config(config)
{
    Msg("* [GBufferPass] Created (%ux%u)", config.width, config.height);
}

GBufferPass::~GBufferPass() {
    Msg("* [GBufferPass] Destroyed");
}

GBufferOutputs GBufferPass::Setup(FrameGraph& fg) {
    Msg("~ [GBufferPass] Setting up in FrameGraph");

    GBufferOutputs outputs;

    // ═══════════════════════════════════════════════════════
    //  CREATE G-BUFFER RESOURCES
    // ═══════════════════════════════════════════════════════

    // Albedo + Metallic
    ResourceDesc albedoDesc;
    albedoDesc.type = ResourceDesc::Type::Texture2D;
    albedoDesc.width = m_config.width;
    albedoDesc.height = m_config.height;
    albedoDesc.format = m_config.albedoFormat;
    albedoDesc.isRenderTarget = true;
    albedoDesc.isTransient = true;
    albedoDesc.debugName = "GBuffer.Albedo";

    outputs.albedo = fg.CreateTexture("GBuffer.Albedo", albedoDesc);

    // Normal + Roughness
    ResourceDesc normalDesc;
    normalDesc.type = ResourceDesc::Type::Texture2D;
    normalDesc.width = m_config.width;
    normalDesc.height = m_config.height;
    normalDesc.format = m_config.normalFormat;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    normalDesc.debugName = "GBuffer.Normal";

    outputs.normal = fg.CreateTexture("GBuffer.Normal", normalDesc);

    // Material ID
    ResourceDesc materialDesc;
    materialDesc.type = ResourceDesc::Type::Texture2D;
    materialDesc.width = m_config.width;
    materialDesc.height = m_config.height;
    materialDesc.format = m_config.materialFormat;
    materialDesc.isRenderTarget = true;
    materialDesc.isTransient = true;
    materialDesc.debugName = "GBuffer.Material";

    outputs.material = fg.CreateTexture("GBuffer.Material", materialDesc);

    // Depth + Stencil
    ResourceDesc depthDesc;
    depthDesc.type = ResourceDesc::Type::Texture2D;
    depthDesc.width = m_config.width;
    depthDesc.height = m_config.height;
    depthDesc.format = m_config.depthFormat;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;
    depthDesc.debugName = "GBuffer.Depth";

    outputs.depth = fg.CreateTexture("GBuffer.Depth", depthDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE GBUFFER PASS
    // ═══════════════════════════════════════════════════════

    PassHandle gbufferPass = fg.AddPass("GBuffer");

    // Declare resource writes (render targets)
    fg.PassWrite(gbufferPass, outputs.albedo, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.normal, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.material, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.depth, ResourceState::DepthStencilWrite);

    // Set execution callback
    fg.SetPassCallback(gbufferPass, [this, outputs](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, outputs);
    });

    Msg("  ✓ G-Buffer pass configured");

    return outputs;
}

void GBufferPass::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    const GBufferOutputs& outputs
) {
    Msg("~ [GBufferPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    // Reset statistics
    m_stats = Stats{};

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(outputs.depth);

    // ═══════════════════════════════════════════════════════
    //  SET RENDER STATE
    // ═══════════════════════════════════════════════════════

    // Setup render pass descriptor
    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = albedo;
    passDesc.renderTargets[1] = normal;
    passDesc.renderTargets[2] = material;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depth;
    passDesc.clearValue.color[0] = m_config.clearColor[0];
    passDesc.clearValue.color[1] = m_config.clearColor[1];
    passDesc.clearValue.color[2] = m_config.clearColor[2];
    passDesc.clearValue.color[3] = m_config.clearColor[3];
    passDesc.clearValue.depth = m_config.clearDepth;
    passDesc.clearValue.stencil = m_config.clearStencil;
    passDesc.clearColor = true;
    passDesc.clearDepth = true;
    passDesc.clearStencil = true;

    // Begin render pass
    ctx.BeginRenderPass(passDesc);

    // Set viewport
    ctx.SetViewport(0, 0,
        static_cast<float>(m_config.width),
        static_cast<float>(m_config.height));

    // Set scissor (full screen)
    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  RENDER GEOMETRY
    // ═══════════════════════════════════════════════════════

    // TODO: Will be implemented in Task 49.2
    // For now, just clear - geometry rendering comes next

    Msg("  (Geometry rendering not yet implemented)");

    // End render pass
    ctx.EndRenderPass();

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    // Copy from RenderContext stats
    const auto& ctxStats = ctx.GetStats();
    m_stats.numDrawCalls = ctxStats.numDrawCalls;
    // Note: numTriangles not in RenderStats yet
    m_stats.numTriangles = 0;

    Msg("  ✓ G-Buffer pass complete: %u draws, %.2f ms",
        m_stats.numDrawCalls,
        m_stats.cpuTimeMs);
}

} // namespace xray::render::passes
