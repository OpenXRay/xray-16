#include "stdafx.h"
#include "DepthPrepassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrCDB/Frustum.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg::passes
{
using namespace framegraph;
using namespace bindless;

struct DepthPrepassData {
    VirtualResourceHandle depth;
    fg::RenderDevice* device = nullptr;
    const GeometryCollector* geometry = nullptr;
    MaterialCache* materialCache = nullptr;
    DepthPrepassState* passState = nullptr;
    BindlessForwardConfig bindlessConfig;
    u32 width = 0;
    u32 height = 0;
};

static void InitializeDepthPrepass(fg::RenderDevice* device, DepthPrepassState& state)
{
    if (state.initialized)
        return;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_depth", "main");
    auto opaquePsResult = shaderLoader->LoadPixelShader("bindless_depth_opaque", "main");
    if (!vsResult.handle || !psResult.handle)
    {
        Msg("! [DepthPrepass] Failed to load bindless_forward/bindless_depth shaders");
        return;
    }
    state.vs = vsResult.handle;
    state.ps = psResult.handle;
    if (opaquePsResult.handle)
        state.terrainPs = opaquePsResult.handle;

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "DepthPrepass_v2_FwdVS", *vsResult.reflection, *psResult.reflection, nvDevice);
    if (!state.layout)
        return;

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.inputLayout = nvDevice->createInputLayout(attrs, attrCount, state.vs);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = state.inputLayout;
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = {state.layout, bindlessLayout};
    else
        pipeDesc.bindingLayouts = {state.layout};

    state.pipeline = cache.GetOrCreatePipeline("DepthPrepass_v2_FwdVS", pipeDesc, fbInfo, nvDevice);
    if (!state.pipeline)
    {
        Msg("! [DepthPrepass] Pipeline create failed");
        return;
    }

    // Opaque terrain: same VS, empty PS (no material SRV needed — layout still OK if unused)
    if (state.terrainPs && opaquePsResult.reflection)
    {
        state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "DepthPrepass_Terrain_v2_FwdVS", *vsResult.reflection, *opaquePsResult.reflection, nvDevice);
        if (state.terrainLayout)
        {
            nvrhi::GraphicsPipelineDesc terrainDesc = pipeDesc;
            terrainDesc.PS = state.terrainPs;
            if (bindlessLayout)
                terrainDesc.bindingLayouts = {state.terrainLayout, bindlessLayout};
            else
                terrainDesc.bindingLayouts = {state.terrainLayout};
            state.terrainPipeline = cache.GetOrCreatePipeline(
                "DepthPrepass_Terrain_v2_FwdVS", terrainDesc, fbInfo, nvDevice);
        }
    }

#if !defined(XR_PLATFORM_APPLE)
    {
        auto tessVs = shaderLoader->LoadVertexShader("bindless_forward", "main");
        auto tessHs = shaderLoader->LoadHullShader("bindless_tess", "main");
        auto tessDs = shaderLoader->LoadDomainShader("bindless_tess", "main");
        auto tessPs = shaderLoader->LoadPixelShader("bindless_depth_tess", "main");
        if (tessVs.handle && tessHs.handle && tessDs.handle && tessPs.handle &&
            tessVs.reflection && tessPs.reflection)
        {
            state.tessVs = tessVs.handle;
            state.tessHs = tessHs.handle;
            state.tessDs = tessDs.handle;
            state.tessPs = tessPs.handle;
            state.tessLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "DepthPrepass_Tess_v1", *tessVs.reflection, *tessPs.reflection, nvDevice);
            if (state.tessLayout)
            {
                state.tessInputLayout = nvDevice->createInputLayout(attrs, attrCount, state.tessVs);
                nvrhi::GraphicsPipelineDesc tessDesc = pipeDesc;
                tessDesc.VS = state.tessVs;
                tessDesc.HS = state.tessHs;
                tessDesc.DS = state.tessDs;
                tessDesc.PS = state.tessPs;
                tessDesc.inputLayout = state.tessInputLayout;
                tessDesc.primType = nvrhi::PrimitiveType::PatchList;
                tessDesc.patchControlPoints = 3;
                tessDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                if (bindlessLayout)
                    tessDesc.bindingLayouts = {state.tessLayout, bindlessLayout};
                else
                    tessDesc.bindingLayouts = {state.tessLayout};
                state.tessPipeline = cache.GetOrCreatePipeline(
                    "DepthPrepass_Tess_v1", tessDesc, fbInfo, nvDevice);
            }
            tessVs.reflection = nullptr;
            tessPs.reflection = nullptr;
        }
        if (state.tessPipeline)
            Msg("* [DepthPrepass] Tessellation pipeline ready");
        else
            Msg("! [DepthPrepass] Tessellation pipeline unavailable");
    }
