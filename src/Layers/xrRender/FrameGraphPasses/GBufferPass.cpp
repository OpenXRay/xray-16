// xrRender/FrameGraphPasses/GBufferPass.cpp
#include "stdafx.h"
#include "GBufferPass.h"
#include "ShaderConstants.h"  // CB layout definitions
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

    // Validate resolution was set correctly
    if (config.width == 0 || config.height == 0) {
        Msg("! [GBufferPass] ERROR: Invalid resolution %ux%u! Must set config.width/height to Device.dwWidth/dwHeight!",
            config.width, config.height);
        FATAL("GBufferPass created with invalid resolution");
    }

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

// ═══════════════════════════════════════════════════════
//  IPASS INTERFACE: SETUP
// ═══════════════════════════════════════════════════════

void GBufferPass::Setup(FrameGraph& fg) {
    Msg("~ [GBufferPass] Setting up in FrameGraph");

    m_outputs = GBufferOutputs{};

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

    m_outputs.albedo = fg.CreateTexture("GBuffer.Albedo", albedoDesc);

    // Normal + Roughness
    ResourceDesc normalDesc;
    normalDesc.type = ResourceDesc::Type::Texture2D;
    normalDesc.width = m_config.width;
    normalDesc.height = m_config.height;
    normalDesc.format = m_config.normalFormat;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    normalDesc.debugName = "GBuffer.Normal";

    m_outputs.normal = fg.CreateTexture("GBuffer.Normal", normalDesc);

    // Material ID
    ResourceDesc materialDesc;
    materialDesc.type = ResourceDesc::Type::Texture2D;
    materialDesc.width = m_config.width;
    materialDesc.height = m_config.height;
    materialDesc.format = m_config.materialFormat;
    materialDesc.isRenderTarget = true;
    materialDesc.isTransient = true;
    materialDesc.debugName = "GBuffer.Material";

    m_outputs.material = fg.CreateTexture("GBuffer.Material", materialDesc);

    // Depth + Stencil
    ResourceDesc depthDesc;
    depthDesc.type = ResourceDesc::Type::Texture2D;
    depthDesc.width = m_config.width;
    depthDesc.height = m_config.height;
    depthDesc.format = m_config.depthFormat;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;
    depthDesc.debugName = "GBuffer.Depth";

    m_outputs.depth = fg.CreateTexture("GBuffer.Depth", depthDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE GBUFFER PASS
    // ═══════════════════════════════════════════════════════

    PassHandle gbufferPass = fg.AddPass("GBuffer");

    // Declare resource writes (render targets)
    fg.PassWrite(gbufferPass, m_outputs.albedo, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, m_outputs.normal, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, m_outputs.material, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, m_outputs.depth, ResourceState::DepthStencilWrite);

    // Set execution callback
    // Set execution callback (IPass::Execute will use m_outputs)
    fg.SetPassCallback(gbufferPass, [this](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg);
    });

    Msg("  ✓ G-Buffer pass configured");
}

// ═══════════════════════════════════════════════════════
//  IPASS INTERFACE: EXECUTE
// ═══════════════════════════════════════════════════════

