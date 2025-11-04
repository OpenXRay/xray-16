// xrRender/FrameGraphPasses/GBufferPass.cpp
#include "stdafx.h"
#include "GBufferPass.h"
#include "ShaderConstants.h"  // CB layout definitions
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"  // For CB analysis
#include "Layers/xrRender/ResourceManager.h"  // For texture description manager
#include "Layers/xrRender/TextureDescrManager.h"  // For GetDetailTexture
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics (skeleton bones)
#include "Layers/xrRender/FSkinned.h"  // For CSkeletonX (skinned mesh parent pointer)
#include "xrEngine/Render.h"  // For RImplementation

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

    // Create VCB pool for dynamic constant buffer management
    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    // Create material cache (with VCB pool for dynamic CB sizing)
    m_materialCache = xr_make_unique<MaterialCache>(device, m_vcbPool.get());

    // Load shaders (legacy - will be removed once MaterialCache is fully integrated)
    if (!LoadShaders())
    {
        Msg("! [GBufferPass] Failed to load shaders");
    }
}

GBufferPass::~GBufferPass() {
    // Clean up shader bytecode
    if (m_vertexShaderBytecode) {
        m_vertexShaderBytecode->Release();
        m_vertexShaderBytecode = nullptr;
    }
    if (m_pixelShaderBytecode) {
        m_pixelShaderBytecode->Release();
        m_pixelShaderBytecode = nullptr;
    }
    Msg("* [GBufferPass] Destroyed");
}

