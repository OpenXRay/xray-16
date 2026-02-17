// DetailPassSetup.cpp - Framegraph pass for detail objects (grass, etc.)
#include "stdafx.h"
#include "DetailPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "xrCDB/Frustum.h"

// Detail rendering console variables
extern ENGINE_API float ps_r__Detail_l_aniso;
extern ENGINE_API float ps_r__Detail_l_ambient;

// Phase 5: Grass wind tuning parameters (defined in xrEngine)
extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;

// Grass color parameters (defined in xrEngine)
extern ENGINE_API Fvector3 ps_r3_grass_color_tip;
extern ENGINE_API Fvector3 ps_r3_grass_color_base;
extern ENGINE_API float ps_r3_grass_color_variation;
extern ENGINE_API Fvector3 ps_r3_grass_sss_color;
extern ENGINE_API float ps_r3_grass_sss_intensity;
extern ENGINE_API Fvector3 ps_r3_grass_object_tints[64];

// Grass blade geometry parameters (defined in xrEngine)
extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;

namespace xray::render::RENDER_NAMESPACE
{
    extern int ps_r__detail_gpu;
}

namespace xray::render::RENDER_NAMESPACE::passes
{
using namespace framegraph;

DefaultOutputLayout setupDetailPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    RENDER_NAMESPACE::FGDetailManager* detailManager,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    VirtualResourceHandle hiZPyramid,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    const Fmatrix* prevViewProj,
    xray::profiler::GPUProfiler* gpuProfiler,
    DetailPassState* detailState
)
{

    // Capture prevViewProj by value (it's a pointer, we need to copy the data)
    Fmatrix capturedPrevViewProj;
    bool hasPrevViewProj = (prevViewProj != nullptr);
    if (hasPrevViewProj) {
        capturedPrevViewProj = *prevViewProj;
    } else {
        capturedPrevViewProj.identity();  // Fallback to identity if no previous data
    }

    auto& passData = fg.addCallbackPass<DetailPassData>(
        "Details",
        [&, width, height, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels, capturedPrevViewProj, hasPrevViewProj, gpuProfiler, detailState](
            FrameGraph& builder, PassHandle passHandle, DetailPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.detailManager = detailManager;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;
            data.prevViewProj = capturedPrevViewProj;
            data.hasPrevViewProj = hasPrevViewProj;
            data.gpuProfiler = gpuProfiler;
            data.detailState = detailState;

            // Add Hi-Z as read dependency
            data.hiZPyramid = passBuilder.read(hiZPyramid, ResourceState::ShaderResource);

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);
            data.outputColor = passBuilder.write(forwardInputs.albedo);
            data.outputNormal = passBuilder.readWrite(forwardInputs.normal, ResourceState::RenderTarget);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.depth = data.depth;
        },
        [](const DetailPassData& data, const FrameGraph& fg, ng::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailPass", 10);

            if (!data.detailManager)
                return;

            // Check if detail rendering is enabled
            if (!psDeviceFlags.is(rsDrawDetails))
                return;

            bool detailPipelineValid = (data.detailManager->instanceGenPipeline && data.detailManager->slotDataBuffer);
            if (!detailPipelineValid)
                return;

            // Get physical resources
            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.outputColor);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depth);

            if (!colorTexture || !depthTexture)
                return;

            // Get command list (already opened by framegraph system)
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;

            if (data.detailState && !data.detailState->detailDataUploaded)
            {
                data.detailManager->UploadBufferData(cmdList);
                data.detailState->detailDataUploaded = true;
            }

            // === AUTO-REGENERATE GEOMETRY WHEN WIDTH CHANGES ===
            if (data.detailState && data.detailState->lastBladeWidth != ps_r3_grass_blade_width)
            {
                data.detailManager->RegenerateBladeGeometry(cmdList);
                data.detailState->lastBladeWidth = ps_r3_grass_blade_width;
            }

            // === WIND PARAMETERS UPDATE ===
            // Wind animation is computed via multi-scale texture sampling in the vertex shader.
            // Just update wind speed/direction from environment (no compute dispatch needed).
            if (g_pGamePersistent)
            {
                data.detailManager->windSpeed = _max(
                    g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier,
                    ps_r3_grass_wind_min);
                float wind_rad = deg2rad(g_pGamePersistent->Environment().CurrentEnv.wind_direction);
                data.detailManager->windDirection.set(_cos(wind_rad), _sin(wind_rad));
            }

            // === GPU CULLING COMPUTE PASS ===
            // Extract frustum planes from view-projection matrix
            CFrustum frustum;
            frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

            // IMPORTANT: CFrustum::fplane is 32 bytes (alignas(16) + aabb_overlap_id)
            // but Fplane is 16 bytes. We must manually extract to avoid stride mismatch!
            Fvector4 extractedPlanes[6];
            for (u32 i = 0; i < frustum.p_count && i < 6; i++)
            {
                extractedPlanes[i].set(
                    frustum.planes[i].n.x,
                    frustum.planes[i].n.y,
                    frustum.planes[i].n.z,
                    frustum.planes[i].d
                );
            }

            // Get Hi-Z pyramid texture
            nvrhi::ITexture* hiZTexture = fg.GetPhysicalTexture(data.hiZPyramid);

            // Dispatch GPU culling
            const float fadeDistance = g_pGamePersistent->Environment().CurrentEnv.far_plane;

            // Use previous frame's viewProj for temporal Hi-Z, or current if not available
            Fmatrix effectivePrevViewProj = data.hasPrevViewProj ? data.prevViewProj : Device.mFullTransform;

            data.detailManager->DispatchCulling(
                cmdList,
                data.device->GetNVRHIDevice(),
                hiZTexture,
                Device.mFullTransform,
                effectivePrevViewProj,  // Previous frame's viewProj for temporal Hi-Z
                extractedPlanes,
                frustum.p_count,
                Device.vCameraPosition,
                fadeDistance,
                data.hiZWidth,
                data.hiZHeight,
                data.hiZMipLevels,
                data.gpuProfiler
            );

            // Schedule stats readback for profiling (visible counts per LOD)
            data.detailManager->ScheduleStatsReadback(cmdList, data.device->GetNVRHIDevice());

            // === GRAPHICS PASS ===
            // Log texture formats for debugging
            const nvrhi::TextureDesc& colorDesc = colorTexture->getDesc();
            const nvrhi::TextureDesc& depthDesc = depthTexture->getDesc();

            nvrhi::ITexture* normalTexture = fg.GetPhysicalTexture(data.outputNormal);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
            if (normalTexture)
                fbDesc.addColorAttachment(normalTexture);
            fbDesc.setDepthAttachment(depthTexture);

            nvrhi::FramebufferHandle framebuffer = data.device->GetNVRHIDevice()->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            // Create graphics pipeline (lazy - needs framebuffer)
            if (!data.detailManager->graphicsPipeline)
            {
                data.detailManager->CreateGraphicsPipeline(data.device, framebuffer);
            }

            if (!data.detailManager->graphicsPipeline)
            {
                Msg("! [DetailPass] Graphics pipeline not available");
                return;
            }

            // Use cached per-frame resources (created once in CreateCachedResources)
            auto* dm = data.detailManager;

            // b0: dynamic_transforms
            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dm->cachedDynTransformsCB, &dynTrans, sizeof(dynTrans));

            // b1: shader_params (dummy)
            u8 dummyParams[32] = {};
            cmdList->writeBuffer(dm->cachedShaderParamsCB, dummyParams, 32);

            // b2: static_globals
            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(dm->cachedStaticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            // b3: DetailGlobals
            float windAngleDeg = 0.0f;
            float windSpeed = dm->windSpeed;
            if (g_pGamePersistent)
                windAngleDeg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;

            FGDetailManager::DetailFrameConstants frameConstants;
            const float quant = 16384.0f;
            frameConstants.consts.set(1.0f / quant, 1.0f / quant, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
            frameConstants.wave.set(1.0f / 5.0f, 1.0f / 7.0f, 1.0f / 3.0f, Device.fTimeGlobal);
            frameConstants.dir2D.set(dm->windDirection.x, dm->windDirection.y, 0.0f, 0.0f);
            frameConstants.dir2D_2.set(-dm->windDirection.y, dm->windDirection.x, 0.0f, 0.0f);
            frameConstants.viewProj.transpose(Device.mFullTransform);
            frameConstants.detail_params.set(
                float(dm->dtH.x_size()), float(dm->dtH.z_size()),
                float(dm->dtH.x_offs()), float(dm->dtH.z_offs()));
            frameConstants.g_wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
            frameConstants.grass_wind_displacement = ps_r3_grass_wind_displacement;
            frameConstants.grass_interaction_displacement = ps_r3_grass_interaction_displacement;
            frameConstants.interaction_atlas_index = 0;
            frameConstants.wind_texture_index = dm->windTextureBindlessIndex;
            frameConstants.grass_color_tip.set(ps_r3_grass_color_tip.x, ps_r3_grass_color_tip.y, ps_r3_grass_color_tip.z, 0.0f);
            frameConstants.grass_color_base.set(ps_r3_grass_color_base.x, ps_r3_grass_color_base.y, ps_r3_grass_color_base.z, 0.0f);
            frameConstants.grass_sss_color.set(ps_r3_grass_sss_color.x, ps_r3_grass_sss_color.y, ps_r3_grass_sss_color.z, ps_r3_grass_sss_intensity);
            frameConstants.grass_color_variation = ps_r3_grass_color_variation;
            frameConstants.grass_blade_height = ps_r3_grass_blade_height;
            frameConstants.buildDetailsIndex = dm->buildDetailsBindlessIndex;
            frameConstants.buildDetailsPbrIndex = dm->buildDetailsPbrBindlessIndex;
            cmdList->writeBuffer(dm->cachedDetailGlobalsCB, &frameConstants, sizeof(frameConstants));

            // b4: dynamic_light (dummy)
            u8 dummyLight[48] = {};
            cmdList->writeBuffer(dm->cachedDynLightCB, dummyLight, 48);

            // Update grass tints
            FGDetailManager::GrassObjectTint tintData[64];
            for (int i = 0; i < 64; i++)
            {
                tintData[i].r = ps_r3_grass_object_tints[i].x;
                tintData[i].g = ps_r3_grass_object_tints[i].y;
                tintData[i].b = ps_r3_grass_object_tints[i].z;
                tintData[i].pad = 1.0f;
            }
            cmdList->writeBuffer(dm->cachedGrassTintsBuffer, tintData, sizeof(tintData));

            // SM6.6 bindless: Get the descriptor table from D3D12 backend
            nvrhi::IBindingSet* bindlessTable = nullptr;
            auto* backend = data.device->GetBackend();
            if (backend) {
                bindlessTable = backend->GetBindlessDescriptorTable();
            }

            if (data.gpuProfiler)
                data.gpuProfiler->BeginPass(cmdList, "Details.Draw");

            auto makeGrassBindingSet = [&](nvrhi::BufferHandle visibleIndicesBuffer) {
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(0, dm->cachedDynTransformsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(1, dm->cachedShaderParamsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(2, dm->cachedStaticGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(3, dm->cachedDetailGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(4, dm->cachedDynLightCB),
                    nvrhi::BindingSetItem::TypedBuffer_SRV(32, dm->cachedDummySlotIndirection),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(33, visibleIndicesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(34, dm->cachedGrassTintsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(35, dm->detailModelsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(37, dm->generatedInstancesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(38, dm->slotDataBuffer),
                    nvrhi::BindingSetItem::Sampler(0, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(1, dm->cachedSmp_PointClamp),
                    nvrhi::BindingSetItem::Sampler(2, dm->cachedSmp_LinearClamp),
                    nvrhi::BindingSetItem::Sampler(3, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(4, dm->cachedSmp_AnisoWrap),
                    nvrhi::BindingSetItem::Sampler(5, dm->cachedSmp_LinearWrap),
                };
                return framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, dm->graphicsBindingLayout, data.device->GetNVRHIDevice());
            };

            auto makePulledBindingSet = [&](nvrhi::BufferHandle visibleIndicesBuffer, nvrhi::BindingLayoutHandle layout) {
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(0, dm->cachedDynTransformsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(1, dm->cachedShaderParamsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(2, dm->cachedStaticGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(3, dm->cachedDetailGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(4, dm->cachedDynLightCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(33, visibleIndicesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(35, dm->detailModelsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(36, dm->pulledVertexBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(37, dm->generatedInstancesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(38, dm->slotDataBuffer),
                    nvrhi::BindingSetItem::Sampler(0, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(1, dm->cachedSmp_PointClamp),
                    nvrhi::BindingSetItem::Sampler(2, dm->cachedSmp_LinearClamp),
                    nvrhi::BindingSetItem::Sampler(3, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(4, dm->cachedSmp_AnisoWrap),
                    nvrhi::BindingSetItem::Sampler(5, dm->cachedSmp_LinearWrap),
                };
                return framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, layout, data.device->GetNVRHIDevice());
            };

            bool billboardMode = !ps_r__detail_gpu;

            if (billboardMode && dm->billboardGraphicsPipeline && dm->visibleBillboardInstancesBuffer &&
                dm->billboardDrawArgsBuffer && dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0)
            {
                nvrhi::BindingSetHandle bbBindingSet = makePulledBindingSet(dm->visibleBillboardInstancesBuffer, dm->billboardBindingLayout);

                nvrhi::GraphicsState state;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                state.pipeline = dm->billboardGraphicsPipeline;
                state.bindings = { bbBindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.indexBuffer = { dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = dm->billboardDrawArgsBuffer;

                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirect(0);
            }
            else
            {
                for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++)
                {
                    nvrhi::BindingSetHandle bindingSet = makeGrassBindingSet(dm->visibleInstancesBuffer[lod]);

                    nvrhi::GraphicsState state;
                    state.framebuffer = framebuffer;
                    state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                    state.pipeline = dm->graphicsPipeline;
                    state.bindings = { bindingSet };
                    if (bindlessTable)
                        state.addBindingSet(bindlessTable);
                    state.indexBuffer = { dm->bladeIndexBuffer[lod], nvrhi::Format::R16_UINT, 0 };
                    state.vertexBuffers = {{ dm->bladeVertexBuffer[lod], 0, 0 }};
                    state.indirectParams = dm->drawArgsBuffer[lod];

                    cmdList->setGraphicsState(state);
                    cmdList->drawIndexedIndirect(0);
                }
            }

            if (dm->decalGraphicsPipeline && dm->visibleDecalInstancesBuffer && dm->decalDrawArgsBuffer && dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0)
            {
                nvrhi::BindingSetHandle decalBindingSet = makePulledBindingSet(dm->visibleDecalInstancesBuffer, dm->decalBindingLayout);

                nvrhi::GraphicsState state;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                state.pipeline = dm->decalGraphicsPipeline;
                state.bindings = { decalBindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.indexBuffer = { dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = dm->decalDrawArgsBuffer;

                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirect(0);
            }

            if (data.gpuProfiler)
                data.gpuProfiler->EndPass(cmdList, "Details.Draw");
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.normal = passData.outputNormal;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
