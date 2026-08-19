#include "stdafx.h"
#include "ShadowPassSetup.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/ResourceManager/NativeRTFactory.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IRenderBackend.h"
#include <cmath>

extern ENGINE_API float ps_r3_grass_blade_height;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r_rt_detail_dist;

namespace xray::render::fg {
extern int ps_r__detail_gpu;
}

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

void ComputeSunNearOrtho(const Fvector& sunDir, float radius, u32 smapRes, Fmatrix& outClipVP, Fmatrix& outSampleVP)
{
    Fvector C = Device.vCameraPosition;
    const float e = std::max(radius, 8.f);
    const float texel = (e * 2.f) / float(std::max(smapRes, 1u));
    C.x = std::floor(C.x / texel) * texel;
    C.z = std::floor(C.z / texel) * texel;
    Fvector dir = sunDir;
    if (dir.square_magnitude() < 1e-6f)
        dir.set(0.f, -1.f, 0.f);
    dir.normalize();
    Fvector eye;
    eye.mad(C, dir, -radius * 1.5f);
    Fvector up(0.f, 1.f, 0.f);
    if (_abs(dir.dotproduct(up)) > 0.95f)
        up.set(0.f, 0.f, 1.f);
    Fmatrix view;
    view.build_camera_dir(eye, dir, up);
    Fmatrix proj;
    proj.build_projection_ortho(e * 2.f, e * 2.f, 1.f, e * 4.f);
    outClipVP.mul(proj, view);
    Fmatrix toUV;
    toUV.identity();
    toUV._11 = 0.5f;
    toUV._22 = -0.5f;
    toUV._33 = 1.f;
    toUV._41 = 0.5f;
    toUV._42 = 0.5f;
    outSampleVP.mul(toUV, outClipVP);
}

}

