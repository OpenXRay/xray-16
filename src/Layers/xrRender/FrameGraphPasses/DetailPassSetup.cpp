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
#include "xrCDB/Frustum.h"

// Detail rendering console variables
extern ENGINE_API float ps_r__Detail_l_aniso;
extern ENGINE_API float ps_r__Detail_l_ambient;

// Phase 5: Grass wind tuning parameters (defined in xrEngine)
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

            // Add Hi-Z as read dependency
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

            // Check if detail rendering is enabled
            if (!psDeviceFlags.is(rsDrawDetails))
                return;

            // Check if GPU buffers are ready
            if (!data.detailManager->instanceBuffer || data.detailManager->total_instance_count == 0)
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

            // === UPLOAD BUFFER DATA (first frame only) ===
            static bool s_detailDataUploaded = false;
            if (!s_detailDataUploaded && data.detailManager->total_instance_count > 0)
            {
                data.detailManager->UploadBufferData(cmdList);
                s_detailDataUploaded = true;
            }

            // === WIND COMPUTE PASS ===
            // Update FBM wind texture before culling (wind affects instance data)
            data.detailManager->DispatchWindCompute(cmdList, data.device->GetNVRHIDevice(), Device.fTimeGlobal);

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

            // === GRAPHICS PASS ===
            // Log texture formats for debugging
            const nvrhi::TextureDesc& colorDesc = colorTexture->getDesc();
            const nvrhi::TextureDesc& depthDesc = depthTexture->getDesc();

            // Create framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
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

            // Create common system constant buffers (b0, b1, b2, b4)
            nvrhi::BufferDesc cbDesc;
            cbDesc.isConstantBuffer = true;
            cbDesc.isVolatile = true;
            cbDesc.maxVersions = 16;

            // b0: dynamic_transforms (208 bytes)
            cbDesc.byteSize = sizeof(DynamicTransforms);
            cbDesc.debugName = "DynTransforms_Detail";
            auto dynTransformsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dynTransformsCB, &dynTrans, sizeof(dynTrans));

            // b1: shader_params (32 bytes) - create dummy for now
            cbDesc.byteSize = 32;
            cbDesc.debugName = "ShaderParams_Detail";
            auto shaderParamsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            u8 dummyParams[32] = {};
            cmdList->writeBuffer(shaderParamsCB, dummyParams, 32);

            // b2: static_globals (384 bytes)
            cbDesc.byteSize = sizeof(StaticGlobals);
            cbDesc.debugName = "StaticGlobals_Detail";
            auto staticGlobalsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            StaticGlobals staticGlobals = {};
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            // b3: DetailGlobals (must match HLSL cbuffer - 176 bytes)
            struct DetailFrameConstants
            {
                Fvector4 consts;                  // scale, scale, aniso, ambient
                Fvector4 wave;                    // wave animation params
                Fvector4 dir2D;                   // wind direction 1 (XY = direction)
                Fvector4 dir2D_2;                 // wind direction 2 (perpendicular)
                Fmatrix viewProj;                 // View-projection matrix
                Fvector4 detail_params;           // slot size/offset (unused for now)
                Fvector4 g_wind_direction;        // x=angle degrees, y=speed, z/w=unused
                float grass_wind_displacement;    // Wind strength
                float grass_interaction_displacement;  // Interaction strength
                u32 interaction_atlas_index;      // Bindless index for interaction atlas
                u32 wind_texture_index;           // Bindless index for wind texture
            };

            // Get wind parameters
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
            frameConstants.detail_params.set(64.0f, 64.0f, 0.0f, 0.0f);  // Default slot size
            frameConstants.g_wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
            frameConstants.grass_wind_displacement = ps_r3_grass_wind_displacement;
            frameConstants.grass_interaction_displacement = ps_r3_grass_interaction_displacement;
            frameConstants.interaction_atlas_index = 0;  // TODO: Add interaction atlas
            frameConstants.wind_texture_index = data.detailManager->windTextureBindlessIndex;

            cbDesc.byteSize = sizeof(DetailFrameConstants);
            cbDesc.debugName = "DetailGlobals";
            auto detailGlobalsCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            cmdList->writeBuffer(detailGlobalsCB, &frameConstants, sizeof(frameConstants));

            // b4: dynamic_light (48 bytes) - create dummy for now
            cbDesc.byteSize = 48;
            cbDesc.debugName = "DynLight_Detail";
            auto dynLightCB = data.device->GetNVRHIDevice()->createBuffer(cbDesc);
            u8 dummyLight[48] = {};
            cmdList->writeBuffer(dynLightCB, dummyLight, 48);

            // BINDLESS MODE: Create only slot indirection buffer and sampler
            // Textures are fetched via GetBindlessTexture() using indices in $Globals constant buffer

            // Create dummy g_Materials buffer (required by bindless_common.h, even if unused)
            struct MaterialData { u32 diffuseIndex; u32 normalIndex; u32 detailIndex; u32 pbrIndex; float detailScale; float alphaRef; u32 flags; float padding; };
            nvrhi::BufferDesc matBufDesc;
            matBufDesc.byteSize = sizeof(MaterialData); // 1 material
            matBufDesc.structStride = sizeof(MaterialData);
            matBufDesc.canHaveRawViews = false;
            matBufDesc.isVertexBuffer = false;
            matBufDesc.debugName = "DummyMaterials_Detail";
            matBufDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            matBufDesc.keepInitialState = true;
            nvrhi::BufferHandle dummyMaterials = data.device->GetNVRHIDevice()->createBuffer(matBufDesc);

            // Create dummy buffer for slot indirection (TODO: implement proper virtual texture system)
            nvrhi::BufferDesc dummyBufDesc;
            dummyBufDesc.byteSize = 4; // 1 uint
            dummyBufDesc.structStride = 0; // Typed buffer
            dummyBufDesc.format = nvrhi::Format::R32_UINT;
            dummyBufDesc.canHaveTypedViews = true;
            dummyBufDesc.isVertexBuffer = false;
            dummyBufDesc.debugName = "DummySlotIndirection";
            dummyBufDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            dummyBufDesc.keepInitialState = true;
            nvrhi::BufferHandle dummySlotIndirection = data.device->GetNVRHIDevice()->createBuffer(dummyBufDesc);

            // Create samplers (match common_samplers.h declarations)
            // s0: g_LinearSampler (from bindless_common.h)
            nvrhi::SamplerDesc s0Desc;
            s0Desc.setAllFilters(true);  // Linear filtering
            s0Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s0_LinearSampler = data.device->GetNVRHIDevice()->createSampler(s0Desc);

            // s1: smp_nofilter (CLAMP, POINT, NONE, POINT)
            nvrhi::SamplerDesc s1Desc;
            s1Desc.setAllFilters(false);
            s1Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            nvrhi::SamplerHandle s1_nofilter = data.device->GetNVRHIDevice()->createSampler(s1Desc);

            // s2: smp_rtlinear (CLAMP, LINEAR, NONE, LINEAR)
            nvrhi::SamplerDesc s2Desc;
            s2Desc.setAllFilters(true);
            s2Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            nvrhi::SamplerHandle s2_rtlinear = data.device->GetNVRHIDevice()->createSampler(s2Desc);

            // s3: smp_linear (WRAP, LINEAR, LINEAR, LINEAR)
            nvrhi::SamplerDesc s3Desc;
            s3Desc.setAllFilters(true);
            s3Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s3_linear = data.device->GetNVRHIDevice()->createSampler(s3Desc);

            // s4: smp_base (WRAP, ANISOTROPIC, LINEAR, ANISOTROPIC)
            nvrhi::SamplerDesc s4Desc;
            s4Desc.setAllFilters(true);
            s4Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            s4Desc.setMaxAnisotropy(16.0f);
            nvrhi::SamplerHandle s4_base = data.device->GetNVRHIDevice()->createSampler(s4Desc);

            // s5: smp_material (generic sampler)
            nvrhi::SamplerDesc s5Desc;
            s5Desc.setAllFilters(true);
            s5Desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            nvrhi::SamplerHandle s5_material = data.device->GetNVRHIDevice()->createSampler(s5Desc);

            // Create binding set (BINDLESS: b0-b4, t8, t32-t33, s0-s5)
            // NOTE: t8 required by bindless_common.h (g_Materials)
            // NOTE: s0-s5 required by common_samplers.h (even if unused)
            // NOTE: t32-t33 avoid common_samplers.h conflict (t0-t31)
            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),                                 // b0: dynamic_transforms
                nvrhi::BindingSetItem::ConstantBuffer(1, shaderParamsCB),                                  // b1: shader_params
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),                                 // b2: static_globals
                nvrhi::BindingSetItem::ConstantBuffer(3, detailGlobalsCB),                                 // b3: $Globals (with texture indices)
                nvrhi::BindingSetItem::ConstantBuffer(4, dynLightCB),                                      // b4: dynamic_light
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, dummyMaterials),                           // t8: g_Materials (from bindless_common.h)
                nvrhi::BindingSetItem::TypedBuffer_SRV(32, dummySlotIndirection),                         // t32: slot_indirection (dummy for now)
                nvrhi::BindingSetItem::StructuredBuffer_SRV(33, data.detailManager->visibleIndicesBuffer), // t33: detail_buffer (visible instances)
                nvrhi::BindingSetItem::Sampler(0, s0_LinearSampler),                                       // s0: g_LinearSampler
                nvrhi::BindingSetItem::Sampler(1, s1_nofilter),                                            // s1: smp_nofilter
                nvrhi::BindingSetItem::Sampler(2, s2_rtlinear),                                            // s2: smp_rtlinear
                nvrhi::BindingSetItem::Sampler(3, s3_linear),                                              // s3: smp_linear
                nvrhi::BindingSetItem::Sampler(4, s4_base),                                                // s4: smp_base
                nvrhi::BindingSetItem::Sampler(5, s5_material),                                            // s5: smp_material
            };

            nvrhi::BindingSetHandle bindingSet = data.device->GetNVRHIDevice()->createBindingSet(bindDesc, data.detailManager->graphicsBindingLayout);

            // Setup graphics state
            nvrhi::GraphicsState state;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
            state.pipeline = data.detailManager->graphicsPipeline;
            state.bindings = { bindingSet };

            // SM6.6 bindless: Add the descriptor table from D3D12 backend
            // IDescriptorTable derives from IBindingSet, so we add it to bindings
            auto* backend = data.device->GetBackend();
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable) {
                    state.addBindingSet(bindlessTable);
                }
            }

            // Set up vertex/index buffers (blade geometry)
            state.indexBuffer = { data.detailManager->bladeIndexBuffer, nvrhi::Format::R16_UINT, 0 };
            state.vertexBuffers = {{ data.detailManager->bladeVertexBuffer, 0, 0 }};

            // Set up indirect draw args buffer
            state.indirectParams = data.detailManager->drawArgsBuffer;

            // Draw indexed indirect
            cmdList->setGraphicsState(state);
            cmdList->drawIndexedIndirect(0);  // Offset 0 in draw args buffer
        }
    );

    return passData.outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
