// xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"  // For loading bindless shaders
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"  // For IndirectDrawArgs struct
#include "Layers/xrRender/Bindless/TextureAtlas.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "xrCore/FMesh.hpp"  // For MT_NORMAL, MT_TREE, etc.

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Render phase enum for marker tracking
enum class RenderPhase { None, Opaque, AlphaTested, Transparent };

// ═══════════════════════════════════════════════════════
//  FORWARD RENDERING IMPLEMENTATION
// ═══════════════════════════════════════════════════════
// Extracted from original GBufferPass::Execute, simplified for single-RT

void renderForwardGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    nvrhi::IBuffer* drawArgsBuffer)
{
    using RENDER_NAMESPACE::CSkeletonX_ST;
    using RENDER_NAMESPACE::CSkeletonX_PM;

    if (!geometry || !materialCache || !device) {
        return;
    }

    const auto& batches = geometry->GetBatches();
    if (batches.empty()) {
        return;
    }

    // Check if GPU culling is available (draw args buffer passed through framegraph)
    const bool useIndirectDraw = (drawArgsBuffer != nullptr);

    // ═══════════════════════════════════════════════════════
    //  FILL CONSTANT BUFFERS FROM DEVICE STATE
    // ═══════════════════════════════════════════════════════

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    // ═══════════════════════════════════════════════════════
    //  HDR SUN LIGHTING (from RImplementation.Lights.sun)
    // ═══════════════════════════════════════════════════════
    // Get actual sun data from the game's lighting system
    // HDR intensity 2.0 = sun is 2x brighter than 1.0 (basic HDR)
    // For true HDR, this should be much higher (10-100+)
    SunLightData sunData;
    GetSunLightData(sunData, 2.0f);  // HDR multiplier
    FillSunConstants(staticGlobalsCB, sunData);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    // ═══════════════════════════════════════════════════════
    //  UPDATE GLOBAL CBS BEFORE RENDER PASS
    // ═══════════════════════════════════════════════════════
    // Global CBs must be updated OUTSIDE render passes!
    // Pre-scan batches to find unique CB buffers and update them.

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            MaterialPSO* matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg, RenderPassType::ForwardColor);
            if (matPSO) {
                // Update all global CBs (constantBuffers only contains global CBs, VCBs are in vcbRequirements)
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.nvrhiBuffer) {
                        if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                            // Fill buffer based on CB name
                            if (cbInfo.name == "static_globals") {
                                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            } else if (cbInfo.name == "dynamic_transforms") {
                                u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicTransformsCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            }
                        }
                    }
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  SETUP RENDER PASS (Single RT for Forward+)
    // ═══════════════════════════════════════════════════════
    // Forward+ uses a single HDR color buffer
    // Shaders must output to SV_Target0 only

    ng::RenderPassDesc passDesc;
    passDesc.passName = "Forward+ Color Pass";
    passDesc.renderTargets[0] = colorRT;     // SV_Target0 - Color output
    passDesc.numRenderTargets = 1;           // Single RT for Forward+
    passDesc.depthStencil = depthRT;

    // DON'T clear color - sky pass already rendered background
    passDesc.clearColor = false;

    // DON'T clear depth - we're reusing it from depth prepass for early-Z!
    passDesc.clearDepth = false;
    passDesc.clearStencil = false;

    // Begin render pass
    ctx->BeginRenderPass(passDesc);

    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    ctx->SetViewport(0, 0,
        static_cast<float>(rtDesc.width),
        static_cast<float>(rtDesc.height));

    // Set scissor (full screen)
    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = rtDesc.width;
    scissor.height = rtDesc.height;
    ctx->SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  RENDER GEOMETRY BATCHES (Pre-sorted by GeometryCollector)
    // ═══════════════════════════════════════════════════════
    // Batches are sorted using vanilla's SSA (Screen Space Area) logic:
    //   SSA = R / distSQ (larger = closer/bigger = more visually important)
    //
    // Sort order (matches vanilla r__dsgraph):
    //   1. Priority 0 (Opaque) - SSA descending (front-to-back for early-Z)
    //   2. Priority 1 (Alpha-tested) - SSA descending (front-to-back)
    //   3. bStrictB2F (Transparent) - SSA ascending (back-to-front for blending)
    //
    // This ensures opaque geometry fills the depth buffer FIRST,
    // then alpha-tested geometry renders with correct backgrounds.

    u32 numDraws = 0;
    u32 numTriangles = 0;
    u32 numCulled = 0;

    ng::PipelineState* currentPipeline = nullptr;

    // drawArgsBuffer is already passed in from framegraph (proper state transition guaranteed)

    // Helper lambda to render a single batch (returns true if rendered, false if skipped)
    // batchIndex is used for indirect draw offset
    auto renderBatch = [&](const GeometryBatch& batch, u32 batchIndex) -> bool {
        // Get per-material PSO from MaterialCache
        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg, RenderPassType::ForwardColor);

            if (matPSO && matPSO->pso) {
                pipelineToUse = matPSO->pso;
            }
        }

        if (!pipelineToUse) {
            return false;  // Skip batches without valid PSO
        }

        // Bind pipeline if changed
        if (pipelineToUse != currentPipeline) {
            ctx->SetPipeline(pipelineToUse->GetNativePipeline());
            currentPipeline = pipelineToUse;
        }

        // ═══════════════════════════════════════════════════════
        //  UPDATE MATERIAL AND INSTANCE CONSTANTS (USING FGCONSTANTSYSTEM)
        // ═══════════════════════════════════════════════════════

        if (matPSO) {
            // Initialize FGConstantSystem for reflection-driven constant binding
            fgconstants::FGConstantSystem constants(matPSO, materialCache->GetVCBPool());

            // ─────────────────────────────────────────────────
            //  MATERIAL-FREQUENCY CONSTANTS (once per material)
            // ─────────────────────────────────────────────────

            // Set material-specific detail scale
            Fvector4 dt_params(
                matPSO->detail_scale,
                matPSO->detail_scale,
                matPSO->detail_scale,
                1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range
            );
            constants.Set("dt_params", dt_params);

            // Commit material-frequency constants to GPU
            constants.CommitMaterial(ctx);

            // ─────────────────────────────────────────────────
            //  INSTANCE-FREQUENCY CONSTANTS (per draw call)
            // ─────────────────────────────────────────────────

            // Determine if this is a skeleton mesh
            bool isSkeleton = false;
            u32 visualType = 0;
            if (batch.visual && batch.renderable) {
                visualType = batch.visual->getType();
                isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
            }

            if (isSkeleton) {
                // Update global dynamic_transforms CB with skeleton's world matrix
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB, batch.worldMatrix);
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                // Get Parent skeleton from skinned mesh visual
                RENDER_NAMESPACE::CKinematics* Parent = nullptr;

                if (visualType == MT_SKELETON_GEOMDEF_ST) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_ST*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                }
                else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_PM*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                }

                if (Parent) {
                    // Calculate bones BEFORE accessing transforms
                    Parent->CalculateBones(TRUE);

                    // Get bone count
                    u32 count = Parent->LL_BoneCount();

                    // Build bone matrix array
                    constexpr u32 MAX_BONES = 78;  // From vanilla
                    Fmatrix boneMatrices[MAX_BONES];
                    u32 bonesToUpload = std::min(count, MAX_BONES);

                    for (u32 i = 0; i < bonesToUpload; i++) {
                        boneMatrices[i] = Parent->LL_GetTransform_R(u16(i));
                    }

                    // Use FGConstantSystem matrix array API
                    constants.SetArray("sbones_array", boneMatrices, bonesToUpload);
                }
            }
            else {
                // Compute matrices
                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                // mul_43(A,B) = B then A, so View*World = World then View (object->world->view)
                xform_v.mul_43(Device.mView, batch.worldMatrix);

                // Use FGConstantSystem matrix APIs
                constants.Set("m_xform", xform);
                constants.Set("m_xform_v", xform_v);

                // Write tree scale constant
                using namespace xray::render::RENDER_NAMESPACE;
                float tree_scale = 1.0f / float(FTreeVisual_quant);
                Fvector4 consts(tree_scale, tree_scale, 0.0f, 0.0f);
                constants.Set("consts", consts);
            }

            // Commit per-instance constants to GPU (uploads dirty VCBs)
            constants.CommitInstance(ctx);

            // Get or create cached binding sets
            materialCache->GetOrCreateBindingSet(matPSO);

            // Bind BOTH per-stage binding sets
            ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
            ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
        }

        // ═══════════════════════════════════════════════════════
        //  BIND VERTEX/INDEX BUFFERS AND DRAW
        // ═══════════════════════════════════════════════════════

        nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
        nvrhi::IBuffer* ib = batch.indexBuffer.Get();

        if (!vb || !ib) {
            return false;  // Skip batches with null buffers
        }

        ctx->SetVertexBuffer(0, vb, 0);
        ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);  // X-Ray uses 16-bit indices

        // Draw - use indirect draw if GPU culling is available
        if (useIndirectDraw && drawArgsBuffer) {
            // DrawIndexedIndirect reads args from GPU buffer
            // Each IndirectDrawArgs is 20 bytes (5 u32s)
            // instanceCount will be 0 (culled) or 1 (visible) as set by culling shader
            u32 argsOffset = batchIndex * sizeof(IndirectDrawArgs);
            ctx->DrawIndexedIndirect(drawArgsBuffer, argsOffset);
        } else {
            // Standard draw
            ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
        }

        numDraws++;
        numTriangles += batch.indexCount / 3;
        return true;
    };

    // ═══════════════════════════════════════════════════════
    //  RENDER ALL BATCHES WITH PHASE MARKERS
    // ═══════════════════════════════════════════════════════
    // GeometryCollector::Sort() ensures proper render order:
    //   1. Priority 0 (opaque) - SSA descending (front-to-back)
    //   2. Priority 1 (alpha-tested) - SSA descending (front-to-back)
    //   3. bStrictB2F (transparent) - SSA ascending (back-to-front)
    //
    // We track phase transitions to insert debug markers for RenderDoc.

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    RenderPhase currentPhase = RenderPhase::None;

    for (u32 batchIndex = 0; batchIndex < batches.size(); batchIndex++) {
        const auto& batch = batches[batchIndex];

        // Determine batch's render phase based on shader flags:
        // - iPriority == 0: Opaque (default)
        // - iPriority == 1: Alpha-tested (set by blender_deffer_aref.cpp)
        // - bStrictB2F: Transparent (back-to-front sorting required)
        RenderPhase batchPhase;
        if (batch.IsStrictB2F()) {
            batchPhase = RenderPhase::Transparent;
        } else if (batch.IsAlphaTested()) {
            batchPhase = RenderPhase::AlphaTested;
        } else {
            batchPhase = RenderPhase::Opaque;
        }

        // Insert markers on phase transition
        if (batchPhase != currentPhase) {
            // End previous marker (if any)
            if (currentPhase != RenderPhase::None) {
                cmdList->endMarker();
            }

            // Begin new phase marker
            switch (batchPhase) {
                case RenderPhase::Opaque:
                    cmdList->beginMarker("Forward Opaque (Priority 0)");
                    break;
                case RenderPhase::AlphaTested:
                    cmdList->beginMarker("Forward Alpha-Tested (Priority 1)");
                    break;
                case RenderPhase::Transparent:
                    cmdList->beginMarker("Forward Transparent (bStrictB2F)");
                    break;
                default:
                    break;
            }
            currentPhase = batchPhase;
        }

        renderBatch(batch, batchIndex);
    }

    // End final marker
    if (currentPhase != RenderPhase::None) {
        cmdList->endMarker();
    }

    // End render pass
    ctx->EndRenderPass();

    // Log statistics (optional, can remove for performance)
    // Msg("ForwardColor: %u draws, %u triangles", numDraws, numTriangles);
}

