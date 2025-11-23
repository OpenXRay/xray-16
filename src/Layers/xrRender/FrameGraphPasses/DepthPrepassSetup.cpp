// xrRender/FrameGraphPasses/DepthPrepassSetup.cpp
#include "stdafx.h"
#include "DepthPrepassSetup.h"
#include "ShaderConstants.h"
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

namespace xray::render::passes {

// Forward declaration of depth rendering function
void renderDepthOnlyGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::FrameGraph& fg);

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
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
    };

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "DepthPrepass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            // Store pass configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;

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
            if (!data.geometry || data.geometry->GetBatches().empty()) {
                // No geometry - just clear depth and exit
                // TODO: Add depth clear command if needed
                return;
            }

            // Render geometry depth-only (no color output)
            renderDepthOnlyGeometry(
                ctx,
                data.device,
                data.geometry,
                depthRT,
                data.materialCache,
                fg
            );
        }
    );

    // Return the depth buffer handle for reuse in forward pass
    return passData.depth;
}

// ═══════════════════════════════════════════════════════
//  DEPTH-ONLY RENDERING IMPLEMENTATION
// ═══════════════════════════════════════════════════════
// Simplified version of forward rendering - only writes depth, no color

void renderDepthOnlyGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
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
    //  SETUP DEPTH-ONLY RENDER PASS
    // ═══════════════════════════════════════════════════════
    // No color targets - depth buffer only
    // This is the key optimization: minimal pixel shader work

    ng::RenderPassDesc passDesc;
    passDesc.numRenderTargets = 0;           // NO color output!
    passDesc.depthStencil = depthRT;

    // Clear depth to far plane (1.0)
    passDesc.clearValue.depth = 1.0f;
    passDesc.clearValue.stencil = 0;
    passDesc.clearDepth = true;
    passDesc.clearStencil = true;

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
    //  RENDER GEOMETRY BATCHES (Depth-only)
    // ═══════════════════════════════════════════════════════

    ng::PipelineState* currentPipeline = nullptr;

    for (const auto& batch : batches) {
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
    }

    // End render pass
    ctx->EndRenderPass();
}

} // namespace xray::render::passes
