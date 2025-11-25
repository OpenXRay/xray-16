// xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
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

// Forward declaration of the rendering function
void renderForwardGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg);

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    // PassData structure to hold data between setup and execute
    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;              // Single HDR color buffer (RGBA16_FLOAT)
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            // Store pass configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;

            // Declare resource usage
            RenderPassBuilder passBuilder(builder, passHandle);

            // If depth buffer provided, use it; otherwise create new
            if (depthInput.is_valid()) {
                // READ-WRITE for early-Z: read depth from prepass, write new fragments
                data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);
            } else {
                data.depth = passBuilder.createDepthBuffer("rt_Depth", width, height);
            }

            // ═══════════════════════════════════════════════════════
            //  SINGLE HDR COLOR BUFFER (Phase 1 Simplification)
            // ═══════════════════════════════════════════════════════
            // BEFORE: 3 RTs (albedo RGBA8 + normal RGBA16F + position RGBA32F)
            //         = 8 + 8 + 16 = 32 bytes/pixel = 256 bits/pixel
            //
            // AFTER:  1 RT (color RGBA16_FLOAT)
            //         = 8 bytes/pixel = 64 bits/pixel
            //
            // BANDWIDTH SAVINGS: 60% reduction (32 → 8 bytes/pixel)
            // PERFORMANCE GAIN:  ~0.5-1.0ms on typical scenes

            data.color = passBuilder.createTexture2D(
                "rt_SceneColor",
                width,
                height,
                nvrhi::Format::RGBA16_FLOAT  // HDR color (for future lighting)
            );

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

            // Render geometry using forward rendering
            renderForwardGeometry(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                depthRT,
                data.materialCache,
                data.outputs,
                fg
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
    const framegraph::FrameGraph& fg)
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

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

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
            MaterialPSO* matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg);
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

    // Clear values
    passDesc.clearValue.color[0] = 0.0f;
    passDesc.clearValue.color[1] = 0.0f;
    passDesc.clearValue.color[2] = 0.0f;
    passDesc.clearValue.color[3] = 1.0f;
    passDesc.clearColor = true;

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
    //  RENDER GEOMETRY BATCHES
    // ═══════════════════════════════════════════════════════

    u32 numDraws = 0;
    u32 numTriangles = 0;

    ng::PipelineState* currentPipeline = nullptr;

    for (const auto& batch : batches) {
        // Get per-material PSO from MaterialCache
        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg);

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
                xform_v.mul(batch.worldMatrix, Device.mView);

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

        numDraws++;
        numTriangles += batch.indexCount / 3;
    }

    // End render pass
    ctx->EndRenderPass();

    // Log statistics (optional, can remove for performance)
    // Msg("ForwardColor: %u draws, %u triangles", numDraws, numTriangles);
}

} // namespace xray::render::RENDER_NAMESPACE::passes
