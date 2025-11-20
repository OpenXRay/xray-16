#include "stdafx.h"
#include "ForwardColorPassSetup.h"
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

    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "ForwardColor",

        [&, width, height](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;

            RenderPassBuilder passBuilder(builder, passHandle);

            if (depthInput.is_valid()) {
                data.depth = passBuilder.write(depthInput, ResourceState::DepthStencilWrite);
            } else {
                data.depth = passBuilder.createDepthBuffer("rt_Depth", width, height);
            }

            data.color = passBuilder.createTexture2D(
                "rt_SceneColor",
                width,
                height,
                nvrhi::Format::RGBA16_FLOAT
            );

            data.outputs.albedo = data.color;
            data.outputs.normal = data.color;
            data.outputs.material = data.color;
            data.outputs.depth = data.depth;
        },

        [](const ForwardColorPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);

            if (!depthRT || !colorRT) {
                return;
            }

            if (!data.geometry || data.geometry->GetBatches().empty()) {
                return;
            }

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

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.color;
    outputs.material = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}

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

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            MaterialPSO* matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg);
            if (matPSO) {
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

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = colorRT;
    passDesc.renderTargets[1] = colorRT;
    passDesc.renderTargets[2] = colorRT;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depthRT;

    passDesc.clearValue.color[0] = 0.0f;
    passDesc.clearValue.color[1] = 0.0f;
    passDesc.clearValue.color[2] = 0.0f;
    passDesc.clearValue.color[3] = 1.0f;
    passDesc.clearValue.depth = 1.0f;
    passDesc.clearValue.stencil = 0;
    passDesc.clearColor = true;
    passDesc.clearDepth = true;
    passDesc.clearStencil = true;

    ctx->BeginRenderPass(passDesc);

    const auto& rtDesc = colorRT->getDesc();
    ctx->SetViewport(0, 0,
        static_cast<float>(rtDesc.width),
        static_cast<float>(rtDesc.height));

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = rtDesc.width;
    scissor.height = rtDesc.height;
    ctx->SetScissor(scissor);

    u32 numDraws = 0;
    u32 numTriangles = 0;

    ng::PipelineState* currentPipeline = nullptr;

    for (const auto& batch : batches) {
        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg);

            if (matPSO && matPSO->pso) {
                pipelineToUse = matPSO->pso;
            }
        }

        if (!pipelineToUse) {
            continue;
        }

        if (pipelineToUse != currentPipeline) {
            ctx->SetPipeline(pipelineToUse->GetNativePipeline());
            currentPipeline = pipelineToUse;
        }

        if (matPSO) {
            fgconstants::FGConstantSystem constants(matPSO, materialCache->GetVCBPool());

            Fvector4 dt_params(
                matPSO->detail_scale,
                matPSO->detail_scale,
                matPSO->detail_scale,
                1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range
            );
            constants.Set("dt_params", dt_params);

            constants.CommitMaterial(ctx);

            bool isSkeleton = false;
            u32 visualType = 0;
            if (batch.visual && batch.renderable) {
                visualType = batch.visual->getType();
                isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
            }

            if (isSkeleton) {
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB, batch.worldMatrix);
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

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
                    Parent->CalculateBones(TRUE);

                    u32 count = Parent->LL_BoneCount();

                    constexpr u32 MAX_BONES = 78;
                    Fmatrix boneMatrices[MAX_BONES];
                    u32 bonesToUpload = std::min(count, MAX_BONES);

                    for (u32 i = 0; i < bonesToUpload; i++) {
                        boneMatrices[i] = Parent->LL_GetTransform_R(u16(i));
                    }

                    constants.SetArray("sbones_array", boneMatrices, bonesToUpload);
                }
            }
            else {
                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                xform_v.mul(batch.worldMatrix, Device.mView);

                constants.Set("m_xform", xform);
                constants.Set("m_xform_v", xform_v);

                using namespace xray::render::RENDER_NAMESPACE;
                float tree_scale = 1.0f / float(FTreeVisual_quant);
                Fvector4 consts(tree_scale, tree_scale, 0.0f, 0.0f);
                constants.Set("consts", consts);
            }

            constants.CommitInstance(ctx);

            materialCache->GetOrCreateBindingSet(matPSO);

            ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
            ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
        }

        nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
        nvrhi::IBuffer* ib = batch.indexBuffer.Get();

        if (!vb || !ib) {
            continue;
        }

        ctx->SetVertexBuffer(0, vb, 0);
        ctx->SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

        ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);

        numDraws++;
        numTriangles += batch.indexCount / 3;
    }

    ctx->EndRenderPass();

}

}
