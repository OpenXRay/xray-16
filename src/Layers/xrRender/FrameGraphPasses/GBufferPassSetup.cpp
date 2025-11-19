// xrRender/FrameGraphPasses/GBufferPassSetup.cpp
#include "stdafx.h"
#include "GBufferPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"  // For ng::PipelineState full definition
#include "Layers/xrRender/RenderContext/RenderDevice.h"   // For GetNativeBuffer
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics (skeleton bones)
#include "Layers/xrRender/FSkinned.h"  // For CSkeletonX (skinned mesh parent pointer)
#include "Layers/xrRender/FTreeVisual.h"  // For FTreeVisual_quant constant
#include "Layers/xrRender/SkeletonX.h"  // For CSkeletonX_ST, CSkeletonX_PM
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"  // NEW: Reflection-driven constant binding

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::passes {

// Forward declaration of the rendering function
void renderGBufferGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* albedoRT,
    nvrhi::ITexture* normalRT,
    nvrhi::ITexture* positionRT,
    nvrhi::ITexture* depthRT,
    MaterialCache* materialCache,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg);

framegraph::DefaultOutputLayout setupGBufferPass(
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
    struct GBufferPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle albedo;
        VirtualResourceHandle normal;
        VirtualResourceHandle position;
        ng::RenderDevice* device;            // Render device for buffer access
        const GeometryCollector* geometry;   // Capture by value!
        MaterialCache* materialCache;        // Capture by value!
        DefaultOutputLayout outputs;         // Store outputs for MaterialCache
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<GBufferPassData>(
        "GBuffer",

        // Setup lambda (captures by reference, executes inline)
        [&, width, height](FrameGraph& builder, PassHandle passHandle, GBufferPassData& data) {
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
                data.depth = passBuilder.write(depthInput, ResourceState::DepthStencilWrite);
            } else {
                data.depth = passBuilder.createDepthBuffer("rt_Depth", width, height);
            }

            // Create G-Buffer render targets
            data.albedo = passBuilder.createTexture2D(
                "rt_Albedo",
                width,
                height,
                nvrhi::Format::RGBA8_UNORM  // Albedo + metallic
            );

            data.normal = passBuilder.createTexture2D(
                "rt_Normal",
                width,
                height,
                nvrhi::Format::RGBA16_FLOAT  // Normal + roughness
            );

            data.position = passBuilder.createTexture2D(
                "rt_Position",
                width,
                height,
                nvrhi::Format::RGBA32_FLOAT  // World position
            );

            // Store outputs in PassData for MaterialCache
            data.outputs.albedo = data.albedo;
            data.outputs.normal = data.normal;
            data.outputs.material = data.position;  // Using position as material
            data.outputs.depth = data.depth;
        },

        // Execute lambda (captures by value, executes deferred)
        [](const GBufferPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            // Get physical resources from virtual handles
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* albedoRT = fg.GetPhysicalTexture(data.albedo);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* positionRT = fg.GetPhysicalTexture(data.position);

            if (!depthRT || !albedoRT || !normalRT || !positionRT) {
                return;
            }

            // Check if we have geometry to render
            if (!data.geometry || data.geometry->GetBatches().empty()) {
                // No geometry - clear RTs and exit
                // TODO: Add clear commands
                return;
            }

            // Render geometry using the existing GBuffer rendering logic
            renderGBufferGeometry(
                ctx,
                data.device,
                data.geometry,
                albedoRT,
                normalRT,
                positionRT,
                depthRT,
                data.materialCache,
                data.outputs,
                fg
            );
        }
    );

    // Return the G-Buffer outputs
    DefaultOutputLayout outputs;
    outputs.albedo = passData.albedo;
    outputs.normal = passData.normal;
    outputs.material = passData.position;  // Using position as material for now
    outputs.depth = passData.depth;
    return outputs;
}