#else
    Msg("* [DepthPrepass] Tessellation disabled on Apple/MoltenVK (IOGPU safety)");
#endif

    state.initialized = true;
    Msg("* [DepthPrepass] Pipeline ready (terrain=%d tess=%d)",
        state.terrainPipeline ? 1 : 0, state.tessPipeline ? 1 : 0);
}

VirtualResourceHandle setupDepthPrepass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const BindlessForwardConfig& bindlessConfig,
    DepthPrepassState* state,
    VirtualResourceHandle cullDrawArgs)
{
    if (!ps_r_depth_prepass || !state || !depthInput.is_valid())
        return depthInput;

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "DepthPrepass",
        [&, width, height, depthInput, bindlessConfig, state, cullDrawArgs](
            FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.write(depthInput, ResourceState::DepthStencilWrite);
            if (cullDrawArgs.is_valid())
                passBuilder.read(cullDrawArgs, ResourceState::IndirectArgument);
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.passState = state;
            data.bindlessConfig = bindlessConfig;
            data.width = width;
            data.height = height;
        },
        [](const DepthPrepassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            ZoneScoped;
            ZoneName("DepthPrepass", 12);

            if (!data.passState || !data.device)
                return;

            InitializeDepthPrepass(data.device, *data.passState);
            if (!data.passState->initialized || !data.passState->pipeline)
                return;

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
                return;

            auto* depthRT = fgGraph.GetPhysicalTexture(data.depth);
            if (!depthRT)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            if (!cmdList || !nvDevice)
                return;

            cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 1.0f, false, 0);

            if (data.materialCache)
                data.materialCache->FinalizePendingMaterials(ctx);

            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto& cache = GetPassResourceCache();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("DepthPrepass", nvDevice);
            if (!drawIndexBuffer)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("DepthPrepass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("bindless_depth", ".ps");
            if (!vsRefl || !psRefl)
                return;

            nvrhi::IBindingSet* bindlessTable = nullptr;
            if (auto* backend = data.device->GetBackend())
                bindlessTable = backend->GetBindlessDescriptorTable();

            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(data.width),
                0.0f, static_cast<float>(data.height),
                0.0f, 1.0f);

            auto drawSet = [&](const BindlessDrawSet& set) {
                if (!set.IsValid())
                    return;

                BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "DepthPrepass");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                BindBindlessMaterialTables(bsb);
                bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
                bsb.BufferSRV("g_CompactBatchIndices", set.compactBatchIndicesBuffer);
                bsb.BufferSRV("g_CompactMaterialIDs", set.compactMaterialIDBuffer);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::GraphicsState gs;
                gs.pipeline = data.passState->pipeline;
                gs.framebuffer = framebuffer;
                gs.bindings = {bindingSet};
                if (bindlessTable)
                    gs.addBindingSet(bindlessTable);
                gs.vertexBuffers = {
                    {data.bindlessConfig.megaVertexBuffer, 0, 0},
                    {drawIndexBuffer, 1, 0}};
                gs.indexBuffer = {data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                gs.viewport.addViewport(viewport);
                gs.viewport.addScissorRect(nvrhi::Rect(data.width, data.height));
                gs.indirectParams = set.compactDrawArgsBuffer;
                gs.indirectCountBuffer = set.compactCountBuffer;

                cmdList->setGraphicsState(gs);
                DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, set.totalObjectCount);
            };

            drawSet(data.bindlessConfig.staticSet);
            drawSet(data.bindlessConfig.dynamicSet);

#if !defined(XR_PLATFORM_APPLE)
            const bool tessEnabled = ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);
#else
            const bool tessEnabled = false;
