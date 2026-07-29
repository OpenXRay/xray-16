// xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "IBLPrefilterPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"  // For loading bindless shaders
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"  // For IndirectDrawArgs struct
#include "Layers/xrRender/Backend/D3D12Backend.h"  // For SM6 bindless descriptor heap
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"  // For terrain rendering
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"  // For variant textures
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "PassCommon.h"
#include "xrCDB/Frustum.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrCore/FMesh.hpp"

namespace xray::render::fg
{
    extern float r__dtex_range;
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

void InitializeForwardResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ForwardColorPassState& state)
{
    if (state.bindlessInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

    if (!vsResult.handle || !psResult.handle) {
        Msg("! [BindlessForward] Failed to load shaders");
        return;
    }

    state.bindlessVS = vsResult.handle;
    state.bindlessPS = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();

    state.bindlessLayout = cache.GetOrCreateBindingLayoutFromReflection("ForwardColor_PBR_v5_DynHemi", *vsResult.reflection, *psResult.reflection, nvDevice);

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.bindlessInputLayout = nvDevice->createInputLayout(attrs, attrCount, state.bindlessVS);

    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);
    if (!drawIndexBuffer) {
        Msg("! [BindlessForward] Failed to create draw index buffer");
        return;
    }

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.bindlessVS;
    pipeDesc.PS = state.bindlessPS;
    pipeDesc.inputLayout = state.bindlessInputLayout;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    if (bindlessLayout) {
        pipeDesc.bindingLayouts = { state.bindlessLayout, bindlessLayout };
    } else {
        pipeDesc.bindingLayouts = { state.bindlessLayout };
    }

    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    state.bindlessPipeline = cache.GetOrCreatePipeline("ForwardColor_PBR_v5_DynHemi", pipeDesc, fbInfo, nvDevice);
    if (!state.bindlessPipeline) {
        Msg("! [BindlessForward] Failed to create pipeline");
        return;
    }

    {
        nvrhi::GraphicsPipelineDesc equalDesc = pipeDesc;
        equalDesc.renderState.depthStencilState.depthWriteEnable = false;
        equalDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        state.bindlessEqualPipeline = cache.GetOrCreatePipeline(
            "ForwardColor_PBR_AfterDepth_v1", equalDesc, fbInfo, nvDevice);
    }

    QueryBindingLayoutFromPipeline(state.bindlessPipeline, state.bindlessLayout);

    // Optional tessellation PSO (PatchList).
    // Apple/MoltenVK: do not create — Metal tess temp buffers can kernel-panic.
#if !defined(XR_PLATFORM_APPLE)
    {
        auto hsResult = shaderLoader->LoadHullShader("bindless_tess", "main");
        auto dsResult = shaderLoader->LoadDomainShader("bindless_tess", "main");
        if (hsResult.handle && dsResult.handle) {
            state.bindlessHS = hsResult.handle;
            state.bindlessDS = dsResult.handle;

            nvrhi::GraphicsPipelineDesc tessDesc = pipeDesc;
            tessDesc.HS = state.bindlessHS;
            tessDesc.DS = state.bindlessDS;
            tessDesc.primType = nvrhi::PrimitiveType::PatchList;
            tessDesc.patchControlPoints = 3;
            tessDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
            state.bindlessTessPipeline = cache.GetOrCreatePipeline(
                "ForwardColor_Tess_v5", tessDesc, fbInfo, nvDevice);
            if (state.bindlessTessPipeline)
                Msg("* [BindlessForward] Tessellation pipeline ready");
            else
                Msg("! [BindlessForward] Tessellation pipeline create failed (VkResult logged by NVRHI)");
        } else {
            Msg("! [BindlessForward] Tess HS/DS shaders missing — tessellation disabled");
        }
    }
#else
    Msg("* [BindlessForward] Tessellation disabled on Apple/MoltenVK (IOGPU safety)");
#endif

    auto terrainPsResult = shaderLoader->LoadPixelShader("bindless_terrain", "main");
    if (terrainPsResult.handle) {
        state.terrainPS = terrainPsResult.handle;
        state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "ForwardColor_Terrain_PBR_v4_NoLmapRaster", *vsResult.reflection, *terrainPsResult.reflection, nvDevice);

        if (state.terrainLayout) {
            nvrhi::GraphicsPipelineDesc terrainPipeDesc;
            terrainPipeDesc.VS = state.bindlessVS;
            terrainPipeDesc.PS = state.terrainPS;
            terrainPipeDesc.inputLayout = state.bindlessInputLayout;
            if (bindlessLayout)
                terrainPipeDesc.bindingLayouts = { state.terrainLayout, bindlessLayout };
            else
                terrainPipeDesc.bindingLayouts = { state.terrainLayout };
            terrainPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            terrainPipeDesc.renderState.depthStencilState.depthTestEnable = true;
            terrainPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            terrainPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
            terrainPipeDesc.renderState.rasterState.frontCounterClockwise = false;
            terrainPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
            state.terrainPipeline = cache.GetOrCreatePipeline("ForwardColor_Terrain_PBR_v4_NoLmapRaster", terrainPipeDesc, fbInfo, nvDevice);
            if (state.terrainPipeline)
            {
                nvrhi::GraphicsPipelineDesc terrainEqual = terrainPipeDesc;
                terrainEqual.renderState.depthStencilState.depthWriteEnable = false;
                terrainEqual.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                state.terrainEqualPipeline = cache.GetOrCreatePipeline(
                    "ForwardColor_Terrain_AfterDepth_v1", terrainEqual, fbInfo, nvDevice);
                state.terrainInitialized = true;
            }
        }
    }

    state.bindlessInitialized = true;
    Msg("* [BindlessForward] Pipeline initialized");
}

