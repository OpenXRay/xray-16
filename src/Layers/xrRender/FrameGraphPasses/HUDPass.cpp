#include "stdafx.h"
#include "HUDPass.h"
#include "GBufferPass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/TextureDescrManager.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/SkeletonX.h"
#include "xrEngine/Render.h"

extern ENGINE_API float psHUD_FOV;
namespace xray::render::passes {

using namespace framegraph;
using RENDER_NAMESPACE::CSkeletonX_ST;
using RENDER_NAMESPACE::CSkeletonX_PM;

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

HUDPass::HUDPass(ng::RenderDevice* device, const HUDPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    if (config.width == 0 || config.height == 0) {
        Msg("! [HUDPass] ERROR: Invalid resolution %ux%u", config.width, config.height);
    }
}

HUDPass::~HUDPass() {
}

void HUDPass::Setup(FrameGraph& fg) {

    PassIO io;

    io.reads.push_back({m_outputs.depth, ResourceState::DepthStencilRead});

    io.writes.push_back({m_outputs.albedo, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.normal, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.material, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.depth, ResourceState::DepthStencilWrite});

    RegisterPass(fg, "HUD", io);

    Msg("  ✓ HUD pass configured");
}

void HUDPass::Execute(ng::RenderContext& ctx, const FrameGraph& fg) {
    auto execStart = std::chrono::high_resolution_clock::now();
    m_stats.numDrawCalls = 0;
    m_stats.numTriangles = 0;
    m_stats.numBatches = 0;

    if (!m_hudBatches || m_hudBatches->empty() || !m_materialCache) {
        m_stats.cpuTimeMs = 0.0f;
        return;
    }

    Msg("! [HUDPass] Rendering %u HUD batches with depth range [0.0, 0.1]", (u32)m_hudBatches->size());

    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    if (!normal || !albedo || !material || !depth) {
        Msg("! [HUDPass] ERROR: Missing render targets!");
        return;
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normal;
    passDesc.renderTargets[1] = albedo;
    passDesc.renderTargets[2] = material;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depth;
    passDesc.clearColor = false;
    passDesc.clearDepth = false;
    passDesc.clearStencil = false;

    ctx.BeginRenderPass(passDesc);

    ng::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_config.width;
    viewport.height = (float)m_config.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.1f;
    ctx.SetViewport(viewport);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    ng::PipelineState* currentPipeline = nullptr;
    nvrhi::IBindingSet* currentBindingSet = nullptr;
    float lastDetailScale = -1.0f;

    for (const auto& batch : *m_hudBatches) {
        if (!batch.visual) continue;

        MaterialPSO* matPSO = m_materialCache->GetOrCreatePSO(batch.visual, m_outputs, fg);
        if (!matPSO || !matPSO->pso) continue;

        if (matPSO->pso != currentPipeline) {
            ctx.SetPipeline(matPSO->pso->GetNativePipeline());
            currentPipeline = matPSO->pso;
        }

        Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);

        if (matPSO->detail_scale != lastDetailScale) {
            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB, adjustedWorldMatrix);

            extern float r__dtex_range;
            dynamicCB.dt_params.set(matPSO->detail_scale, matPSO->detail_scale, matPSO->detail_scale,
                                   1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);

            for (const auto& cbInfo : matPSO->constantBuffers) {
                if (cbInfo.name == "dynamic_transforms") {
                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                    break;
                }
            }

            lastDetailScale = matPSO->detail_scale;
        }

        if (!matPSO->vcbRequirements.empty()) {
            ng::BufferHandle vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
            u32 vcbSize = matPSO->vcbRequirements[0].size;

            if (!vcbHandle.IsValid()) {
                Msg("! [HUDPass] Material has no valid VCB");
                continue;
            }

            constexpr u32 MAX_CB_SIZE = 4096;
            if (vcbSize > MAX_CB_SIZE) {
                Msg("! [HUDPass] VCB size %u exceeds maximum %u", vcbSize, MAX_CB_SIZE);
                continue;
            }

            u8 cbData[MAX_CB_SIZE];
            ZeroMemory(cbData, vcbSize);

            auto CopyBoneMatrix = [](u8* dest, const Fmatrix& M) {
                float* destF = reinterpret_cast<float*>(dest);
                destF[0]  = M._11; destF[1]  = M._21; destF[2]  = M._31; destF[3]  = M._41;
                destF[4]  = M._12; destF[5]  = M._22; destF[6]  = M._32; destF[7]  = M._42;
                destF[8]  = M._13; destF[9]  = M._23; destF[10] = M._33; destF[11] = M._43;
            };

            bool isSkeleton = false;
            if (batch.visual && batch.renderable) {
                auto visualType = batch.visual->getType();
                isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
            }

            if (isSkeleton) {
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB, adjustedWorldMatrix, 0.f);
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                RENDER_NAMESPACE::CKinematics* Parent = nullptr;
                auto visualType = batch.visual->getType();
                if (visualType == MT_SKELETON_GEOMDEF_ST) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_ST*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                    auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_PM*>(batch.visual);
                    Parent = skeletonMesh->GetParent();
                }

                if (Parent) {
                    Parent->CalculateBones(TRUE);

                    u32 count = Parent->LL_BoneCount();
                    u32 maxBones = vcbSize / 48;
                    count = std::min(count, maxBones);

                    float* array = reinterpret_cast<float*>(cbData);
                    for (u32 mid = 0; mid < count; mid++)
                    {
                        Fmatrix& M = Parent->LL_GetTransform_R(u16(mid));
                        u32 id = mid * 3;
                        array[(id + 0) * 4 + 0] = M._11;
                        array[(id + 0) * 4 + 1] = M._21;
                        array[(id + 0) * 4 + 2] = M._31;
                        array[(id + 0) * 4 + 3] = M._41;
                        array[(id + 1) * 4 + 0] = M._12;
                        array[(id + 1) * 4 + 1] = M._22;
                        array[(id + 1) * 4 + 2] = M._32;
                        array[(id + 1) * 4 + 3] = M._42;
                        array[(id + 2) * 4 + 0] = M._13;
                        array[(id + 2) * 4 + 1] = M._23;
                        array[(id + 2) * 4 + 2] = M._33;
                        array[(id + 2) * 4 + 3] = M._43;
                    }
                } else {
                    Msg("! [HUDPass] WARNING: Skeleton visual has no Parent! visual=%p", batch.visual);
                }
            } else {
                Fmatrix identity;
                identity.identity();
                CopyBoneMatrix(cbData, identity);
            }

            nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(vcbHandle);
            ctx.WriteBuffer(vcbBuffer, cbData, vcbSize);

            m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer, matPSO->pass);

            ctx.SetBindingSet(0, matPSO->vsBindingSet.Get());
            ctx.SetBindingSet(1, matPSO->psBindingSet.Get());
            currentBindingSet = matPSO->vsBindingSet.Get();
        }

        nvrhi::IBuffer* vb = batch.vertexBuffer.Get();
        nvrhi::IBuffer* ib = batch.indexBuffer.Get();
        if (!vb || !ib) {
            Msg("! [HUDPass] Batch has null buffers, skipping");
            continue;
        }

        ctx.SetVertexBuffer(0, vb, 0);
        ctx.SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

        ctx.DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);

        m_stats.numDrawCalls++;
        m_stats.numTriangles += batch.indexCount / 3;
    }

    ctx.EndRenderPass();

    auto execEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(execEnd - execStart).count();

    Msg("! [HUDPass] Complete: %u draws, %u tris, %.2f ms",
        m_stats.numDrawCalls, m_stats.numTriangles, m_stats.cpuTimeMs);
}

}
