#include "stdafx.h"
#include "HUDPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"

extern ENGINE_API float psHUD_FOV;

namespace xray::render::passes {

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
        VirtualResourceHandle inputAlbedo;
        VirtualResourceHandle inputNormal;
        VirtualResourceHandle inputMaterial;
        VirtualResourceHandle depth;

        VirtualResourceHandle outputAlbedo;
        VirtualResourceHandle outputNormal;
        VirtualResourceHandle outputMaterial;

        ng::RenderDevice* device;
        const xr_vector<GeometryBatch>* hudBatches;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<HUDPassData>(
        "HUD",

        [&, width, height](FrameGraph& builder, PassHandle passHandle, HUDPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;

            data.inputAlbedo = passBuilder.read(gbufferInputs.albedo);
            data.inputNormal = passBuilder.read(gbufferInputs.normal);
            data.inputMaterial = passBuilder.read(gbufferInputs.material);

            data.outputAlbedo = passBuilder.write(gbufferInputs.albedo, ResourceState::RenderTarget);
            data.outputNormal = passBuilder.write(gbufferInputs.normal, ResourceState::RenderTarget);
            data.outputMaterial = passBuilder.write(gbufferInputs.material, ResourceState::RenderTarget);

            data.depth = passBuilder.readWrite(gbufferInputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.outputAlbedo;
            data.outputs.normal = data.outputNormal;
            data.outputs.material = data.outputMaterial;
            data.outputs.depth = data.depth;
        },

        [](const HUDPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("HUD Pass");

            using RENDER_NAMESPACE::CSkeletonX_ST;
            using RENDER_NAMESPACE::CSkeletonX_PM;

            if (!data.hudBatches || data.hudBatches->empty() || !data.materialCache || !data.device) {
                cmdList->endMarker();
                return;
            }

            auto* albedoRT = fg.GetPhysicalTexture(data.outputAlbedo);
            auto* normalRT = fg.GetPhysicalTexture(data.outputNormal);
            auto* materialRT = fg.GetPhysicalTexture(data.outputMaterial);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!albedoRT || !normalRT || !materialRT || !depthRT) {
                Msg("! [HUDPass] Failed to get physical textures");
                cmdList->endMarker();
                return;
            }

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

            ng::PipelineState* currentPipeline = nullptr;
            float lastDetailScale = -1.0f;
            u32 numDraws = 0;

            for (const auto& batch : *data.hudBatches) {
                if (!batch.visual) continue;

                MaterialPSO* matPSO = data.materialCache->GetOrCreatePSO(batch.visual, data.outputs, fg);
                if (!matPSO || !matPSO->pso) continue;

                if (matPSO->pso != currentPipeline) {
                    ctx->SetPipeline(matPSO->pso->GetNativePipeline());
                    currentPipeline = matPSO->pso;
                }

                Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);

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

                if (!matPSO->vcbRequirements.empty()) {
                    ng::BufferHandle vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
                    u32 vcbSize = matPSO->vcbRequirements[0].size;

                    if (vcbHandle.IsValid() && vcbSize <= 4096) {
                        u8 cbData[4096];
                        ZeroMemory(cbData, vcbSize);

                        bool isSkeleton = false;
                        if (batch.visual && batch.renderable) {
                            auto visualType = batch.visual->getType();
                            isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
                        }

                        if (isSkeleton) {
                            DynamicTransforms dynamicCB = {};
                            FillDynamicTransforms(dynamicCB, adjustedWorldMatrix, 1.0f);
                            for (const auto& cbInfo : matPSO->constantBuffers) {
                                if (cbInfo.name == "dynamic_transforms") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                    ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                                    break;
                                }
                            }

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

                        nvrhi::IBuffer* vcbBuffer = data.device->GetNativeBuffer(vcbHandle);
                        if (vcbBuffer) {
                            ctx->WriteBuffer(vcbBuffer, cbData, vcbSize);

                            data.materialCache->GetOrCreateBindingSet(matPSO);

                            ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
                            ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
                        }
                    }
                }

                nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
                nvrhi::IBuffer* ib = batch.indexBuffer.Get();
                if (!vb || !ib) continue;

                ctx->SetVertexBuffer(0, vb, 0);
                ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

                ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
                numDraws++;
            }

            ctx->EndRenderPass();

            Msg("* [HUDPass] Rendered %u draws", numDraws);

            cmdList->endMarker();
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputAlbedo;
    outputs.normal = passData.outputNormal;
    outputs.material = passData.outputMaterial;
    outputs.depth = passData.depth;
    return outputs;
}

}