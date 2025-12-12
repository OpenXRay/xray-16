#include "stdafx.h"
#include "DetailPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "xrCDB/Frustum.h"

extern ENGINE_API float ps_r__Detail_l_aniso;
extern ENGINE_API float ps_r__Detail_l_ambient;

extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;

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
    u32 hiZMipLevels
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
    };

    auto& passData = fg.addCallbackPass<DetailPassData>(
        "Details",
        [&, width, height, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels](
            FrameGraph& builder, PassHandle passHandle, DetailPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.detailManager = detailManager;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;

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

            if (!data.detailManager->instanceBuffer || data.detailManager->total_instance_count == 0)
                return;

            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.outputColor);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depth);

            if (!colorTexture || !depthTexture)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;

            static bool s_detailDataUploaded = false;
            if (!s_detailDataUploaded && data.detailManager->total_instance_count > 0)
            {
                data.detailManager->UploadBufferData(cmdList);
                s_detailDataUploaded = true;
            }

            data.detailManager->DispatchWindCompute(cmdList, data.device->GetNVRHIDevice(), Device.fTimeGlobal);

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
            data.detailManager->DispatchCulling(
                cmdList,
                data.device->GetNVRHIDevice(),
                hiZTexture,
                Device.mFullTransform,
                extractedPlanes,
                frustum.p_count,
                Device.vCameraPosition,
                fadeDistance,
                data.hiZWidth,
                data.hiZHeight,
                data.hiZMipLevels
            );

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

            nvrhi::BufferDesc cbDesc;
            cbDesc.isConstantBuffer = true;
            cbDesc.isVolatile = true;
            cbDesc.maxVersions = 16;

            cbDesc.byteSize = sizeof(DynamicTransforms);
            cbDesc.debugName = "DynTransforms_Detail";
            auto dynTransformsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dynTransformsCB, &dynTrans, sizeof(dynTrans));

            cbDesc.byteSize = 32;
            cbDesc.debugName = "ShaderParams_Detail";
            auto shaderParamsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            u8 dummyParams[32] = {};
            cmdList->writeBuffer(shaderParamsCB, dummyParams, 32);

            cbDesc.byteSize = sizeof(StaticGlobals);
            cbDesc.debugName = "StaticGlobals_Detail";
            auto staticGlobalsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            struct DetailFrameConstants
            {
                Fvector4 consts;
                Fvector4 wave;
                Fvector4 dir2D;
                Fvector4 dir2D_2;
                Fmatrix viewProj;
                Fvector4 detail_params;
                Fvector4 g_wind_direction;
                float grass_wind_displacement;
                float grass_interaction_displacement;
                u32 interaction_atlas_index;
                u32 wind_texture_index;
            };

            float windAngleDeg = 0.0f;
            float windSpeed = data.detailManager->windSpeed;
            if (g_pGamePersistent)
                windAngleDeg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;

            DetailFrameConstants frameConstants;
            const float quant = 16384.0f;
            frameConstants.consts.set(1.0f / quant, 1.0f / quant, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
            frameConstants.wave.set(1.0f / 5.0f, 1.0f / 7.0f, 1.0f / 3.0f, Device.fTimeGlobal);
            frameConstants.dir2D.set(data.detailManager->windDirection.x, data.detailManager->windDirection.y, 0.0f, 0.0f);
            frameConstants.dir2D_2.set(-data.detailManager->windDirection.y, data.detailManager->windDirection.x, 0.0f, 0.0f);
            frameConstants.viewProj = Device.mFullTransform;
            frameConstants.detail_params.set(64.0f, 64.0f, 0.0f, 0.0f);
            frameConstants.g_wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
            frameConstants.grass_wind_displacement = ps_r3_grass_wind_displacement;
            frameConstants.grass_interaction_displacement = ps_r3_grass_interaction_displacement;
            frameConstants.interaction_atlas_index = 0;
            frameConstants.wind_texture_index = data.detailManager->windTextureBindlessIndex;

            cbDesc.byteSize = sizeof(DetailFrameConstants);
            cbDesc.debugName = "DetailGlobals";
            auto detailGlobalsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            cmdList->writeBuffer(detailGlobalsCB, &frameConstants, sizeof(frameConstants));

            cbDesc.byteSize = 48;
            cbDesc.debugName = "DynLight_Detail";
            auto dynLightCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            u8 dummyLight[48] = {};
            cmdList->writeBuffer(dynLightCB, dummyLight, 48);

            struct MaterialData { u32 diffuseIndex; u32 normalIndex; u32 detailIndex; u32 pbrIndex; float detailScale; float alphaRef; u32 flags; float padding; };
            nvrhi::BufferDesc matBufDesc;
            matBufDesc.byteSize = sizeof(MaterialData);
            matBufDesc.structStride = sizeof(MaterialData);
            matBufDesc.canHaveRawViews = false;
            matBufDesc.isVertexBuffer = false;
            matBufDesc.debugName = "DummyMaterials_Detail";
            matBufDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            matBufDesc.keepInitialState = true;
            nvrhi::BufferHandle dummyMaterials = data.device->GetNVRHIDevice()->createBuffer(matBufDesc);

            nvrhi::BufferDesc dummyBufDesc;
            dummyBufDesc.byteSize = 4;
            dummyBufDesc.structStride = 0;
            dummyBufDesc.format = nvrhi::Format::R32_UINT;
            dummyBufDesc.canHaveTypedViews = true;
            dummyBufDesc.isVertexBuffer = false;
            dummyBufDesc.debugName = "DummySlotIndirection";
            dummyBufDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            dummyBufDesc.keepInitialState = true;
            nvrhi::BufferHandle dummySlotIndirection = data.device->GetNVRHIDevice()->createBuffer(dummyBufDesc);

            nvrhi::SamplerDesc s0Desc;
            s0Desc.setAllFilters(true);
            s0Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s0_LinearSampler = data.device->GetNVRHIDevice()->createSampler(s0Desc);

            nvrhi::SamplerDesc s1Desc;
            s1Desc.setAllFilters(false);
            s1Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            nvrhi::SamplerHandle s1_nofilter = data.device->GetNVRHIDevice()->createSampler(s1Desc);

            nvrhi::SamplerDesc s2Desc;
            s2Desc.setAllFilters(true);
            s2Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            nvrhi::SamplerHandle s2_rtlinear = data.device->GetNVRHIDevice()->createSampler(s2Desc);

            nvrhi::SamplerDesc s3Desc;
            s3Desc.setAllFilters(true);
            s3Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s3_linear = data.device->GetNVRHIDevice()->createSampler(s3Desc);

            nvrhi::SamplerDesc s4Desc;
            s4Desc.setAllFilters(true);
            s4Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            s4Desc.setMaxAnisotropy(16.0f);
            nvrhi::SamplerHandle s4_base = data.device->GetNVRHIDevice()->createSampler(s4Desc);

            nvrhi::SamplerDesc s5Desc;
            s5Desc.setAllFilters(true);
            s5Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s5_material = data.device->GetNVRHIDevice()->createSampler(s5Desc);

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
                nvrhi::BindingSetItem::ConstantBuffer(1, shaderParamsCB),
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::ConstantBuffer(3, detailGlobalsCB),
                nvrhi::BindingSetItem::ConstantBuffer(4, dynLightCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, dummyMaterials),
                nvrhi::BindingSetItem::TypedBuffer_SRV(32, dummySlotIndirection),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(33, data.detailManager->visibleIndicesBuffer),
                nvrhi::BindingSetItem::Sampler(0, s0_LinearSampler),
                nvrhi::BindingSetItem::Sampler(1, s1_nofilter),
                nvrhi::BindingSetItem::Sampler(2, s2_rtlinear),
                nvrhi::BindingSetItem::Sampler(3, s3_linear),
                nvrhi::BindingSetItem::Sampler(4, s4_base),
                nvrhi::BindingSetItem::Sampler(5, s5_material),
            };

            nvrhi::BindingSetHandle bindingSet = data.device->GetNVRHIDevice()->createBindingSet(bindDesc, data.detailManager->graphicsBindingLayout);

            nvrhi::GraphicsState state;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
            state.pipeline = data.detailManager->graphicsPipeline;
            state.bindings = { bindingSet };

            auto* backend = data.device->GetBackend();
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable) {
                    state.addBindingSet(bindlessTable);
                }
            }

            state.indexBuffer = { data.detailManager->bladeIndexBuffer, nvrhi::Format::R16_UINT, 0 };
            state.vertexBuffers = {{ data.detailManager->bladeVertexBuffer, 0, 0 }};

            state.indirectParams = data.detailManager->drawArgsBuffer;

            cmdList->setGraphicsState(state);
            cmdList->drawIndexedIndirect(0);
        }
    );

    return passData.outputs;
}

}
