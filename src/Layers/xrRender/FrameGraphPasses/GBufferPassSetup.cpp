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
        //  UPDATE MATERIAL AND INSTANCE CONSTANTS (USING FGCONSTANTSYSTEM)
        // ═══════════════════════════════════════════════════════

        if (matPSO) {
            // Initialize FGConstantSystem for reflection-driven constant binding
            // Single instance handles both material and instance-level constants
            fgconstants::FGConstantSystem constants(matPSO, materialCache->GetVCBPool());

            // ─────────────────────────────────────────────────
            //  MATERIAL-FREQUENCY CONSTANTS (once per material)
            // ─────────────────────────────────────────────────

            // Set material-specific detail scale (now in shader_params b1, not dynamic_transforms b0)
            // dt_params.xyz = detail_scale, dt_params.w = 1.0 / dtex_range
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

                    // Build bone matrix array (FGConstantSystem will handle 3x4 conversion)
                    constexpr u32 MAX_BONES = 78;  // From vanilla (3744 bytes / 48 bytes per bone)
                    Fmatrix boneMatrices[MAX_BONES];
                    u32 bonesToUpload = std::min(count, MAX_BONES);

                    for (u32 i = 0; i < bonesToUpload; i++) {
                        boneMatrices[i] = Parent->LL_GetTransform_R(u16(i));
                    }

                    // Use FGConstantSystem matrix array API
                    // NOTE: Shader constant is named "sbones_array", not "m_xform"
                    // SetArray() automatically detects float3x4 format and converts properly
                    constants.SetArray("sbones_array", boneMatrices, bonesToUpload);
                }
            }
            else {
                // Compute matrices
                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                xform_v.mul(batch.worldMatrix, Device.mView);

                // Use FGConstantSystem matrix APIs
                // Set() automatically detects float3x4 vs float4x4 and converts properly
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

            // Get or create cached binding sets (unchanged)
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
}

} // namespace xray::render::passes
