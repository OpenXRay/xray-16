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

struct BindlessDepthPerDrawConstants {
    Fmatrix world;
    u32 materialID;
    float padding[3];
};
static_assert(sizeof(BindlessDepthPerDrawConstants) == 80, "BindlessDepthPerDrawConstants must be 80 bytes");

static nvrhi::ShaderHandle s_bindlessDepthVS;
static nvrhi::InputLayoutHandle s_bindlessDepthInputLayout;
static nvrhi::GraphicsPipelineHandle s_bindlessDepthPipeline;
static nvrhi::BindingLayoutHandle s_bindlessDepthBindingLayout;
static nvrhi::BufferHandle s_bindlessDepthStaticGlobalsCB;
static nvrhi::BufferHandle s_bindlessDepthPerDrawCB;
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

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            MaterialPSO* matPSO = materialCache->GetOrCreateDepthPSO(batch.visual, fg);

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

    ng::RenderPassDesc passDesc;
    passDesc.passName = "Depth Prepass (Early-Z)";
    passDesc.numRenderTargets = 0;
    passDesc.depthStencil = depthRT;

    passDesc.clearValue.depth = 1.0f;
    passDesc.clearDepth = true;

    passDesc.clearStencil = false;

    ctx->BeginRenderPass(passDesc);

    const auto& depthDesc = depthRT->getDesc();
    ctx->SetViewport(0, 0,
        static_cast<float>(depthDesc.width),
        static_cast<float>(depthDesc.height));

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = depthDesc.width;
    scissor.height = depthDesc.height;
    ctx->SetScissor(scissor);

    ng::PipelineState* currentPipeline = nullptr;

    for (const auto& batch : batches) {
        if (!batch.IsOpaque()) {
            continue;
        }

        if (batch.megaBufferAlloc.valid) {
            continue;
        }

        ng::PipelineState* pipelineToUse = nullptr;
        MaterialPSO* matPSO = nullptr;

        if (batch.visual && materialCache) {
            matPSO = materialCache->GetOrCreateDepthPSO(batch.visual, fg);

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
                xform_v.mul_43(Device.mView, batch.worldMatrix);

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
    }

    if (bindlessConfig.UseMegaBuffers()) {
        nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
        nvrhi::ICommandList* cmdList = ctx->GetCommandList();

        if (!s_bindlessDepthInitialized) {
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

                    nvrhi::BindingLayoutDesc bindLayoutDesc;
                    bindLayoutDesc.visibility = nvrhi::ShaderType::Vertex;
                    bindLayoutDesc.bindings = {
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                    };
                    s_bindlessDepthBindingLayout = cache.GetOrCreateBindingLayout("BindlessDepthPass", bindLayoutDesc, nvDevice);

                    nvrhi::BufferDesc globalsCBDesc;
                    globalsCBDesc.debugName = "BindlessDepthStaticGlobalsCB";
                    globalsCBDesc.byteSize = sizeof(StaticGlobals);
                    globalsCBDesc.isConstantBuffer = true;
                    globalsCBDesc.isVolatile = true;
                    globalsCBDesc.maxVersions = 16;
                    globalsCBDesc.keepInitialState = true;
                    globalsCBDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                    s_bindlessDepthStaticGlobalsCB = nvDevice->createBuffer(globalsCBDesc);

                    nvrhi::BufferDesc cbDesc;
                    cbDesc.debugName = "BindlessDepthPerDrawCB";
                    cbDesc.byteSize = sizeof(BindlessDepthPerDrawConstants);
                    cbDesc.isConstantBuffer = true;
                    cbDesc.isVolatile = true;
                    cbDesc.maxVersions = 256;
                    cbDesc.keepInitialState = true;
                    cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                    s_bindlessDepthPerDrawCB = nvDevice->createBuffer(cbDesc);

                    nvrhi::GraphicsPipelineDesc pipeDesc;
                    pipeDesc.VS = s_bindlessDepthVS;
                    pipeDesc.PS = nullptr;
                    pipeDesc.inputLayout = s_bindlessDepthInputLayout;
                    pipeDesc.bindingLayouts = { s_bindlessDepthBindingLayout };
                    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
                    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
                    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
                    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
                    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

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

        if (s_bindlessDepthInitialized && s_bindlessDepthPipeline) {
            auto& cache = framegraph::GetPassResourceCache();

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(depthRT);
            nvrhi::FramebufferHandle framebuffer = cache.GetOrCreateFramebuffer("BindlessDepthPass_Render", fbDesc, nvDevice);

            const auto& depthDesc = depthRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(depthDesc.width),
                                     0.0f, static_cast<float>(depthDesc.height), 0.0f, 1.0f);

            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            cmdList->writeBuffer(s_bindlessDepthStaticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            u32 bindlessDraws = 0;

            for (const auto& batch : batches) {
                if (!batch.IsOpaque() || !batch.megaBufferAlloc.valid) {
                    continue;
                }

                BindlessDepthPerDrawConstants perDrawData;
                perDrawData.world.transpose(batch.worldMatrix);
                perDrawData.materialID = batch.bindlessMaterialID;
                perDrawData.padding[0] = perDrawData.padding[1] = perDrawData.padding[2] = 0.0f;
                cmdList->writeBuffer(s_bindlessDepthPerDrawCB, &perDrawData, sizeof(perDrawData));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(2, s_bindlessDepthStaticGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(5, s_bindlessDepthPerDrawCB),
                };
                auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessDepthBindingLayout);

                nvrhi::GraphicsState state;
                state.pipeline = s_bindlessDepthPipeline;
                state.framebuffer = framebuffer;
                state.bindings = { bindingSet };
                state.vertexBuffers = { {bindlessConfig.megaVertexBuffer, 0, 0} };
                state.indexBuffer = { bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
                state.viewport.addViewport(viewport);
                state.viewport.addScissorRect(nvrhi::Rect(depthDesc.width, depthDesc.height));

                cmdList->setGraphicsState(state);

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

        [&, width, height, bindlessConfig](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.createDepthBuffer(
                "rt_DepthPrepass",
                width,
                height
            );
        },

        [](const DepthPrepassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!depthRT) {
                return;
            }

            if (!data.geometry) {
                return;
            }

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

    return passData.depth;
}

}
