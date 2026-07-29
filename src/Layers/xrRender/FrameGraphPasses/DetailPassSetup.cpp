// DetailPassSetup.cpp - Framegraph pass for detail objects (grass, etc.)
#include "stdafx.h"
#include "PassCommon.h"
#include "DetailPassSetup.h"
#include "IBLPrefilterPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"

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
extern ENGINE_API Fvector3 ps_r3_grass_object_tints[64];
// Grass SSS intensity uses shared r_foliage_sss / r_foliage_sss_intensity (ShaderConstants.h)

// Grass blade geometry parameters (defined in xrEngine)
extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;

namespace xray::render::fg
{
    extern int ps_r__detail_gpu;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

DefaultOutputLayout setupDetailPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    xray::profiler::GPUProfiler* gpuProfiler,
    nvrhi::ITexture* shadowMapArray,
    VirtualResourceHandle shadowMapHandle,
    nvrhi::ITexture* contactDepth,
    nvrhi::ITexture* contactHistory
    , VirtualResourceHandle perlinReady
    , nvrhi::ITexture* const* shadowCascades
    , nvrhi::ITexture* localShadowAtlas
    , nvrhi::ITexture* localShadowESM
)
{
    if (detailManager)
    {
        static u32 s_detailPipeEpoch = 0;
        constexpr u32 kDetailPipeEpoch = 4;
        if (s_detailPipeEpoch != kDetailPipeEpoch)
        {
            detailManager->graphicsPipeline = nullptr;
            detailManager->decalGraphicsPipeline = nullptr;
            detailManager->billboardGraphicsPipeline = nullptr;
            detailManager->graphicsBindingLayout = nullptr;
            detailManager->decalBindingLayout = nullptr;
            detailManager->billboardBindingLayout = nullptr;
            s_detailPipeEpoch = kDetailPipeEpoch;
        }
    }
    if (detailManager && (!detailManager->graphicsPipeline || !detailManager->decalGraphicsPipeline))
    {
        if (detailManager->graphicsPipeline && !detailManager->decalGraphicsPipeline)
            detailManager->graphicsPipeline = nullptr;
        // Shaders must be loaded before PSO creation (else "invalid parameters")
        if (!detailManager->vertexShader || !detailManager->pixelShader)
        {
            if (auto* loader = GEnv.Render->GetShaderLoader())
                detailManager->LoadGraphicsShaders(loader);
        }
        if (detailManager->vertexShader && detailManager->pixelShader)
        {
            nvrhi::FramebufferInfo fbInfo;
            fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
            fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
            fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
            fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
            fbInfo.depthFormat = nvrhi::Format::D32;
            detailManager->CreateGraphicsPipeline(device, fbInfo);
        }
    }

    auto& passData = fg.addCallbackPass<DetailPassData>(
        "DetailDraw",
        [&, width, height, gpuProfiler, shadowMapArray, shadowMapHandle, contactDepth, contactHistory, perlinReady, shadowCascades, localShadowAtlas, localShadowESM](
            FrameGraph& builder, PassHandle passHandle, DetailPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.detailManager = detailManager;
            data.gpuProfiler = gpuProfiler;
            data.shadowMapArray = shadowMapArray;
            for (int i = 0; i < 3; ++i)
                data.shadowCascades[i] = (shadowCascades && shadowCascades[i]) ? shadowCascades[i] : shadowMapArray;
            data.localShadowAtlas = localShadowAtlas;
            data.localShadowESM = localShadowESM;
            data.contactDepth = contactDepth;
            data.contactHistory = contactHistory;

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);
            data.outputColor = passBuilder.write(forwardInputs.albedo);
            data.outputNormal = passBuilder.readWrite(forwardInputs.normal, ResourceState::RenderTarget);
            data.baseColor = passBuilder.readWrite(forwardInputs.baseColor, ResourceState::RenderTarget);
            data.worldPos = passBuilder.readWrite(forwardInputs.worldPos, ResourceState::RenderTarget);
            if (shadowMapHandle.is_valid())
                data.shadowMap = passBuilder.read(shadowMapHandle, ResourceState::ShaderResource);
            if (perlinReady.is_valid())
                passBuilder.read(perlinReady);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.worldPos = data.worldPos;
            data.outputs.depth = data.depth;
        },
        [](const DetailPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
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

            if (g_pGamePersistent)
            {
                data.detailManager->windSpeed = _max(
                    g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier,
                    ps_r3_grass_wind_min);
                float wind_rad = deg2rad(g_pGamePersistent->Environment().CurrentEnv.wind_direction);
                data.detailManager->windDirection.set(_cos(wind_rad), _sin(wind_rad));
            }

            nvrhi::ITexture* normalTexture = fg.GetPhysicalTexture(data.outputNormal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;
            if (!normalTexture || !baseColorRT || !worldPosRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
            fbDesc.addColorAttachment(normalTexture);
            fbDesc.addColorAttachment(baseColorRT);
            fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthTexture);

            auto framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer(
                "DetailDraw", fbDesc, data.device->GetNVRHIDevice());
            if (!framebuffer)
                return;

            if (!data.detailManager->graphicsPipeline)
                return;

            auto* dm = data.detailManager;
            auto* renderDevice = data.device;
            auto& cache = framegraph::GetPassResourceCache();

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), renderDevice);
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }
            auto detailGlobalsCB = cache.GetOrCreateVolatileCB("Detail", "DetailGlobals", sizeof(FGDetailManager::DetailFrameConstants), renderDevice);
            auto dynLightCB = cache.GetOrCreateVolatileCB("Detail", "DynLight", 48, renderDevice);

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
            frameConstants.viewProj = Device.mFullTransform;
            frameConstants.detail_params.set(
                float(dm->dtH.x_size()), float(dm->dtH.z_size()),
                float(dm->dtH.x_offs()), float(dm->dtH.z_offs()));
            frameConstants.g_wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
            frameConstants.grass_wind_displacement = ps_r3_grass_wind_displacement;
            frameConstants.grass_interaction_displacement = ps_r3_grass_interaction_displacement;
            frameConstants.interaction_atlas_index = 0;
            frameConstants.perlin4d_texture_index = dm->perlin4dBindlessIndex;
            frameConstants.grass_color_tip.set(ps_r3_grass_color_tip.x, ps_r3_grass_color_tip.y, ps_r3_grass_color_tip.z, 0.0f);
            frameConstants.grass_color_base.set(ps_r3_grass_color_base.x, ps_r3_grass_color_base.y, ps_r3_grass_color_base.z, 0.0f);
            // Same enable/intensity as trees/leaves (r_foliage_sss*, parallax.z path)
            const float grassSss =
                (ps_r_foliage_sss != 0) ? ps_r_foliage_sss_intensity : 0.0f;
            frameConstants.grass_sss_color.set(
                ps_r3_grass_sss_color.x, ps_r3_grass_sss_color.y, ps_r3_grass_sss_color.z, grassSss);
            frameConstants.grass_color_variation = ps_r3_grass_color_variation;
            frameConstants.grass_blade_height = ps_r3_grass_blade_height;
            frameConstants.buildDetailsIndex = dm->buildDetailsBindlessIndex;
            frameConstants.buildDetailsPbrIndex = dm->buildDetailsPbrBindlessIndex;
            frameConstants.grassVeinIndex = dm->grassVeinBindlessIndex;
            frameConstants.pad0 = frameConstants.pad1 = frameConstants.pad2 = 0;
            cmdList->writeBuffer(detailGlobalsCB, &frameConstants, sizeof(frameConstants));

            u8 dummyLight[48] = {};
            cmdList->writeBuffer(dynLightCB, dummyLight, 48);

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

            auto* nvDev = data.device->GetNVRHIDevice();
            nvrhi::ITexture* dummy2D = cache.GetDummyShadowMap2D(nvDev);

            auto bindShadowMap = [&](framegraph::BindingSetBuilder& bsb) {
                static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
                for (u32 i = 0; i < 3; ++i)
                {
                    nvrhi::ITexture* t = data.shadowCascades[i] ? data.shadowCascades[i]
                        : (i == 0 && data.shadowMapArray ? data.shadowMapArray : dummy2D);
                    if (t)
                        bsb.Texture(kNames[i], t);
                }
                nvrhi::ITexture* contactHist = data.contactHistory
                    ? data.contactHistory
                    : cache.GetDummyContactHistory(nvDev);
                if (contactHist)
                    bsb.Texture("g_ContactHistory", contactHist);
                nvrhi::ITexture* localAtlas = data.localShadowAtlas
                    ? data.localShadowAtlas
                    : cache.GetDummyShadowMap(nvDev);
                if (localAtlas)
                    bsb.Texture("g_LocalShadowAtlas", localAtlas);
                nvrhi::ITexture* localEsm = data.localShadowESM
                    ? data.localShadowESM
                    : cache.GetDummyLocalShadowESM(nvDev);
                if (localEsm)
                    bsb.Texture("g_LocalShadowESM", localEsm);
                static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
                nvrhi::ITexture* dummyHzb = cache.GetDummyContactDepth(nvDev);
                for (u32 i = 0; i < 3; ++i)
                {
                    if (dummyHzb)
                        bsb.Texture(kHzb[i], dummyHzb);
                }
            };

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* grassVsRefl = shaderLoader->GetCachedReflection("detail_gpu", ".vs");
            auto* grassPsRefl = shaderLoader->GetCachedReflection("detail_gpu", ".ps");

            // detail_*.ps includes bindless_common.h → reflection keeps g_Materials /
            // g_TerrainMaterials / g_VariantTextures (t8/t9/t10) even when unused.
            auto bindBindlessMaterialTables = [&](framegraph::BindingSetBuilder& bsb) {
                BindBindlessMaterialTables(bsb);
            };

            auto makeGrassBindingSet = [&](nvrhi::BufferHandle visibleIndicesBuffer) {
                framegraph::BindingSetBuilder bsb(*grassVsRefl, *grassPsRefl, nvDev, "Detail.Grass");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
                bsb.ConstantBuffer("$Globals", dynLightCB);
                bindBindlessMaterialTables(bsb);
                bsb.BufferSRV("visible_indices", visibleIndicesBuffer);
                bsb.BufferSRV("grass_object_tints", dm->cachedGrassTintsBuffer);
                bsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                bsb.BufferSRV("slot_data", dm->slotDataBuffer);
                bsb.BufferSRV("slot_indirection", dm->cachedDummySlotIndirection);
                bsb.Texture("g_Perlin4D", dm->perlin4dTexture);
                bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
                if (ClusteredLightManager::Instance().GetShadowDataBuffer())
                    bsb.BufferSRV("g_ShadowData", ClusteredLightManager::Instance().GetShadowDataBuffer());
                bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                bindShadowMap(bsb);
                const auto& ibl = GetCurrentIBLBindResources();
                nvrhi::ITexture* sky0 = ibl.spec0 ? ibl.spec0 : cache.GetDummyCubeMap(nvDev);
                nvrhi::ITexture* sky1 = ibl.spec1 ? ibl.spec1 : sky0;
                if (sky0) bsb.Texture("s_env0", sky0);
                if (sky1) bsb.Texture("s_env1", sky1);
                BindIBLResources(bsb, ibl, nvDev);
                return cache.GetOrCreateBindingSet(bsb.Build(), dm->graphicsBindingLayout, nvDev);
            };

            auto* bbVsRefl = shaderLoader->GetCachedReflection("detail_billboard", ".vs");
            auto* bbPsRefl = shaderLoader->GetCachedReflection("detail_billboard", ".ps");

            auto makePulledBindingSet = [&](nvrhi::BufferHandle visibleIndicesBuffer, nvrhi::BindingLayoutHandle layout) {
                framegraph::BindingSetBuilder bsb(*bbVsRefl, *bbPsRefl, nvDev, "Detail.Billboard");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
                bindBindlessMaterialTables(bsb);
                bsb.BufferSRV("visible_indices", visibleIndicesBuffer);
                bsb.BufferSRV("detail_models", dm->detailModelsBuffer);
                bsb.BufferSRV("pulled_vertices", dm->pulledVertexBuffer);
                bsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                bsb.BufferSRV("slot_data", dm->slotDataBuffer);
                bsb.Texture("g_Perlin4D", dm->perlin4dTexture);
                bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
                if (ClusteredLightManager::Instance().GetShadowDataBuffer())
                    bsb.BufferSRV("g_ShadowData", ClusteredLightManager::Instance().GetShadowDataBuffer());
                bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                bindShadowMap(bsb);
                const auto& ibl = GetCurrentIBLBindResources();
                nvrhi::ITexture* sky0 = ibl.spec0 ? ibl.spec0 : cache.GetDummyCubeMap(nvDev);
                nvrhi::ITexture* sky1 = ibl.spec1 ? ibl.spec1 : sky0;
                if (sky0) bsb.Texture("s_env0", sky0);
                if (sky1) bsb.Texture("s_env1", sky1);
                BindIBLResources(bsb, ibl, nvDev);
                return cache.GetOrCreateBindingSet(bsb.Build(), layout, nvDev);
            };

            bool billboardMode = !ps_r__detail_gpu;

            if (billboardMode && dm->billboardGraphicsPipeline && dm->visibleBillboardInstancesBuffer &&
                dm->billboardDrawArgsBuffer && dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0)
            {
                nvrhi::BindingSetHandle bbBindingSet = makePulledBindingSet(dm->visibleBillboardInstancesBuffer, dm->billboardBindingLayout);
                if (!bbBindingSet)
                {
                    static bool s_logged = false;
                    if (!s_logged)
                    {
                        Msg("! [DetailPass] Billboard binding set failed (missing t8/t9/t10 or CSM)");
                        s_logged = true;
                    }
                }
                else
                {
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
            }
            else if (!billboardMode)
            {
                for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++)
                {
                    nvrhi::BindingSetHandle bindingSet = makeGrassBindingSet(dm->visibleInstancesBuffer[lod]);
                    if (!bindingSet)
                    {
                        static bool s_logged = false;
                        if (!s_logged)
                        {
                            Msg("! [DetailPass] Grass binding set failed (layout/SRV mismatch — check t50/t51)");
                            s_logged = true;
                        }
                        continue;
                    }

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
                auto* decalVsRefl = shaderLoader->GetCachedReflection("detail_decal", ".vs");
                auto* decalPsRefl = shaderLoader->GetCachedReflection("detail_decal", ".ps");
                framegraph::BindingSetBuilder decalBsb(*decalVsRefl, *decalPsRefl, nvDev, "Detail.Decal");
                decalBsb.ConstantBuffer("static_globals", staticGlobalsCB);
                decalBsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
                bindBindlessMaterialTables(decalBsb);
                decalBsb.BufferSRV("visible_indices", dm->visibleDecalInstancesBuffer);
                decalBsb.BufferSRV("detail_models", dm->detailModelsBuffer);
                decalBsb.BufferSRV("decal_vertices", dm->pulledVertexBuffer);
                decalBsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                decalBsb.BufferSRV("slot_data", dm->slotDataBuffer);
                decalBsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
                if (ClusteredLightManager::Instance().GetShadowDataBuffer())
                    decalBsb.BufferSRV("g_ShadowData", ClusteredLightManager::Instance().GetShadowDataBuffer());
                decalBsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                decalBsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                bindShadowMap(decalBsb);
                const auto& ibl = GetCurrentIBLBindResources();
                nvrhi::ITexture* sky0 = ibl.spec0 ? ibl.spec0 : cache.GetDummyCubeMap(nvDev);
                nvrhi::ITexture* sky1 = ibl.spec1 ? ibl.spec1 : sky0;
                if (sky0) decalBsb.Texture("s_env0", sky0);
                if (sky1) decalBsb.Texture("s_env1", sky1);
                BindIBLResources(decalBsb, ibl, nvDev);
                nvrhi::BindingSetHandle decalBindingSet = cache.GetOrCreateBindingSet(decalBsb.Build(), dm->decalBindingLayout, nvDev);

                if (decalBindingSet)
                {
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
            }

            if (data.gpuProfiler)
                data.gpuProfiler->EndPass(cmdList, "Details.Draw");
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.normal = passData.outputNormal;
    outputs.baseColor = passData.baseColor;
    outputs.worldPos = passData.worldPos;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::fg::passes
