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

extern ENGINE_API float psHUD_FOV;

namespace xray::render::passes {

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
        // Input from GBuffer
        VirtualResourceHandle inputAlbedo;
        VirtualResourceHandle inputNormal;
        VirtualResourceHandle inputMaterial;
        VirtualResourceHandle depth;  // Read-write depth

        // Output (same as input, HUD renders on top)
        VirtualResourceHandle outputAlbedo;
        VirtualResourceHandle outputNormal;
        VirtualResourceHandle outputMaterial;

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

            // Read GBuffer inputs
            data.inputAlbedo = passBuilder.read(gbufferInputs.albedo);
            data.inputNormal = passBuilder.read(gbufferInputs.normal);
            data.inputMaterial = passBuilder.read(gbufferInputs.material);

            // Write to the same targets (HUD renders on top)
            data.outputAlbedo = passBuilder.write(gbufferInputs.albedo, ResourceState::RenderTarget);
            data.outputNormal = passBuilder.write(gbufferInputs.normal, ResourceState::RenderTarget);
            data.outputMaterial = passBuilder.write(gbufferInputs.material, ResourceState::RenderTarget);

            // Read-write depth (for depth testing)
            data.depth = passBuilder.readWrite(gbufferInputs.depth, ResourceState::DepthStencilWrite);

            // Store outputs for MaterialCache
            data.outputs.albedo = data.outputAlbedo;
            data.outputs.normal = data.outputNormal;
            data.outputs.material = data.outputMaterial;
            data.outputs.depth = data.depth;
        },

        // Execute lambda
        [](const HUDPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("HUD Pass");

            using RENDER_NAMESPACE::CSkeletonX_ST;
            using RENDER_NAMESPACE::CSkeletonX_PM;

            // Skip if no HUD batches
            if (!data.hudBatches || data.hudBatches->empty() || !data.materialCache || !data.device) {
                cmdList->endMarker();
                return;
            }

            // Get physical resources
            auto* albedoRT = fg.GetPhysicalTexture(data.outputAlbedo);
            auto* normalRT = fg.GetPhysicalTexture(data.outputNormal);
            auto* materialRT = fg.GetPhysicalTexture(data.outputMaterial);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!albedoRT || !normalRT || !materialRT || !depthRT) {
                Msg("! [HUDPass] Failed to get physical textures");
                cmdList->endMarker();
                return;
            }

            // Setup render pass (no clear - HUD renders on top of world)
            ng::RenderPassDesc passDesc;
            passDesc.renderTargets[0] = normalRT;
            passDesc.renderTargets[1] = albedoRT;
            passDesc.renderTargets[2] = materialRT;
            passDesc.numRenderTargets = 3;
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
            float lastDetailScale = -1.0f;
            u32 numDraws = 0;

            for (const auto& batch : *data.hudBatches) {
                if (!batch.visual) continue;

                // Get per-material PSO
                MaterialPSO* matPSO = data.materialCache->GetOrCreatePSO(batch.visual, data.outputs, fg);
                if (!matPSO || !matPSO->pso) continue;

                // Bind pipeline
                if (matPSO->pso != currentPipeline) {
                    ctx->SetPipeline(matPSO->pso->GetNativePipeline());
                    currentPipeline = matPSO->pso;
                }

                // Apply HUD FOV adjustment
                Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);

                // Update DynamicTransforms CB
                if (matPSO->detail_scale != lastDetailScale) {
                    DynamicTransforms dynamicCB = {};
                    FillDynamicTransforms(dynamicCB, adjustedWorldMatrix, 1.0f);

                    extern float r__dtex_range;
                    dynamicCB.dt_params.set(matPSO->detail_scale, matPSO->detail_scale, matPSO->detail_scale,
                                           1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);

                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (cbInfo.name == "dynamic_transforms") {
                            u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                            ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                            break;
                        }
                    }
                    lastDetailScale = matPSO->detail_scale;
                }

                // Handle VCB for skeletons
                if (!matPSO->vcbRequirements.empty()) {
                    ng::BufferHandle vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
                    u32 vcbSize = matPSO->vcbRequirements[0].size;

                    if (vcbHandle.IsValid() && vcbSize <= 4096) {
                        u8 cbData[4096];
                        ZeroMemory(cbData, vcbSize);

                        // Check if skeleton
                        bool isSkeleton = false;
                        if (batch.visual && batch.renderable) {
                            auto visualType = batch.visual->getType();
                            isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
                        }

                        if (isSkeleton) {
                            // Update DynamicTransforms with FOV-adjusted matrix
                            DynamicTransforms dynamicCB = {};
                            FillDynamicTransforms(dynamicCB, adjustedWorldMatrix, 1.0f);
                            for (const auto& cbInfo : matPSO->constantBuffers) {
                                if (cbInfo.name == "dynamic_transforms") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                    ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                                    break;
                                }
                            }

                            // Get Parent skeleton
                            RENDER_NAMESPACE::CKinematics* Parent = nullptr;
                            auto visualType = batch.visual->getType();
                            if (visualType == MT_SKELETON_GEOMDEF_ST) {
                                auto* skeletonMesh = static_cast<CSkeletonX_ST*>(batch.visual);
                                Parent = skeletonMesh->GetParent();
                            } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                                auto* skeletonMesh = static_cast<CSkeletonX_PM*>(batch.visual);
                                Parent = skeletonMesh->GetParent();
                            }

                            if (Parent) {
                                Parent->CalculateBones(TRUE);

                                u32 count = Parent->LL_BoneCount();
                                u32 maxBones = vcbSize / 48;
                                count = std::min(count, maxBones);

                                float* array = reinterpret_cast<float*>(cbData);
                                for (u32 mid = 0; mid < count; mid++) {
                                    Fmatrix& M = Parent->LL_GetTransform_R(u16(mid));
                                    u32 id = mid * 3;
                                    array[(id + 0) * 4 + 0] = M._11; array[(id + 0) * 4 + 1] = M._21; array[(id + 0) * 4 + 2] = M._31; array[(id + 0) * 4 + 3] = M._41;
                                    array[(id + 1) * 4 + 0] = M._12; array[(id + 1) * 4 + 1] = M._22; array[(id + 1) * 4 + 2] = M._32; array[(id + 1) * 4 + 3] = M._42;
                                    array[(id + 2) * 4 + 0] = M._13; array[(id + 2) * 4 + 1] = M._23; array[(id + 2) * 4 + 2] = M._33; array[(id + 2) * 4 + 3] = M._43;
                                }
                            }
                        }

                        // Write VCB
                        nvrhi::IBuffer* vcbBuffer = data.device->GetNativeBuffer(vcbHandle);
                        if (vcbBuffer) {
                            ctx->WriteBuffer(vcbBuffer, cbData, vcbSize);

                            // Get or create binding sets (queries VCB pool directly)
                            data.materialCache->GetOrCreateBindingSet(matPSO);

                            // Bind both VS and PS binding sets
                            ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
                            ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
                        }
                    }
                }

                // Bind vertex/index buffers
                nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
                nvrhi::IBuffer* ib = batch.indexBuffer.Get();
                if (!vb || !ib) continue;

                ctx->SetVertexBuffer(0, vb, 0);
                ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

                // Draw
                ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
                numDraws++;
            }

            ctx->EndRenderPass();

            Msg("* [HUDPass] Rendered %u draws", numDraws);

            cmdList->endMarker();
        }
    );

    // Return the modified outputs (same as GBuffer but with HUD rendered on top)
    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputAlbedo;
    outputs.normal = passData.outputNormal;
    outputs.material = passData.outputMaterial;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::passes