void GBufferPass::Execute(ng::RenderContext& ctx, const FrameGraph& fg) {
    Msg("~ [GBufferPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    // Reset statistics
    m_gbufferStats = Stats{};

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    // Create pipeline if needed (once per frame, using actual G-Buffer formats)
    if (!m_pipeline)
    {
        if (!CreatePipeline(m_outputs, fg))
        {
            Msg("! [GBufferPass] Failed to create pipeline");
            return;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  SET RENDER STATE
    // ═══════════════════════════════════════════════════════

    // ═══════════════════════════════════════════════════════
    //  UPDATE GLOBAL CONSTANT BUFFERS (BEFORE RENDER PASS!)
    // ═══════════════════════════════════════════════════════
    // Global CBs (slot 1+) contain view/projection matrices, lighting, fog, etc.
    // We must update them BEFORE the render pass since they're not volatile.
    // Using direct buffer writes with our own CB layout definitions.

    // Fill global constants from Device state
    GlobalConstants globalCB = {};
    FillGlobalConstants(globalCB);

    // Get D3D11 device context for UpdateSubresource calls
    // NO RCache usage - get directly from NVRHI device!
    ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(
        m_device->GetNativeDevice()->getNativeObject(nvrhi::ObjectTypes::D3D11_Device).pointer);
    VERIFY(d3dDevice);

    ID3D11DeviceContext* d3dContext = nullptr;
    d3dDevice->GetImmediateContext(&d3dContext);
    VERIFY(d3dContext);

    // Track unique CB buffers to avoid duplicate updates
    xr_set<ID3D11Buffer*> updatedBuffers;

    Msg("  [GBufferPass] Global CB matrices:");
    Msg("    m_V:  [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[0], globalCB.m_V[1], globalCB.m_V[2], globalCB.m_V[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[4], globalCB.m_V[5], globalCB.m_V[6], globalCB.m_V[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[8], globalCB.m_V[9], globalCB.m_V[10], globalCB.m_V[11]);
    Msg("    m_P:  [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[0], globalCB.m_P[1], globalCB.m_P[2], globalCB.m_P[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[4], globalCB.m_P[5], globalCB.m_P[6], globalCB.m_P[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[8], globalCB.m_P[9], globalCB.m_P[10], globalCB.m_P[11]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[12], globalCB.m_P[13], globalCB.m_P[14], globalCB.m_P[15]);
    Msg("    m_VP: [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[0], globalCB.m_VP[1], globalCB.m_VP[2], globalCB.m_VP[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[4], globalCB.m_VP[5], globalCB.m_VP[6], globalCB.m_VP[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[8], globalCB.m_VP[9], globalCB.m_VP[10], globalCB.m_VP[11]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[12], globalCB.m_VP[13], globalCB.m_VP[14], globalCB.m_VP[15]);
    Msg("    eye:  (%.1f, %.1f, %.1f)", globalCB.eye_position.x, globalCB.eye_position.y, globalCB.eye_position.z);

    // ═══════════════════════════════════════════════════════
    //  UPDATE ALL GLOBAL CBS BEFORE RENDER PASS
    // ═══════════════════════════════════════════════════════
    // WriteBuffer must be called OUTSIDE render passes!
    // Pre-scan all batches to find unique global CB buffers and update them now

    if (!m_batches.empty()) {
        xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

        for (const auto* batch : m_batches) {
            if (batch->visual && m_materialCache) {
                MaterialPSO* matPSO = m_materialCache->GetOrCreatePSO(batch->visual, m_outputs, fg);
                if (matPSO) {
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (!cbInfo.isPerObject && cbInfo.nvrhiBuffer) {
                            if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                                u32 sizeToWrite = std::min<u32>(sizeof(GlobalConstants), cbInfo.size);
                                Msg("  [GBufferPass] Pre-updating global CB slot %u (%u bytes) BEFORE render pass",
                                    cbInfo.slot, sizeToWrite);
                                ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &globalCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            }
                        }
                    }
                }
            }
        }
        Msg("  [GBufferPass] Pre-updated %u unique global CB buffers", (u32)updatedGlobalBuffers.size());
    }

    // ═══════════════════════════════════════════════════════
    //  SETUP RENDER PASS DESCRIPTOR
    // ═══════════════════════════════════════════════════════
    // X-Ray convention (validated from vanilla, inferred by ShaderReflector):
    // - Slot 0 → Normal   (RTSemantic::Normal)
    // - Slot 1 → Albedo   (RTSemantic::Albedo)
    // - Slot 2 → Material (RTSemantic::Material)
    //
    // ShaderReflector analyzes each shader and assigns semantics to slots.
    // MaterialCache::SetupRenderTargets() uses these semantics to configure PSO formats.
    // We bind physical textures here in the standard slot order.
    // Shaders write to whichever slots they need based on their reflection data.

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normal;    // SV_Target0 (RTSemantic::Normal)
    passDesc.renderTargets[1] = albedo;    // SV_Target1 (RTSemantic::Albedo)
    passDesc.renderTargets[2] = material;  // SV_Target2 (RTSemantic::Material)
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

    Msg("! [GBufferPass] Framebuffer binding (X-Ray convention):");
    Msg("!   Slot 0 → Normal   (texture %p)", normal);
    Msg("!   Slot 1 → Albedo   (texture %p)", albedo);
    Msg("!   Slot 2 → Material (texture %p)", material);

    // Begin render pass
    ctx.BeginRenderPass(passDesc);

    // IMPORTANT: Update global CBs AFTER beginning render pass but BEFORE drawing!
    // If we update before the pass, NVRHI or RCache might overwrite our data.

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
    //  GET GEOMETRY TO RENDER (Week 16: Use IPass batches)
    // ═══════════════════════════════════════════════════════

    m_gbufferStats.numObjects = static_cast<u32>(m_batches.size());
    Msg("  Rendering %u geometry batches (from IPass routing)", m_gbufferStats.numObjects);

    if (!m_batches.empty()) {
        // ═══════════════════════════════════════════════════════
        //  RENDER GEOMETRY BATCHES
        // ═══════════════════════════════════════════════════════

        ng::PipelineState* currentPipeline = nullptr;
        nvrhi::IBindingSet* currentBindingSet = nullptr;

        // Keep binding sets alive for the duration of rendering
        // NOTE: No longer need tempBindingSets - binding sets are now cached in MaterialPSO!

        for (const auto* batch : m_batches) {
            // Get per-material PSO from MaterialCache
            ng::PipelineState* pipelineToUse = m_pipeline;  // Default fallback
            MaterialPSO* matPSO = nullptr;

            if (batch->visual && m_materialCache) {
                // Get or create PSO for this material
                matPSO = m_materialCache->GetOrCreatePSO(
                    batch->visual,
                    m_outputs,
                    fg);

                if (matPSO && matPSO->pso) {
                    pipelineToUse = matPSO->pso;
                    // Global CBs are already updated before render pass
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

                // Helper to copy Fmatrix as HLSL float3x4 (row-major, 48 bytes)
                // CRITICAL: Fmatrix is column-major, HLSL expects row-major
                // float3x4 = 3 rows of 4 floats, including translation in 4th column
                auto CopyMatrix3x4 = [](u8* dest, const Fmatrix& src) {
                    // Transpose: Fmatrix columns become HLSL rows
                    Fmatrix transposed;
                    transposed.transpose(src);

                    // Copy first 3 rows (12 floats = 48 bytes)
                    float* destF = reinterpret_cast<float*>(dest);
                    destF[0]  = transposed._11; destF[1]  = transposed._12; destF[2]  = transposed._13; destF[3]  = transposed._14;
                    destF[4]  = transposed._21; destF[5]  = transposed._22; destF[6]  = transposed._23; destF[7]  = transposed._24;
                    destF[8]  = transposed._31; destF[9]  = transposed._32; destF[10] = transposed._33; destF[11] = transposed._34;
                };

                // Compute matrices
                Fmatrix xform = batch->worldMatrix;
                Fmatrix xform_v;
                xform_v.mul(batch->worldMatrix, Device.mView);

                // Write as float3x4 (48 bytes each) to match shader layout
                CopyMatrix3x4(cbData + 0, xform);     // m_xform at offset 0
                CopyMatrix3x4(cbData + 48, xform_v);  // m_xform_v at offset 48
                // Rest stays zero (consts, c_scale, c_bias, wind, wave, c_sun, etc.)

                VERIFY(m_perObjectCB.IsValid());
                nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(m_perObjectCB);

                // Write VCB within render pass command list (NVRHI handles versioning)
                // CRITICAL: Write the FULL buffer size (256 bytes) to avoid partial update errors!
                ctx.WriteBuffer(vcbBuffer, cbData, VCB_SIZE);

                // Step 2: Get or create cached binding set (created once, reused!)
                nvrhi::BindingSetHandle fullBindingSet =
                    m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer, matPSO->pass);

                if (fullBindingSet) {
                    ctx.SetBindingSet(0, fullBindingSet.Get());
                    currentBindingSet = fullBindingSet.Get();
                }
            } else {
                // Fallback to old path
                UpdatePerObjectConstants(ctx, *batch);

                if (batch->bindingSet != currentBindingSet) {
                    ctx.SetBindingSet(0, batch->bindingSet);
                    currentBindingSet = batch->bindingSet;
                }
            }

            // Bind vertex/index buffers (convert nvrhi::BufferHandle to IBuffer*)
            nvrhi::IBuffer* vb = batch->vertexBuffer.Get();
            nvrhi::IBuffer* ib = batch->indexBuffer.Get();

            Msg("! [GBufferPass] Draw %u: VB=%p (size=%u, stride from PSO), IB=%p (size=%u), indexCount=%u, startIndex=%u, baseVertex=%d",
                m_gbufferStats.numDrawCalls,
                vb, vb ? vb->getDesc().byteSize : 0,
                ib, ib ? ib->getDesc().byteSize : 0,
                batch->indexCount, batch->startIndex, batch->baseVertex);

            ctx.SetVertexBuffer(0, vb, 0);
            ctx.SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);  // X-Ray uses 16-bit indices

            // Draw
            ctx.DrawIndexed(batch->indexCount, batch->startIndex, batch->baseVertex);

            m_gbufferStats.numDrawCalls++;
            m_gbufferStats.numTriangles += batch->indexCount / 3;
        }
    }

    // End render pass
    ctx.EndRenderPass();

    // Release D3D11 context (GetImmediateContext AddRefs it)
    if (d3dContext) {
        d3dContext->Release();
    }

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_gbufferStats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    // Copy from RenderContext stats
    const auto& ctxStats = ctx.GetStats();
    m_gbufferStats.numDrawCalls = ctxStats.numDrawCalls;
    // Note: numTriangles not in RenderStats yet
    m_gbufferStats.numTriangles = 0;

    Msg("  ✓ G-Buffer pass complete: %u draws, %u tris, %.2f ms",
        m_gbufferStats.numDrawCalls,
        m_gbufferStats.numTriangles,
        m_gbufferStats.cpuTimeMs);
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