// Implementation of the actual rendering (extracted from original GBufferPass::Execute)
void renderGBufferGeometry(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* albedoRT,
    nvrhi::ITexture* normalRT,
    nvrhi::ITexture* positionRT,
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
    //
    // NOTE: We still use legacy CB name checks here because:
    // 1. Global CBs (static_globals, dynamic_transforms) are PERSISTENT (not VCBs)
    // 2. FGConstantSystem handles VCBs only (per-instance data)
    // 3. These global CBs will be migrated to FGConstantSystem.SetStatic() in Phase 7

    xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

    for (const auto& batch : batches) {
        if (batch.visual && materialCache) {
            MaterialPSO* matPSO = materialCache->GetOrCreatePSO(batch.visual, outputs, fg);
            if (matPSO) {
                // Update all global CBs (constantBuffers only contains global CBs, VCBs are in vcbRequirements)
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.nvrhiBuffer) {
                        if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                            // TEMPORARY: Fill buffer based on CB name (will migrate to FGConstantSystem.SetStatic() later)
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
    //  SETUP RENDER PASS
    // ═══════════════════════════════════════════════════════

    // X-Ray convention (from ShaderReflector):
    // - Slot 0 → Normal   (RTSemantic::Normal)
    // - Slot 1 → Albedo   (RTSemantic::Albedo)
    // - Slot 2 → Material (RTSemantic::Material)

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normalRT;    // SV_Target0
    passDesc.renderTargets[1] = albedoRT;    // SV_Target1
    passDesc.renderTargets[2] = positionRT;  // SV_Target2 (using position as material)
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depthRT;

    // Clear values
    passDesc.clearValue.color[0] = 0.0f;
    passDesc.clearValue.color[1] = 0.0f;
    passDesc.clearValue.color[2] = 0.0f;
    passDesc.clearValue.color[3] = 1.0f;
    passDesc.clearValue.depth = 1.0f;
    passDesc.clearValue.stencil = 0;
    passDesc.clearColor = true;
    passDesc.clearDepth = true;
    passDesc.clearStencil = true;

    // Begin render pass
    ctx->BeginRenderPass(passDesc);

    // Set viewport
    const auto& rtDesc = albedoRT->getDesc();
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
        //  UPDATE PER-MATERIAL DYNAMIC CONSTANTS
        // ═══════════════════════════════════════════════════════
        // TEMPORARY: Still using legacy CB name check for dynamic_transforms
        // Will migrate to FGConstantSystem.CommitMaterial() in Phase 7

        if (matPSO) {
            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB);

            // Override dt_params with per-material detail scale

            dynamicCB.dt_params.set(matPSO->detail_scale, matPSO->detail_scale, matPSO->detail_scale,
                                   1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);

            // TEMPORARY: Update DynamicTransforms CB (will use FGConstantSystem.Set("dt_params") later)
            for (const auto& cbInfo : matPSO->constantBuffers) {
                if (cbInfo.name == "dynamic_transforms") {
                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                    ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                    break;
                }
            }
        }

        // ═══════════════════════════════════════════════════════
        //  UPDATE PER-INSTANCE CONSTANTS (REFLECTION-DRIVEN)
        // ═══════════════════════════════════════════════════════

        if (matPSO) {
            // Get VCB based on visual type
            ng::BufferHandle vcbHandle;
            u32 vcbSize = 0;

            // Determine if this is a skeleton mesh
            bool isSkeleton = false;
            u32 visualType = 0;
            if (batch.visual && batch.renderable) {
                visualType = batch.visual->getType();
                isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
            }

            for (const auto& vcbReq : matPSO->vcbRequirements) {
                if (vcbReq.name == "$Globals") { // yohji TODO: turn vcbReq into map for o(1) lookup
                    vcbHandle = vcbReq.vcbHandle;
                    vcbSize = vcbReq.size;
                    break;
                }
            }

            // If no $Globals found, use first VCB as fallback
            if ((!vcbHandle.IsValid() || !vcbSize) && !matPSO->vcbRequirements.empty()) {
                vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
                vcbSize = matPSO->vcbRequirements[0].size;
            }

            if (!vcbHandle.IsValid()) {
                continue;  // Skip if no VCB
            }

            // Allocate buffer for CB data (max 4096 bytes for skeleton)
            constexpr u32 MAX_CB_SIZE = 4096;
            if (vcbSize > MAX_CB_SIZE) {
                continue;
            }

            u8 cbData[MAX_CB_SIZE];
            ZeroMemory(cbData, vcbSize);

            // Helper to copy bone matrix (3 float4s, column-major)
            auto CopyBoneMatrix = [](u8* dest, const Fmatrix& M) {
                float* destF = reinterpret_cast<float*>(dest);
                destF[0] = M._11; destF[1] = M._21; destF[2] = M._31; destF[3] = M._41;
                destF[4] = M._12; destF[5] = M._22; destF[6] = M._32; destF[7] = M._42;
                destF[8] = M._13; destF[9] = M._23; destF[10] = M._33; destF[11] = M._43;
                };

            // Helper for tree/regular meshes (transposed 3x4)
            auto CopyMatrix3x4 = [](u8* dest, const Fmatrix& src) {
                Fmatrix transposed;
                transposed.transpose(src);
                float* destF = reinterpret_cast<float*>(dest);
                destF[0] = transposed._11; destF[1] = transposed._12; destF[2] = transposed._13; destF[3] = transposed._14;
                destF[4] = transposed._21; destF[5] = transposed._22; destF[6] = transposed._23; destF[7] = transposed._24;
                destF[8] = transposed._31; destF[9] = transposed._32; destF[10] = transposed._33; destF[11] = transposed._34;
                };

            // isSkeleton and visualType already determined above (lines 336-342)
            if (isSkeleton) {
                // ─────────────────────────────────────────────────
                //  SKELETON VCB: sbones_array
                // ─────────────────────────────────────────────────

                // Update global dynamic_transforms CB with skeleton's world matrix
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB, batch.worldMatrix, 1.0f);
                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                // Get Parent from skinned mesh visual itself
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

                    // Fill sbones_array - EXACT copy of vanilla code
                    u32 count = Parent->LL_BoneCount();
                    u32 maxBones = vcbSize / 48;
                    count = std::min(count, maxBones);

                    float* array = reinterpret_cast<float*>(cbData);
                    for (u32 mid = 0; mid < count; mid++)
                    {
                        Fmatrix& M = Parent->LL_GetTransform_R(u16(mid));
                        u32 id = mid * 3;
                        // array[id + 0] = float4(M._11, M._21, M._31, M._41)
                        array[(id + 0) * 4 + 0] = M._11;
                        array[(id + 0) * 4 + 1] = M._21;
                        array[(id + 0) * 4 + 2] = M._31;
                        array[(id + 0) * 4 + 3] = M._41;
                        // array[id + 1] = float4(M._12, M._22, M._32, M._42)
                        array[(id + 1) * 4 + 0] = M._12;
                        array[(id + 1) * 4 + 1] = M._22;
                        array[(id + 1) * 4 + 2] = M._32;
                        array[(id + 1) * 4 + 3] = M._42;
                        // array[id + 2] = float4(M._13, M._23, M._33, M._43)
                        array[(id + 2) * 4 + 0] = M._13;
                        array[(id + 2) * 4 + 1] = M._23;
                        array[(id + 2) * 4 + 2] = M._33;
                        array[(id + 2) * 4 + 3] = M._43;
                    }
                }
            }
            else {
                // ─────────────────────────────────────────────────
                //  TREE/REGULAR VCB: m_xform, m_xform_v, consts
                // ─────────────────────────────────────────────────

                // Compute matrices
                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                xform_v.mul(batch.worldMatrix, Device.mView);

                // Write transforms
                CopyMatrix3x4(cbData + 0, xform);     // m_xform at offset 0
                CopyMatrix3x4(cbData + 48, xform_v);  // m_xform_v at offset 48

                // Write consts at offset 96
                using namespace xray::render::RENDER_NAMESPACE;
                float tree_scale = 1.0f / float(FTreeVisual_quant);

                float* constsPtr = reinterpret_cast<float*>(cbData + 96);
                constsPtr[0] = tree_scale;
                constsPtr[1] = tree_scale;
                constsPtr[2] = 0.0f;
                constsPtr[3] = 0.0f;
            }

            // Get NVRHI buffer from BufferHandle using RenderDevice
            nvrhi::IBuffer* vcbBuffer = device->GetNativeBuffer(vcbHandle);

            if (vcbBuffer) {
                // Write VCB within render pass
                ctx->WriteBuffer(vcbBuffer, cbData, vcbSize);

                // Get or create cached binding sets
                materialCache->GetOrCreateBindingSet(matPSO);

                // Bind BOTH per-stage binding sets
                ctx->SetBindingSet(0, matPSO->vsBindingSet.Get());
                ctx->SetBindingSet(1, matPSO->psBindingSet.Get());
            }
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
}

} // namespace xray::render::passes
