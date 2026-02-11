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
#include "xrCDB/Frustum.h"

extern ENGINE_API float ps_r__Detail_l_aniso;
extern ENGINE_API float ps_r__Detail_l_ambient;

extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;

extern ENGINE_API Fvector3 ps_r3_grass_color_tip;
extern ENGINE_API Fvector3 ps_r3_grass_color_base;
extern ENGINE_API float ps_r3_grass_color_variation;
extern ENGINE_API Fvector3 ps_r3_grass_sss_color;
extern ENGINE_API float ps_r3_grass_sss_intensity;
extern ENGINE_API Fvector3 ps_r3_grass_object_tints[64];

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;

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
    xray::profiler::GPUProfiler* gpuProfiler
)
{
    struct DetailPassData {
        VirtualResourceHandle inputColor;
        VirtualResourceHandle depth;
        VirtualResourceHandle outputColor;
        VirtualResourceHandle hiZPyramid;
        ng::RenderDevice* device;
        RENDER_NAMESPACE::FGDetailManager* detailManager;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        u32 hiZWidth;
        u32 hiZHeight;
        u32 hiZMipLevels;
        Fmatrix prevViewProj;
        bool hasPrevViewProj;
        xray::profiler::GPUProfiler* gpuProfiler;
    };

    Fmatrix capturedPrevViewProj;
    bool hasPrevViewProj = (prevViewProj != nullptr);
    if (hasPrevViewProj) {
        capturedPrevViewProj = *prevViewProj;
    } else {
        capturedPrevViewProj.identity();
    }

    auto& passData = fg.addCallbackPass<DetailPassData>(
        "Details",
        [&, width, height, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels, capturedPrevViewProj, hasPrevViewProj, gpuProfiler](
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

            data.hiZPyramid = passBuilder.read(hiZPyramid, ResourceState::ShaderResource);

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.depth = passBuilder.read(forwardInputs.depth);
            data.outputColor = passBuilder.write(forwardInputs.albedo);

            data.outputs.albedo = data.outputColor;
            data.outputs.depth = data.depth;
        },
        [](const DetailPassData& data, const FrameGraph& fg, ng::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailPass", 10);

            if (!data.detailManager)
                return;

            if (!psDeviceFlags.is(rsDrawDetails))
                return;

            bool detailPipelineValid = (data.detailManager->instanceGenPipeline && data.detailManager->slotDataBuffer);
            if (!detailPipelineValid)
                return;

            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.outputColor);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depth);

            if (!colorTexture || !depthTexture)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;

            static bool s_detailDataUploaded = false;
            if (!s_detailDataUploaded)
            {
                data.detailManager->UploadBufferData(cmdList);
                s_detailDataUploaded = true;
            }

            static float s_lastBladeWidth = ps_r3_grass_blade_width;
            if (s_lastBladeWidth != ps_r3_grass_blade_width)
            {
                data.detailManager->RegenerateBladeGeometry(cmdList);
                s_lastBladeWidth = ps_r3_grass_blade_width;
            }

            if (g_pGamePersistent)
            {
                data.detailManager->windSpeed = _max(
                    g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier,
                    ps_r3_grass_wind_min);
                float wind_rad = deg2rad(g_pGamePersistent->Environment().CurrentEnv.wind_direction);
                data.detailManager->windDirection.set(_cos(wind_rad), _sin(wind_rad));
            }

            CFrustum frustum;
            frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

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

            nvrhi::ITexture* hiZTexture = fg.GetPhysicalTexture(data.hiZPyramid);

            const float fadeDistance = g_pGamePersistent->Environment().CurrentEnv.far_plane;

            Fmatrix effectivePrevViewProj = data.hasPrevViewProj ? data.prevViewProj : Device.mFullTransform;

            data.detailManager->DispatchCulling(
                cmdList,
                data.device->GetNVRHIDevice(),
                hiZTexture,
                Device.mFullTransform,
                effectivePrevViewProj,
                extractedPlanes,
                frustum.p_count,
                Device.vCameraPosition,
                fadeDistance,
                data.hiZWidth,
                data.hiZHeight,
                data.hiZMipLevels,
                data.gpuProfiler
            );

            data.detailManager->ScheduleStatsReadback(cmdList, data.device->GetNVRHIDevice());

            const nvrhi::TextureDesc& colorDesc = colorTexture->getDesc();
            const nvrhi::TextureDesc& depthDesc = depthTexture->getDesc();

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
            fbDesc.setDepthAttachment(depthTexture);

            nvrhi::FramebufferHandle framebuffer = data.device->GetNVRHIDevice()->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            if (!data.detailManager->graphicsPipeline)
            {
                data.detailManager->CreateGraphicsPipeline(data.device, framebuffer);
            }

            if (!data.detailManager->graphicsPipeline)
            {
                Msg("! [DetailPass] Graphics pipeline not available");
                return;
            }

            auto* dm = data.detailManager;

            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dm->cachedDynTransformsCB, &dynTrans, sizeof(dynTrans));

            u8 dummyParams[32] = {};
            cmdList->writeBuffer(dm->cachedShaderParamsCB, dummyParams, 32);

            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(dm->cachedStaticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

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

            u8 dummyLight[48] = {};
            cmdList->writeBuffer(dm->cachedDynLightCB, dummyLight, 48);

            FGDetailManager::GrassObjectTint tintData[64];
            for (int i = 0; i < 64; i++)
            {
                tintData[i].r = ps_r3_grass_object_tints[i].x;
                tintData[i].g = ps_r3_grass_object_tints[i].y;
                tintData[i].b = ps_r3_grass_object_tints[i].z;
                tintData[i].pad = 1.0f;
            }
            cmdList->writeBuffer(dm->cachedGrassTintsBuffer, tintData, sizeof(tintData));

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
                return data.device->GetNVRHIDevice()->createBindingSet(bindDesc, dm->graphicsBindingLayout);
            };

            auto makeDecalBindingSet = [&]() {
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(0, dm->cachedDynTransformsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(1, dm->cachedShaderParamsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(2, dm->cachedStaticGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(3, dm->cachedDetailGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(4, dm->cachedDynLightCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(33, dm->visibleDecalInstancesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(35, dm->detailModelsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(36, dm->decalPulledVertexBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(37, dm->generatedInstancesBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(38, dm->slotDataBuffer),
                    nvrhi::BindingSetItem::Sampler(0, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(1, dm->cachedSmp_PointClamp),
                    nvrhi::BindingSetItem::Sampler(2, dm->cachedSmp_LinearClamp),
                    nvrhi::BindingSetItem::Sampler(3, dm->cachedSmp_LinearWrap),
                    nvrhi::BindingSetItem::Sampler(4, dm->cachedSmp_AnisoWrap),
                    nvrhi::BindingSetItem::Sampler(5, dm->cachedSmp_LinearWrap),
                };
                return data.device->GetNVRHIDevice()->createBindingSet(bindDesc, dm->decalBindingLayout);
            };

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

            if (dm->decalGraphicsPipeline && dm->visibleDecalInstancesBuffer && dm->decalDrawArgsBuffer && dm->decalIndexBuffer && dm->maxDecalIndexCount > 0)
            {
                nvrhi::BindingSetHandle decalBindingSet = makeDecalBindingSet();

                nvrhi::GraphicsState state;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                state.pipeline = dm->decalGraphicsPipeline;
                state.bindings = { decalBindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.indexBuffer = { dm->decalIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = dm->decalDrawArgsBuffer;

                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirect(0);
            }

            if (data.gpuProfiler)
                data.gpuProfiler->EndPass(cmdList, "Details.Draw");
        }
    );

    return passData.outputs;
}

}
