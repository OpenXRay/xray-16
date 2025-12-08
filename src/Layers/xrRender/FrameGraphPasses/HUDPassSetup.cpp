// xrRender/FrameGraphPasses/HUDPassSetup.cpp
#include "stdafx.h"
#include "HUDPassSetup.h"
#include "ShaderConstants.h"  // For DynamicTransforms, FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics
#include "Layers/xrRender/FSkinned.h"  // For CSkeletonX
#include "Layers/xrRender/SkeletonX.h"  // For CSkeletonX_ST, CSkeletonX_PM
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"  // For reflection-driven constants
#include "Layers/xrRender/FTreeVisual.h"  // For FTreeVisual_quant constant

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {

// Apply HUD FOV adjustment to world matrix
static Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    float fovScale = 1.0f / psHUD_FOV;
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;
    fovScaleMatrix._22 = fovScale;
    fovScaleMatrix._33 = 1.0f;

    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);
    temp2.mul(fovScaleMatrix, temp1);
    result.mul(invView, temp2);

    return result;
}

framegraph::DefaultOutputLayout setupHUDPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& gbufferInputs,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct HUDPassData {
        // Input from Forward+ color pass
        VirtualResourceHandle inputColor;
        VirtualResourceHandle depth;  // Read-write depth

        // Output (same as input, HUD renders on top)
        VirtualResourceHandle outputColor;

        // HUD geometry
        ng::RenderDevice* device;
        const xr_vector<GeometryBatch>* hudBatches;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;  // Store for MaterialCache
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<HUDPassData>(
        "HUD",

        // Setup lambda
        [&, width, height](FrameGraph& builder, PassHandle passHandle, HUDPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            // Store configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;

            // Read Forward+ color input
            data.inputColor = passBuilder.read(gbufferInputs.albedo);

            // Write to the same target (HUD renders on top)
            data.outputColor = passBuilder.write(gbufferInputs.albedo, ResourceState::RenderTarget);

            // Read-write depth (for depth testing)
            data.depth = passBuilder.readWrite(gbufferInputs.depth, ResourceState::DepthStencilWrite);

            // Store outputs for MaterialCache (Forward+ single-RT)
            data.outputs.albedo = data.outputColor;
            data.outputs.depth = data.depth;
        },

        // Execute lambda
        [](const HUDPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            using RENDER_NAMESPACE::CSkeletonX_ST;
            using RENDER_NAMESPACE::CSkeletonX_PM;

            // Skip if no HUD batches
            if (!data.hudBatches || data.hudBatches->empty() || !data.materialCache || !data.device) {
                //Msg("* [HUDPass] No batches (batches=%p, empty=%d, matCache=%p, device=%p)",
                //    data.hudBatches, data.hudBatches ? data.hudBatches->empty() : -1,
                //    data.materialCache, data.device);
                return;
            }

            // Processing HUD batches (logging disabled to reduce spam)

            // Get physical resources (Forward+ single-RT)
            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!colorRT || !depthRT) {
                Msg("! [HUDPass] Failed to get physical textures");
                return;
            }

            // ═══════════════════════════════════════════════════════
            //  PREPARE GLOBAL CB DATA
            // ═══════════════════════════════════════════════════════

            StaticGlobals staticGlobalsCB = {};
            FillGlobalConstants(staticGlobalsCB);

            // Get actual sun data from RImplementation.Lights.sun
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);  // HDR multiplier
            FillSunConstants(staticGlobalsCB, sunData);

            DynamicTransforms dynamicTransformsCB = {};
            FillDynamicTransforms(dynamicTransformsCB);

            // ═══════════════════════════════════════════════════════
            //  UPDATE GLOBAL CBS BEFORE RENDER PASS
            // ═══════════════════════════════════════════════════════

            xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

            for (const auto& batch : *data.hudBatches) {
                if (batch.visual && data.materialCache) {
                    MaterialPSO* matPSO = data.materialCache->GetOrCreatePSO(batch.visual, data.outputs, fg, RenderPassType::HUD);
                    if (matPSO) {
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

            // Setup render pass (no clear - HUD renders on top of world)
            ng::RenderPassDesc passDesc;
            passDesc.passName = "HUD Pass";
            passDesc.renderTargets[0] = colorRT;
            passDesc.numRenderTargets = 1;
            passDesc.depthStencil = depthRT;
            passDesc.clearColor = false;
            passDesc.clearDepth = false;
            passDesc.clearStencil = false;

            ctx->BeginRenderPass(passDesc);

            // Set viewport with depth range [0.0, 0.1]
            ng::Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)data.width;
            viewport.height = (float)data.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 0.1f;
            ctx->SetViewport(viewport);

            ng::Rect scissor;
            scissor.x = 0;
            scissor.y = 0;
            scissor.width = data.width;
            scissor.height = data.height;
            ctx->SetScissor(scissor);

            // Render HUD batches
            ng::PipelineState* currentPipeline = nullptr;
            MaterialPSO* lastMatPSO = nullptr;
            u32 numDraws = 0;

            for (const auto& batch : *data.hudBatches) {
                if (!batch.visual) {
                    Msg("! [HUDPass] Batch has no visual");
                    continue;
                }

                // Get per-material PSO (HUD pass type for correct depth state)
                MaterialPSO* matPSO = data.materialCache->GetOrCreatePSO(batch.visual, data.outputs, fg, RenderPassType::HUD);
                if (!matPSO || !matPSO->pso) {
                    // Failed to get PSO (logging disabled to reduce spam)
                    continue;
                }

                // Bind pipeline
                if (matPSO->pso != currentPipeline) {
                    ctx->SetPipeline(matPSO->pso->GetNativePipeline());
                    currentPipeline = matPSO->pso;
                }

                // Apply HUD FOV adjustment to world matrix
                Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);

                // ═══════════════════════════════════════════════════════
                //  UPDATE MATERIAL AND INSTANCE CONSTANTS (USING FGCONSTANTSYSTEM)
                // ═══════════════════════════════════════════════════════

                // Initialize FGConstantSystem for this batch
                fgconstants::FGConstantSystem constants(matPSO, data.materialCache->GetVCBPool());

                // ─────────────────────────────────────────────────
                //  MATERIAL-FREQUENCY CONSTANTS (once per material)
                // ─────────────────────────────────────────────────

                // Only update material constants if material changed
                if (matPSO != lastMatPSO) {
                    // Set material-specific detail scale (now in shader_params b1)
                    Fvector4 dt_params(
                        matPSO->detail_scale,
                        matPSO->detail_scale,
                        matPSO->detail_scale,
                        1.0f / r__dtex_range
                    );
                    constants.Set("dt_params", dt_params);

                    // Commit material-frequency constants to GPU
                    constants.CommitMaterial(ctx);

                    lastMatPSO = matPSO;
                }

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
                    // Skeleton meshes: Upload dynamic_transforms manually (includes dt_params from material)
                    // FIXME: This is a temporary solution until we migrate dynamic_transforms to FGConstantSystem
                    DynamicTransforms dynamicCB = {};
                    FillDynamicTransforms(dynamicCB, adjustedWorldMatrix);
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (cbInfo.name == "dynamic_transforms") {
                            u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                            ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                            break;
                        }
                    }

                    // Get Parent skeleton
                    RENDER_NAMESPACE::CKinematics* Parent = nullptr;
                    if (visualType == MT_SKELETON_GEOMDEF_ST) {
                        auto* skeletonMesh = static_cast<CSkeletonX_ST*>(batch.visual);
                        Parent = skeletonMesh->GetParent();
                    } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                        auto* skeletonMesh = static_cast<CSkeletonX_PM*>(batch.visual);
                        Parent = skeletonMesh->GetParent();
                    }

                    if (Parent) {
                        // Calculate bones BEFORE accessing transforms
                        Parent->CalculateBones(TRUE);

                        // Get bone count
                        u32 count = Parent->LL_BoneCount();

                        // Build bone matrix array
                        constexpr u32 MAX_BONES = 78;
                        Fmatrix boneMatrices[MAX_BONES];
                        u32 bonesToUpload = std::min(count, MAX_BONES);

                        for (u32 i = 0; i < bonesToUpload; i++) {
                            boneMatrices[i] = Parent->LL_GetTransform_R(u16(i));
                        }

                        // Use FGConstantSystem for bone array upload
                        constants.SetArray("sbones_array", boneMatrices, bonesToUpload);
                    }
                } else {
                    // Non-skeleton meshes (trees, weapons): Use FGConstantSystem
                    Fmatrix xform_v;
                    // mul_43(A,B) = B then A, so View*World = World then View (object->world->view)
                    xform_v.mul_43(Device.mView, adjustedWorldMatrix);

                    Fmatrix invW;
                    invW.invert(adjustedWorldMatrix);

                    constants.Set("m_xform", adjustedWorldMatrix);
                    constants.Set("m_xform_v", xform_v);
                    constants.Set("m_invW", invW);

                    // Tree scale constant (for tree meshes)
                    using namespace xray::render::RENDER_NAMESPACE;
                    float tree_scale = 1.0f / float(FTreeVisual_quant);
                    Fvector4 consts_vec(tree_scale, tree_scale, 0.0f, 0.0f);
                    constants.Set("consts", consts_vec);
                }

                // Commit per-instance constants to GPU
                constants.CommitInstance(ctx);

                // Get or create cached binding sets
                data.materialCache->GetOrCreateBindingSet(matPSO);

                // Bind BOTH per-stage binding sets
                ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
                ctx->SetBindingSet(1, matPSO->psBindingSet.Get());

                // Bind vertex/index buffers
                nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
                nvrhi::IBuffer* ib = batch.indexBuffer.Get();
                if (!vb || !ib) {
                    Msg("! [HUDPass] Missing buffers (vb=%p, ib=%p)", vb, ib);
                    continue;
                }

                ctx->SetVertexBuffer(0, vb, 0);
                ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

                // Draw
                ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
                numDraws++;
            }

            // HUD pass complete (logging disabled to reduce spam)
            ctx->EndRenderPass();
        }
    );

    // Return the modified outputs (Forward+ single-RT with HUD rendered on top)
    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
