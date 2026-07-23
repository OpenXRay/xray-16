#include "stdafx.h"
#include "LocalShadowPassSetup.h"
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
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/ResourceManager/NativeRTFactory.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{

namespace
{

u32 LocalShadowResolution()
{
    // Prefer sharper long floor shadows (beds/tables). Cap at 2048.
    const u32 half = std::max(ps_r2_smapsize / 2u, 512u);
    return std::min(half, 2048u);
}

} // namespace

void InitializeLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state)
{
    if (!device)
        return;

    const u32 res = LocalShadowResolution();
    const u32 slices = MAX_LOCAL_SHADOW_TILES;
    if (state.initialized && state.enabled && state.resolution == res &&
        state.sliceCount == slices && state.atlas)
        return;

    if (state.initialized)
        ShutdownLocalShadowPass(device, state);

    // Pipelines are resolution-independent but clear so bias-tuned ones rebuild cleanly.
    state.opaquePipeline = nullptr;
    state.foliagePipeline = nullptr;

    auto* resMgr = device->GetFGResourceManager();
    if (!resMgr || !resMgr->GetRTFactory() || !resMgr->GetTextureManager())
    {
        state.initialized = true;
        state.enabled = false;
        return;
    }

    state.resolution = res;
    state.sliceCount = slices;
    state.atlasHandle = resMgr->GetRTFactory()->CreateCascadedShadowMap(
        res, slices, true, "rt_LocalShadowAtlas");
    state.atlas = resMgr->GetTextureManager()->GetNVRHITexture(state.atlasHandle);
    state.initialized = true;
    state.enabled = (state.atlas != nullptr);
    if (state.enabled)
        Msg("* [LocalShadow] Init: OK (%ux%u × %u slices)", res, res, slices);
    else
        Msg("! [LocalShadow] Failed to create local shadow atlas");
}

void ShutdownLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state)
{
    if (device && device->GetFGResourceManager() && device->GetFGResourceManager()->GetRTFactory())
    {
        if (state.atlasHandle.IsValid())
            device->GetFGResourceManager()->GetRTFactory()->ReleaseRenderTarget(state.atlasHandle);
    }
    state.atlasHandle = {};
    state.atlas = nullptr;
    state.opaquePipeline = nullptr;
    state.foliagePipeline = nullptr;
    state.resolution = 0;
    state.sliceCount = 0;
    state.initialized = false;
    state.enabled = false;
}

