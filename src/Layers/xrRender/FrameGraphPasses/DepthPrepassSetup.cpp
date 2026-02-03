// xrRender/FrameGraphPasses/DepthPrepassSetup.cpp
#include "stdafx.h"
#include "DepthPrepassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
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

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Per-draw constants for bindless depth - MUST match shader layout exactly
struct BindlessDepthPerDrawConstants {
    Fmatrix world;
    u32 materialID;
    float padding[3];
};
static_assert(sizeof(BindlessDepthPerDrawConstants) == 80, "BindlessDepthPerDrawConstants must be 80 bytes");

// Static bindless depth pipeline and shaders
static nvrhi::ShaderHandle s_bindlessDepthVS;
static nvrhi::InputLayoutHandle s_bindlessDepthInputLayout;
static nvrhi::GraphicsPipelineHandle s_bindlessDepthPipeline;
static nvrhi::BindingLayoutHandle s_bindlessDepthBindingLayout;
static nvrhi::BufferHandle s_bindlessDepthStaticGlobalsCB;  // m_VP matrix (b0-b2 in common.h)
static nvrhi::BufferHandle s_bindlessDepthPerDrawCB;         // g_World matrix (b5)
static bool s_bindlessDepthInitialized = false;

void renderDepthOnlyGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::FrameGraph& fg,
    const BindlessDepthConfig& bindlessConfig = {})
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

    // ═══════════════════════════════════════════════════════
    //  FILL CONSTANT BUFFERS FROM DEVICE STATE
    // ═══════════════════════════════════════════════════════
    // Depth prepass only needs transforms, not lighting

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    // ═══════════════════════════════════════════════════════
    //  UPDATE GLOBAL CBS BEFORE RENDER PASS
    // ═══════════════════════════════════════════════════════

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            // Phase 2.4: Use optimized depth-only PSO
            MaterialPSO* matPSO = materialCache->GetOrCreateDepthPSO(batch.visual, fg);

            if (matPSO) {
                // Update all global CBs
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.nvrhiBuffer) {
                        if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                            if (cbInfo.name == "static_globals") {
                                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            }
                            else if (cbInfo.name == "dynamic_transforms") {
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
    //  SETUP DEPTH-ONLY RENDER PASS
    // ═══════════════════════════════════════════════════════
    // No color targets - depth buffer only
    // This is the key optimization: minimal pixel shader work

    ng::RenderPassDesc passDesc;
    passDesc.passName = "Depth Prepass (Early-Z)";
    passDesc.numRenderTargets = 0;           // NO color output!
    passDesc.depthStencil = depthRT;

    // Clear depth to far plane (1.0)
    passDesc.clearValue.depth = 1.0f;
    passDesc.clearDepth = true;

    // D32 format has NO stencil channel - don't try to clear it!
    passDesc.clearStencil = false;

    // Begin render pass
    ctx->BeginRenderPass(passDesc);

    // Set viewport
    const auto& depthDesc = depthRT->getDesc();
    ctx->SetViewport(0, 0,
        static_cast<float>(depthDesc.width),
        static_cast<float>(depthDesc.height));

    // Set scissor (full screen)
    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = depthDesc.width;
    scissor.height = depthDesc.height;
    ctx->SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  RENDER GEOMETRY BATCHES (Depth-only, OPAQUE ONLY)
    // ═══════════════════════════════════════════════════════
    // Depth prepass only renders opaque geometry (iPriority == 0):
    //   - Skip alpha-tested (iPriority == 1) - they use clip() which creates holes
    //   - Skip transparent (bStrictB2F) - they don't write depth
    //
    // Alpha-tested geometry will write depth during the color pass.

    ng::PipelineState* currentPipeline = nullptr;

    for (const auto& batch : batches) {
        // Skip non-opaque geometry (alpha-tested and transparent)
        if (!batch.IsOpaque()) {
            continue;
        }

        // Skip bindless geometry here - render it separately with bindless depth shader
        // This ensures same vertex transformation as forward pass (no Z-fighting)
        if (batch.megaBufferAlloc.valid) {
            continue;
        }

        // Get per-material PSO from MaterialCache
        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            // Phase 2.4: Use optimized depth-only PSO
            matPSO = materialCache->GetOrCreateDepthPSO(batch.visual, fg);

            if (matPSO && matPSO->pso) {
                pipelineToUse = matPSO->pso;
            }
        }

        if (!pipelineToUse) {
            continue;  // Skip batches without valid PSO
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
            //  MATERIAL-FREQUENCY CONSTANTS
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
                    //Parent->CalculateBones(TRUE);

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
            continue;  // Skip batches with null buffers
        }

        ctx->SetVertexBuffer(0, vb, 0);
        ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);  // X-Ray uses 16-bit indices

        // Draw
        ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
    }

    // ═══════════════════════════════════════════════════════
    //  BINDLESS DEPTH RENDERING (same transform as forward pass)
    // ═══════════════════════════════════════════════════════
    // Render bindless geometry with bindless_depth.vs to match forward pass depth values
    // This prevents Z-fighting between depth prepass and bindless forward pass

    if (bindlessConfig.UseMegaBuffers()) {
        nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
        nvrhi::ICommandList* cmdList = ctx->GetCommandList();

        // Initialize bindless depth pipeline on first use
        if (!s_bindlessDepthInitialized) {
            // Load bindless_depth.vs shader using proper ShaderLoader API
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            if (!shaderLoader) {
                Msg("! [BindlessDepth] ShaderLoader not available");
            }
            else {
                auto vsResult = shaderLoader->LoadVertexShader("bindless_depth", "main");
                if (!vsResult.handle) {
                    Msg("! [BindlessDepth] Failed to load bindless_depth.vs");
                }
                else {
                    s_bindlessDepthVS = vsResult.handle;

                    auto& cache = framegraph::GetPassResourceCache();

                    // Create input layout for UnifiedVertex (48 bytes) - matches forward pass
                    constexpr u32 vertexStride = 48;
                    nvrhi::VertexAttributeDesc vertexAttribs[] = {
                        nvrhi::VertexAttributeDesc()
                            .setName("POSITION")
                            .setFormat(nvrhi::Format::RGB32_FLOAT)
                            .setOffset(0)
                            .setElementStride(vertexStride),
                        nvrhi::VertexAttributeDesc()
                            .setName("NORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setOffset(12)
                            .setElementStride(vertexStride),
                        nvrhi::VertexAttributeDesc()
                            .setName("TANGENT")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setOffset(16)
                            .setElementStride(vertexStride),
                        nvrhi::VertexAttributeDesc()
                            .setName("BINORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setOffset(20)
                            .setElementStride(vertexStride),
                        nvrhi::VertexAttributeDesc()
                            .setName("TEXCOORD")
                            .setFormat(nvrhi::Format::RG32_FLOAT)
                            .setArraySize(2)
                            .setOffset(24)
                            .setElementStride(vertexStride),
                        nvrhi::VertexAttributeDesc()
                            .setName("COLOR")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setOffset(40)
                            .setElementStride(vertexStride),
                    };
                    s_bindlessDepthInputLayout = cache.GetOrCreateInputLayout(
                        "BindlessDepthPass", vertexAttribs, std::size(vertexAttribs), s_bindlessDepthVS, nvDevice);

                    // Create binding layout - needs static globals (m_VP) and per-draw CB (g_World)
                    // common.h defines m_VP in static_globals at register(b2), NOT b0!
                    nvrhi::BindingLayoutDesc bindLayoutDesc;
                    bindLayoutDesc.visibility = nvrhi::ShaderType::Vertex;
                    bindLayoutDesc.bindings = {
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (m_VP at b2)
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // PerDrawConstants (g_World)
                    };
                    s_bindlessDepthBindingLayout = cache.GetOrCreateBindingLayout("BindlessDepthPass", bindLayoutDesc, nvDevice);

                    // Create static globals CB for m_VP (matches common.h layout at b2)
                    // Must be full StaticGlobals size (768 bytes) to match shader expectations
                    nvrhi::BufferDesc globalsCBDesc;
                    globalsCBDesc.debugName = "BindlessDepthStaticGlobalsCB";
                    globalsCBDesc.byteSize = sizeof(StaticGlobals);  // 768 bytes, contains m_VP at offset 112
                    globalsCBDesc.isConstantBuffer = true;
                    globalsCBDesc.isVolatile = true;
                    globalsCBDesc.maxVersions = 16;
                    globalsCBDesc.keepInitialState = true;
                    globalsCBDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                    s_bindlessDepthStaticGlobalsCB = nvDevice->createBuffer(globalsCBDesc);

                    // Create per-draw constant buffer - size MUST match struct exactly
                    nvrhi::BufferDesc cbDesc;
                    cbDesc.debugName = "BindlessDepthPerDrawCB";
                    cbDesc.byteSize = sizeof(BindlessDepthPerDrawConstants);  // 80 bytes
                    cbDesc.isConstantBuffer = true;
                    cbDesc.isVolatile = true;
                    cbDesc.maxVersions = 256;
                    cbDesc.keepInitialState = true;
                    cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                    s_bindlessDepthPerDrawCB = nvDevice->createBuffer(cbDesc);

                    // Create depth-only pipeline (cached)
                    nvrhi::GraphicsPipelineDesc pipeDesc;
                    pipeDesc.VS = s_bindlessDepthVS;
                    pipeDesc.PS = nullptr;  // No pixel shader - depth only
                    pipeDesc.inputLayout = s_bindlessDepthInputLayout;
                    pipeDesc.bindingLayouts = { s_bindlessDepthBindingLayout };
                    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
                    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
                    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
                    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
                    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                    // Create framebuffer desc for pipeline creation (cached)
                    nvrhi::FramebufferDesc fbDesc;
                    fbDesc.setDepthAttachment(depthRT);

                    nvrhi::FramebufferHandle fb = cache.GetOrCreateFramebuffer("BindlessDepthPass", fbDesc, nvDevice);
                    nvrhi::FramebufferInfoEx fbInfo = fb->getFramebufferInfo();
                    s_bindlessDepthPipeline = cache.GetOrCreatePipeline("BindlessDepthPass", pipeDesc, fbInfo, nvDevice);

                    if (s_bindlessDepthPipeline) {
                        Msg("* [BindlessDepth] Pipeline created successfully");
                        s_bindlessDepthInitialized = true;
                    } else {
                        Msg("! [BindlessDepth] Failed to create pipeline");
                    }
                }
            }
        }

        // Render bindless geometry
        if (s_bindlessDepthInitialized && s_bindlessDepthPipeline) {
            auto& cache = framegraph::GetPassResourceCache();

            // Create framebuffer for this render (cached)
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(depthRT);
            nvrhi::FramebufferHandle framebuffer = cache.GetOrCreateFramebuffer("BindlessDepthPass_Render", fbDesc, nvDevice);

            // Set viewport
            const auto& depthDesc = depthRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(depthDesc.width),
                                     0.0f, static_cast<float>(depthDesc.height), 0.0f, 1.0f);

            // Fill static globals CB once per frame (contains m_VP at offset 112)
            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            cmdList->writeBuffer(s_bindlessDepthStaticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            u32 bindlessDraws = 0;

            for (const auto& batch : batches) {
                // Only render opaque bindless geometry
                if (!batch.IsOpaque() || !batch.megaBufferAlloc.valid) {
                    continue;
                }

                // Update per-draw constants (world matrix)
                BindlessDepthPerDrawConstants perDrawData;
                perDrawData.world.transpose(batch.worldMatrix);
                perDrawData.materialID = batch.bindlessMaterialID;
                perDrawData.padding[0] = perDrawData.padding[1] = perDrawData.padding[2] = 0.0f;
                cmdList->writeBuffer(s_bindlessDepthPerDrawCB, &perDrawData, sizeof(perDrawData));

                // Create binding set - slots must match binding layout (b2 and b5)
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(2, s_bindlessDepthStaticGlobalsCB),  // static_globals (m_VP at b2)
                    nvrhi::BindingSetItem::ConstantBuffer(5, s_bindlessDepthPerDrawCB),        // PerDrawConstants (g_World at b5)
                };
                auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessDepthBindingLayout);

                // Set graphics state
                nvrhi::GraphicsState state;
                state.pipeline = s_bindlessDepthPipeline;
                state.framebuffer = framebuffer;
                state.bindings = { bindingSet };
                state.vertexBuffers = { {bindlessConfig.megaVertexBuffer, 0, 0} };
                state.indexBuffer = { bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
                state.viewport.addViewport(viewport);
                state.viewport.addScissorRect(nvrhi::Rect(depthDesc.width, depthDesc.height));

                cmdList->setGraphicsState(state);

                // Draw
                nvrhi::DrawArguments drawArgs;
                drawArgs.vertexCount = batch.indexCount;
                drawArgs.startIndexLocation = batch.megaBufferAlloc.indexOffset;
                drawArgs.startVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
                drawArgs.instanceCount = 1;
                drawArgs.startInstanceLocation = 0;
                cmdList->drawIndexed(drawArgs);

                bindlessDraws++;
            }
        }
    }

    // End render pass
    ctx->EndRenderPass();
}

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const BindlessDepthConfig& bindlessConfig)
{
    using namespace framegraph;

    // PassData structure to hold data between setup and execute
    struct DepthPrepassData {
        VirtualResourceHandle depth;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        u32 width;
        u32 height;
        BindlessDepthConfig bindlessConfig;
    };

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "Depth Prepass (Early-Z)",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height, bindlessConfig](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            // Store pass configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            // Declare resource usage
            RenderPassBuilder passBuilder(builder, passHandle);

            // ═══════════════════════════════════════════════════════
            //  DEPTH BUFFER CREATION (Phase 2.1)
            // ═══════════════════════════════════════════════════════
            // Create depth buffer that will be reused in forward pass
            // Early-Z optimization: depth written here, tested in forward pass
            // Format: D24S8 or D32F (GPU-dependent, NVRHI abstracts this)

            data.depth = passBuilder.createDepthBuffer(
                "rt_DepthPrepass",
                width,
                height
            );
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA (Renders depth-only geometry)
        // ═══════════════════════════════════════════════════════
        [](const DepthPrepassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            // Get physical depth buffer from virtual handle
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!depthRT) {
                return;
            }

            // Check if we have geometry to render
            if (!data.geometry) {
                return;
            }

            // Render geometry depth-only (no color output)
            renderDepthOnlyGeometry(
                ctx,
                data.device,
                data.geometry,
                depthRT,
                data.materialCache,
                fg,
                data.bindlessConfig
            );
        }
    );

    // Return the depth buffer handle for reuse in forward pass
    return passData.depth;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