static void renderBindlessForward(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* normalRT,
    nvrhi::ITexture* baseColorRT,
    nvrhi::ITexture* worldPosRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache,
    ForwardColorPassState& ps)
{
    using namespace fg::bindless;

    if (!config.UseGPUCulling() || !geometry || geometry->GetBatches().empty())
        return;

    // Finalize any pending materials (register textures to bindless descriptor heap)
    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
        materialCache->FinalizePendingTerrainMaterials(ctx);  // Terrain materials (4-layer detail blending)
    }

    // Upload material buffer to GPU
    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList)
        return;

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAMEBUFFER AND RENDER STATE
    // ═══════════════════════════════════════════════════════
    if (!normalRT || !baseColorRT || !worldPosRT)
        return;

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.addColorAttachment(normalRT);
    fbDesc.addColorAttachment(baseColorRT);
    fbDesc.addColorAttachment(worldPosRT);
    fbDesc.setDepthAttachment(depthRT);
    auto& cache = framegraph::GetPassResourceCache();
    auto framebuffer = cache.GetOrCreateFramebuffer("ForwardColor", fbDesc, nvDevice);

    auto lightingCB = cache.GetOrCreateVolatileCB("ForwardColor", "LightingCB", sizeof(LightingConstants), device);
    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), device);
    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);

    {
        StaticGlobals sg = BuildStaticGlobals();
        cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
    }

    auto transparentDrawCB = cache.GetOrCreateVolatileCB("Frame", "TransparentDrawCB", 16, device);
    {
        u32 passMode[4] = { 0, 0, 0, 0 };
        cmdList->writeBuffer(transparentDrawCB, passMode, sizeof(passMode));
    }

    auto lightingData = FillLightingConstants();
    cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

    auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
    auto* psReflection = shaderLoader->GetCachedReflection("bindless_forward", ".ps");

    auto& clm = ClusteredLightManager::Instance();
    auto& passCache = framegraph::GetPassResourceCache();

    nvrhi::ITexture* shadowTex = config.shadowCascades[0]
        ? config.shadowCascades[0]
        : (config.shadowMapArray ? config.shadowMapArray : nullptr);
    if (!shadowTex)
        shadowTex = passCache.GetDummyShadowMap2D(nvDevice);
    nvrhi::ITexture* sky0 = config.envSky0
        ? config.envSky0
        : passCache.GetDummyCubeMap(nvDevice);
    nvrhi::ITexture* sky1 = config.envSky1
        ? config.envSky1
        : passCache.GetDummyCubeMap(nvDevice);

    auto bindShadowMap = [&](framegraph::BindingSetBuilder& bsb) {
        nvrhi::ITexture* dummy2D = passCache.GetDummyShadowMap2D(nvDevice);
        static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
        for (u32 i = 0; i < 3; ++i)
        {
            nvrhi::ITexture* t = (config.shadowCascades[i]) ? config.shadowCascades[i]
                : (i == 0 && shadowTex ? shadowTex : dummy2D);
            if (t)
                bsb.Texture(kNames[i], t);
        }
        // Bind by name — unused g_ContactDepth must not be forced into the set
        nvrhi::ITexture* contactHist = config.contactHistory
            ? config.contactHistory
            : passCache.GetDummyContactHistory(nvDevice);
        if (contactHist)
            bsb.Texture("g_ContactHistory", contactHist);
        nvrhi::ITexture* localAtlas = config.localShadowAtlas
            ? config.localShadowAtlas
            : passCache.GetDummyShadowMap(nvDevice);
        if (localAtlas)
            bsb.Texture("g_LocalShadowAtlas", localAtlas);
        nvrhi::ITexture* localEsm = config.localShadowESM
            ? config.localShadowESM
            : passCache.GetDummyLocalShadowESM(nvDevice);
        if (localEsm)
            bsb.Texture("g_LocalShadowESM", localEsm);
        static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
        nvrhi::ITexture* dummyHzb = passCache.GetDummyContactDepth(nvDevice);
        for (u32 i = 0; i < 3; ++i)
        {
            nvrhi::ITexture* hz = config.shadowHZB[i] ? config.shadowHZB[i] : dummyHzb;
            if (hz)
                bsb.Texture(kHzb[i], hz);
        }
    };

    auto bindShadowMask = [&](framegraph::BindingSetBuilder& bsb) {
        nvrhi::ITexture* shadowMask = config.shadowMask
            ? config.shadowMask
            : passCache.GetDummyContactHistory(nvDevice);
        if (shadowMask)
            bsb.Texture("g_ShadowMask", shadowMask);
    };

    auto bindSkyCubes = [&](framegraph::BindingSetBuilder& bsb) {
        if (sky0)
            bsb.Texture("s_env0", sky0);
        if (sky1)
            bsb.Texture("s_env1", sky1);
        IBLBindResources ibl{};
        ibl.brdfLut = config.envBrdfLut;
        ibl.shBuffer = config.envSkySH;
        ibl.probeBuffer = config.envProbes;
        ibl.probeCubeArray = config.envProbeCubes;
        BindIBLResources(bsb, ibl, nvDevice);
    };

    auto createBindingSetForSet = [&](const BindlessDrawSet& set) -> nvrhi::BindingSetHandle {
        framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "ForwardColor");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
        bsb.ConstantBuffer("TransparentDrawCB", transparentDrawCB);
        BindBindlessMaterialTables(bsb);
        bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
        bsb.BufferSRV("g_CompactBatchIndices", set.compactBatchIndicesBuffer);
        bsb.BufferSRV("g_CompactMaterialIDs", set.compactMaterialIDBuffer);
        bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
        if (clm.GetShadowDataBuffer())
            bsb.BufferSRV("g_ShadowData", clm.GetShadowDataBuffer());
        bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
        bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
        bindShadowMap(bsb);
        bindShadowMask(bsb);
        bindSkyCubes(bsb);

        auto bs = passCache.GetOrCreateBindingSet(bsb.Build(), ps.bindlessLayout, nvDevice);
        if (!bs)
            Msg("! [ForwardColor] Binding set create failed (missing CSM/sky?)");
        return bs;
    };

    // ═══════════════════════════════════════════════════════
    //  RENDER VISIBLE BATCHES
    // ═══════════════════════════════════════════════════════
    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    // ═══════════════════════════════════════════════════════
    //  GPU-DRIVEN CULLED RENDERING
    // ═══════════════════════════════════════════════════════
    // Only visible batches are drawn using indirect draw commands
    // Draw args come from GPU culling's compact buffer
    if (!config.UseGPUCulling() || !config.UseMegaBuffers()) {
        // GPU culling not available - skip bindless forward
        // (regular deferred path will handle rendering)
        return;
    }

    // Validate all required resources before draw loop
    if (!ps.bindlessPipeline || !framebuffer) {
        return;
    }

    nvrhi::IGraphicsPipeline* opaquePipe = ps.bindlessPipeline.Get();
    if (config.useEqualDepth && ps.bindlessEqualPipeline)
        opaquePipe = ps.bindlessEqualPipeline.Get();

    nvrhi::GraphicsState state;
    state.pipeline = opaquePipe;
    state.framebuffer = framebuffer;

    // SM6.6 bindless: Add the descriptor table from D3D12 backend
    // IDescriptorTable derives from IBindingSet, so we add it to bindings
    auto* backend = device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = nullptr;
    if (backend) {
        bindlessTable = backend->GetBindlessDescriptorTable();
    }

    // Validate draw index buffer exists
    if (!drawIndexBuffer) {
        Msg("! [BindlessForward] Draw index buffer not initialized!");
        return;
    }

    state.vertexBuffers = {
        {config.megaVertexBuffer, 0, 0},    // Slot 0: Per-vertex geometry (stride 48)
        {drawIndexBuffer, 1, 0}           // Slot 1: Per-instance draw indices (stride 4)
    };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    auto drawSet = [&](const BindlessDrawSet& set) {
        if (!set.IsValid())
            return;

        auto bindingSet = createBindingSetForSet(set);
        if (!bindingSet)
        {
            Msg("! [ForwardColor] Skipping draw — binding set is null");
            return;
        }

        state.bindings = { bindingSet };
        if (bindlessTable) {
            state.addBindingSet(bindlessTable);
        }
        state.indirectParams = set.compactDrawArgsBuffer;
        state.indirectCountBuffer = set.compactCountBuffer;

        cmdList->setGraphicsState(state);
        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, set.totalObjectCount);
    };

    if (config.variantPartition.Enabled()) {
        auto* backendDev = device->GetBackend();
        VariantPartitionDrawConfig vpCfg;
        vpCfg.defaultPipeline = opaquePipe;
        vpCfg.inputLayout = ps.bindlessInputLayout;
        vpCfg.passLayout = ps.bindlessLayout;
        vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
        vpCfg.bindlessTable = bindlessTable;
        vpCfg.sampler = ps.linearSampler;
        vpCfg.staticGlobalsCB = staticGlobalsCB;
        vpCfg.lightingCB = lightingCB;
        vpCfg.materialBuffer = matBuffer.GetBuffer();
        vpCfg.variantTexBuffer = variantTexBuffer.GetBuffer();
        vpCfg.instanceBuffer = config.staticSet.instanceBuffer;
        vpCfg.megaVertexBuffer = config.megaVertexBuffer;
        vpCfg.shadowMapArray = shadowTex;
        for (u32 i = 0; i < 3; ++i)
        {
            vpCfg.shadowCascades[i] = config.shadowCascades[i];
            vpCfg.shadowHZB[i] = config.shadowHZB[i];
        }
        vpCfg.localShadowAtlas = config.localShadowAtlas;
        vpCfg.localShadowESM = config.localShadowESM;
        vpCfg.contactHistory = config.contactHistory;
        vpCfg.shadowMask = config.shadowMask;
        vpCfg.envSky0 = sky0;
        vpCfg.envSky1 = sky1;
        {
            auto& clmVp = ClusteredLightManager::Instance();
            vpCfg.lightDataBuffer = clmVp.GetLightDataBuffer();
            vpCfg.shadowDataBuffer = clmVp.GetShadowDataBuffer();
            vpCfg.clusterGridBuffer = clmVp.GetClusterGridBuffer();
            vpCfg.lightIndexListBuffer = clmVp.GetLightIndexListBuffer();
        }
        vpCfg.partition = config.variantPartition;
        vpCfg.selectTransparent = false;

        DrawVariantPartition(cmdList, nvDevice, framebuffer, state, vpCfg);

        state.pipeline = opaquePipe;
        state.vertexBuffers = {
            {config.megaVertexBuffer, 0, 0},
            {drawIndexBuffer, 1, 0}
        };
    } else {
        drawSet(config.staticSet);
    }
    drawSet(config.dynamicSet);

            // Tessellation: PatchList PSO + direct drawIndexed (CPU frustum).
            // Avoid DrawIndexedIndirect+tess — MoltenVK/Vulkan freezes on some camera angles.
            // Apple/MoltenVK: hard-disabled — Metal tess temp buffers can kernel-panic
            // (IOGPUGroupMemory::remove_memory_object) regardless of the console flag.