LocalShadowOutputs setupLocalShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    LocalShadowPassState& state,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches,
    fg::GPUCullingManager* gpuCulling)
{
    using namespace framegraph;
    LocalShadowOutputs outputs;
    outputs.valid = false;

    if (!device)
        return outputs;

    auto& clm = ClusteredLightManager::Instance();
    const auto& tiles = clm.GetLocalShadowTiles();
    if (ps_r_local_shadows == 0 || tiles.empty())
        return outputs;

    InitializeLocalShadowPass(device, state);
    if (!state.enabled || !state.atlas || !shadowState.enabled || !shadowState.pipeline)
        return outputs;

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = state.resolution;
    desc.height = state.resolution;
    desc.depth = 1;
    desc.arraySize = state.sliceCount;
    desc.format = nvrhi::Format::D32;
    desc.isDepthStencil = true;
    desc.isImported = true;
    desc.debugName = "rt_LocalShadowAtlas";

    auto atlasHandle = fg.ImportTexture("rt_LocalShadowAtlas", state.atlas, desc);

    struct PassData
    {
        VirtualResourceHandle atlas;
        LocalShadowPassState* localState = nullptr;
        ShadowPassState* shadowState = nullptr;
        BindlessForwardConfig bindlessConfig;
        fg::RenderDevice* device = nullptr;
        fg::GPUCullingManager* gpuCulling = nullptr;
        const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches = nullptr;
        xr_vector<LocalShadowTile> tiles;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "LocalShadows",
        [&, atlasHandle, worldSkinnedBatches, gpuCulling](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.atlas = pb.write(atlasHandle, ResourceState::DepthStencilWrite);
            data.localState = &state;
            data.shadowState = &shadowState;
            data.bindlessConfig = bindlessConfig;
            data.device = device;
            data.gpuCulling = gpuCulling;
            data.worldSkinnedBatches = worldSkinnedBatches;
            data.tiles = tiles;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            ZoneScopedN("LocalShadows::Execute");
            if (!data.localState || !data.shadowState || data.tiles.empty())
                return;
            auto* atlasTex = graph.GetPhysicalTexture(data.atlas);
            if (!atlasTex)
                atlasTex = data.localState->atlas;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !atlasTex)
                return;

            ShadowPassState& st = *data.shadowState;
            LocalShadowPassState& ls = *data.localState;

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
            {
                static bool s_warned = false;
                if (!s_warned)
                {
                    Msg("! [LocalShadow] GPU bindless culling unavailable — atlas empty");
                    s_warned = true;
                }
                return;
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("LocalShadow", nv);
            if (!vsRefl || !psRefl || !drawIndexBuffer || !st.pipeline || !st.layout || !st.cascadeCB)
                return;

            auto& cache = GetPassResourceCache();
            auto& matBuffer = bindless::MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto* backend = GEnv.Backend;
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // Perspective local SMs need far less slope bias than sun ortho CSM —
            // high bias was shrinking bed/table/NPC contact shadows.
            if (!ls.opaquePipeline && st.vs && st.ps && st.inputLayout && st.layout)
            {
                nvrhi::GraphicsPipelineDesc d;
                d.VS = st.vs;
                d.PS = st.ps;
                d.inputLayout = st.inputLayout;
                d.primType = nvrhi::PrimitiveType::TriangleList;
                d.renderState.depthStencilState.setDepthTestEnable(true);
                d.renderState.depthStencilState.setDepthWriteEnable(true);
                d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                d.renderState.rasterState.depthBias = 0;
                d.renderState.rasterState.slopeScaledDepthBias = 0.5f;
                d.renderState.rasterState.depthBiasClamp = 0.001f;
                if (auto* bl = backend ? backend->GetBindlessLayout() : nullptr)
                    d.bindingLayouts = {st.layout, bl};
                else
                    d.bindingLayouts = {st.layout};
                nvrhi::FramebufferInfoEx fbInfo;
                fbInfo.depthFormat = nvrhi::Format::D32;
                ls.opaquePipeline = cache.GetOrCreatePipeline("LocalShadowOpaque", d, fbInfo, nv);
                if (st.foliagePs && st.foliagePipeline)
                {
                    d.PS = st.foliagePs;
                    ls.foliagePipeline = cache.GetOrCreatePipeline("LocalShadowFoliage", d, fbInfo, nv);
                }
            }
            nvrhi::IGraphicsPipeline* opaquePipe =
                ls.opaquePipeline ? ls.opaquePipeline.Get() : st.pipeline.Get();
            nvrhi::IGraphicsPipeline* foliagePipe =
                ls.foliagePipeline ? ls.foliagePipeline.Get() : st.foliagePipeline.Get();

            const u32 cres = ls.resolution;
            nvrhi::Viewport vp(0.f, float(cres), 0.f, float(cres), 0.f, 1.f);

            {
                static bool s_once = false;
                if (!s_once)
                {
                    Msg("* [LocalShadow] Drawing %u tiles @ %ux%u (skinned=%d)",
                        (u32)data.tiles.size(), cres, cres,
                        (ps_r_skinned_shadows != 0 && data.worldSkinnedBatches) ? 1 : 0);
                    s_once = true;
                }
            }

            cmd->clearDepthStencilTexture(
                atlasTex,
                nvrhi::TextureSubresourceSet(0, 1, 0, ls.sliceCount),
                true, 1.0f, false, 0);

            // Restore identity caster maps (CSM light-cull overwrites with compact subset).
            if (data.gpuCulling)
                data.gpuCulling->EnsureShadowCasterIdentityBuffers(cmd, nv);

            const bool drawWorldSkinned =
                (ps_r_skinned_shadows != 0) && data.worldSkinnedBatches && data.gpuCulling &&
                (st.skinnedPipeline || st.skinned4wPipeline || st.skinnedHqPipeline ||
                 st.skinned2wPipeline || st.skinned3wPipeline);
            nvrhi::IBuffer* skBoneBuf =
                drawWorldSkinned ? data.gpuCulling->GetGlobalBoneBuffer() : nullptr;
            auto* skPsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            auto* skVsReflNonHQ = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned", ".vs");
            auto* skVsRefl4W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_4w", ".vs");
            auto* skVsReflHQ = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_hq", ".vs");
            auto* skVsRefl2W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_2w", ".vs");
            auto* skVsRefl3W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_3w", ".vs");
            nvrhi::IBuffer* skDynCB = nullptr;
            nvrhi::IBuffer* skMatCB = nullptr;
            const bool worldSkinnedReady = drawWorldSkinned && skBoneBuf && skPsRefl;
            constexpr u32 kSkinnedVCBVersions = 2048;
            constexpr u32 kMaxSkinnedPerTile = 128;
            u32 skinnedWritesLeft = kSkinnedVCBVersions;
            if (worldSkinnedReady)
            {
                skDynCB = cache.GetOrCreateVolatileCB(
                    "LocalShadow", "WorldSkinDyn2", sizeof(DynamicTransforms), data.device,
                    kSkinnedVCBVersions);
                skMatCB = cache.GetOrCreateVolatileCB(
                    "LocalShadow", "WorldSkinMat2", sizeof(SkinnedMaterialCB), data.device,
                    kSkinnedVCBVersions);
            }

            for (const LocalShadowTile& tile : data.tiles)
            {
                ZoneScopedN("LocalShadows::Tile");
                if (tile.slice >= ls.sliceCount)
                    continue;

                ShadowCascadeCB cbData{};
                cbData.lightVP = tile.clipVP;
                cmd->writeBuffer(st.cascadeCB, &cbData, sizeof(cbData));

                nvrhi::FramebufferDesc fbDesc;
                nvrhi::TextureSubresourceSet depthSub;
                depthSub.baseArraySlice = tile.slice;
                depthSub.numArraySlices = 1;
                fbDesc.setDepthAttachment(atlasTex, depthSub);
                auto fb = cache.GetOrCreateFramebuffer(
                    make_string("LocalShadow_%u_%u", cres, tile.slice).c_str(), fbDesc, nv);
                if (!fb)
                    continue;

                auto drawSet = [&](const BindlessDrawSet& set, nvrhi::IGraphicsPipeline* pipe) {
                    if (!pipe)
                        return;
                    if (!set.IsCastAllValid() && !set.IsValid())
                        return;
                    const bool castAll = set.IsCastAllValid();
                    nvrhi::IBuffer* batchIndices = castAll
                        ? set.castAllBatchIndicesBuffer : set.compactBatchIndicesBuffer;
                    nvrhi::IBuffer* materialIDs = castAll
                        ? set.castAllMaterialIDBuffer : set.compactMaterialIDBuffer;
                    nvrhi::IBuffer* drawArgs = castAll
                        ? set.castAllDrawArgsBuffer : set.compactDrawArgsBuffer;
                    nvrhi::IBuffer* countBuffer = castAll
                        ? set.castAllCountBuffer : set.compactCountBuffer;

                    BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "LocalShadow");
                    bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                    bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
                    bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
                    bsb.BufferSRV("g_CompactBatchIndices", batchIndices);
                    bsb.BufferSRV("g_CompactMaterialIDs", materialIDs);
                    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), st.layout, nv);
                    if (!bindingSet)
                        return;

                    nvrhi::GraphicsState gs;
                    gs.pipeline = pipe;
                    gs.framebuffer = fb;
                    gs.bindings = {bindingSet};
                    if (bindlessTable)
                        gs.addBindingSet(bindlessTable);
                    gs.vertexBuffers = {
                        {data.bindlessConfig.megaVertexBuffer, 0, 0},
                        {drawIndexBuffer, 1, 0}};
                    gs.indexBuffer = {data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                    gs.viewport.addViewport(vp);
                    gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                    gs.indirectParams = drawArgs;
                    gs.indirectCountBuffer = countBuffer;
                    cmd->setGraphicsState(gs);
                    DrawIndexedIndirectCountOrFallback(cmd, 0, 0, set.totalObjectCount);
                };

                drawSet(data.bindlessConfig.staticSet, opaquePipe);
                drawSet(data.bindlessConfig.dynamicSet, opaquePipe);
                if (foliagePipe)
                    drawSet(data.bindlessConfig.transparentCasterSet, foliagePipe);

                if (st.terrainPipeline && st.terrainLayout && data.bindlessConfig.HasTerrain())
                {
                    if (auto* matCache = GEnv.Render ? GEnv.Render->GetMaterialCache() : nullptr)
                        matCache->FinalizePendingTerrainMaterials(ctx);
                    auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
                    terrainMatBuffer.Upload(ctx);
                    auto* terrainPsRefl = shaderLoader->GetCachedReflection(
                        "shadow\\shadow_cascade_terrain", ".ps");
                    const bool useCompact = data.bindlessConfig.UseTerrainCompaction();
                    nvrhi::IBuffer* tInstance = data.bindlessConfig.terrainInstanceBuffer;
                    nvrhi::IBuffer* tBatch = useCompact
                        ? data.bindlessConfig.terrainCompactBatchIndicesBuffer
                        : data.bindlessConfig.terrainBatchIndicesBuffer;
                    nvrhi::IBuffer* tMat = useCompact
                        ? data.bindlessConfig.terrainCompactMaterialIDBuffer
                        : data.bindlessConfig.terrainMaterialIDBuffer;
                    nvrhi::IBuffer* tArgs = useCompact
                        ? data.bindlessConfig.terrainCompactDrawArgsBuffer
                        : data.bindlessConfig.terrainDrawArgsBuffer;
                    nvrhi::IBuffer* tCount = useCompact
                        ? data.bindlessConfig.terrainCompactCountBuffer
                        : nullptr;
                    if (terrainPsRefl && terrainMatBuffer.GetBuffer() && tInstance && tBatch && tMat && tArgs)
                    {
                        BindingSetBuilder tbsb(*vsRefl, *terrainPsRefl, nv, "LocalShadow.Terrain");
                        tbsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                        tbsb.BufferSRV("g_InstanceData", tInstance);
                        tbsb.BufferSRV("g_CompactBatchIndices", tBatch);
                        tbsb.BufferSRV("g_CompactMaterialIDs", tMat);
                        auto terrainSet = cache.GetOrCreateBindingSet(tbsb.Build(), st.terrainLayout, nv);
                        if (terrainSet)
                        {
                            nvrhi::GraphicsState gs;
                            gs.pipeline = st.terrainPipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {terrainSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {data.bindlessConfig.megaVertexBuffer, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {
                                data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                            gs.viewport.addViewport(vp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            gs.indirectParams = tArgs;
                            if (tCount)
                                gs.indirectCountBuffer = tCount;
                            cmd->setGraphicsState(gs);
                            DrawIndexedIndirectCountOrFallback(
                                cmd, 0, 0, data.bindlessConfig.terrainObjectCount);
                        }
                    }
                }

                if (worldSkinnedReady && skDynCB && skMatCB && skinnedWritesLeft > 0 && tile.L)
                {
                    // Cast beyond light range a bit so NPC/body shadows reach across the floor.
                    const float maxR = std::max(tile.L->range * 2.0f, 4.f);
                    const float maxR2 = maxR * maxR;
                    const Fvector& lp = tile.L->position;
                    u32 drawnThisTile = 0;

                    for (const auto& batch : *data.worldSkinnedBatches)
                    {
                        if (skinnedWritesLeft == 0 || drawnThisTile >= kMaxSkinnedPerTile)
                            break;
                        if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
                            continue;
                        if (lp.distance_to_sqr(batch.worldMatrix.c) > maxR2)
                            continue;

                        CKinematics* parent = nullptr;
                        const u32 visualType = batch.visual ? batch.visual->getType() : 0;
                        if (visualType == MT_SKELETON_GEOMDEF_ST)
                            parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                        else if (visualType == MT_SKELETON_GEOMDEF_PM)
                            parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                        if (!parent)
                            continue;

                        const u16 rm = batch.skinningRenderMode;
                        const u32 stride = batch.vertexStride;
                        nvrhi::IGraphicsPipeline* pipe = nullptr;
                        nvrhi::IBindingLayout* layout = nullptr;
                        nvrhi::IInputLayout* il = nullptr;
                        auto* vsReflSk = skVsReflNonHQ;
                        auto pick3w = [&] {
                            pipe = st.skinned3wPipeline.Get(); layout = st.skinned3wLayout.Get();
                            il = st.skinned3wInputLayout.Get(); vsReflSk = skVsRefl3W;
                        };
                        auto pick2w = [&] {
                            pipe = st.skinned2wPipeline.Get(); layout = st.skinned2wLayout.Get();
                            il = st.skinned2wInputLayout.Get(); vsReflSk = skVsRefl2W;
                        };
                        auto pick4w = [&] {
                            pipe = st.skinned4wPipeline.Get(); layout = st.skinned4wLayout.Get();
                            il = st.skinned4wInputLayout.Get(); vsReflSk = skVsRefl4W;
                        };
                        auto pickHq = [&] {
                            pipe = st.skinnedHqPipeline.Get(); layout = st.skinnedHqLayout.Get();
                            il = st.skinnedHqInputLayout.Get(); vsReflSk = skVsReflHQ;
                        };
                        auto pickNonHq = [&] {
                            pipe = st.skinnedPipeline.Get(); layout = st.skinnedLayout.Get();
                            il = st.skinnedInputLayout.Get(); vsReflSk = skVsReflNonHQ;
                        };
                        if (rm == 7 || rm == 8) pick3w();
                        else if (rm == 5 || rm == 6) pick2w();
                        else if (rm == 9 || rm == 10) pick4w();
                        else if (rm == 4 || rm == 2) pickHq();
                        else if (rm == 3 || rm == 1) pickNonHq();
                        else if (stride == 36) pickHq();
                        else if (stride == 40) pick4w();
                        else if (stride == 44) pick2w();
                        else pickNonHq();
                        if (!pipe || !layout || !il || !vsReflSk)
                            continue;

                        const u32 boneOffset = data.gpuCulling->GetOrUploadSkeleton(cmd, parent);
                        DynamicTransforms dyn{};
                        FillDynamicTransforms(dyn, batch.worldMatrix);
                        cmd->writeBuffer(skDynCB, &dyn, sizeof(dyn));
                        SkinnedMaterialCB matId{};
                        matId.materialID = batch.bindlessMaterialID;
                        matId.skeletonBoneOffset = boneOffset;
                        cmd->writeBuffer(skMatCB, &matId, sizeof(matId));

                        BindingSetBuilder bsb(*vsReflSk, *skPsRefl, nv, "LocalShadow.WorldSkin");
                        bsb.ConstantBuffer("dynamic_transforms", skDynCB);
                        bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                        bsb.ConstantBuffer("SkinnedMaterialCB", skMatCB);
                        bsb.BufferSRV("g_BoneMatrices", skBoneBuf);
                        bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
                        auto set = cache.GetOrCreateBindingSet(bsb.Build(), layout, nv);
                        if (!set)
                            continue;

                        nvrhi::GraphicsState gs;
                        gs.pipeline = pipe;
                        gs.framebuffer = fb;
                        gs.bindings = {set};
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.vertexBuffers = {{batch.vertexBuffer, 0, 0}};
                        gs.indexBuffer = {batch.indexBuffer, nvrhi::Format::R16_UINT, 0};
                        gs.viewport.addViewport(vp);
                        gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                        cmd->setGraphicsState(gs);
                        cmd->drawIndexed(
                            nvrhi::DrawArguments()
                                .setVertexCount(batch.indexCount)
                                .setStartIndexLocation(batch.startIndex)
                                .setStartVertexLocation(batch.baseVertex));
                        ++drawnThisTile;
                        --skinnedWritesLeft;
                    }
                }
            }
        });

    outputs.atlas = passData.atlas;
    outputs.atlasTex = state.atlas;
    outputs.valid = true;
    return outputs;
}

} // namespace xray::render::fg::passes
