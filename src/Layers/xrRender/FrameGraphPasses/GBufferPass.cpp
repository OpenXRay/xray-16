// xrRender/FrameGraphPasses/GBufferPass.cpp
#include "stdafx.h"
#include "GBufferPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

GBufferPass::GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    Msg("* [GBufferPass] Created (%ux%u)", config.width, config.height);

    // Load shaders
    if (!LoadShaders())
    {
        Msg("! [GBufferPass] Failed to load shaders");
    }
}

GBufferPass::~GBufferPass() {
    Msg("* [GBufferPass] Destroyed");
}

bool GBufferPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    // Load G-Buffer vertex shader
    m_vertexShader = loader.LoadVertexShader("gbuffer");
    if (!m_vertexShader)
    {
        Msg("! [GBufferPass] Failed to load gbuffer vertex shader");
        return false;
    }

    // Load G-Buffer pixel shader
    m_pixelShader = loader.LoadPixelShader("gbuffer");
    if (!m_pixelShader)
    {
        Msg("! [GBufferPass] Failed to load gbuffer pixel shader");
        return false;
    }

    Msg("  ✓ G-Buffer shaders loaded successfully");
    return true;
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
    //  GET GEOMETRY TO RENDER
    // ═══════════════════════════════════════════════════════

    // Access global geometry collector (defined in GeometryBatch.cpp)

    if (g_geometryCollector != nullptr) {
        const auto& batches = g_geometryCollector->GetBatches();
        m_stats.numObjects = static_cast<u32>(batches.size());

        Msg("  Rendering %u geometry batches", m_stats.numObjects);

        // ═══════════════════════════════════════════════════════
        //  RENDER GEOMETRY BATCHES
        // ═══════════════════════════════════════════════════════

        nvrhi::IGraphicsPipeline* currentPipeline = nullptr;
        nvrhi::IBindingSet* currentBindingSet = nullptr;

        for (const auto& batch : batches) {
            // Set pipeline (if changed)
            if (batch.pipeline != currentPipeline) {
                ctx.SetPipeline(batch.pipeline);
                currentPipeline = batch.pipeline;
            }

            // Update per-object constants
            UpdatePerObjectConstants(ctx, batch);

            // Bind textures (if changed)
            if (batch.bindingSet != currentBindingSet) {
                ctx.SetBindingSet(0, batch.bindingSet);
                currentBindingSet = batch.bindingSet;
            }

            // Bind vertex/index buffers
            ctx.SetVertexBuffer(0, batch.vertexBuffer, 0);
            ctx.SetIndexBuffer(batch.indexBuffer, nvrhi::Format::R32_UINT, 0);

            // Draw
            ctx.DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);

            m_stats.numDrawCalls++;
            m_stats.numTriangles += batch.indexCount / 3;
        }
    } else {
        Msg("  (No geometry collector available)");
    }

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

    Msg("  ✓ G-Buffer pass complete: %u draws, %u tris, %.2f ms",
        m_stats.numDrawCalls,
        m_stats.numTriangles,
        m_stats.cpuTimeMs);
}

void GBufferPass::UpdatePerObjectConstants(
    ng::RenderContext& ctx,
    const GeometryBatch& batch
) {
    // Update world matrix constant buffer
    struct PerObjectConstants {
        Fmatrix worldViewProj;
        Fmatrix world;
        Fmatrix worldIT;  // Inverse transpose for normals
    };

    PerObjectConstants constants;

    // Get view-projection matrix from device
    Fmatrix viewProj;
    viewProj.mul(Device.mView, Device.mProject);

    // Compute world-view-projection
    constants.worldViewProj.mul(batch.worldMatrix, viewProj);

    // World matrix
    constants.world = batch.worldMatrix;

    // World inverse transpose (for normals)
    Fmatrix temp;
    temp.invert(batch.worldMatrix);
    constants.worldIT.transpose(temp);

    // Update constant buffer
    // TODO: Create and bind constant buffer properly
    // For now, this is a placeholder
    // ctx.UpdateConstantBuffer(0, &constants, sizeof(constants));
}

} // namespace xray::render::passes
