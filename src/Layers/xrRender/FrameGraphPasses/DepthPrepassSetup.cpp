// xrRender/FrameGraphPasses/DepthPrepassSetup.cpp
#include "stdafx.h"
#include "DepthPrepassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "PassCommon.h"

namespace xray::render::fg::passes {

using namespace framegraph;

static void InitializeDepthPrepassResources(fg::RenderDevice* device, DepthPrepassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_depth_at", "main");
    auto psOpaqueResult = shaderLoader->LoadPixelShader("bindless_depth_opaque", "main");

    if (!vsResult.handle || !psResult.handle || !psOpaqueResult.handle) {
        Msg("! [DepthPrepass] Failed to load shaders");
        return;
    }

    state.vs = vsResult.handle;
    state.ps = psResult.handle;
    state.psOpaque = psOpaqueResult.handle;

    auto& cache = framegraph::GetPassResourceCache();

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("DepthPrepass", *vsResult.reflection, *psResult.reflection, nvDevice);
    state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection("DepthPrepass_Terrain", *vsResult.reflection, *psOpaqueResult.reflection, nvDevice);

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.inputLayout = nvDevice->createInputLayout(attrs, attrCount, state.vs);

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = state.inputLayout;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { state.layout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { state.layout };
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    state.pipeline = cache.GetOrCreatePipeline("DepthPrepass", pipeDesc, fbInfo, nvDevice);
    if (!state.pipeline) {
        Msg("! [DepthPrepass] Failed to create pipeline");
        return;
    }

    if (state.terrainLayout) {
        nvrhi::GraphicsPipelineDesc terrainPipeDesc;
        terrainPipeDesc.VS = state.vs;
        terrainPipeDesc.PS = state.psOpaque;
        terrainPipeDesc.inputLayout = state.inputLayout;
        terrainPipeDesc.bindingLayouts = { state.terrainLayout };
        terrainPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        terrainPipeDesc.renderState.depthStencilState.depthTestEnable = true;
        terrainPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
        terrainPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        terrainPipeDesc.renderState.rasterState.frontCounterClockwise = false;
        terrainPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

        state.terrainPipeline = cache.GetOrCreatePipeline("DepthPrepass_Terrain", terrainPipeDesc, fbInfo, nvDevice);
    }

    auto clusterVsResult = shaderLoader->LoadVertexShader("cluster_pull", "main");
    if (clusterVsResult.handle) {
        state.clusterVS = clusterVsResult.handle;
        state.clusterLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "DepthPrepass_Cluster", *clusterVsResult.reflection, *psResult.reflection, nvDevice);

        if (state.clusterLayout) {
            nvrhi::GraphicsPipelineDesc clusterPipeDesc;
            clusterPipeDesc.VS = state.clusterVS;
            clusterPipeDesc.PS = state.ps;
            clusterPipeDesc.inputLayout = nullptr;
            if (bindlessLayout)
                clusterPipeDesc.bindingLayouts = { state.clusterLayout, bindlessLayout };
            else
                clusterPipeDesc.bindingLayouts = { state.clusterLayout };
            clusterPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            clusterPipeDesc.renderState.depthStencilState.depthTestEnable = true;
            clusterPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            clusterPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
            clusterPipeDesc.renderState.rasterState.frontCounterClockwise = false;
            clusterPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            state.clusterPipeline = cache.GetOrCreatePipeline("DepthPrepass_Cluster", clusterPipeDesc, fbInfo, nvDevice);
        }
    }

    state.initialized = true;
    Msg("* [DepthPrepass] Pipeline initialized");
}

static void renderDepthPrepass(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache,
    DepthPrepassState& ps)
{
    using namespace fg::bindless;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList || !depthRT)
        return;

    cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 0.0f, false, 0);

    if (!config.UseGPUCulling() || !config.UseMegaBuffers())
        return;

    if (!ps.pipeline)
        return;

    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
        materialCache->FinalizePendingTerrainMaterials(ctx);
    }

    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = cache.GetOrCreateFramebuffer("DepthPrepass", fbDesc, nvDevice);
    if (!framebuffer)
        return;

    auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), device);
    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ForwardColor", nvDevice);
    if (!drawIndexBuffer)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
    auto* psReflection = shaderLoader->GetCachedReflection("bindless_depth_at", ".ps");
    if (!vsReflection || !psReflection)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

    const auto& rtDesc = depthRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    nvrhi::GraphicsState state;
    state.pipeline = ps.pipeline;
    state.framebuffer = framebuffer;
    state.vertexBuffers = {
        {config.megaVertexBuffer, 0, 0},
        {drawIndexBuffer, 1, 0}
    };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    auto drawSet = [&](const BindlessDrawSet& set, nvrhi::IBuffer* indexStream) {
        if (!set.IsValid())
            return;

        framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "DepthPrepass");
        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
        bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
        bsb.BufferSRV("g_CompactBatchIndices", set.compactBatchIndicesBuffer);
        bsb.BufferSRV("g_CompactMaterialIDs", set.compactMaterialIDBuffer);
        bsb.BufferSRV("g_DrawFades", set.fadeBuffer);

        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps.layout, nvDevice);
        R_ASSERT2(bindingSet, "Depth prepass binding set creation failed");

        state.vertexBuffers = {
            {config.megaVertexBuffer, 0, 0},
            {indexStream, 1, 0}
        };
        state.bindings = { bindingSet };
        if (bindlessTable)
            state.addBindingSet(bindlessTable);
        state.indirectParams = set.compactDrawArgsBuffer;
        state.indirectCountBuffer = set.compactCountBuffer;

        cmdList->setGraphicsState(state);
        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, set.totalObjectCount);
    };

    drawSet(config.staticSet, drawIndexBuffer);
    drawSet(config.dynamicSet, drawIndexBuffer);

    if (config.cluster.IsValid() && ps.clusterPipeline && ps.clusterLayout) {
        auto* clusterVsRefl = shaderLoader->GetCachedReflection("cluster_pull", ".vs");
        if (clusterVsRefl) {
            framegraph::BindingSetBuilder cbsb(*clusterVsRefl, *psReflection, nvDevice, "DepthPrepass.Cluster");
            cbsb.ConstantBuffer("static_globals", staticGlobalsCB);
            cbsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
            cbsb.BufferSRV("g_InstanceData", config.cluster.instanceBuffer);
            cbsb.BufferSRV("g_VisibleEntries", config.cluster.visibleEntryBuffer);
            cbsb.BufferSRV("g_Entries", config.cluster.entryBuffer);
            cbsb.BufferSRV("g_MegaVB", config.megaVertexBuffer);
            cbsb.BufferSRV("g_MegaIB", config.megaIndexBuffer);
            cbsb.BufferSRV("g_DrawFades", config.cluster.fadeBuffer);

            auto clusterBindingSet = cache.GetOrCreateBindingSet(cbsb.Build(), ps.clusterLayout, nvDevice);
            if (clusterBindingSet) {
                nvrhi::GraphicsState clusterState;
                clusterState.pipeline = ps.clusterPipeline;
                clusterState.framebuffer = framebuffer;
                clusterState.bindings = { clusterBindingSet };
                if (bindlessTable)
                    clusterState.addBindingSet(bindlessTable);
                clusterState.indirectParams = config.cluster.argsBuffer;
                clusterState.viewport.addViewport(viewport);
                clusterState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

                cmdList->setGraphicsState(clusterState);
                cmdList->drawIndirect(0, 1);
            }
        }
    }

    if (config.HasTerrain() && config.UseTerrainCompaction() && ps.terrainPipeline) {
        auto* psOpaqueReflection = shaderLoader->GetCachedReflection("bindless_depth_opaque", ".ps");
        if (!psOpaqueReflection)
            return;

        framegraph::BindingSetBuilder terrainBsb(*vsReflection, *psOpaqueReflection, nvDevice, "DepthPrepass.Terrain");
        terrainBsb.ConstantBuffer("static_globals", staticGlobalsCB);
        terrainBsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
        terrainBsb.BufferSRV("g_InstanceData", config.terrainInstanceBuffer);
        terrainBsb.BufferSRV("g_CompactBatchIndices", config.terrainCompactBatchIndicesBuffer);
        terrainBsb.BufferSRV("g_CompactMaterialIDs", config.terrainCompactMaterialIDBuffer);

        auto terrainBindingSet = cache.GetOrCreateBindingSet(terrainBsb.Build(), ps.terrainLayout, nvDevice);
        R_ASSERT2(terrainBindingSet, "Depth prepass terrain binding set creation failed");

        nvrhi::GraphicsState terrainState;
        terrainState.pipeline = ps.terrainPipeline;
        terrainState.framebuffer = framebuffer;
        terrainState.bindings = { terrainBindingSet };
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

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle drawArgsBuffer,
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    DepthPrepassState* state)
{
    if (state)
        InitializeDepthPrepassResources(device, *state);

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "Depth Prepass",

        [&, width, height, drawArgsBuffer, bindlessConfig, state](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;
            data.passState = state;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.write(depthTarget, ResourceState::DepthStencilWrite);
            if (drawArgsBuffer.is_valid())
                data.drawArgsBuffer = passBuilder.read(drawArgsBuffer, ResourceState::IndirectArgument);
        },

        [](const DepthPrepassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!depthRT || !data.passState)
                return;

            renderDepthPrepass(
                ctx,
                data.device,
                depthRT,
                data.bindlessConfig,
                data.materialCache,
                *data.passState
            );
        }
    );

    return passData.depth;
}

} // namespace xray::render::fg::passes