bool GBufferPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    // Load G-Buffer vertex shader (with bytecode for reflection)
    m_vertexShaderNative = loader.LoadVertexShader("gbuffer", "main", &m_vertexShaderBytecode);
    if (!m_vertexShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer vertex shader");
        return false;
    }

    // Load G-Buffer pixel shader (with bytecode for reflection)
    m_pixelShaderNative = loader.LoadPixelShader("gbuffer", "main", &m_pixelShaderBytecode);
    if (!m_pixelShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer pixel shader");
        return false;
    }

    // ═══════════════════════════════════════════════════
    //  ANALYZE CONSTANT BUFFERS & REGISTER WITH VCB POOL
    // ═══════════════════════════════════════════════════

    Msg("* [GBufferPass] Analyzing constant buffer requirements...");

    // Analyze vertex shader CBs
    if (m_vertexShaderBytecode) {
        auto vsCBs = ShaderReflector::AnalyzeConstantBuffers(m_vertexShaderBytecode);
        Msg("  → Vertex shader has %u constant buffers", vsCBs.buffers.size());

        // Register each CB layout with the VCB pool
        for (const auto& cbInfo : vsCBs.buffers) {
            VolatileConstantBufferPool::CBLayout layout(cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
            m_vcbPool->GetOrCreateVCB(layout);

            // Save layout for later use (if it's a per-object CB)
            if (xr_strcmp(cbInfo.name.c_str(), "dynamic_transforms") == 0) {
                m_dynamicTransformsLayout = layout;
            }
        }
    }

    // Analyze pixel shader CBs
    if (m_pixelShaderBytecode) {
        auto psCBs = ShaderReflector::AnalyzeConstantBuffers(m_pixelShaderBytecode);
        Msg("  → Pixel shader has %u constant buffers", psCBs.buffers.size());

        // Register each CB layout with the VCB pool
        for (const auto& cbInfo : psCBs.buffers) {
            VolatileConstantBufferPool::CBLayout layout(cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
            m_vcbPool->GetOrCreateVCB(layout);

            // Save layout for later use
            if (xr_strcmp(cbInfo.name.c_str(), "Material") == 0) {
                m_materialLayout = layout;
            }
        }
    }

    // Log VCB pool statistics
    m_vcbPool->LogStats();

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

    // LEGACY: Old static per-object constant buffer creation (now handled by VCB pool)
    // The VCB pool dynamically creates appropriately-sized VCBs based on shader requirements.
    // This avoids wasting ring buffer space with oversized allocations.

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

    // DEBUG: Check buffer state at start of Execute
    Msg("  [GBufferPass] Checking %u batches for buffer validity...", m_batches.size());
    u32 nullVBCount = 0, nullIBCount = 0;
    for (u32 i = 0; i < m_batches.size(); ++i) {
        const auto* batch = m_batches[i];
        if (!batch->vertexBuffer) {
            nullVBCount++;
            if (i < 5) Msg("!   Batch %u: VB is NULL! (debugName='%s')", i, batch->debugName.c_str());
        }
        if (!batch->indexBuffer) {
            nullIBCount++;
            if (i < 5) Msg("!   Batch %u: IB is NULL! (debugName='%s')", i, batch->debugName.c_str());
        }
    }
    if (nullVBCount > 0 || nullIBCount > 0) {
        Msg("! [GBufferPass] ERROR: Found %u batches with null VB, %u with null IB (total %u batches)",
            nullVBCount, nullIBCount, m_batches.size());
    } else {
        Msg("  [GBufferPass] All batches have valid buffers");
    }

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

    // Fill constant buffers from Device/RCache state
    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    // Debug: Print first few values of dynamic_transforms
    Msg("  [GBufferPass] DynamicTransforms computed:");
    Msg("    m_WVP[0-3]: %.3f, %.3f, %.3f, %.3f",
        dynamicTransformsCB.m_WVP[0], dynamicTransformsCB.m_WVP[1],
        dynamicTransformsCB.m_WVP[2], dynamicTransformsCB.m_WVP[3]);
    Msg("    m_W[0-3]:   %.3f, %.3f, %.3f, %.3f",
        dynamicTransformsCB.m_W[0], dynamicTransformsCB.m_W[1],
        dynamicTransformsCB.m_W[2], dynamicTransformsCB.m_W[3]);

    // Get D3D11 device context for UpdateSubresource calls
    // NO RCache usage - get directly from NVRHI device!
    ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(
        m_device->GetNativeDevice()->getNativeObject(nvrhi::ObjectTypes::D3D11_Device).pointer);
    VERIFY(d3dDevice);

    // Track unique CB buffers to avoid duplicate updates
    xr_set<ID3D11Buffer*> updatedBuffers;

    Msg("  [GBufferPass] Filled constant buffers:");
    Msg("    static_globals: m_V, m_P, m_VP, lighting, fog, etc.");
    Msg("    dynamic_transforms: m_W, m_WV, m_WVP from RCache.xforms");

    // ═══════════════════════════════════════════════════════
    //  UPDATE ALL GLOBAL CBS BEFORE RENDER PASS
    // ═══════════════════════════════════════════════════════
    // WriteBuffer must be called OUTSIDE render passes!
    // Pre-scan all batches to find unique CB buffers and update them by name.

    if (!m_batches.empty()) {
        xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

        for (const auto* batch : m_batches) {
            if (batch->visual && m_materialCache) {
                MaterialPSO* matPSO = m_materialCache->GetOrCreatePSO(batch->visual, m_outputs, fg);
                if (matPSO) {
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (!cbInfo.isPerObject && cbInfo.nvrhiBuffer) {
                            if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                                // Fill buffer based on CB name
                                if (cbInfo.name == "static_globals") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                    updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                                } else if (cbInfo.name == "dynamic_transforms") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicTransformsCB, sizeToWrite);
                                    updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                                }
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

        // Track last detail scale to avoid redundant CB updates
        float lastDetailScale = -1.0f;  // Start with invalid value to force first update

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

            // ═══════════════════════════════════════════════════════
            //  UPDATE PER-MATERIAL DYNAMIC CONSTANTS
            // ═══════════════════════════════════════════════════════
            // dt_params is per-material (comes from texture .thm metadata)
            // We need to update DynamicTransforms CB with the material's detail scale

            if (matPSO && matPSO->detail_scale != lastDetailScale) {
                // Recompute DynamicTransforms with new detail scale
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB);

                // Override dt_params with per-material detail scale
                extern float r__dtex_range;  // Defined in TextureDescrManager.cpp
                dynamicCB.dt_params.set(matPSO->detail_scale, matPSO->detail_scale, matPSO->detail_scale,
                                       1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);

                // Update DynamicTransforms CB (Slot 1) for this material
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                lastDetailScale = matPSO->detail_scale;
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

                // Write consts at offset 96 (Fvector4 consts; in PerObjectConstants)
                // For trees: (scale, scale, 0, 0) where scale comes from FTreeVisual::tvs.scale
                // For static geometry: (1.0, 1.0, 0, 0) - texture coordinate dequant factor
                // Legacy X-Ray: cmd_list.tree.set_consts(tvs.scale, tvs.scale, 0, 0);

                // Attempt to get scale from visual if it's a tree
                float tc_scale = 1.0f;  // Default for static geometry
                if (batch->visual) {
                    // TODO: Check visual type and cast to FTreeVisual* to get tvs.scale
                    // For now, use default 1.0 for all geometry (trees need their actual scale)
                    // FTreeVisual has m_tree_scale, need to expose it or use visual type checking
                }

                float* constsPtr = reinterpret_cast<float*>(cbData + 96);
                constsPtr[0] = tc_scale;  // x: U coordinate scale (1/quant for SHORT2 texcoords)
                constsPtr[1] = tc_scale;  // y: V coordinate scale
                constsPtr[2] = 0.0f;      // z: unused
                constsPtr[3] = 0.0f;      // w: unused

                // Rest stays zero (c_scale, c_bias, wind, wave, c_sun, etc.)

                // Get appropriate VCB from MaterialPSO (each material knows its CB requirements)
                ng::BufferHandle vcbHandle;
                if (!matPSO->vcbRequirements.empty()) {
                    // Use the first VCB requirement (typically b0 for per-object data)
                    // TODO: Handle multiple VCBs if needed
                    vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
                }

                if (!vcbHandle.IsValid()) {
                    Msg("! [GBufferPass] Material has no valid VCB");
                    continue;
                }

                nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(vcbHandle);

                // Get the VCB size from material requirements
                u32 vcbSize = matPSO->vcbRequirements[0].size;

                // Write VCB within render pass command list (NVRHI handles versioning)
                // CRITICAL: Write the FULL buffer size to avoid partial update errors!
                ctx.WriteBuffer(vcbBuffer, cbData, vcbSize);

                // Step 2: Get or create cached binding sets (VS and PS - created once, reused!)
                // GetOrCreateBindingSet creates BOTH vsBindingSet and psBindingSet
                m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer, matPSO->pass);

                // Bind BOTH per-stage binding sets:
                // Slot 0: VS binding set (VS constant buffers)
                // Slot 1: PS binding set (PS constant buffers + textures + samplers)
                ctx.SetBindingSet(0, matPSO->vsBindingSet.Get());
                ctx.SetBindingSet(1, matPSO->psBindingSet.Get());
                currentBindingSet = matPSO->vsBindingSet.Get();
            }

            // Bind vertex/index buffers (convert nvrhi::BufferHandle to IBuffer*)
            nvrhi::IBuffer* vb = batch->vertexBuffer.Get();
            nvrhi::IBuffer* ib = batch->indexBuffer.Get();

            // DEFENSIVE: Check for null buffers before binding
            if (!vb || !ib) {
                Msg("! [GBufferPass] ERROR: Batch %u has null buffers! VB=%p, IB=%p, visual=%p, debugName='%s'",
                    m_gbufferStats.numDrawCalls,
                    vb, ib, batch->visual, batch->debugName.c_str());
                Msg("!   vertexBuffer handle valid: %s", batch->vertexBuffer ? "YES" : "NO");
                Msg("!   indexBuffer handle valid: %s", batch->indexBuffer ? "YES" : "NO");
                continue;  // Skip this batch
            }

            //Msg("! [GBufferPass] Draw %u: VB=%p (size=%u, stride from PSO), IB=%p (size=%u), indexCount=%u, startIndex=%u, baseVertex=%d",
            //    m_gbufferStats.numDrawCalls,
            //    vb, vb ? vb->getDesc().byteSize : 0,
            //    ib, ib ? ib->getDesc().byteSize : 0,
            //    batch->indexCount, batch->startIndex, batch->baseVertex);

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

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_gbufferStats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    // Stats are already accumulated in the draw loop (lines 532-533)
    // Do NOT overwrite them here!

    Msg("  ✓ G-Buffer pass complete: %u draws, %u tris, %.2f ms",
        m_gbufferStats.numDrawCalls,
        m_gbufferStats.numTriangles,
        m_gbufferStats.cpuTimeMs);
}

} // namespace xray::render::passes