#if defined(XR_PLATFORM_APPLE)
            const bool tessEnabled = false;
#else
            const bool tessEnabled = ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);
#endif
            if (tessEnabled && ps.bindlessTessPipeline &&
                config.tessObjectCount > 0 && config.tessDrawArgs && config.tessObjects &&
                config.tessMaterialIDBuffer && config.tessBatchIndicesBuffer && config.tessInstanceBuffer)
            {
                framegraph::BindingSetBuilder tessBsb(*vsReflection, *psReflection, nvDevice, "ForwardColor.Tess");
                tessBsb.ConstantBuffer("static_globals", staticGlobalsCB);
                tessBsb.ConstantBuffer("TransparentDrawCB", transparentDrawCB);
                BindBindlessMaterialTables(tessBsb);
                tessBsb.BufferSRV("g_InstanceData", config.tessInstanceBuffer);
                tessBsb.BufferSRV("g_CompactBatchIndices", config.tessBatchIndicesBuffer);
                tessBsb.BufferSRV("g_CompactMaterialIDs", config.tessMaterialIDBuffer);
                tessBsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
                if (clm.GetShadowDataBuffer())
                    tessBsb.BufferSRV("g_ShadowData", clm.GetShadowDataBuffer());
                tessBsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
                tessBsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
                bindShadowMap(tessBsb);
                bindShadowMask(tessBsb);
                bindSkyCubes(tessBsb);

                auto tessBindingSet = passCache.GetOrCreateBindingSet(tessBsb.Build(), ps.bindlessLayout, nvDevice);
                if (tessBindingSet)
                {
                    state.pipeline = ps.bindlessTessPipeline;
                    state.bindings = { tessBindingSet };
                    if (bindlessTable)
                        state.addBindingSet(bindlessTable);
                    state.indirectParams = nullptr;
                    state.indirectCountBuffer = nullptr;
                    cmdList->setGraphicsState(state);

                    // Soft frustum cull: PatchList amplification is expensive and has
                    // caused view-angle freezes; skip batches clearly outside the view.
                    CFrustum tessFrustum;
                    tessFrustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

                    for (u32 i = 0; i < config.tessObjectCount; ++i)
                    {
                        const auto& args = config.tessDrawArgs[i];
                        if (args.indexCountPerInstance == 0)
                            continue;

                        const auto& obj = config.tessObjects[i];
                        if (!tessFrustum.testSphere_dirty(obj.position, obj.radius * 1.15f))
                            continue;

                        nvrhi::DrawArguments da;
                        da.vertexCount = args.indexCountPerInstance;
                        da.instanceCount = 1;
                        da.startIndexLocation = args.startIndexLocation;
                        da.startVertexLocation = args.baseVertexLocation;
                        da.startInstanceLocation = i;
                        cmdList->drawIndexed(da);
                    }

                    state.pipeline = opaquePipe;
                }
                else
                {
                    Msg("! [ForwardColor] Tess binding set create failed — skip tess draws");
                }
            }

    // ═══════════════════════════════════════════════════════
    //  TERRAIN RENDERING (4-layer detail blending)
    // ═══════════════════════════════════════════════════════
    // Terrain uses separate pipeline and TerrainMaterialBuffer
    if (config.HasTerrain() && config.UseMegaBuffers()) {
        R_ASSERT2(config.UseTerrainCompaction(), "Terrain compaction buffers missing");

        if (ps.terrainInitialized && ps.terrainPipeline) {
            // Upload terrain materials
            auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
            terrainMatBuffer.Upload(ctx);

            // Validate all required terrain-specific buffers are available
            R_ASSERT2(terrainMatBuffer.GetBuffer() && config.terrainInstanceBuffer &&
                          config.terrainCompactBatchIndicesBuffer && config.terrainCompactMaterialIDBuffer &&
                          config.terrainCompactDrawArgsBuffer && config.terrainCompactCountBuffer,
                "Terrain buffers not ready for compaction rendering");

            // Terrain PS includes bindless_common.h → layout needs t8/t9/t10 even if
            // only g_TerrainMaterials is sampled (same issue as DetailPass).
            auto* terrainVsRefl = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* terrainPsRefl = shaderLoader->GetCachedReflection("bindless_terrain", ".ps");
            framegraph::BindingSetBuilder terrainBsb(*terrainVsRefl, *terrainPsRefl, nvDevice, "ForwardColor.Terrain");
            terrainBsb.ConstantBuffer("static_globals", staticGlobalsCB);
            BindBindlessMaterialTables(terrainBsb);
            terrainBsb.BufferSRV("g_InstanceData", config.terrainInstanceBuffer);
            terrainBsb.BufferSRV("g_CompactBatchIndices", config.terrainCompactBatchIndicesBuffer);
            terrainBsb.BufferSRV("g_CompactMaterialIDs", config.terrainCompactMaterialIDBuffer);
            terrainBsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            if (clm.GetShadowDataBuffer())
                terrainBsb.BufferSRV("g_ShadowData", clm.GetShadowDataBuffer());
            terrainBsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            terrainBsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
            bindShadowMap(terrainBsb);
            bindSkyCubes(terrainBsb);

            auto terrainBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainBsb.Build(), ps.terrainLayout, nvDevice);
            if (!terrainBindingSet)
            {
                Msg("! [ForwardColor] Terrain binding set create failed");
            }
            else
            {
                nvrhi::IGraphicsPipeline* terrainPipe = ps.terrainPipeline.Get();
                if (config.useEqualDepth && ps.terrainEqualPipeline)
                    terrainPipe = ps.terrainEqualPipeline.Get();

                nvrhi::GraphicsState terrainState;
                terrainState.pipeline = terrainPipe;
                terrainState.framebuffer = framebuffer;
                terrainState.bindings = { terrainBindingSet };

                if (backend) {
                    auto* bindlessTable = backend->GetBindlessDescriptorTable();
                    if (bindlessTable)
                        terrainState.addBindingSet(bindlessTable);
                }

                terrainState.vertexBuffers = {
                    {config.megaVertexBuffer, 0, 0},
                    {drawIndexBuffer, 1, 0}
                };
                terrainState.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
                terrainState.indirectParams = config.terrainCompactDrawArgsBuffer;
                terrainState.indirectCountBuffer = config.terrainCompactCountBuffer;
                terrainState.viewport.addViewport(viewport);
                terrainState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

                cmdList->setGraphicsState(terrainState);
                DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, config.terrainObjectCount);
            }
        }
    }

    // Transparent geometry is rendered in a separate TransparentPass (after Detail pass)
    // Skinned meshes are rendered in the SkinningPass (see SkinningPassSetup.cpp)
}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle normalInput,
    framegraph::VirtualResourceHandle baseColorInput,
    framegraph::VirtualResourceHandle worldPosInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsInput,
    const BindlessForwardConfig& bindlessConfig,
    ForwardColorPassState* state)
{
    using namespace framegraph;

    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeForwardResources(device, fbInfo, *state);
    }

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height, colorInput, normalInput, baseColorInput, worldPosInput, drawArgsInput, bindlessConfig, state](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;
            data.passState = state;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);
            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
            data.normal = passBuilder.write(normalInput, ResourceState::RenderTarget);
            data.baseColor = passBuilder.write(baseColorInput, ResourceState::RenderTarget);
            data.worldPos = passBuilder.write(worldPosInput, ResourceState::RenderTarget);

            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            if (bindlessConfig.shadowMapHandle.is_valid()) {
                passBuilder.read(bindlessConfig.shadowMapHandle, ResourceState::ShaderResource);
            }
            if (bindlessConfig.localShadowHandle.is_valid()) {
                passBuilder.read(bindlessConfig.localShadowHandle, ResourceState::ShaderResource);
            }
            for (u32 i = 0; i < 3; ++i) {
                if (bindlessConfig.shadowHZBHandles[i].is_valid())
                    passBuilder.read(bindlessConfig.shadowHZBHandles[i], ResourceState::ShaderResource);
            }
            if (bindlessConfig.shadowMaskHandle.is_valid()) {
                passBuilder.read(bindlessConfig.shadowMaskHandle, ResourceState::ShaderResource);
            }

            data.outputs.albedo = data.color;
            data.outputs.normal = data.normal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.worldPos = data.worldPos;
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA (Renders geometry)
        // ═══════════════════════════════════════════════════════
        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            fg::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;

            if (!depthRT || !colorRT || !normalRT || !baseColorRT || !worldPosRT)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (cmdList) {
                // DepthPrepass already cleared+filled depth — clearing here would kill early-Z.
                if (!ps_r_depth_prepass)
                    cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 0.0f, false, 0);
                if (normalRT)
                    cmdList->clearTextureFloat(normalRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
                if (baseColorRT)
                    cmdList->clearTextureFloat(baseColorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
                if (worldPosRT)
                    cmdList->clearTextureFloat(worldPosRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            }

            // Check if we have geometry to render
            if (!data.geometry)
                return;

            // Get draw args buffer through framegraph (proper dependency tracking)
            // This ensures state transition UAV -> IndirectArgument happened
            nvrhi::IBuffer* drawArgsBuffer = nullptr;
            if (data.drawArgsBuffer.is_valid()) {
                drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
            }

            // ═══════════════════════════════════════════════════════
            //  BINDLESS RENDERING PATH (GPU-DRIVEN MULTI-DRAW)
            // ═══════════════════════════════════════════════════════
            // Render static geometry with GPU-driven multi-draw
            // NOTE: Skinned meshes are rendered in SkinningPass (see SkinningPassSetup.cpp)
            renderBindlessForward(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                normalRT,
                baseColorRT,
                worldPosRT,
                depthRT,
                data.bindlessConfig,
                data.materialCache,
                *data.passState
            );
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.worldPos = passData.worldPos;
    outputs.depth = passData.depth;
    return outputs;
}
} // namespace xray::render::fg::passes
