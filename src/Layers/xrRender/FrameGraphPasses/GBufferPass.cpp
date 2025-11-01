// xrRender/FrameGraphPasses/GBufferPass.cpp
#include "stdafx.h"
#include "GBufferPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

GBufferPass::GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    Msg("* [GBufferPass] Created (%ux%u)", config.width, config.height);

    // Create material cache
    m_materialCache = xr_make_unique<MaterialCache>(device);

    // Load shaders (legacy - will be removed once MaterialCache is fully integrated)
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
    m_vertexShaderNative = loader.LoadVertexShader("gbuffer");
    if (!m_vertexShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer vertex shader");
        return false;
    }

    // Load G-Buffer pixel shader
    m_pixelShaderNative = loader.LoadPixelShader("gbuffer");
    if (!m_pixelShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer pixel shader");
        return false;
    }

    // Wrap shaders in RCShader for our abstraction layer
    m_vertexShader = xr_make_unique<ng::RCShader>(
        ng::ShaderStage::Vertex,
        m_vertexShaderNative,
        "gbuffer.vs"
    );

    m_pixelShader = xr_make_unique<ng::RCShader>(
        ng::ShaderStage::Pixel,
        m_pixelShaderNative,
        "gbuffer.ps"
    );

    // Create per-object constant buffer through our abstraction layer
    // Matches shader cbuffer PerObject : register(b0)
    // NOTE: Size must accommodate largest shader CB requirement (X-Ray shaders need 224 bytes)
    //       Using 256 bytes (aligned to CB alignment requirement)
    struct PerObjectConstants {
        Fmatrix worldViewProj;  // 64 bytes
        Fmatrix world;          // 64 bytes
        Fmatrix worldIT;        // 64 bytes
        // Total: 192 bytes - but X-Ray shaders expect 224!
        // Padding to 256 bytes for safety and alignment
        u8 padding[64];
    };

    ng::RenderDevice::BufferDesc cbDesc;
    cbDesc.byteSize = 256;  // Aligned size, enough for all X-Ray shaders
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;  // Updated every draw call
    cbDesc.debugName = "PerObjectCB";

    m_perObjectCB = m_device->CreateBuffer(cbDesc);
    if (!m_perObjectCB.IsValid()) {
        Msg("! [GBufferPass] Failed to create per-object constant buffer");
        return false;
    }

    Msg("  ✓ G-Buffer shaders loaded successfully");
    return true;
}

