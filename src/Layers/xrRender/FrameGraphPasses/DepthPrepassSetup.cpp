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
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "PassCommon.h"
#include <algorithm>

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

    state.initialized = true;
    Msg("* [DepthPrepass] Pipeline initialized");
}

static bool EnsureSkinnedPrepassPipelines(fg::RenderDevice* device, DepthPrepassState& state, const SkinningPassState& sk)
{
    if (state.skinnedReady)
        return true;
    if (state.skinnedFailed || !sk.initialized)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return false;

    auto psResult = shaderLoader->LoadPixelShader("bindless_skinned_depth", "main");
    auto* vsRefl = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
    if (!psResult.handle || !psResult.reflection || !vsRefl) {
        Msg("! [DepthPrepass] skinned depth shader failed to load");
        state.skinnedFailed = true;
        return false;
    }
    state.skinnedDepthPS = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    state.skinnedLayout = cache.GetOrCreateBindingLayoutFromReflection("DepthPrepassSkinned", *vsRefl, *psResult.reflection, nvDevice);
    if (!state.skinnedLayout) {
        state.skinnedFailed = true;
        return false;
    }

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    static_assert(kSunShadowSkinnedFormats == SkinnedGeometryPools::FORMAT_COUNT, "skinned format count mismatch");
    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
        const SkinningPipelineVariant* variant = SkinnedVariant(sk, f, false);
        if (!variant || !variant->vs || !variant->inputLayout)
            continue;
        nvrhi::GraphicsPipelineDesc desc;
        desc.VS = variant->vs;
        desc.PS = state.skinnedDepthPS;
        desc.inputLayout = variant->inputLayout;
        if (bindlessLayout)
            desc.bindingLayouts = { state.skinnedLayout, bindlessLayout };
        else
            desc.bindingLayouts = { state.skinnedLayout };
        desc.primType = nvrhi::PrimitiveType::TriangleList;
        desc.renderState.depthStencilState.depthTestEnable = true;
        desc.renderState.depthStencilState.depthWriteEnable = true;
        desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
        desc.renderState.rasterState.frontCounterClockwise = false;
        desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
        string64 name;
        xr_sprintf(name, "DepthPrepassSkinned_%u", f);
        state.skinnedPipelines[f] = cache.GetOrCreatePipeline(name, desc, fbInfo, nvDevice);
    }

    state.skinnedReady = true;
    Msg("* [DepthPrepass] skinned pipelines initialized");
    return true;
}

struct SkinnedPrepassItem {
    const GeometryBatch* batch;
    u32 boneOffset;
    u32 splatOffset;
    u32 splatCount;
    u32 fmt;
};

static void prepareSkinnedPrepass(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    const DepthPrepassSkinnedConfig& cfg,
    DepthPrepassState& ps,
    xr_vector<SkinnedPrepassItem>& items)
{
    using namespace fg::bindless;

    items.clear();
    if (!cfg.skinning || !cfg.geometry || !cfg.gpuCulling)
        return;

    u32 worldSkinnedCount = 0;
    for (const auto& batch : cfg.geometry->GetBatches()) {
        if (batch.isSkinned)
            ++worldSkinnedCount;
    }
    if (worldSkinnedCount == 0)
        return;
    if (!EnsureSkinnedPrepassPipelines(device, ps, *cfg.skinning))
        return;
    if (!cfg.gpuCulling->GetGlobalBoneBuffer())
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (cfg.overlayMgr)
        cfg.overlayMgr->UploadSplats(cmdList);

    auto& matBuffer = MaterialBuffer::Instance();
    for (const auto& batch : cfg.geometry->GetBatches()) {
        if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
            continue;
        if (matBuffer.GetShaderVariant(batch.bindlessMaterialID) != 0)
            continue;
        const u32 fmt = SkinnedVertexFormat(batch.skinningRenderMode, batch.vertexStride);
        if (fmt >= kSunShadowSkinnedFormats || !ps.skinnedPipelines[fmt])
            continue;
        const auto splats = GetSplatRange(batch, cfg.overlayMgr);
        SkinnedPrepassItem item;
        item.batch = &batch;
        item.boneOffset = SkeletonBoneOffset(cmdList, *cfg.gpuCulling, batch);
        item.splatOffset = splats.offset;
        item.splatCount = splats.count;
        item.fmt = fmt;
        items.push_back(item);
    }

    std::stable_sort(items.begin(), items.end(), [](const SkinnedPrepassItem& a, const SkinnedPrepassItem& b) {
        return a.fmt < b.fmt;
    });
}