void InitializeGrassShadowPass(fg::RenderDevice* device, GrassShadowPassState& state)
{
    if (!device)
        return;
    const u32 res = std::max(512u, std::min(ps_r2_smapsize, 4096u));
    if (state.initialized && state.resolution == res)
        return;
    if (state.initialized)
        ShutdownGrassShadowPass(device, state);

    auto* resMgr = device->GetFGResourceManager();
    if (!resMgr || !resMgr->GetRTFactory() || !resMgr->GetTextureManager()) {
        state.initialized = true;
        state.enabled = false;
        return;
    }
    state.resolution = res;
    state.shadowHandle = resMgr->GetRTFactory()->CreateShadowMap(res, true, "rt_GrassShadow");
    state.shadowMap = resMgr->GetTextureManager()->GetNVRHITexture(state.shadowHandle);

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nv || !loader) {
        state.initialized = true;
        state.enabled = false;
        return;
    }

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(ShadowCascadeCB);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "GrassShadowCascadeCB";
    state.cascadeCB = nv->createBuffer(cbDesc);
    cbDesc.byteSize = sizeof(GrassShadowCB);
    cbDesc.debugName = "GrassShadowCB";
    state.grassCB = nv->createBuffer(cbDesc);

    auto& cache = GetPassResourceCache();
    nvrhi::IBindingLayout* bindlessLayout = GEnv.Backend ? GEnv.Backend->GetBindlessLayout() : nullptr;
    nvrhi::FramebufferInfo fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    auto grassVs = loader->LoadVertexShader("detail_gpu_shadow", "main");
    auto grassPs = loader->LoadPixelShader("detail_gpu_shadow", "main");
    if (grassVs.handle && grassVs.reflection && grassPs.handle && grassPs.reflection) {
        state.grassVs = grassVs.handle;
        state.grassPs = grassPs.handle;
        state.grassLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "GrassShadowBlade", *grassVs.reflection, *grassPs.reflection, nv);
        if (state.grassLayout) {
            nvrhi::VertexAttributeDesc grassAttrs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(FGDetailManager::BladeVertex)),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(12).setElementStride(sizeof(FGDetailManager::BladeVertex)),
                nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::R32_FLOAT).setArraySize(2).setOffset(20).setElementStride(sizeof(FGDetailManager::BladeVertex)),
            };
            state.grassInputLayout = nv->createInputLayout(grassAttrs, 3, state.grassVs);
            nvrhi::GraphicsPipelineDesc grassDesc;
            grassDesc.VS = state.grassVs;
            grassDesc.PS = state.grassPs;
            grassDesc.inputLayout = state.grassInputLayout;
            grassDesc.primType = nvrhi::PrimitiveType::TriangleList;
            grassDesc.bindingLayouts = { state.grassLayout };
            if (bindlessLayout)
                grassDesc.bindingLayouts.push_back(bindlessLayout);
            grassDesc.renderState.depthStencilState.setDepthTestEnable(true);
            grassDesc.renderState.depthStencilState.setDepthWriteEnable(true);
            grassDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
            grassDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
            grassDesc.renderState.rasterState.depthBias = 4;
            grassDesc.renderState.rasterState.slopeScaledDepthBias = 3.0f;
            state.grassPipeline = cache.GetOrCreatePipeline("GrassShadowBlade_v2", grassDesc, fbInfo, nv);
        }
        grassVs.reflection = nullptr;
        grassPs.reflection = nullptr;
    }

    auto bbVs = loader->LoadVertexShader("detail_billboard_shadow", "main");
    auto bbPs = loader->LoadPixelShader("detail_billboard_shadow", "main");
    if (bbVs.handle && bbVs.reflection && bbPs.handle && bbPs.reflection) {
        state.billboardGrassVs = bbVs.handle;
        state.billboardGrassPs = bbPs.handle;
        state.billboardGrassLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "GrassShadowBillboard", *bbVs.reflection, *bbPs.reflection, nv);
        if (state.billboardGrassLayout) {
            nvrhi::GraphicsPipelineDesc bbDesc;
            bbDesc.VS = state.billboardGrassVs;
            bbDesc.PS = state.billboardGrassPs;
            bbDesc.primType = nvrhi::PrimitiveType::TriangleList;
            bbDesc.bindingLayouts = { state.billboardGrassLayout };
            if (bindlessLayout)
                bbDesc.bindingLayouts.push_back(bindlessLayout);
            bbDesc.renderState.depthStencilState.setDepthTestEnable(true);
            bbDesc.renderState.depthStencilState.setDepthWriteEnable(true);
            bbDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
            bbDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
            state.billboardGrassPipeline = cache.GetOrCreatePipeline("GrassShadowBillboard_v2", bbDesc, fbInfo, nv);
        }
        bbVs.reflection = nullptr;
        bbPs.reflection = nullptr;
    }

    state.enabled = state.shadowMap && state.cascadeCB && state.grassCB && (state.grassPipeline || state.billboardGrassPipeline);
    state.initialized = true;
}

void ShutdownGrassShadowPass(fg::RenderDevice* device, GrassShadowPassState& state)
{
    (void)device;
    state.grassPipeline = nullptr;
    state.grassLayout = nullptr;
    state.billboardGrassPipeline = nullptr;
    state.billboardGrassLayout = nullptr;
    state.grassInputLayout = nullptr;
    state.grassVs = nullptr;
    state.grassPs = nullptr;
    state.billboardGrassVs = nullptr;
    state.billboardGrassPs = nullptr;
    state.cascadeCB = nullptr;
    state.grassCB = nullptr;
    state.shadowHandle = {};
    state.shadowMap = nullptr;
    state.resolution = 0;
    state.initialized = false;
    state.enabled = false;
}