bool GBufferPass::CreatePipeline(const GBufferOutputs& outputs, const FrameGraph& fg)
{
    VERIFY(m_vertexShader != nullptr);
    VERIFY(m_pixelShader != nullptr);

    // Create pipeline descriptor using our abstraction
    ng::PipelineStateDesc psoDesc;

    // Shaders
    psoDesc.vertexShader = m_vertexShader.get();
    psoDesc.pixelShader = m_pixelShader.get();

    // Vertex input layout - define attributes matching gbuffer.vs
    // ng::VertexAttribute order: semanticName, semanticIndex, format, offset, bufferIndex, isInstanced
    psoDesc.vertexAttributes = {
        { "POSITION", 0, nvrhi::Format::RGB32_FLOAT,  0, 0, false },   // float3 position
        { "NORMAL",   0, nvrhi::Format::RGB32_FLOAT, 12, 0, false },   // float3 normal
        { "TEXCOORD", 0, nvrhi::Format::RG32_FLOAT,  24, 0, false },   // float2 texcoord
        { "TANGENT",  0, nvrhi::Format::RGB32_FLOAT, 32, 0, false },   // float3 tangent
        { "BINORMAL", 0, nvrhi::Format::RGB32_FLOAT, 44, 0, false },   // float3 binormal
    };

    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    // Rasterizer state
    psoDesc.rasterizerState.cullMode = ng::CullMode::Back;  // Backface culling
    psoDesc.rasterizerState.fillMode = ng::FillMode::Solid;
    psoDesc.rasterizerState.frontCounterClockwise = false;

    // Blend state - no blending (opaque geometry)
    for (int i = 0; i < 3; ++i)  // 3 MRTs
    {
        psoDesc.blendState.renderTargets[i].blendEnable = false;
        psoDesc.blendState.renderTargets[i].writeMask = ng::ColorWriteMask::All;
    }

    // Depth/stencil state - enable depth testing and writing
    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthWriteEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
    psoDesc.depthStencilState.stencilEnable = false;

    // Render target formats (MRT: Albedo, Normal, Material + Depth)
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(outputs.depth);

    psoDesc.renderTargetFormats[0] = albedo->getDesc().format;
    psoDesc.renderTargetFormats[1] = normal->getDesc().format;
    psoDesc.renderTargetFormats[2] = material->getDesc().format;
    psoDesc.depthStencilFormat = depth->getDesc().format;
    psoDesc.renderTargetCount = 3;

    psoDesc.debugName = "GBufferPass";

    // Get or create pipeline through cache
    m_pipeline = m_device->GetPipelineCache()->GetOrCreate(psoDesc);

    if (!m_pipeline)
    {
        Msg("! [GBufferPass] Failed to create graphics pipeline");
        return false;
    }

    Msg("  ✓ GBufferPass pipeline created successfully (MRT: 3 targets + depth)");
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

    // Create pipeline if needed (once per frame, using actual G-Buffer formats)
    if (!m_pipeline)
    {
        if (!CreatePipeline(outputs, fg))
        {
            Msg("! [GBufferPass] Failed to create pipeline");
            return;
        }
    }

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

        ng::PipelineState* currentPipeline = nullptr;
        nvrhi::IBindingSet* currentBindingSet = nullptr;

        // Keep binding sets alive for the duration of rendering
        // NOTE: No longer need tempBindingSets - binding sets are now cached in MaterialPSO!

        for (const auto& batch : batches) {
            // Get per-material PSO from MaterialCache
            ng::PipelineState* pipelineToUse = m_pipeline;  // Default fallback
            MaterialPSO* matPSO = nullptr;

            if (batch.visual && m_materialCache) {
                // Get or create PSO for this material
                matPSO = m_materialCache->GetOrCreatePSO(
                    batch.visual,
                    outputs,
                    fg);

                if (matPSO && matPSO->pso) {
                    pipelineToUse = matPSO->pso;
                }
            }

            if (pipelineToUse != currentPipeline) {
                ctx.SetPipeline(pipelineToUse->GetNativePipeline());
                currentPipeline = pipelineToUse;
            }

            // Update per-object constants using VCB + get/create cached binding set
            if (matPSO) {
                // Step 1: Write VCB data inline in command list (proper NVRHI pattern)
                // MUST be done BEFORE setGraphicsState/SetBindingSet that uses the VCB!

                // IMPORTANT: We MUST write the FULL VCB size (256 bytes) to avoid partial updates!
                // D3D11 constant buffers do NOT support partial updates (UpdateSubresource with pDstBox)
                // If we write less than the buffer size, NVRHI will try a partial update and fail.
                constexpr u32 VCB_SIZE = 256;  // Must match VCB creation size!

                // Allocate buffer on stack (256 bytes for VCB)
                u8 cbData[VCB_SIZE] = {};  // Zero-initialized!

                // Fill in the matrices we actually use
                Fmatrix* pWorldViewProj = reinterpret_cast<Fmatrix*>(cbData + 0);
                Fmatrix* pWorld = reinterpret_cast<Fmatrix*>(cbData + 64);
                Fmatrix* pWorldIT = reinterpret_cast<Fmatrix*>(cbData + 128);

                // Get view-projection matrix from device
                Fmatrix viewProj;
                viewProj.mul(Device.mView, Device.mProject);

                // Compute matrices
                pWorldViewProj->mul(batch.worldMatrix, viewProj);
                *pWorld = batch.worldMatrix;

                Fmatrix temp;
                temp.invert(batch.worldMatrix);
                pWorldIT->transpose(temp);

                VERIFY(m_perObjectCB.IsValid());
                nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(m_perObjectCB);

                // Write VCB within render pass command list (NVRHI handles versioning)
                // CRITICAL: Write the FULL buffer size (256 bytes) to avoid partial update errors!
                ctx.WriteBuffer(vcbBuffer, cbData, VCB_SIZE);

                // Step 2: Get or create cached binding set (created once, reused!)
                nvrhi::BindingSetHandle fullBindingSet =
                    m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer);

                if (fullBindingSet) {
                    ctx.SetBindingSet(0, fullBindingSet.Get());
                    currentBindingSet = fullBindingSet.Get();
                }
            } else {
                // Fallback to old path
                UpdatePerObjectConstants(ctx, batch);

                if (batch.bindingSet != currentBindingSet) {
                    ctx.SetBindingSet(0, batch.bindingSet);
                    currentBindingSet = batch.bindingSet;
                }
            }

            // Bind vertex/index buffers (convert nvrhi::BufferHandle to IBuffer*)
            ctx.SetVertexBuffer(0, batch.vertexBuffer.Get(), 0);
            ctx.SetIndexBuffer(batch.indexBuffer.Get(), nvrhi::Format::R16_UINT, 0);  // X-Ray uses 16-bit indices

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

void GBufferPass::UpdatePerObjectConstantsData(const GeometryBatch& batch)
{
    // Update world matrix constant buffer DATA only (no binding)
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

    // Update constant buffer through our abstraction layer
    VERIFY(m_perObjectCB.IsValid());
    m_device->UpdateBuffer(m_perObjectCB, &constants, sizeof(constants));
}

void GBufferPass::UpdatePerObjectConstants(
    ng::RenderContext& ctx,
    const GeometryBatch& batch
) {
    // Update buffer data
    UpdatePerObjectConstantsData(batch);

    // Bind constant buffer to slot b0 (using our abstraction layer)
    ctx.SetConstantBuffer(0, m_perObjectCB);
}

} // namespace xray::render::passes