#endif
            if (tessEnabled && data.passState->tessPipeline && data.passState->tessLayout &&
                data.bindlessConfig.tessObjectCount > 0 && data.bindlessConfig.tessDrawArgs &&
                data.bindlessConfig.tessObjects && data.bindlessConfig.tessMaterialIDBuffer &&
                data.bindlessConfig.tessBatchIndicesBuffer && data.bindlessConfig.tessInstanceBuffer)
            {
                auto* tessVsRefl = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
                auto* tessPsRefl = shaderLoader->GetCachedReflection("bindless_depth_tess", ".ps");
                if (tessVsRefl && tessPsRefl)
                {
                    BindingSetBuilder tessBsb(*tessVsRefl, *tessPsRefl, nvDevice, "DepthPrepass.Tess");
                    tessBsb.ConstantBuffer("static_globals", staticGlobalsCB);
                    BindBindlessMaterialTables(tessBsb);
                    tessBsb.BufferSRV("g_InstanceData", data.bindlessConfig.tessInstanceBuffer);
                    tessBsb.BufferSRV("g_CompactBatchIndices", data.bindlessConfig.tessBatchIndicesBuffer);
                    tessBsb.BufferSRV("g_CompactMaterialIDs", data.bindlessConfig.tessMaterialIDBuffer);
                    auto tessBindingSet = cache.GetOrCreateBindingSet(
                        tessBsb.Build(), data.passState->tessLayout, nvDevice);
                    if (tessBindingSet)
                    {
                        nvrhi::GraphicsState gs;
                        gs.pipeline = data.passState->tessPipeline;
                        gs.framebuffer = framebuffer;
                        gs.bindings = {tessBindingSet};
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.vertexBuffers = {
                            {data.bindlessConfig.megaVertexBuffer, 0, 0},
                            {drawIndexBuffer, 1, 0}};
                        gs.indexBuffer = {
                            data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                        gs.viewport.addViewport(viewport);
                        gs.viewport.addScissorRect(nvrhi::Rect(data.width, data.height));
                        cmdList->setGraphicsState(gs);

                        CFrustum tessFrustum;
                        tessFrustum.CreateFromMatrix(
                            Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

                        for (u32 i = 0; i < data.bindlessConfig.tessObjectCount; ++i)
                        {
                            const auto& args = data.bindlessConfig.tessDrawArgs[i];
                            if (args.indexCountPerInstance == 0)
                                continue;

                            const auto& obj = data.bindlessConfig.tessObjects[i];
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
                    }
                }
            }

            // Terrain fills most outdoor pixels — critical for early-Z
            if (data.passState->terrainPipeline && data.passState->terrainLayout &&
                data.bindlessConfig.HasTerrain() && data.bindlessConfig.UseTerrainCompaction())
            {
                if (data.materialCache)
                    data.materialCache->FinalizePendingTerrainMaterials(ctx);

                auto* opaquePsRefl = shaderLoader->GetCachedReflection("bindless_depth_opaque", ".ps");
                if (opaquePsRefl &&
                    data.bindlessConfig.terrainInstanceBuffer &&
                    data.bindlessConfig.terrainCompactBatchIndicesBuffer &&
                    data.bindlessConfig.terrainCompactMaterialIDBuffer &&
                    data.bindlessConfig.terrainCompactDrawArgsBuffer &&
                    data.bindlessConfig.terrainCompactCountBuffer)
                {
                    BindingSetBuilder tbsb(*vsRefl, *opaquePsRefl, nvDevice, "DepthPrepass.Terrain");
                    tbsb.ConstantBuffer("static_globals", staticGlobalsCB);
                    BindBindlessMaterialTables(tbsb);
                    tbsb.BufferSRV("g_InstanceData", data.bindlessConfig.terrainInstanceBuffer);
                    tbsb.BufferSRV("g_CompactBatchIndices", data.bindlessConfig.terrainCompactBatchIndicesBuffer);
                    tbsb.BufferSRV("g_CompactMaterialIDs", data.bindlessConfig.terrainCompactMaterialIDBuffer);
                    auto terrainSet = cache.GetOrCreateBindingSet(
                        tbsb.Build(), data.passState->terrainLayout, nvDevice);
                    if (terrainSet)
                    {
                        nvrhi::GraphicsState gs;
                        gs.pipeline = data.passState->terrainPipeline;
                        gs.framebuffer = framebuffer;
                        gs.bindings = {terrainSet};
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.vertexBuffers = {
                            {data.bindlessConfig.megaVertexBuffer, 0, 0},
                            {drawIndexBuffer, 1, 0}};
                        gs.indexBuffer = {
                            data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                        gs.viewport.addViewport(viewport);
                        gs.viewport.addScissorRect(nvrhi::Rect(data.width, data.height));
                        gs.indirectParams = data.bindlessConfig.terrainCompactDrawArgsBuffer;
                        gs.indirectCountBuffer = data.bindlessConfig.terrainCompactCountBuffer;
                        cmdList->setGraphicsState(gs);
                        DrawIndexedIndirectCountOrFallback(
                            cmdList, 0, 0, data.bindlessConfig.terrainObjectCount);
                    }
                }
            }
        });

    return passData.depth;
}

} // namespace xray::render::fg::passes