static void drawSkinnedPrepass(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    nvrhi::IFramebuffer* framebuffer,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    nvrhi::IBindingSet* bindlessTable,
    nvrhi::IBuffer* staticGlobalsCB,
    const DepthPrepassSkinnedConfig& cfg,
    DepthPrepassState& ps,
    const xr_vector<SkinnedPrepassItem>& items)
{
    using namespace fg::bindless;

    if (items.empty())
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("bindless_skinned_depth", ".ps");
    if (!vsRefl || !psRefl)
        return;

    auto& matBuffer = MaterialBuffer::Instance();
    auto dynCB = cache.GetOrCreateVolatileCB("DepthPrepass", "DynTransforms", sizeof(DynamicTransforms), device, 1024 * 8);
    auto matCB = cache.GetOrCreateVolatileCB("DepthPrepass", "MaterialId", sizeof(SkinnedMaterialCB), device, 1024 * 8);

    framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, nvDevice, "DepthPrepass.Skinned");
    bsb.ConstantBuffer("dynamic_transforms", dynCB);
    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
    bsb.BufferSRV("g_BoneMatrices", cfg.gpuCulling->GetGlobalBoneBuffer());
    bsb.ConstantBuffer("SkinnedMaterialCB", matCB);
    bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
    bsb.BufferSRV("g_PaintSplats", cfg.overlayMgr ? cfg.overlayMgr->GetSplatBuffer() : nullptr);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps.skinnedLayout, nvDevice);
    if (!bindingSet)
        return;

    for (const SkinnedPrepassItem& item : items) {
        const GeometryBatch& batch = *item.batch;

        DynamicTransforms dynTransData = {};
        FillDynamicTransforms(dynTransData, batch.worldMatrix);
        cmdList->writeBuffer(dynCB, &dynTransData, sizeof(dynTransData));

        SkinnedMaterialCB matIdData = {};
        matIdData.materialID = batch.bindlessMaterialID;
        matIdData.skeletonBoneOffset = item.boneOffset;
        matIdData.splatOffset = item.splatOffset;
        matIdData.splatCount = item.splatCount;
        cmdList->writeBuffer(matCB, &matIdData, sizeof(matIdData));

        nvrhi::GraphicsState gs;
        gs.pipeline = ps.skinnedPipelines[item.fmt];
        gs.framebuffer = framebuffer;
        gs.bindings = { bindingSet };
        if (bindlessTable)
            gs.addBindingSet(bindlessTable);
        gs.vertexBuffers = { { batch.vertexBuffer, 0, 0 } };
        gs.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
        gs.viewport.addViewport(viewport);
        gs.viewport.addScissorRect(scissor);
        cmdList->setGraphicsState(gs);
        cmdList->drawIndexed(nvrhi::DrawArguments()
            .setVertexCount(batch.indexCount)
            .setStartIndexLocation(batch.startIndex)
            .setStartVertexLocation(batch.baseVertex));
    }
}

static void renderDepthPrepass(
    fg::RenderContext* ctx,
    fg::RenderDevice* device,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    const DepthPrepassSkinnedConfig& skinned,
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

    static thread_local xr_vector<SkinnedPrepassItem> s_skinnedItems;
    prepareSkinnedPrepass(ctx, device, skinned, ps, s_skinnedItems);

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

    drawSkinnedPrepass(ctx, device, framebuffer, viewport, nvrhi::Rect(rtDesc.width, rtDesc.height),
        bindlessTable, staticGlobalsCB, skinned, ps, s_skinnedItems);
}

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthTarget,
    framegraph::VirtualResourceHandle drawArgsBuffer,
    const BindlessForwardConfig& bindlessConfig,
    const DepthPrepassSkinnedConfig& skinned,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    DepthPrepassState* state)
{
    if (state)
        InitializeDepthPrepassResources(device, *state);

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "Depth Prepass",

        [&, width, height, drawArgsBuffer, bindlessConfig, skinned, state](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;
            data.skinned = skinned;
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
                data.skinned,
                data.materialCache,
                *data.passState
            );
        }
    );

    return passData.depth;
}

} // namespace xray::render::fg::passes