GrassShadowOutputs setupGrassShadowPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    FGDetailManager* detailManager,
    const Fvector& sunDirection,
    GrassShadowPassState& state,
    VirtualResourceHandle orderAfter,
    VirtualResourceHandle cullArgs)
{
    GrassShadowOutputs out{};
    if (!device || !detailManager || !ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
        return out;

    InitializeGrassShadowPass(device, state);
    if (!state.enabled || !state.shadowMap)
        return out;

    ComputeSunNearOrtho(sunDirection, std::max(ps_r2_sun_near, ps_r_rt_detail_dist), state.resolution, state.clipVP, state.sampleVP);

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = state.resolution;
    desc.height = state.resolution;
    desc.format = nvrhi::Format::D32;
    desc.isDepthStencil = true;
    desc.isImported = true;
    desc.debugName = "rt_GrassShadow";
    auto shadowHandle = fg.ImportTexture("rt_GrassShadow", state.shadowMap, desc);

    struct PassData {
        VirtualResourceHandle shadow;
        VirtualResourceHandle orderDep;
        GrassShadowPassState* st = nullptr;
        FGDetailManager* dm = nullptr;
        fg::RenderDevice* device = nullptr;
    };

    auto& pass = fg.addCallbackPass<PassData>(
        "GrassShadow",
        [&, shadowHandle, orderAfter](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            if (orderAfter.is_valid())
                data.orderDep = pb.read(orderAfter, ResourceState::ShaderResource);
            if (cullArgs.is_valid())
                pb.read(cullArgs, ResourceState::IndirectArgument);
            data.shadow = pb.write(shadowHandle, ResourceState::DepthStencilWrite);
            data.st = &state;
            data.dm = detailManager;
            data.device = device;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.st || !data.dm)
                return;
            auto* shadowTex = graph.GetPhysicalTexture(data.shadow);
            if (!shadowTex)
                shadowTex = data.st->shadowMap;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !shadowTex)
                return;

            GrassShadowPassState& st = *data.st;
            cmd->clearDepthStencilTexture(shadowTex, nvrhi::TextureSubresourceSet(0, 1, 0, 1), true, 1.0f, false, 0);

            ShadowCascadeCB ccb{};
            ccb.lightVP = st.clipVP;
            cmd->writeBuffer(st.cascadeCB, &ccb, sizeof(ccb));
            GrassShadowCB gcb{};
            gcb.grassBladeHeight = ps_r3_grass_blade_height;
            gcb.buildDetailsIndex = data.dm->buildDetailsBindlessIndex;
            if (g_pGamePersistent)
                gcb.windAngleDeg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
            gcb.windSpeed = data.dm->windSpeed;
            gcb.time = Device.fTimeGlobal;
            gcb.windDisplacement = ps_r3_grass_wind_displacement;
            cmd->writeBuffer(st.grassCB, &gcb, sizeof(gcb));

            auto& cache = GetPassResourceCache();
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(shadowTex);
            auto fb = cache.GetOrCreateFramebuffer(make_string("GrassShadow_%u", st.resolution).c_str(), fbDesc, nv);
            if (!fb)
                return;
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            nvrhi::IBindingSet* bindlessTable = GEnv.Backend ? GEnv.Backend->GetBindlessDescriptorTable() : nullptr;
            const u32 cres = st.resolution;
            nvrhi::Viewport vp(0.f, float(cres), 0.f, float(cres), 0.f, 1.f);
            const bool billboardMode = !fg::ps_r__detail_gpu;

            if (billboardMode && st.billboardGrassPipeline && st.billboardGrassLayout &&
                data.dm->visibleBillboardInstancesBuffer && data.dm->billboardDrawArgsBuffer &&
                data.dm->pulledIndexBuffer && data.dm->maxPulledIndexCount > 0) {
                auto* vsRefl = shaderLoader->GetCachedReflection("detail_billboard_shadow", ".vs");
                auto* psRefl = shaderLoader->GetCachedReflection("detail_billboard_shadow", ".ps");
                if (vsRefl && psRefl) {
                    BindingSetBuilder gsb(*vsRefl, *psRefl, nv, "GrassShadow.BB");
                    gsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                    gsb.ConstantBuffer("GrassShadowCB", st.grassCB);
                    BindBindlessMaterialTables(gsb);
                    gsb.BufferSRV("visible_indices", data.dm->visibleBillboardInstancesBuffer);
                    gsb.BufferSRV("detail_models", data.dm->detailModelsBuffer);
                    gsb.BufferSRV("pulled_vertices", data.dm->pulledVertexBuffer);
                    gsb.BufferSRV("all_instances", data.dm->generatedInstancesBuffer);
                    gsb.Texture("g_Perlin4D", data.dm->perlin4dTexture);
                    auto set = cache.GetOrCreateBindingSet(gsb.Build(), st.billboardGrassLayout, nv);
                    if (set) {
                        nvrhi::GraphicsState gs;
                        gs.pipeline = st.billboardGrassPipeline;
                        gs.framebuffer = fb;
                        gs.bindings = { set };
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.indexBuffer = { data.dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                        gs.viewport.addViewport(vp);
                        gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                        gs.indirectParams = data.dm->billboardDrawArgsBuffer;
                        cmd->setGraphicsState(gs);
                        cmd->drawIndexedIndirect(0);
                    }
                    if (data.dm->visibleDecalInstancesBuffer && data.dm->decalDrawArgsBuffer) {
                    BindingSetBuilder dsb(*vsRefl, *psRefl, nv, "GrassShadow.Decal");
                    dsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                    dsb.ConstantBuffer("GrassShadowCB", st.grassCB);
                    BindBindlessMaterialTables(dsb);
                    dsb.BufferSRV("visible_indices", data.dm->visibleDecalInstancesBuffer);
                    dsb.BufferSRV("detail_models", data.dm->detailModelsBuffer);
                    dsb.BufferSRV("pulled_vertices", data.dm->pulledVertexBuffer);
                    dsb.BufferSRV("all_instances", data.dm->generatedInstancesBuffer);
                    dsb.Texture("g_Perlin4D", data.dm->perlin4dTexture);
                    auto dset = cache.GetOrCreateBindingSet(dsb.Build(), st.billboardGrassLayout, nv);
                    if (dset) {
                        nvrhi::GraphicsState gs;
                        gs.pipeline = st.billboardGrassPipeline;
                        gs.framebuffer = fb;
                        gs.bindings = { dset };
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.indexBuffer = { data.dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                        gs.viewport.addViewport(vp);
                        gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                        gs.indirectParams = data.dm->decalDrawArgsBuffer;
                        cmd->setGraphicsState(gs);
                        cmd->drawIndexedIndirect(0);
                    }
                    }
                }
            } else if (!billboardMode && st.grassPipeline && st.grassLayout) {
                auto* vsRefl = shaderLoader->GetCachedReflection("detail_gpu_shadow", ".vs");
                auto* psRefl = shaderLoader->GetCachedReflection("detail_gpu_shadow", ".ps");
                if (vsRefl && psRefl) {
                    for (u32 lod = 0; lod < 2 && lod < FGDetailManager::LOD_COUNT; ++lod) {
                        if (!data.dm->visibleInstancesBuffer[lod] || !data.dm->drawArgsBuffer[lod] ||
                            !data.dm->bladeVertexBuffer[lod] || !data.dm->bladeIndexBuffer[lod])
                            continue;
                        BindingSetBuilder gsb(*vsRefl, *psRefl, nv, "GrassShadow.Blade");
                        gsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                        gsb.ConstantBuffer("GrassShadowCB", st.grassCB);
                        gsb.BufferSRV("visible_indices", data.dm->visibleInstancesBuffer[lod]);
                        gsb.BufferSRV("all_instances", data.dm->generatedInstancesBuffer);
                        auto set = cache.GetOrCreateBindingSet(gsb.Build(), st.grassLayout, nv);
                        if (!set)
                            continue;
                        nvrhi::GraphicsState gs;
                        gs.pipeline = st.grassPipeline;
                        gs.framebuffer = fb;
                        gs.bindings = { set };
                        if (bindlessTable)
                            gs.addBindingSet(bindlessTable);
                        gs.vertexBuffers = { { data.dm->bladeVertexBuffer[lod], 0, 0 } };
                        gs.indexBuffer = { data.dm->bladeIndexBuffer[lod], nvrhi::Format::R16_UINT, 0 };
                        gs.viewport.addViewport(vp);
                        gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                        gs.indirectParams = data.dm->drawArgsBuffer[lod];
                        cmd->setGraphicsState(gs);
                        cmd->drawIndexedIndirect(0);
                    }
                }
            }
        });

    out.shadowMap = pass.shadow;
    out.shadowTex = state.shadowMap;
    out.sampleVP = state.sampleVP;
    out.valid = true;
    return out;
}

}