// ═══════════════════════════════════════════════════════
//  BINDLESS FORWARD RENDERING (GPU-DRIVEN)
// ═══════════════════════════════════════════════════════
// Single PSO, single multi-draw, all materials via buffer
// This is the high-performance GPU-driven rendering path

// Static resources for bindless rendering (initialized once)
static nvrhi::GraphicsPipelineHandle s_bindlessPipeline;
static nvrhi::BindingLayoutHandle s_bindlessLayout;
static nvrhi::InputLayoutHandle s_bindlessInputLayout;
static nvrhi::SamplerHandle s_linearSampler;
static nvrhi::ShaderHandle s_bindlessVS;
static nvrhi::ShaderHandle s_bindlessPS;
static nvrhi::TextureHandle s_placeholderTexture;
static bool s_bindlessInitialized = false;

void renderBindlessForward(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!config.IsValid() || !geometry || geometry->GetBatches().empty())
        return;

    // Finalize any pending materials (populate texture atlases)
    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
    }

    // Upload material buffer to GPU
    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    // ═══════════════════════════════════════════════════════
    //  LAZY INITIALIZATION OF BINDLESS RESOURCES
    // ═══════════════════════════════════════════════════════
    if (!s_bindlessInitialized) {
        // Load shaders using ShaderLoader
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        if (!shaderLoader) {
            Msg("! [BindlessForward] ShaderLoader not available");
            return;
        }

        auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
        auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

        if (!vsResult.handle || !psResult.handle) {
            Msg("! [BindlessForward] Failed to load bindless shaders");
            return;
        }

        s_bindlessVS = vsResult.handle;
        s_bindlessPS = psResult.handle;

        // Create sampler
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);

        // Create binding layout
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(2),  // static_globals (b2) - engine matrices
            nvrhi::BindingLayoutItem::ConstantBuffer(4),  // LightingConstants (b4)
            nvrhi::BindingLayoutItem::ConstantBuffer(5),  // PerDrawConstants (b5) - world matrix + materialID
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),   // g_Materials
            nvrhi::BindingLayoutItem::Sampler(0),
            // 4 texture atlas arrays (simplified from 72)
            nvrhi::BindingLayoutItem::Texture_SRV(10),    // g_DiffuseAtlas
            nvrhi::BindingLayoutItem::Texture_SRV(11),    // g_NormalAtlas
            nvrhi::BindingLayoutItem::Texture_SRV(12),    // g_DetailAtlas
            nvrhi::BindingLayoutItem::Texture_SRV(13),    // g_PBRAtlas
        };
        s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

        // Create input layout matching UnifiedVertex format for GPU-driven rendering
        // UnifiedVertex has pre-unpacked UVs and vertex color:
        //   Position:  float3    at offset  0 (12 bytes)
        //   Normal:    D3DCOLOR  at offset 12 (4 bytes) - packed [-1,1] -> [0,1], A=hemi
        //   Tangent:   D3DCOLOR  at offset 16 (4 bytes) - packed
        //   Binormal:  D3DCOLOR  at offset 20 (4 bytes) - packed
        //   TexCoord0: float2    at offset 24 (8 bytes) - PRE-UNPACKED base UV
        //   TexCoord1: float2    at offset 32 (8 bytes) - lightmap UV
        //   Color:     D3DCOLOR  at offset 40 (4 bytes) - vertex color
        //   Flags:     uint      at offset 44 (4 bytes) - reserved
        // Total stride: 48 bytes
        constexpr u32 vertexStride = 48;
        nvrhi::VertexAttributeDesc vertexAttribs[] = {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RGB32_FLOAT)
                .setBufferIndex(0)
                .setOffset(0)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("NORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed normal + hemi
                .setBufferIndex(0)
                .setOffset(12)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TANGENT")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed tangent
                .setBufferIndex(0)
                .setOffset(16)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("BINORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed binormal
                .setBufferIndex(0)
                .setOffset(20)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)   // float2 = pre-unpacked UV
                .setArraySize(2)                        // TEXCOORD0 at offset 24, TEXCOORD1 at offset 32
                .setBufferIndex(0)
                .setOffset(24)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = vertex color
                .setBufferIndex(0)
                .setOffset(40)
                .setElementStride(vertexStride),
        };
        s_bindlessInputLayout = nvDevice->createInputLayout(vertexAttribs, 6, s_bindlessVS);

        // Create placeholder texture for empty atlas slots
        nvrhi::TextureDesc texDesc;
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.arraySize = 1;
        texDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
        texDesc.format = nvrhi::Format::RGBA8_UNORM;
        texDesc.isShaderResource = true;
        texDesc.debugName = "BindlessPlaceholder";
        texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        texDesc.keepInitialState = true;
        s_placeholderTexture = nvDevice->createTexture(texDesc);

        // Create graphics pipeline
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_bindlessVS;
        pipeDesc.PS = s_bindlessPS;
        pipeDesc.inputLayout = s_bindlessInputLayout;
        pipeDesc.bindingLayouts = { s_bindlessLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        // Depth prepass now renders bindless geometry with bindless_depth.vs
        // which uses IDENTICAL vertex transformation as bindless_forward.vs
        // So we can use Equal depth test for optimal early-Z performance
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;  // Already written in prepass
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
        pipeDesc.renderState.rasterState.frontCounterClockwise = false;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

        nvrhi::FramebufferDesc fbDesc;
        fbDesc.addColorAttachment(colorRT);
        fbDesc.setDepthAttachment(depthRT);
        auto framebuffer = nvDevice->createFramebuffer(fbDesc);

        s_bindlessPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!s_bindlessPipeline) {
            Msg("! [BindlessForward] Failed to create bindless pipeline");
            return;
        }

        s_bindlessInitialized = true;
        Msg("* [BindlessForward] Bindless rendering initialized");
    }

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAMEBUFFER AND RENDER STATE
    // ═══════════════════════════════════════════════════════
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════════
    // Note: Buffer size MUST match struct size exactly for D3D11 constant buffers
    // (partial updates via UpdateSubresource are not allowed)

    // Lighting constants struct - 96 bytes (6 x float4)
    struct LightingConstants {
        Fvector4 sunDirection;
        Fvector4 sunColor;
        Fvector4 ambientColor;
        Fvector4 cameraPosition;
        Fvector4 fogParams;
        Fvector4 fogColor;
    } lightingData;
    static_assert(sizeof(LightingConstants) == 96, "LightingConstants must be 96 bytes");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(LightingConstants);  // Must match exactly!
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    auto lightingCB = nvDevice->createBuffer(cbDesc);

    if (g_pGamePersistent) {
        auto& env = g_pGamePersistent->Environment().CurrentEnv;
        lightingData.sunDirection.set(env.sun_dir.x, env.sun_dir.y, env.sun_dir.z, 0.0f);
        lightingData.sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
        lightingData.fogParams.set(env.fog_near, env.fog_far, env.fog_density, 0.0f);
        lightingData.fogColor.set(env.fog_color.x, env.fog_color.y, env.fog_color.z, 1.0f);
    } else {
        lightingData.sunDirection.set(0.5f, -0.7f, 0.5f, 0.0f);
        lightingData.sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
        lightingData.fogParams.set(50.0f, 300.0f, 0.001f, 0.0f);
        lightingData.fogColor.set(0.5f, 0.5f, 0.6f, 1.0f);
    }
    lightingData.ambientColor.set(0.1f, 0.1f, 0.15f, 1.0f);
    lightingData.cameraPosition.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 1.0f);

    cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

    // Per-draw constants struct - 80 bytes (float4x4 + uint + padding)
    struct PerDrawConstants {
        Fmatrix world;
        u32 materialID;   // Direct material ID - no indirection through g_DrawMaterialIDs
        float padding[3];
    };
    static_assert(sizeof(PerDrawConstants) == 80, "PerDrawConstants must be 80 bytes");

    cbDesc.byteSize = sizeof(PerDrawConstants);  // Must match exactly!
    auto perDrawCB = nvDevice->createBuffer(cbDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE STATIC_GLOBALS BUFFER (engine matrices)
    // ═══════════════════════════════════════════════════════
    cbDesc.byteSize = sizeof(StaticGlobals);  // 768 bytes
    auto staticGlobalsCB = nvDevice->createBuffer(cbDesc);

    // Fill static globals with view/projection matrices
    StaticGlobals staticGlobals;
    FillGlobalConstants(staticGlobals);
    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING SET WITH ATLASES (Simplified - 4 arrays)
    // ═══════════════════════════════════════════════════════
    // Just 4 texture arrays: Diffuse (t10), Normal (t11), Detail (t12), PBR (t13)
    auto& atlasManager = TextureAtlasManager::Instance();

    // Helper to get atlas array or placeholder for empty atlases
    auto getAtlasOrPlaceholder = [&](AtlasType type) -> nvrhi::ITexture* {
        auto* atlas = atlasManager.GetAtlas(type).GetArray();
        return atlas ? atlas : s_placeholderTexture.Get();
    };

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),  // b2 - engine matrices (m_VP, etc.)
        nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),       // b4 - our lighting constants
        nvrhi::BindingSetItem::ConstantBuffer(5, perDrawCB),        // b5 - per-draw world matrix + materialID
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        // 4 texture atlas arrays
        nvrhi::BindingSetItem::Texture_SRV(10, getAtlasOrPlaceholder(AtlasType::Diffuse)),
        nvrhi::BindingSetItem::Texture_SRV(11, getAtlasOrPlaceholder(AtlasType::Normal)),
        nvrhi::BindingSetItem::Texture_SRV(12, getAtlasOrPlaceholder(AtlasType::Detail)),
        nvrhi::BindingSetItem::Texture_SRV(13, getAtlasOrPlaceholder(AtlasType::PBR)),
    };

    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessLayout);
    if (!bindingSet) {
        Msg("! [BindlessForward] Failed to create binding set");
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  RENDER VISIBLE BATCHES
    // ═══════════════════════════════════════════════════════
    cmdList->beginMarker("Bindless Forward");

    const auto& batches = geometry->GetBatches();
    // Don't limit draw count by GPU culling results - we're using direct draw, not indirect
    // GPU culling's previousFrameVisibleCount is for indirect draw path only
    u32 drawCount = static_cast<u32>(batches.size());

    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    // Check if mega-buffers are available for GPU-driven rendering
    const bool useMegaBuffers = config.UseMegaBuffers();

    // Track mega-buffer vs legacy draw counts
    static u32 s_megaBufferDraws = 0;
    static u32 s_legacyDraws = 0;
    static u32 s_skippedNoMaterial = 0;
    static u32 s_skippedNoAlloc = 0;
    static u32 s_visTypeCounts[16] = {0};  // Track visual types being rendered

    for (u32 i = 0; i < drawCount; i++) {
        const auto& batch = batches[i];

        // Skip batches with invalid material ID
        if (batch.bindlessMaterialID == UINT32_MAX) {
            s_skippedNoMaterial++;
            continue;
        }

        // Track visual type
        if (batch.visual && batch.visual->Type < 16) {
            s_visTypeCounts[batch.visual->Type]++;
        }

        // No visual type filtering - rely on megaBufferAlloc.valid instead
        // If geometry was successfully converted during level load, it has a valid allocation
        // Unsupported formats (skeleton animations, particles) won't have valid allocations

        // Determine which buffers to use
        nvrhi::IBuffer* vb = nullptr;
        nvrhi::IBuffer* ib = nullptr;
        nvrhi::Format indexFormat = nvrhi::Format::R16_UINT;
        u32 startIndex = batch.startIndex;
        s32 baseVertex = batch.baseVertex;

        if (useMegaBuffers && batch.megaBufferAlloc.valid) {
            // GPU-driven path: use unified mega-buffers
            vb = config.megaVertexBuffer;
            ib = config.megaIndexBuffer;
            indexFormat = nvrhi::Format::R32_UINT;  // Mega-buffer uses 32-bit indices
            startIndex = batch.megaBufferAlloc.indexOffset;
            baseVertex = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
            s_megaBufferDraws++;
        } else {
            // Legacy path: use per-batch D3D11 buffers
            if (!batch.vertexBuffer || !batch.indexBuffer) {
                s_skippedNoAlloc++;
                continue;
            }
            vb = batch.vertexBuffer.Get();
            ib = batch.indexBuffer.Get();
            s_legacyDraws++;
        }

        // Update per-draw constants (PerDrawConstants struct defined above)
        // CRITICAL: HLSL expects row-major matrices, X-Ray uses column-major
        // Must transpose to match FillGlobalConstants convention for m_VP
        PerDrawConstants perDrawData;
        perDrawData.world.transpose(batch.worldMatrix);
        perDrawData.materialID = batch.bindlessMaterialID;  // Direct material ID lookup
        perDrawData.padding[0] = perDrawData.padding[1] = perDrawData.padding[2] = 0.0f;
        cmdList->writeBuffer(perDrawCB, &perDrawData, sizeof(perDrawData));

        // Set graphics state
        nvrhi::GraphicsState state;
        state.pipeline = s_bindlessPipeline;
        state.framebuffer = framebuffer;
        state.bindings = { bindingSet };
        state.vertexBuffers = { {vb, 0, 0} };
        state.indexBuffer = { ib, indexFormat, 0 };
        state.viewport.addViewport(viewport);
        state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

        cmdList->setGraphicsState(state);

        // Draw using computed parameters
        nvrhi::DrawArguments drawArgs;
        drawArgs.vertexCount = batch.indexCount;          // Index count for indexed draw
        drawArgs.startIndexLocation = startIndex;         // Start offset in index buffer
        drawArgs.startVertexLocation = baseVertex;        // Base vertex offset in vertex buffer
        drawArgs.instanceCount = 1;
        drawArgs.startInstanceLocation = 0;
        cmdList->drawIndexed(drawArgs);
    }

    cmdList->endMarker();

    // Only log occasionally to avoid spam
    static u32 s_logCounter = 0;
    if (++s_logCounter % 300 == 0) {
        Msg("* [BindlessForward] Rendered %u draws (mega: %u, legacy: %u, skipped: noMat=%u noAlloc=%u)",
            drawCount, s_megaBufferDraws, s_legacyDraws, s_skippedNoMaterial, s_skippedNoAlloc);
        // MT enum: 0=NORMAL, 1=HIERRARHY, 2=PROGRESSIVE, 3=SKEL_ANIM, 4=SKEL_PM, 5=SKEL_ST, 6=LOD, 7=TREE_ST, 8=PARTICLE, 9=PARTICLE_GRP, 10=SKEL_RIGID, 11=TREE_PM
        Msg("  VisTypes: NORMAL=%u, HIERRARHY=%u, PROGRESSIVE=%u, TREE_ST=%u, TREE_PM=%u, LOD=%u",
            s_visTypeCounts[0], s_visTypeCounts[1], s_visTypeCounts[2],
            s_visTypeCounts[7], s_visTypeCounts[11], s_visTypeCounts[6]);
        s_megaBufferDraws = 0;
        s_legacyDraws = 0;
        s_skippedNoMaterial = 0;
        s_skippedNoAlloc = 0;
        std::memset(s_visTypeCounts, 0, sizeof(s_visTypeCounts));
    }
}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsInput,
    const BindlessForwardConfig& bindlessConfig)
{
    using namespace framegraph;

    // PassData structure to hold data between setup and execute
    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;              // Single HDR color buffer (RGBA16_FLOAT)
        VirtualResourceHandle drawArgsBuffer;     // GPU culling draw args (optional)
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        BindlessForwardConfig bindlessConfig;     // Bindless rendering configuration
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height, colorInput, drawArgsInput, bindlessConfig](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            // Store pass configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            // Declare resource usage
            RenderPassBuilder passBuilder(builder, passHandle);

            // Depth buffer from prepass (READ-WRITE for early-Z)
            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);

            // Color buffer from sky pass (READ-WRITE to preserve sky background)
            // CRITICAL: Using readWrite() creates a dependency on SkyPass!
            // This ensures sky renders first (as background), then forward geometry on top.
            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);

            // Draw args buffer from GPU culling (READ for indirect draw)
            // CRITICAL: This creates proper dependency on GPU culling pass!
            // Without this, culling and forward passes can race, causing "exploding geometry"
            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            // Store outputs in PassData for MaterialCache
            // Forward+ uses single RT output (all shaders converted to f_forward)
            data.outputs.albedo = data.color;    // Single HDR color output
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA (Renders geometry)
        // ═══════════════════════════════════════════════════════
        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

                // Get physical resources from virtual handles
                auto* depthRT = fg.GetPhysicalTexture(data.depth);
                auto* colorRT = fg.GetPhysicalTexture(data.color);

                if (!depthRT || !colorRT) {
                    return;
                }

                // Check if we have geometry to render
                if (!data.geometry || data.geometry->GetBatches().empty()) {
                    // No geometry - clear RTs and exit
                    // TODO: Add clear commands if needed
                    return;
                }

                // Get draw args buffer through framegraph (proper dependency tracking)
                // This ensures state transition UAV -> IndirectArgument happened
                nvrhi::IBuffer* drawArgsBuffer = nullptr;
                if (data.drawArgsBuffer.is_valid()) {
                    drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
                }

                // ═══════════════════════════════════════════════════════
                //  BINDLESS RENDERING PATH (GPU-DRIVEN MULTI-DRAW)
                // ═══════════════════════════════════════════════════════
                // When bindless is enabled and valid, use GPU-driven rendering
                // with all materials accessed via material buffer
                if (data.bindlessConfig.IsValid()) {
                    renderBindlessForward(
                        ctx,
                        data.device,
                        data.geometry,
                        colorRT,
                        depthRT,
                        data.bindlessConfig,
                        data.materialCache
                    );
                    return;
                }

                // ═══════════════════════════════════════════════════════
                //  STANDARD RENDERING PATH (PER-MATERIAL PSO)
                // ═══════════════════════════════════════════════════════
                // Traditional per-material rendering with individual PSOs
                renderForwardGeometry(
                    ctx,
                    data.device,
                    data.geometry,
                    colorRT,
                    depthRT,
                    data.materialCache,
                    data.outputs,
                    fg,
                    drawArgsBuffer
                );
        }
    );

    // Return the forward color outputs
    // All outputs point to the same color buffer for legacy shader compatibility
    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;      // Primary color output
    outputs.normal = passData.color;      // Same buffer (legacy compatibility)
    outputs.material = passData.color;    // Same buffer (legacy compatibility)
    outputs.depth = passData.depth;
    return outputs;
}
} // namespace xray::render::RENDER_NAMESPACE::passes
