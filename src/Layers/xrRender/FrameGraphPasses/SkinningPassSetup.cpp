// xrRender/FrameGraphPasses/SkinningPassSetup.cpp
// Consolidated skinned mesh rendering pass with World and HUD phases
// Uses GPU-driven global bone buffer for efficient skinning
#include "stdafx.h"
#include "SkinningPassSetup.h"
#include "IBLPrefilterPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Decals/OverlayManager.h"
#include "PassCommon.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/ShadersExternalData.h"

extern ENGINE_API float psHUD_FOV;

namespace xray::render::fg::passes {
using namespace bindless;

// ═══════════════════════════════════════════════════════════════════════════
//  HUD FOV ADJUSTMENT
// ═══════════════════════════════════════════════════════════════════════════
static Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    float fovScale = 1.0f / psHUD_FOV;
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;
    fovScaleMatrix._22 = fovScale;
    fovScaleMatrix._33 = 1.0f;

    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);
    temp2.mul(fovScaleMatrix, temp1);
    result.mul(invView, temp2);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void InitializeSkinningResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, SkinningPassState& state)
{
    if (state.initialized)
        return;

    // Drop stale variant PSOs so NPC materials remapped to bindless_skinned pick up IBL/CSM.
    VariantPSOCache::Instance().Shutdown();

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = framegraph::GetPassResourceCache();

    auto skinnedPsResult = shaderLoader->LoadPixelShader("bindless_skinned", "main");
    if (!skinnedPsResult.handle) {
        Msg("! [SkinningPass] Failed to load pixel shader");
        return;
    }
    state.ps = skinnedPsResult.handle;

    auto skinnedVsForReflection = shaderLoader->LoadVertexShader("bindless_skinned", "main");
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("SkinningPass_IBL_CSMLadder_NoMask", *skinnedVsForReflection.reflection, *skinnedPsResult.reflection, nvDevice);

    auto hudPsResult = shaderLoader->LoadPixelShader("bindless_skinned_hud", "main");
    if (hudPsResult.handle) {
        state.hudPS = hudPsResult.handle;
        state.hudLayout = cache.GetOrCreateBindingLayoutFromReflection("SkinningPass_HUD_IBL_NoCSM_v3", *skinnedVsForReflection.reflection, *hudPsResult.reflection, nvDevice);
    }
    if (!state.hudLayout)
        state.hudLayout = state.layout;

    auto scopePsResult = shaderLoader->LoadPixelShader("bindless_skinned_scope", "main");
    if (scopePsResult.handle) {
        state.hudScopePS = scopePsResult.handle;
        state.hudScopeLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "SkinningPass_HUDScope", *skinnedVsForReflection.reflection, *scopePsResult.reflection, nvDevice);
    }
    if (!state.hudScopeLayout)
        state.hudScopeLayout = state.hudLayout;

    auto buildPipelineDesc = [&](nvrhi::IShader* vs, nvrhi::IInputLayout* il, nvrhi::IShader* psOverride = nullptr) {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = vs;
        pipeDesc.PS = psOverride ? nvrhi::ShaderHandle(psOverride) : state.ps;
        pipeDesc.inputLayout = il;
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
        return pipeDesc;
    };

    auto initVariant = [&](SkinningPipelineVariant& variant, const char* shaderName, const char* cacheName,
                           const nvrhi::VertexAttributeDesc* attribs, u32 attrCount) {
        auto vsResult = shaderLoader->LoadVertexShader(shaderName, "main");
        if (!vsResult.handle)
            return;
        variant.vs = vsResult.handle;
        variant.inputLayout = nvDevice->createInputLayout(attribs, attrCount, variant.vs);
        auto pipeDesc = buildPipelineDesc(variant.vs, variant.inputLayout);
        variant.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
        if (variant.pipeline)
            QueryBindingLayoutFromPipeline(variant.pipeline, state.layout);
    };

    auto initHudVariant = [&](SkinningPipelineVariant& hudVariant, const SkinningPipelineVariant& worldVariant, const char* cacheName) {
        if (!worldVariant.pipeline || !state.hudPS)
            return;
        hudVariant.vs = worldVariant.vs;
        hudVariant.inputLayout = worldVariant.inputLayout;
        auto pipeDesc = buildPipelineDesc(worldVariant.vs, worldVariant.inputLayout, state.hudPS);
        pipeDesc.bindingLayouts[0] = state.hudLayout;
        for (u32 rt = 0; rt < 4; ++rt)
        {
            pipeDesc.renderState.blendState.targets[rt].setBlendEnable(false);
            pipeDesc.renderState.blendState.targets[rt].setColorWriteMask(
                nvrhi::ColorMask::Red | nvrhi::ColorMask::Green |
                nvrhi::ColorMask::Blue | nvrhi::ColorMask::Alpha);
        }
        hudVariant.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
    };

    auto initHudScopeVariant = [&](SkinningPipelineVariant& scopeVariant, const SkinningPipelineVariant& worldVariant, const char* cacheName) {
        if (!worldVariant.pipeline || !state.hudScopePS || !state.hudScopeLayout)
            return;
        scopeVariant.vs = worldVariant.vs;
        scopeVariant.inputLayout = worldVariant.inputLayout;
        auto pipeDesc = buildPipelineDesc(worldVariant.vs, worldVariant.inputLayout, state.hudScopePS);
        pipeDesc.bindingLayouts[0] = state.hudScopeLayout;
        pipeDesc.renderState.blendState.targets[0].setBlendEnable(true);
        pipeDesc.renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::OneMinusSrcAlpha);
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        scopeVariant.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
    };

    auto mdiPsResult = shaderLoader->LoadPixelShader("bindless_skinned_mdi", "main");
    auto mdiVsForReflection = shaderLoader->LoadVertexShader("bindless_skinned_mdi", "main");
    if (mdiPsResult.handle && mdiVsForReflection.handle) {
        state.mdiPS = mdiPsResult.handle;
        state.mdiLayout = cache.GetOrCreateBindingLayoutFromReflection("SkinningPass_MDI_FullLit_v3",
            *mdiVsForReflection.reflection, *mdiPsResult.reflection, nvDevice);
    }

    auto initMDIVariant = [&](SkinningPipelineVariant& variant, const char* shaderName, const char* cacheName,
                              const nvrhi::VertexAttributeDesc* baseAttribs, u32 baseAttrCount) {
        if (!state.mdiPS || !state.mdiLayout)
            return;
        auto vsResult = shaderLoader->LoadVertexShader(shaderName, "main");
        if (!vsResult.handle)
            return;

        nvrhi::VertexAttributeDesc attribs[8];
        for (u32 i = 0; i < baseAttrCount; ++i)
            attribs[i] = baseAttribs[i];
        attribs[baseAttrCount] = nvrhi::VertexAttributeDesc()
            .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
            .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true);

        variant.vs = vsResult.handle;
        variant.inputLayout = nvDevice->createInputLayout(attribs, baseAttrCount + 1, variant.vs);
        auto pipeDesc = buildPipelineDesc(variant.vs, variant.inputLayout, state.mdiPS);
        pipeDesc.bindingLayouts[0] = state.mdiLayout;
        variant.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
    };

    {
        constexpr u32 stride = 24;
        nvrhi::VertexAttributeDesc attribs[] = {
            nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA16_SNORM).setOffset(0).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(8).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(12).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG16_SNORM).setOffset(20).setElementStride(stride),
        };
        initVariant(state.nonHQ, "bindless_skinned", "SkinningPass_nonHQ_IBL_CSMLadder_v2", attribs, 5);
        initMDIVariant(state.mdiNonHQ, "bindless_skinned_mdi", "SkinningPass_mdi_nonHQ_FullLit_v3", attribs, 5);
    }

    {
        constexpr u32 stride = 36;
        nvrhi::VertexAttributeDesc attribs[] = {
            nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
        };
        initVariant(state.hq1w, "bindless_skinned_hq", "SkinningPass_hq1w_IBL_CSMLadder_v2", attribs, 5);
        initMDIVariant(state.mdiHQ1w, "bindless_skinned_hq_mdi", "SkinningPass_mdi_hq1w_FullLit_v3", attribs, 5);
    }

    {
        constexpr u32 stride = 40;
        nvrhi::VertexAttributeDesc attribs[] = {
            nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(36).setElementStride(stride),
        };
        initVariant(state.hq4w, "bindless_skinned_4w", "SkinningPass_hq4w_IBL_CSMLadder_v2", attribs, 6);
        initMDIVariant(state.mdiHQ4w, "bindless_skinned_4w_mdi", "SkinningPass_mdi_hq4w_FullLit_v3", attribs, 6);
    }

    {
        constexpr u32 stride = 44;
        nvrhi::VertexAttributeDesc attribs[] = {
            nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
        };
        initVariant(state.hq2w, "bindless_skinned_2w", "SkinningPass_hq2w_IBL_CSMLadder_v2", attribs, 5);
        initMDIVariant(state.mdiHQ2w, "bindless_skinned_2w_mdi", "SkinningPass_mdi_hq2w_FullLit_v3", attribs, 5);
    }

    {
        constexpr u32 stride = 44;
        nvrhi::VertexAttributeDesc attribs[] = {
            nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
            nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
        };
        initVariant(state.hq3w, "bindless_skinned_3w", "SkinningPass_hq3w_IBL_CSMLadder_v2", attribs, 5);
        initMDIVariant(state.mdiHQ3w, "bindless_skinned_3w_mdi", "SkinningPass_mdi_hq3w_FullLit_v3", attribs, 5);
    }

    initHudVariant(state.hudNonHQ, state.nonHQ, "SkinningPass_hud_nonHQ_Opaque_v3");
    initHudVariant(state.hudHQ1w, state.hq1w, "SkinningPass_hud_hq1w_Opaque_v3");
    initHudVariant(state.hudHQ2w, state.hq2w, "SkinningPass_hud_hq2w_Opaque_v3");
    initHudVariant(state.hudHQ3w, state.hq3w, "SkinningPass_hud_hq3w_Opaque_v3");
    initHudVariant(state.hudHQ4w, state.hq4w, "SkinningPass_hud_hq4w_Opaque_v3");

    initHudScopeVariant(state.hudScopeNonHQ, state.nonHQ, "SkinningPass_hudScope_nonHQ");
    initHudScopeVariant(state.hudScopeHQ1w, state.hq1w, "SkinningPass_hudScope_hq1w");
    initHudScopeVariant(state.hudScopeHQ2w, state.hq2w, "SkinningPass_hudScope_hq2w");
    initHudScopeVariant(state.hudScopeHQ3w, state.hq3w, "SkinningPass_hudScope_hq3w");
    initHudScopeVariant(state.hudScopeHQ4w, state.hq4w, "SkinningPass_hudScope_hq4w");

    // Skinned PatchList tessellation is NOT created on Apple/MoltenVK: Metal's
    // tessellation temp buffers have caused IOGPU kernel panics
    // (IOGPUGroupMemory::remove_memory_object). Keep the variant slots empty so
    // SelectSkinnedTessPipeline always returns null.
#if !defined(XR_PLATFORM_APPLE)
    {
        auto hs = shaderLoader->LoadHullShader("bindless_skinned_tess", "main");
        auto ds = shaderLoader->LoadDomainShader("bindless_skinned_tess", "main");
        if (hs.handle && ds.handle)
        {
            state.tessHS = hs.handle;
            state.tessDS = ds.handle;

            auto initTessVariant = [&](SkinningPipelineVariant& tessVar,
                                       const SkinningPipelineVariant& src,
                                       const char* cacheName) {
                if (!src.pipeline || !src.vs || !src.inputLayout)
                    return;
                tessVar.vs = src.vs;
                tessVar.inputLayout = src.inputLayout;
                auto pipeDesc = buildPipelineDesc(src.vs, src.inputLayout);
                pipeDesc.HS = state.tessHS;
                pipeDesc.DS = state.tessDS;
                pipeDesc.primType = nvrhi::PrimitiveType::PatchList;
                pipeDesc.patchControlPoints = 3;
                pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                tessVar.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
            };

            initTessVariant(state.tessNonHQ, state.nonHQ, "SkinningPass_tess_nonHQ_v1");
            initTessVariant(state.tessHQ1w, state.hq1w, "SkinningPass_tess_hq1w_v1");
            initTessVariant(state.tessHQ2w, state.hq2w, "SkinningPass_tess_hq2w_v1");
            initTessVariant(state.tessHQ3w, state.hq3w, "SkinningPass_tess_hq3w_v1");
            initTessVariant(state.tessHQ4w, state.hq4w, "SkinningPass_tess_hq4w_v1");

            if (state.tessNonHQ.pipeline || state.tessHQ1w.pipeline || state.tessHQ4w.pipeline)
                Msg("* [SkinningPass] Skinned tessellation pipelines ready");
            else
                Msg("! [SkinningPass] Skinned tess HS/DS loaded but PSO create failed");
        }
        else
        {
            Msg("! [SkinningPass] bindless_skinned_tess HS/DS missing — skinned tess disabled");
        }
    }
#else
    Msg("* [SkinningPass] Skinned tessellation disabled on Apple/MoltenVK (IOGPU safety)");
#endif

    {
        auto depthPsResult = shaderLoader->LoadPixelShader("bindless_skinned_depth", "main");
        if (depthPsResult.handle && skinnedVsForReflection.handle)
        {
            state.depthPS = depthPsResult.handle;
            state.depthLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "SkinningDepthPass", *skinnedVsForReflection.reflection, *depthPsResult.reflection, nvDevice);

            nvrhi::FramebufferInfoEx depthFb;
            depthFb.depthFormat = nvrhi::Format::D32;

            auto initDepthVariant = [&](SkinningPipelineVariant& depthVar,
                                        const SkinningPipelineVariant& src,
                                        const char* cacheName) {
                if (!state.depthPS || !state.depthLayout || !src.vs || !src.inputLayout)
                    return;
                depthVar.vs = src.vs;
                depthVar.inputLayout = src.inputLayout;
                nvrhi::GraphicsPipelineDesc pipeDesc;
                pipeDesc.VS = src.vs;
                pipeDesc.PS = state.depthPS;
                pipeDesc.inputLayout = src.inputLayout;
                if (bindlessLayout)
                    pipeDesc.bindingLayouts = {state.depthLayout, bindlessLayout};
                else
                    pipeDesc.bindingLayouts = {state.depthLayout};
                pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                pipeDesc.renderState.depthStencilState.depthTestEnable = true;
                pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
                pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                pipeDesc.renderState.rasterState.frontCounterClockwise = false;
                pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
                depthVar.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, depthFb, nvDevice);
            };

            initDepthVariant(state.depthNonHQ, state.nonHQ, "SkinningDepth_nonHQ_v1");
            initDepthVariant(state.depthHQ1w, state.hq1w, "SkinningDepth_hq1w_v1");
            initDepthVariant(state.depthHQ2w, state.hq2w, "SkinningDepth_hq2w_v1");
            initDepthVariant(state.depthHQ3w, state.hq3w, "SkinningDepth_hq3w_v1");
            initDepthVariant(state.depthHQ4w, state.hq4w, "SkinningDepth_hq4w_v1");
            if (state.depthNonHQ.pipeline || state.depthHQ1w.pipeline || state.depthHQ4w.pipeline)
                Msg("* [SkinningPass] Skinned depth pipelines ready");
            else
                Msg("! [SkinningPass] Skinned depth PSO create failed");
        }
        else
        {
            Msg("! [SkinningPass] bindless_skinned_depth missing — skinned depth disabled");
        }
    }

    {
        auto velPs = shaderLoader->LoadPixelShader("bindless_skinned_velocity", "main");
        if (velPs.handle)
        {
            state.velocityPS = velPs.handle;
            nvrhi::FramebufferInfoEx velFb;
            velFb.colorFormats = { nvrhi::Format::RG16_FLOAT };
            velFb.depthFormat = nvrhi::Format::D32;

            auto initVelVariant = [&](SkinningPipelineVariant& var, const char* vsName,
                                      const nvrhi::VertexAttributeDesc* attribs, u32 attrCount,
                                      const char* cacheName, bool mdi) {
                auto vsResult = shaderLoader->LoadVertexShader(vsName, "main");
                if (!vsResult.handle || !vsResult.reflection || !velPs.reflection)
                    return;
                if (!state.velocityLayout && !mdi)
                    state.velocityLayout = cache.GetOrCreateBindingLayoutFromReflection(
                        "SkinningVelocity_v1", *vsResult.reflection, *velPs.reflection, nvDevice);
                if (!state.velocityMdiLayout && mdi)
                    state.velocityMdiLayout = cache.GetOrCreateBindingLayoutFromReflection(
                        "SkinningVelocityMDI_v1", *vsResult.reflection, *velPs.reflection, nvDevice);
                nvrhi::IBindingLayout* layout = mdi ? state.velocityMdiLayout.Get() : state.velocityLayout.Get();
                if (!layout)
                    return;

                nvrhi::VertexAttributeDesc localAttribs[8];
                u32 totalAttrs = attrCount;
                for (u32 i = 0; i < attrCount; ++i)
                    localAttribs[i] = attribs[i];
                if (mdi) {
                    localAttribs[attrCount] = nvrhi::VertexAttributeDesc()
                        .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
                        .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true);
                    totalAttrs = attrCount + 1;
                }

                var.vs = vsResult.handle;
                var.inputLayout = nvDevice->createInputLayout(localAttribs, totalAttrs, var.vs);
                nvrhi::GraphicsPipelineDesc pipeDesc;
                pipeDesc.VS = var.vs;
                pipeDesc.PS = state.velocityPS;
                pipeDesc.inputLayout = var.inputLayout;
                pipeDesc.bindingLayouts = { layout };
                pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                pipeDesc.renderState.depthStencilState.depthTestEnable = true;
                pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
                pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                pipeDesc.renderState.rasterState.frontCounterClockwise = false;
                pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
                pipeDesc.renderState.blendState.targets[0]
                    .setBlendEnable(false)
                    .setColorWriteMask(nvrhi::ColorMask::Red | nvrhi::ColorMask::Green);
                var.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, velFb, nvDevice);
            };

            {
                constexpr u32 stride = 24;
                nvrhi::VertexAttributeDesc attribs[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA16_SNORM).setOffset(0).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(8).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(12).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG16_SNORM).setOffset(20).setElementStride(stride),
                };
                initVelVariant(state.velNonHQ, "bindless_skinned_velocity", attribs, 5, "SkinningVelocity_nonHQ_v1", false);
                initVelVariant(state.velMdiNonHQ, "bindless_skinned_mdi_velocity", attribs, 5, "SkinningVelocity_mdi_nonHQ_v1", true);
            }
            {
                constexpr u32 stride = 36;
                nvrhi::VertexAttributeDesc attribs[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
                };
                initVelVariant(state.velHQ1w, "bindless_skinned_hq_velocity", attribs, 5, "SkinningVelocity_hq1w_v1", false);
                initVelVariant(state.velMdiHQ1w, "bindless_skinned_hq_mdi_velocity", attribs, 5, "SkinningVelocity_mdi_hq1w_v1", true);
            }
            {
                constexpr u32 stride = 40;
                nvrhi::VertexAttributeDesc attribs[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(36).setElementStride(stride),
                };
                initVelVariant(state.velHQ4w, "bindless_skinned_4w_velocity", attribs, 6, "SkinningVelocity_hq4w_v1", false);
                initVelVariant(state.velMdiHQ4w, "bindless_skinned_4w_mdi_velocity", attribs, 6, "SkinningVelocity_mdi_hq4w_v1", true);
            }
            {
                constexpr u32 stride = 44;
                nvrhi::VertexAttributeDesc attribs[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
                };
                initVelVariant(state.velHQ2w, "bindless_skinned_2w_velocity", attribs, 5, "SkinningVelocity_hq2w_v1", false);
                initVelVariant(state.velMdiHQ2w, "bindless_skinned_2w_mdi_velocity", attribs, 5, "SkinningVelocity_mdi_hq2w_v1", true);
                initVelVariant(state.velHQ3w, "bindless_skinned_3w_velocity", attribs, 5, "SkinningVelocity_hq3w_v1", false);
                initVelVariant(state.velMdiHQ3w, "bindless_skinned_3w_mdi_velocity", attribs, 5, "SkinningVelocity_mdi_hq3w_v1", true);
            }

            if (state.velNonHQ.pipeline || state.velHQ1w.pipeline || state.velHQ4w.pipeline)
                Msg("* [SkinningPass] Skinned velocity pipelines ready");
            else
                Msg("! [SkinningPass] Skinned velocity PSO create failed");
        }
        else
        {
            Msg("! [SkinningPass] bindless_skinned_velocity missing — skinned velocity disabled");
        }
    }

    state.initialized = true;
    Msg("* [SkinningPass] Pipeline initialization complete");
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE SELECTION HELPER
// ═══════════════════════════════════════════════════════════════════════════
// Render mode enum values from CSkeletonX (must match SkeletonX.h)
enum {
    RM_SKINNING_SOFT = 0,
    RM_SINGLE = 1,
    RM_SINGLE_HQ = 2,
    RM_SKINNING_1B = 3,
    RM_SKINNING_1B_HQ = 4,
    RM_SKINNING_2B = 5,
    RM_SKINNING_2B_HQ = 6,
    RM_SKINNING_3B = 7,
    RM_SKINNING_3B_HQ = 8,
    RM_SKINNING_4B = 9,
    RM_SKINNING_4B_HQ = 10
};

static nvrhi::IGraphicsPipeline* SelectSkinnedPipelineFromVariants(
    const SkinningPipelineVariant& nonHQ, const SkinningPipelineVariant& hq1w,
    const SkinningPipelineVariant& hq2w, const SkinningPipelineVariant& hq3w,
    const SkinningPipelineVariant& hq4w, u32 vertexStride, u16 renderMode)
{
    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ)
        return hq3w.pipeline.Get();
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ)
        return hq2w.pipeline.Get();
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ)
        return hq4w.pipeline.Get();
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ)
        return hq1w.pipeline.Get();
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE)
        return nonHQ.pipeline.Get();

    if (vertexStride == 36)
        return hq1w.pipeline.Get();
    if (vertexStride == 40)
        return hq4w.pipeline.Get();
    if (vertexStride == 44)
        return hq2w.pipeline.Get();
    if (vertexStride == 24)
        return nonHQ.pipeline.Get();

    if (vertexStride >= 36)
        return hq1w.pipeline.Get();
    return nonHQ.pipeline.Get();
}

static nvrhi::IGraphicsPipeline* SelectSkinnedPipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    return SelectSkinnedPipelineFromVariants(state.nonHQ, state.hq1w, state.hq2w, state.hq3w, state.hq4w, vertexStride, renderMode);
}

static nvrhi::IGraphicsPipeline* SelectSkinnedDepthPipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    return SelectSkinnedPipelineFromVariants(
        state.depthNonHQ, state.depthHQ1w, state.depthHQ2w, state.depthHQ3w, state.depthHQ4w,
        vertexStride, renderMode);
}

static nvrhi::IGraphicsPipeline* SelectSkinnedTessPipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    return SelectSkinnedPipelineFromVariants(state.tessNonHQ, state.tessHQ1w, state.tessHQ2w, state.tessHQ3w, state.tessHQ4w, vertexStride, renderMode);
}

static nvrhi::IGraphicsPipeline* SelectHUDSkinnedPipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    auto* hudPipe = SelectSkinnedPipelineFromVariants(state.hudNonHQ, state.hudHQ1w, state.hudHQ2w, state.hudHQ3w, state.hudHQ4w, vertexStride, renderMode);
    return hudPipe ? hudPipe : SelectSkinnedPipeline(state, vertexStride, renderMode);
}

static nvrhi::IGraphicsPipeline* SelectHUDScopePipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    auto* pipe = SelectSkinnedPipelineFromVariants(
        state.hudScopeNonHQ, state.hudScopeHQ1w, state.hudScopeHQ2w, state.hudScopeHQ3w, state.hudScopeHQ4w,
        vertexStride, renderMode);
    return pipe ? pipe : SelectHUDSkinnedPipeline(state, vertexStride, renderMode);
}

static bool IsScopeBatch(const GeometryBatch& batch)
{
    if (!batch.visual || !batch.visual->shaderName.size())
        return false;
    const char* sh = batch.visual->shaderName.c_str();
    return strstr(sh, "scope") != nullptr || strstr(sh, "lense") != nullptr || strstr(sh, "lens") != nullptr;
}

static u32 GetSkinnedVertexFormatID(u16 renderMode, u32 vertexStride)
{
    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ) return VF_SKINNED_HQ3W;
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ) return VF_SKINNED_HQ2W;
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ) return VF_SKINNED_HQ4W;
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ) return VF_SKINNED_HQ1W;
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE) return VF_SKINNED_NONHQ;
    if (vertexStride == 36) return VF_SKINNED_HQ1W;
    if (vertexStride == 40) return VF_SKINNED_HQ4W;
    if (vertexStride == 44) return VF_SKINNED_HQ2W;
    return VF_SKINNED_NONHQ;
}

static nvrhi::IInputLayout* GetSkinnedInputLayout(const SkinningPassState& state, u32 fmt)
{
    switch (fmt) {
    case VF_SKINNED_HQ1W: return state.hq1w.inputLayout.Get();
    case VF_SKINNED_HQ4W: return state.hq4w.inputLayout.Get();
    case VF_SKINNED_HQ2W: return state.hq2w.inputLayout.Get();
    case VF_SKINNED_HQ3W: return state.hq3w.inputLayout.Get();
    default: return state.nonHQ.inputLayout.Get();
    }
}

static const SkinningPipelineVariant* SelectMDISkinnedVariant(const SkinningPassState& state, u32 fmt)
{
    switch (fmt) {
    case VF_SKINNED_NONHQ: return &state.mdiNonHQ;
    case VF_SKINNED_HQ1W: return &state.mdiHQ1w;
    case VF_SKINNED_HQ2W: return &state.mdiHQ2w;
    case VF_SKINNED_HQ3W: return &state.mdiHQ3w;
    case VF_SKINNED_HQ4W: return &state.mdiHQ4w;
    default: return nullptr;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKELETON BONE OFFSET HELPER
// ═══════════════════════════════════════════════════════════════════════════
// Extracts parent skeleton from batch and uploads bones to global buffer.
// Returns offset into GPUCullingManager's global bone buffer.
static u32 GetSkeletonBoneOffset(
    nvrhi::ICommandList* cmdList,
    GPUCullingManager& gpuCullMgr,
    const GeometryBatch& batch)
{
    CKinematics* parent = nullptr;
    u32 visualType = batch.visual ? batch.visual->getType() : 0;

    if (visualType == MT_SKELETON_GEOMDEF_ST) {
        parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
        parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
    }

    if (!parent)
        return 0;

    return gpuCullMgr.GetOrUploadSkeleton(cmdList, parent);
}

static decals::OverlayManager::SplatRange GetSplatRange(const GeometryBatch& batch, decals::OverlayManager* overlayMgr)
{
    if (!overlayMgr)
        return { 0, 0 };

    CKinematics* parent = nullptr;
    u32 visualType = batch.visual ? batch.visual->getType() : 0;

    if (visualType == MT_SKELETON_GEOMDEF_ST)
        parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    else if (visualType == MT_SKELETON_GEOMDEF_PM)
        parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();

    if (!parent)
        return { 0, 0 };

    return overlayMgr->GetSplatRange(parent);
}

// ═══════════════════════════════════════════════════════════════════════════

struct SkinnedPhaseContext {
    nvrhi::IBindingSet* bindingSet = nullptr;
    nvrhi::IBuffer* dynTransformsCB = nullptr;
    nvrhi::IBuffer* materialIdCB = nullptr;
    nvrhi::IFramebuffer* framebuffer = nullptr;
    nvrhi::IDescriptorTable* bindlessTable = nullptr;
    nvrhi::IBindingLayout* bindlessLayout = nullptr;
    nvrhi::Viewport viewport;
    nvrhi::Rect scissor;
    bool isHUD = false;
};

static void BindSkinnedLightingTextures(
    framegraph::BindingSetBuilder& bsb,
    nvrhi::IDevice* nvDevice,
    nvrhi::ITexture* const* shadowCascades,
    nvrhi::ITexture* hudShadowMap,
    nvrhi::ITexture* contactDepth,
    nvrhi::ITexture* contactHistory,
    nvrhi::ITexture* envSky0,
    nvrhi::ITexture* envSky1,
    bool isHUD,
    nvrhi::ITexture* localShadowAtlas = nullptr,
    nvrhi::ITexture* const* shadowHZB = nullptr,
    nvrhi::ITexture* shadowMask = nullptr,
    nvrhi::ITexture* localShadowESM = nullptr)
{
    auto& cache = framegraph::GetPassResourceCache();
    nvrhi::ITexture* dummy2D = cache.GetDummyShadowMap2D(nvDevice);
    nvrhi::ITexture* dummyArray = cache.GetDummyShadowMap(nvDevice);
    static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
    for (u32 i = 0; i < 3; ++i)
    {
        nvrhi::ITexture* t = (shadowCascades && shadowCascades[i]) ? shadowCascades[i] : dummy2D;
        if (t)
            bsb.Texture(kNames[i], t);
    }
    if (isHUD)
    {
        nvrhi::ITexture* hudTex = hudShadowMap ? hudShadowMap : dummyArray;
        if (hudTex)
            bsb.Texture("g_HUDShadowMap", hudTex);
    }
    (void)contactDepth;
    nvrhi::ITexture* hist = contactHistory ? contactHistory : cache.GetDummyContactHistory(nvDevice);
    if (hist)
        bsb.Texture("g_ContactHistory", hist);
    nvrhi::ITexture* localAtlas = localShadowAtlas ? localShadowAtlas : dummyArray;
    if (localAtlas)
        bsb.Texture("g_LocalShadowAtlas", localAtlas);
    nvrhi::ITexture* localEsm = localShadowESM ? localShadowESM : cache.GetDummyLocalShadowESM(nvDevice);
    if (localEsm)
        bsb.Texture("g_LocalShadowESM", localEsm);
    static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
    nvrhi::ITexture* dummyHzb = cache.GetDummyContactDepth(nvDevice);
    for (u32 i = 0; i < 3; ++i)
    {
        nvrhi::ITexture* hz = (shadowHZB && shadowHZB[i]) ? shadowHZB[i] : dummyHzb;
        if (hz)
            bsb.Texture(kHzb[i], hz);
    }
    (void)shadowMask;
    nvrhi::ITexture* sky0 = envSky0 ? envSky0 : cache.GetDummyCubeMap(nvDevice);
    nvrhi::ITexture* sky1 = envSky1 ? envSky1 : cache.GetDummyCubeMap(nvDevice);
    if (sky0)
        bsb.Texture("s_env0", sky0);
    if (sky1)
        bsb.Texture("s_env1", sky1);
    BindIBLResources(bsb, GetCurrentIBLBindResources(), nvDevice);
}

static SkinnedPhaseContext BuildSkinnedPhaseContext(
    const SkinningPassState& state,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::IBuffer* dynTransformsCB,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* materialIdCB,
    nvrhi::IBuffer* globalBoneBuffer,
    nvrhi::IDescriptorTable* bindlessTable,
    nvrhi::IBindingLayout* bindlessLayout,
    nvrhi::IBuffer* splatBuffer,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    bool isHUD,
    nvrhi::ITexture* const* shadowCascades,
    nvrhi::ITexture* hudShadowMap,
    nvrhi::ITexture* contactDepth,
    nvrhi::ITexture* contactHistory,
    nvrhi::ITexture* envSky0,
    nvrhi::ITexture* envSky1,
    nvrhi::ITexture* localShadowAtlas = nullptr,
    nvrhi::ITexture* const* shadowHZB = nullptr,
    nvrhi::ITexture* shadowMask = nullptr,
    nvrhi::ITexture* localShadowESM = nullptr)
{
    using namespace fg;
    using namespace fg::bindless;

    SkinnedPhaseContext ctx;
    ctx.dynTransformsCB = dynTransformsCB;
    ctx.materialIdCB = materialIdCB;
    ctx.framebuffer = framebuffer;
    ctx.bindlessTable = bindlessTable;
    ctx.bindlessLayout = bindlessLayout;
    ctx.viewport = viewport;
    ctx.scissor = scissor;
    ctx.isHUD = isHUD;

    if (!globalBoneBuffer)
        return ctx;

    auto& matBuffer = MaterialBuffer::Instance();
    auto& cache = framegraph::GetPassResourceCache();

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsReflection = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
    const char* psShaderName = (isHUD && state.hudPS) ? "bindless_skinned_hud" : "bindless_skinned";
    auto* psReflection = shaderLoader->GetCachedReflection(psShaderName, ".ps");
    auto activeLayout = isHUD ? state.hudLayout : state.layout;

    framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, isHUD ? "Skinning.HUD" : "Skinning");
    bsb.ConstantBuffer("dynamic_transforms", dynTransformsCB);
    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
    bsb.BufferSRV("g_BoneMatrices", globalBoneBuffer);
    bsb.ConstantBuffer("SkinnedMaterialCB", materialIdCB);
    BindBindlessMaterialTables(bsb);
    BindPaintSplatBuffer(bsb, nvDevice, splatBuffer);
    bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
    if (ClusteredLightManager::Instance().GetShadowDataBuffer())
        bsb.BufferSRV("g_ShadowData", ClusteredLightManager::Instance().GetShadowDataBuffer());
    bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
    bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
    BindSkinnedLightingTextures(bsb, nvDevice, shadowCascades, hudShadowMap, contactDepth, contactHistory, envSky0, envSky1, isHUD, localShadowAtlas, shadowHZB, shadowMask, localShadowESM);

    ctx.bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), activeLayout, nvDevice);
    return ctx;
}

static void DrawSkinnedBatch(
    const SkinningPassState& state,
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    const SkinnedPhaseContext& ctx,
    const GeometryBatch& batch,
    const Fmatrix& worldMatrix,
    u32 skeletonBoneOffset,
    decals::OverlayManager::SplatRange splatRange = {0, 0},
    nvrhi::IBuffer* indirectArgs = nullptr,
    u32 indirectOffset = 0)
{
    using namespace fg;
    using namespace fg::bindless;

    if (!batch.vertexBuffer || !batch.indexBuffer || !ctx.bindingSet)
        return;

    DynamicTransforms dynTransData = {};
    FillDynamicTransforms(dynTransData, worldMatrix);
    cmdList->writeBuffer(ctx.dynTransformsCB, &dynTransData, sizeof(dynTransData));

    SkinnedMaterialCB matIdData = {};
    matIdData.materialID = batch.bindlessMaterialID;
    matIdData.skeletonBoneOffset = skeletonBoneOffset;
    matIdData.splatOffset = splatRange.offset;
    matIdData.splatCount = splatRange.count;
    cmdList->writeBuffer(ctx.materialIdCB, &matIdData, sizeof(matIdData));

    u32 variantIdx = MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
    const ShaderVariantDesc* variant = nullptr;
    u32 passCount = 1;
    if (variantIdx > 0) {
        variant = ShaderVariantRegistry::Instance().GetVariantByIndex(variantIdx);
        if (variant)
            passCount = variant->GetPassCount();
    }

    for (u32 p = 0; p < passCount; p++) {
        nvrhi::IGraphicsPipeline* pipeline;
        if (variant) {
            u32 fmt = GetSkinnedVertexFormatID(batch.skinningRenderMode, batch.vertexStride);
            pipeline = VariantPSOCache::Instance().GetOrCreatePSO(
                nvDevice, ctx.framebuffer, variantIdx, *variant, p, fmt,
                GetSkinnedInputLayout(state, fmt), state.layout, ctx.bindlessLayout);
        } else {
            // Skinned PatchList tess is disabled on Apple (IOGPU kernel panic).
            // On other platforms it still requires r4_enable_tessellation + tessMethod.
            nvrhi::IGraphicsPipeline* tessPipe = nullptr;
#if !defined(XR_PLATFORM_APPLE)
            constexpr u32 kMaxSkinnedTessIndices = 8192;
            const bool tessOn = !ctx.isHUD &&
                ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION) &&
                batch.indexCount > 0 && batch.indexCount <= kMaxSkinnedTessIndices;
            if (tessOn)
            {
                const auto* mat = MaterialBuffer::Instance().GetMaterial(batch.bindlessMaterialID);
                if (mat && mat->tessMethod != 0)
                    tessPipe = SelectSkinnedTessPipeline(state, batch.vertexStride, batch.skinningRenderMode);
            }
#endif
            if (tessPipe)
            {
                pipeline = tessPipe;
                indirectArgs = nullptr;
            }
            else if (ctx.isHUD)
            {
                if (IsScopeBatch(batch) && state.hudScopePS)
                    pipeline = SelectHUDScopePipeline(state, batch.vertexStride, batch.skinningRenderMode);
                else
                    pipeline = SelectHUDSkinnedPipeline(state, batch.vertexStride, batch.skinningRenderMode);
            }
            else
            {
                pipeline = SelectSkinnedPipeline(state, batch.vertexStride, batch.skinningRenderMode);
            }
        }
        if (!pipeline)
            continue;

        nvrhi::GraphicsState gfxState;
        gfxState.pipeline = pipeline;
        gfxState.framebuffer = ctx.framebuffer;
        gfxState.bindings = { ctx.bindingSet };
        if (ctx.bindlessTable)
            gfxState.addBindingSet(ctx.bindlessTable);
        gfxState.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
        gfxState.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
        gfxState.viewport.addViewport(ctx.viewport);
        gfxState.viewport.addScissorRect(ctx.scissor);
        gfxState.indirectParams = indirectArgs;

        cmdList->setGraphicsState(gfxState);
        if (indirectArgs) {
            cmdList->drawIndexedIndirect(indirectOffset, 1);
        } else {
            cmdList->drawIndexed(
                nvrhi::DrawArguments()
                    .setVertexCount(batch.indexCount)
                    .setStartIndexLocation(batch.startIndex)
                    .setStartVertexLocation(batch.baseVertex));
        }
    }
}

framegraph::VirtualResourceHandle setupSkinnedDepthPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    decals::OverlayManager* overlayMgr)
{
    using namespace framegraph;

    if (!device || !state || !depthInput.is_valid() || !geometry)
        return depthInput;

    {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeSkinningResources(device, fbInfo, *state);
    }

    if (!state->depthLayout || !state->depthPS)
        return depthInput;

    struct DepthPassData
    {
        VirtualResourceHandle depth;
        fg::RenderDevice* device = nullptr;
        const GeometryCollector* geometry = nullptr;
        MaterialCache* materialCache = nullptr;
        fg::GPUCullingManager* gpuCulling = nullptr;
        SkinningPassState* passState = nullptr;
        decals::OverlayManager* overlayMgr = nullptr;
        u32 width = 0, height = 0;
    };

    auto& passData = fg.addCallbackPass<DepthPassData>(
        "SkinnedDepth",
        [&, width, height, geometry, materialCache, gpuCulling, state, overlayMgr, depthInput](
            FrameGraph& builder, PassHandle passHandle, DepthPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.gpuCulling = gpuCulling;
            data.passState = state;
            data.overlayMgr = overlayMgr;
            data.width = width;
            data.height = height;
        },
        [](const DepthPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            if (!data.passState || !data.geometry || !data.device)
                return;

            u32 worldSkinnedCount = 0;
            for (const auto& batch : data.geometry->GetBatches())
            {
                if (batch.isSkinned)
                    ++worldSkinnedCount;
            }
            if (worldSkinnedCount == 0)
                return;

            auto* depthRT = fgGraph.GetPhysicalTexture(data.depth);
            if (!depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            GPUCullingManager* gpuCullMgr = data.gpuCulling;
            if (!gpuCullMgr || !gpuCullMgr->GetGlobalBoneBuffer())
                return;

            if (data.materialCache)
                data.materialCache->FinalizePendingMaterials(ctx);
            MaterialBuffer::Instance().Upload(ctx);
            if (data.overlayMgr)
                data.overlayMgr->UploadSplats(cmdList);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("SkinnedDepth", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            auto dynTransformsCB = cache.GetOrCreateVolatileCB(
                "SkinnedDepth", "DynTransforms", sizeof(DynamicTransforms), data.device, 1024 * 8);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto materialIdCB = cache.GetOrCreateVolatileCB(
                "SkinnedDepth", "MaterialId", sizeof(SkinnedMaterialCB), data.device, 1024 * 8);
            if (!dynTransformsCB || !staticGlobalsCB || !materialIdCB)
                return;

            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_skinned_depth", ".ps");
            if (!vsReflection || !psReflection || !data.passState->depthLayout)
                return;

            nvrhi::IBuffer* splatBuffer = data.overlayMgr ? data.overlayMgr->GetSplatBuffer() : nullptr;
            BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "SkinnedDepth");
            bsb.ConstantBuffer("dynamic_transforms", dynTransformsCB);
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.BufferSRV("g_BoneMatrices", gpuCullMgr->GetGlobalBoneBuffer());
            bsb.ConstantBuffer("SkinnedMaterialCB", materialIdCB);
            BindBindlessMaterialTables(bsb);
            BindPaintSplatBuffer(bsb, nvDevice, splatBuffer);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->depthLayout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(data.width),
                0.0f, static_cast<float>(data.height),
                0.0f, 1.0f);
            nvrhi::Rect scissor(data.width, data.height);

            for (const auto& batch : data.geometry->GetBatches())
            {
                if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
                    continue;

                nvrhi::IGraphicsPipeline* pipeline = SelectSkinnedDepthPipeline(
                    *data.passState, batch.vertexStride, batch.skinningRenderMode);
                if (!pipeline)
                    continue;

                u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                auto sr = GetSplatRange(batch, data.overlayMgr);

                DynamicTransforms dynTransData = {};
                FillDynamicTransforms(dynTransData, batch.worldMatrix);
                cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

                SkinnedMaterialCB matIdData = {};
                matIdData.materialID = batch.bindlessMaterialID;
                matIdData.skeletonBoneOffset = boneOffset;
                matIdData.splatOffset = sr.offset;
                matIdData.splatCount = sr.count;
                cmdList->writeBuffer(materialIdCB, &matIdData, sizeof(matIdData));

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = {bindingSet};
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                gfxState.vertexBuffers = {{batch.vertexBuffer, 0, 0}};
                gfxState.indexBuffer = {batch.indexBuffer, nvrhi::Format::R16_UINT, 0};
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);
                cmdList->drawIndexed(
                    nvrhi::DrawArguments()
                        .setVertexCount(batch.indexCount)
                        .setStartIndexLocation(batch.startIndex)
                        .setStartVertexLocation(batch.baseVertex));
            }
        });

    return passData.depth;
}

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling,
    framegraph::VirtualResourceHandle skinnedDrawArgs,
    SkinningPassState* state,
    decals::OverlayManager* overlayMgr,
    nvrhi::ITexture* shadowMapArray,
    framegraph::VirtualResourceHandle shadowMapHandle,
    nvrhi::ITexture* contactDepth,
    nvrhi::ITexture* contactHistory,
    nvrhi::ITexture* envSky0,
    nvrhi::ITexture* envSky1,
    nvrhi::ITexture* hudShadowMap,
    nvrhi::ITexture* const* shadowCascadesIn,
    nvrhi::ITexture* localShadowAtlas,
    nvrhi::ITexture* const* shadowHZBIn,
    nvrhi::ITexture* shadowMask,
    nvrhi::ITexture* localShadowESM)
{
    using namespace framegraph;

    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeSkinningResources(device, fbInfo, *state);
    }

    auto& passData = fg.addCallbackPass<SkinningPassData>(
        "Skinning Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA
        // ═══════════════════════════════════════════════════════
        [&, width, height, gpuCulling, skinnedDrawArgs, state, overlayMgr,
         shadowMapArray, shadowMapHandle, contactDepth, contactHistory, envSky0, envSky1, hudShadowMap, shadowCascadesIn, localShadowAtlas, shadowHZBIn, shadowMask, localShadowESM](FrameGraph& builder, PassHandle passHandle, SkinningPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;
            data.gpuCulling = gpuCulling;
            data.passState = state;
            data.overlayMgr = overlayMgr;
            data.shadowMapArray = shadowMapArray;
            for (int i = 0; i < 3; ++i)
            {
                data.shadowCascades[i] = (shadowCascadesIn && shadowCascadesIn[i]) ? shadowCascadesIn[i] : shadowMapArray;
                data.shadowHZB[i] = (shadowHZBIn && shadowHZBIn[i]) ? shadowHZBIn[i] : nullptr;
            }
            data.hudShadowMap = hudShadowMap;
            data.localShadowAtlas = localShadowAtlas;
            data.localShadowESM = localShadowESM;
            data.shadowMask = shadowMask;
            data.contactDepth = contactDepth;
            data.contactHistory = contactHistory;
            data.envSky0 = envSky0;
            data.envSky1 = envSky1;

            if (skinnedDrawArgs.is_valid())
                data.skinnedDrawArgs = passBuilder.read(skinnedDrawArgs, ResourceState::IndirectArgument);
            if (shadowMapHandle.is_valid())
                data.shadowMap = passBuilder.read(shadowMapHandle, ResourceState::ShaderResource);

            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.normal = passBuilder.readWrite(inputs.normal, ResourceState::RenderTarget);
            data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::RenderTarget);
            data.worldPos = passBuilder.readWrite(inputs.worldPos, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(inputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.color;
            data.outputs.normal = data.normal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.worldPos = data.worldPos;
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA
        // ═══════════════════════════════════════════════════════
        [](const SkinningPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            using namespace fg;

            // Check if any skinned batches to render
            bool hasWorldSkinned = data.geometry && !data.geometry->GetBatches().empty();
            bool hasHUDSkinned = data.hudBatches && !data.hudBatches->empty();

            // Count actual skinned batches in world geometry
            u32 worldSkinnedCount = 0;
            if (hasWorldSkinned) {
                for (const auto& batch : data.geometry->GetBatches()) {
                    if (batch.isSkinned) worldSkinnedCount++;
                }
            }

            if (worldSkinnedCount == 0 && !hasHUDSkinned) {
                // Msg("! [SkinningPass] No skinned batches to render");
                return;
            }

            if (!data.passState)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT || !normalRT || !baseColorRT || !worldPosRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.addColorAttachment(normalRT);
            fbDesc.addColorAttachment(baseColorRT);
            fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer("SkinningPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            const auto& rtDesc = colorRT->getDesc();

            GPUCullingManager* gpuCullMgr = data.gpuCulling;

            if (!gpuCullMgr || !gpuCullMgr->GetGlobalBoneBuffer()) {
                Msg("! [SkinningPass] GPUCullingManager bone buffer not available!");
                return;
            }

            // Get global bone buffer for shader binding
            nvrhi::IBuffer* globalBoneBuffer = gpuCullMgr->GetGlobalBoneBuffer();

            auto& cache = framegraph::GetPassResourceCache();
            auto dynTransformsCB = cache.GetOrCreateVolatileCB("SkinningPass", "DynTransforms", sizeof(DynamicTransforms), data.device, 1024 * 8);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("SkinningPass", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto shaderParamsCB = cache.GetOrCreateVolatileCB("SkinningPass", "ShaderParams", sizeof(ShaderParams), data.device, 512);
            auto materialIdCB = cache.GetOrCreateVolatileCB("SkinningPass", "MaterialId", sizeof(SkinnedMaterialCB), data.device, 1024 * 8);

            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            ShaderParams shaderParams = {};
            shaderParams.m_AlphaRef = 0.5f;
            shaderParams.dt_params.set(1.0f, 0.0f, 1.0f, 50.0f);
            cmdList->writeBuffer(shaderParamsCB, &shaderParams, sizeof(shaderParams));

            // Get terrain material buffer (t9) - required by bindless_common.h
            auto& terrainMatBuffer = TerrainMaterialBuffer::Instance();
            if (!terrainMatBuffer.GetBuffer()) {
                Msg("! [SkinningPass] TerrainMaterialBuffer is NULL - cannot render skinned meshes");
                return;
            }

            // Finalize any pending materials (register textures to bindless descriptor heap)
            if (data.materialCache) {
                data.materialCache->FinalizePendingMaterials(ctx);
            }

            // Upload material buffer to GPU
            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            if (data.overlayMgr)
                data.overlayMgr->UploadSplats(cmdList);
            nvrhi::IBuffer* splatBuffer = data.overlayMgr ? data.overlayMgr->GetSplatBuffer() : nullptr;

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

            // Scissor rect (same for both phases)
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            // ═══════════════════════════════════════════════════════
            //  PHASE 1: WORLD SKINNED MESHES (depth [0.0, 1.0])
            // ═══════════════════════════════════════════════════════
            if (worldSkinnedCount > 0) {
                nvrhi::Viewport worldViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 1.0f
                );

                SkinnedPhaseContext worldCtx = BuildSkinnedPhaseContext(
                    *data.passState, nvDevice, framebuffer,
                    dynTransformsCB, staticGlobalsCB, materialIdCB,
                    globalBoneBuffer, bindlessTable, bindlessLayout, splatBuffer,
                    worldViewport, scissor, false,
                    data.shadowCascades, data.hudShadowMap, data.contactDepth, data.contactHistory, data.envSky0, data.envSky1, data.localShadowAtlas,
                    data.shadowHZB, data.shadowMask, data.localShadowESM);

                const bool cullActive = data.skinnedDrawArgs.is_valid()
                    && gpuCullMgr->IsSkinnedCullingEnabled()
                    && gpuCullMgr->GetSkinnedObjectCount() == worldSkinnedCount;
                const bool mdiActive = cullActive && gpuCullMgr->IsSkinnedMDIEnabled();

                if (mdiActive) {
                    auto* shaderLoader = GEnv.Render->GetShaderLoader();
                    auto* mdiVsRefl = shaderLoader->GetCachedReflection("bindless_skinned_mdi", ".vs");
                    auto* mdiPsRefl = shaderLoader->GetCachedReflection("bindless_skinned_mdi", ".ps");
                    nvrhi::IBuffer* drawIndexBuffer = GetOrCreateDrawIndexBuffer("SkinningPass", nvDevice);
                    auto& pools = gpuCullMgr->GetSkinnedPools();
                    auto& matBuffer = MaterialBuffer::Instance();

                    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
                        const auto& bucket = gpuCullMgr->GetSkinnedBucket(f);
                        if (bucket.count == 0)
                            continue;

                        const SkinningPipelineVariant* variant = SelectMDISkinnedVariant(*data.passState, f);
                        nvrhi::IBuffer* poolVB = pools.GetVertexBuffer(f);
                        nvrhi::IBuffer* poolIB = pools.GetIndexBuffer(f);
                        if (!variant || !variant->pipeline || !poolVB || !poolIB
                            || !mdiVsRefl || !mdiPsRefl || !drawIndexBuffer || !data.passState->mdiLayout) {
                            Msg("! [SkinningPass] MDI bucket %u unavailable, %u batches dropped", f, bucket.count);
                            continue;
                        }

                        framegraph::BindingSetBuilder bsb(*mdiVsRefl, *mdiPsRefl, nvDevice, "Skinning.MDI");
                        bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                        bsb.BufferSRV("g_BoneMatrices", globalBoneBuffer);
                        BindBindlessMaterialTables(bsb);
                        BindPaintSplatBuffer(bsb, nvDevice, splatBuffer);
                        bsb.BufferSRV("g_SkinnedRecords", bucket.recordsBuffer);
                        bsb.BufferSRV("g_SkinnedCompactIndices", bucket.compactBatchIndicesBuffer);
                        bsb.BufferSRV("g_SkinnedCompactMaterialIDs", bucket.compactMaterialIDBuffer);
                        bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
    if (ClusteredLightManager::Instance().GetShadowDataBuffer())
        bsb.BufferSRV("g_ShadowData", ClusteredLightManager::Instance().GetShadowDataBuffer());
                        bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                        bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                        BindSkinnedLightingTextures(bsb, nvDevice, data.shadowCascades, data.hudShadowMap, data.contactDepth, data.contactHistory, data.envSky0, data.envSky1, false, data.localShadowAtlas, data.shadowHZB, data.shadowMask, data.localShadowESM);

                        auto& cache = framegraph::GetPassResourceCache();
                        nvrhi::BindingSetHandle mdiBindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->mdiLayout, nvDevice);
                        if (!mdiBindingSet)
                            continue;

                        nvrhi::GraphicsState gfxState;
                        gfxState.pipeline = variant->pipeline;
                        gfxState.framebuffer = framebuffer;
                        gfxState.bindings = { mdiBindingSet };
                        if (bindlessTable)
                            gfxState.addBindingSet(bindlessTable);
                        gfxState.vertexBuffers = { {poolVB, 0, 0}, {drawIndexBuffer, 1, 0} };
                        gfxState.indexBuffer = { poolIB, nvrhi::Format::R16_UINT, 0 };
                        gfxState.viewport.addViewport(worldViewport);
                        gfxState.viewport.addScissorRect(scissor);
                        gfxState.indirectParams = bucket.compactDrawArgsBuffer;
                        gfxState.indirectCountBuffer = bucket.compactCountBuffer;

                        cmdList->setGraphicsState(gfxState);
                        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, bucket.count);
                    }
                }

                nvrhi::IBuffer* residualArgs = cullActive ? gpuCullMgr->GetSkinnedDrawArgsBuffer() : nullptr;
                u32 residualIdx = 0;
                for (const auto& batch : data.geometry->GetBatches()) {
                    if (!batch.isSkinned)
                        continue;
                    if (mdiActive) {
                        const u32 variantIdx = MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
                        const bool pooled = variantIdx == 0
                            && batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
                            && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
                        if (pooled)
                            continue;
                    }

                    u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                    auto sr = GetSplatRange(batch, data.overlayMgr);
                    {
                        CKinematics* parent = nullptr;
                        u32 visualType = batch.visual ? batch.visual->getType() : 0;
                        if (visualType == MT_SKELETON_GEOMDEF_ST)
                            parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                        else if (visualType == MT_SKELETON_GEOMDEF_PM)
                            parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                        gpuCullMgr->SetSkeletonCurrWorld(parent, batch.worldMatrix);
                    }

                    DrawSkinnedBatch(*data.passState, cmdList, nvDevice, worldCtx,
                        batch, batch.worldMatrix, boneOffset, sr,
                        residualArgs, residualIdx * (u32)sizeof(IndirectDrawArgs));
                    ++residualIdx;
                }
            }

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

framegraph::VirtualResourceHandle setupHudOverlayPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle baseColor,
    framegraph::VirtualResourceHandle worldPos,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    nvrhi::ITexture* shadowMapArray,
    nvrhi::ITexture* contactDepth,
    nvrhi::ITexture* contactHistory,
    nvrhi::ITexture* envSky0,
    nvrhi::ITexture* envSky1,
    nvrhi::ITexture* hudShadowMap,
    nvrhi::ITexture* const* shadowCascadesIn,
    nvrhi::ITexture* localShadowAtlas,
    nvrhi::ITexture* const* shadowHZBIn,
    nvrhi::ITexture* shadowMask,
    nvrhi::ITexture* localShadowESM,
    bool rtgiGuidePass)
{
    using namespace framegraph;

    if (!device || !state || !hudBatches || hudBatches->empty() || !sceneColor.is_valid() || !depth.is_valid())
        return sceneColor;

    {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeSkinningResources(device, fbInfo, *state);
    }

    struct HudOverlayPassData {
        VirtualResourceHandle color;
        VirtualResourceHandle normal;
        VirtualResourceHandle baseColor;
        VirtualResourceHandle worldPos;
        VirtualResourceHandle depth;
        fg::RenderDevice* device = nullptr;
        const xr_vector<GeometryBatch>* hudBatches = nullptr;
        MaterialCache* materialCache = nullptr;
        fg::GPUCullingManager* gpuCulling = nullptr;
        SkinningPassState* passState = nullptr;
        u32 width = 0;
        u32 height = 0;
        nvrhi::ITexture* shadowCascades[3] = {};
        nvrhi::ITexture* hudShadowMap = nullptr;
        nvrhi::ITexture* localShadowAtlas = nullptr;
        nvrhi::ITexture* localShadowESM = nullptr;
        nvrhi::ITexture* shadowHZB[3] = {};
        nvrhi::ITexture* shadowMask = nullptr;
        nvrhi::ITexture* contactDepth = nullptr;
        nvrhi::ITexture* contactHistory = nullptr;
        nvrhi::ITexture* envSky0 = nullptr;
        nvrhi::ITexture* envSky1 = nullptr;
        bool rtgiGuidePass = false;
    };

    auto& passData = fg.addCallbackPass<HudOverlayPassData>(
        rtgiGuidePass ? "HUD RTGI Guide" : "HUD Overlay",
        [&, width, height, gpuCulling, state, materialCache, shadowMapArray, contactDepth, contactHistory,
         envSky0, envSky1, hudShadowMap, shadowCascadesIn, localShadowAtlas, shadowHZBIn, shadowMask, localShadowESM, rtgiGuidePass](
            FrameGraph& builder, PassHandle passHandle, HudOverlayPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.device = device;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;
            data.gpuCulling = gpuCulling;
            data.passState = state;
            data.rtgiGuidePass = rtgiGuidePass;
            for (int i = 0; i < 3; ++i)
            {
                data.shadowCascades[i] = (shadowCascadesIn && shadowCascadesIn[i]) ? shadowCascadesIn[i] : shadowMapArray;
                data.shadowHZB[i] = (shadowHZBIn && shadowHZBIn[i]) ? shadowHZBIn[i] : nullptr;
            }
            data.hudShadowMap = hudShadowMap;
            data.localShadowAtlas = localShadowAtlas;
            data.localShadowESM = localShadowESM;
            data.shadowMask = shadowMask;
            data.contactDepth = contactDepth;
            data.contactHistory = contactHistory;
            data.envSky0 = envSky0;
            data.envSky1 = envSky1;

            data.color = passBuilder.readWrite(sceneColor, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(depth, ResourceState::DepthStencilWrite);
            if (normal.is_valid())
                data.normal = passBuilder.readWrite(normal, ResourceState::RenderTarget);
            if (baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(baseColor, ResourceState::RenderTarget);
            if (worldPos.is_valid())
                data.worldPos = passBuilder.readWrite(worldPos, ResourceState::RenderTarget);
        },
        [](const HudOverlayPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            if (!data.passState || !data.hudBatches || data.hudBatches->empty())
                return;
            if (!data.gpuCulling || !data.gpuCulling->GetGlobalBoneBuffer())
                return;
            if (!TerrainMaterialBuffer::Instance().GetBuffer())
                return;

            auto* colorRT = fgGraph.GetPhysicalTexture(data.color);
            auto* depthRT = fgGraph.GetPhysicalTexture(data.depth);
            auto* normalRT = data.normal.is_valid() ? fgGraph.GetPhysicalTexture(data.normal) : nullptr;
            auto* baseColorRT = data.baseColor.is_valid() ? fgGraph.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fgGraph.GetPhysicalTexture(data.worldPos) : nullptr;
            if (!colorRT || !depthRT || !normalRT || !baseColorRT || !worldPosRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList || !data.passState->initialized)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.addColorAttachment(normalRT);
            fbDesc.addColorAttachment(baseColorRT);
            fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer(
                data.rtgiGuidePass ? "HudRtgiGuidePass" : "HudOverlayPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            GPUCullingManager* gpuCullMgr = data.gpuCulling;
            nvrhi::IBuffer* globalBoneBuffer = gpuCullMgr->GetGlobalBoneBuffer();
            auto& cache = framegraph::GetPassResourceCache();
            auto dynTransformsCB = cache.GetOrCreateVolatileCB("HudOverlay", "DynTransforms", sizeof(DynamicTransforms), data.device, 1024 * 8);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto materialIdCB = cache.GetOrCreateVolatileCB("HudOverlay", "MaterialId", sizeof(SkinnedMaterialCB), data.device, 1024 * 8);

            {
                StaticGlobals sg = BuildStaticGlobals(2.0f, false);
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            if (data.materialCache)
                data.materialCache->FinalizePendingMaterials(ctx);
            MaterialBuffer::Instance().Upload(ctx);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport hudViewport(
                0.0f, static_cast<float>(rtDesc.width),
                0.0f, static_cast<float>(rtDesc.height),
                0.9f, 1.0f);
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            bool hasScopeBatch = false;
            for (const auto& batch : *data.hudBatches) {
                if (IsScopeBatch(batch)) { hasScopeBatch = true; break; }
            }

            if (hasScopeBatch && data.passState->hudScopePS)
            {
                auto& st = *data.passState;
                const u32 w = rtDesc.width;
                const u32 h = rtDesc.height;
                if (!st.secondVP || st.secondVP->getDesc().width != w || st.secondVP->getDesc().height != h)
                {
                    nvrhi::TextureDesc td = colorRT->getDesc();
                    td.width = w;
                    td.height = h;
                    td.debugName = "rt_SecondVP";
                    td.isRenderTarget = false;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    st.secondVP = nvDevice->createTexture(td);
                }
                if (st.secondVP)
                {
                    cmdList->copyTexture(
                        st.secondVP, nvrhi::TextureSlice(),
                        colorRT, nvrhi::TextureSlice());
                }
            }

            SkinnedPhaseContext hudCtx = BuildSkinnedPhaseContext(
                *data.passState, nvDevice, framebuffer,
                dynTransformsCB, staticGlobalsCB, materialIdCB,
                globalBoneBuffer, bindlessTable, bindlessLayout, nullptr,
                hudViewport, scissor, true,
                data.shadowCascades, data.hudShadowMap, data.contactDepth, data.contactHistory, data.envSky0, data.envSky1, data.localShadowAtlas,
                data.shadowHZB, data.shadowMask, data.localShadowESM);

            SkinnedPhaseContext scopeCtx = hudCtx;
            if (hasScopeBatch && data.passState->hudScopePS && data.passState->hudScopeLayout && data.passState->secondVP)
            {
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* vsReflection = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
                auto* psReflection = shaderLoader->GetCachedReflection("bindless_skinned_scope", ".ps");
                if (vsReflection && psReflection)
                {
                    struct alignas(16) ScopeCB { Fvector4 hud; Fvector4 zoom; };
                    ScopeCB scb{};
                    if (g_pGamePersistent && g_pGamePersistent->m_pGShaderConstants)
                    {
                        scb.hud = g_pGamePersistent->m_pGShaderConstants->hud_params;
                        scb.zoom.set(0.f, 0.f, 1.f, 0.f);
                    }
                    else
                        scb.hud.set(0.f, 0.f, 0.015f, 1.f);

                    auto* scopeParamsCB = cache.GetOrCreateVolatileCB(
                        "HudOverlay", "ScopeParams", sizeof(ScopeCB), data.device);
                    if (scopeParamsCB)
                        cmdList->writeBuffer(scopeParamsCB, &scb, sizeof(scb));

                    framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "HudOverlay.Scope");
                    bsb.ConstantBuffer("dynamic_transforms", dynTransformsCB);
                    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                    bsb.BufferSRV("g_BoneMatrices", globalBoneBuffer);
                    bsb.ConstantBuffer("SkinnedMaterialCB", materialIdCB);
                    BindBindlessMaterialTables(bsb);
                    if (scopeParamsCB)
                        bsb.ConstantBuffer("ScopeParams", scopeParamsCB);
                    bsb.Texture("s_vp2", data.passState->secondVP);
                    scopeCtx.bindingSet = cache.GetOrCreateBindingSet(
                        bsb.Build(), data.passState->hudScopeLayout, nvDevice);
                }
            }

            for (const auto& batch : *data.hudBatches) {
                Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);
                u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                {
                    CKinematics* parent = nullptr;
                    u32 visualType = batch.visual ? batch.visual->getType() : 0;
                    if (visualType == MT_SKELETON_GEOMDEF_ST)
                        parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                    else if (visualType == MT_SKELETON_GEOMDEF_PM)
                        parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                    gpuCullMgr->SetSkeletonCurrWorld(parent, adjustedWorldMatrix);
                }
                const SkinnedPhaseContext& drawCtx =
                    (IsScopeBatch(batch) && scopeCtx.bindingSet) ? scopeCtx : hudCtx;

                DrawSkinnedBatch(*data.passState, cmdList, nvDevice, drawCtx,
                    batch, adjustedWorldMatrix, boneOffset, {0, 0});
            }
        }
    );

    return passData.color;
}

framegraph::VirtualResourceHandle setupSkinnedVelocityPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle motionVectors,
    framegraph::VirtualResourceHandle depth,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    fg::GPUCullingManager* gpuCulling,
    SkinningPassState* state,
    const Fmatrix& viewProj,
    const Fmatrix& prevViewProj,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    if (!device || !state || !gpuCulling || !motionVectors.is_valid() || !depth.is_valid())
        return motionVectors;

    if (!state->initialized || !state->velocityPS || !state->velocityLayout)
    {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats = {
            nvrhi::Format::RGBA16_FLOAT, nvrhi::Format::RGBA16_FLOAT,
            nvrhi::Format::RGBA8_UNORM, nvrhi::Format::RGBA32_FLOAT
        };
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeSkinningResources(device, fbInfo, *state);
    }
    if (!state->velocityPS || !state->velocityLayout)
        return motionVectors;

    struct alignas(16) VelocityTransformsCB {
        Fmatrix m_W;
        Fmatrix m_W_prev;
        Fmatrix m_VP;
        Fmatrix m_VP_prev;
    };
    struct alignas(16) VelocitySkinCB {
        u32 boneOffset;
        u32 prevBoneOffset;
        u32 hasPrev;
        u32 pad;
    };

    struct PassData {
        VirtualResourceHandle motionVectors;
        VirtualResourceHandle depth;
        fg::RenderDevice* device = nullptr;
        const GeometryCollector* geometry = nullptr;
        const xr_vector<GeometryBatch>* hudBatches = nullptr;
        fg::GPUCullingManager* gpuCulling = nullptr;
        SkinningPassState* passState = nullptr;
        Fmatrix viewProj;
        Fmatrix prevViewProj;
        u32 width = 0;
        u32 height = 0;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "Skinned Velocity",
        [&](FrameGraph& builder, PassHandle pass, PassData& data) {
            RenderPassBuilder pb(builder, pass);
            data.motionVectors = pb.readWrite(motionVectors, ResourceState::RenderTarget);
            data.depth = pb.read(depth, ResourceState::DepthStencilRead);
            data.device = device;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.gpuCulling = gpuCulling;
            data.passState = state;
            data.viewProj = viewProj;
            data.prevViewProj = prevViewProj;
            data.width = width;
            data.height = height;
        },
        [](const PassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* mvRT = fgGraph.GetPhysicalTexture(data.motionVectors);
            auto* depthRT = fgGraph.GetPhysicalTexture(data.depth);
            if (!mvRT || !depthRT || !data.gpuCulling || !data.passState)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            GPUCullingManager* gpuCullMgr = data.gpuCulling;
            nvrhi::IBuffer* currBones = gpuCullMgr->GetGlobalBoneBuffer();
            nvrhi::IBuffer* prevBones = gpuCullMgr->GetPrevBoneBuffer();
            if (!currBones || !prevBones)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(mvRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer(
                "SkinnedVelocityPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            auto& cache = framegraph::GetPassResourceCache();
            auto velCB = cache.GetOrCreateVolatileCB("SkinnedVelocity", "VelocityTransforms", sizeof(VelocityTransformsCB), data.device, 1024 * 8);
            auto skinCB = cache.GetOrCreateVolatileCB("SkinnedVelocity", "VelocitySkin", sizeof(VelocitySkinCB), data.device, 1024 * 8);
            if (!velCB || !skinCB)
                return;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* velPsRefl = shaderLoader->GetCachedReflection("bindless_skinned_velocity", ".ps");
            if (!velPsRefl)
                return;

            nvrhi::Viewport viewport(0.f, float(data.width), 0.f, float(data.height), 0.f, 1.f);
            nvrhi::Rect scissor(data.width, data.height);

            auto selectVel = [&](u32 stride, u16 mode) -> const SkinningPipelineVariant* {
                if (mode == RM_SKINNING_3B || mode == RM_SKINNING_3B_HQ) return &data.passState->velHQ3w;
                if (mode == RM_SKINNING_2B || mode == RM_SKINNING_2B_HQ) return &data.passState->velHQ2w;
                if (mode == RM_SKINNING_4B || mode == RM_SKINNING_4B_HQ) return &data.passState->velHQ4w;
                if (mode == RM_SKINNING_1B_HQ || mode == RM_SINGLE_HQ) return &data.passState->velHQ1w;
                if (mode == RM_SKINNING_1B || mode == RM_SINGLE) return &data.passState->velNonHQ;
                if (stride == 36) return &data.passState->velHQ1w;
                if (stride == 40) return &data.passState->velHQ4w;
                if (stride == 44) return &data.passState->velHQ2w;
                return &data.passState->velNonHQ;
            };
            auto selectVelMdi = [&](u32 fmt) -> const SkinningPipelineVariant* {
                switch (fmt) {
                case VF_SKINNED_NONHQ: return &data.passState->velMdiNonHQ;
                case VF_SKINNED_HQ1W: return &data.passState->velMdiHQ1w;
                case VF_SKINNED_HQ2W: return &data.passState->velMdiHQ2w;
                case VF_SKINNED_HQ3W: return &data.passState->velMdiHQ3w;
                case VF_SKINNED_HQ4W: return &data.passState->velMdiHQ4w;
                default: return nullptr;
                }
            };

            auto drawResidual = [&](const GeometryBatch& batch, const Fmatrix& world, bool hudDepth) {
                CKinematics* parent = nullptr;
                u32 visualType = batch.visual ? batch.visual->getType() : 0;
                if (visualType == MT_SKELETON_GEOMDEF_ST)
                    parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                else if (visualType == MT_SKELETON_GEOMDEF_PM)
                    parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();

                u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                const bool hasPrev = gpuCullMgr->HasPrevBones(parent) &&
                    parent && parent->fg_prev_world_frame + 1 == gpuCullMgr->GetBoneUploadFrameId();

                const SkinningPipelineVariant* variant = selectVel(batch.vertexStride, batch.skinningRenderMode);
                if (!variant || !variant->pipeline || !batch.vertexBuffer || !batch.indexBuffer)
                    return;

                auto* vsRefl = shaderLoader->GetCachedReflection(
                    batch.vertexStride == 40 ? "bindless_skinned_4w_velocity" :
                    batch.vertexStride == 44 ? (batch.skinningRenderMode == RM_SKINNING_3B || batch.skinningRenderMode == RM_SKINNING_3B_HQ
                        ? "bindless_skinned_3w_velocity" : "bindless_skinned_2w_velocity") :
                    batch.vertexStride == 36 ? "bindless_skinned_hq_velocity" :
                    "bindless_skinned_velocity", ".vs");
                if (!vsRefl)
                    vsRefl = shaderLoader->GetCachedReflection("bindless_skinned_velocity", ".vs");
                if (!vsRefl)
                    return;

                VelocityTransformsCB vcb{};
                vcb.m_W = world;
                vcb.m_W_prev = hasPrev ? parent->fg_prev_world : world;
                vcb.m_VP = data.viewProj;
                vcb.m_VP_prev = data.prevViewProj;
                cmdList->writeBuffer(velCB, &vcb, sizeof(vcb));

                VelocitySkinCB scb{};
                scb.boneOffset = boneOffset;
                scb.prevBoneOffset = hasPrev ? gpuCullMgr->GetPrevBoneOffset(parent) : boneOffset;
                scb.hasPrev = hasPrev ? 1u : 0u;
                scb.pad = 0;
                cmdList->writeBuffer(skinCB, &scb, sizeof(scb));

                framegraph::BindingSetBuilder bsb(*vsRefl, *velPsRefl, nvDevice, "SkinnedVelocity");
                bsb.ConstantBuffer("VelocityTransforms", velCB);
                bsb.BufferSRV("g_BoneMatrices", currBones);
                bsb.BufferSRV("g_PrevBoneMatrices", prevBones);
                bsb.ConstantBuffer("VelocitySkinCB", skinCB);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->velocityLayout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = variant->pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = { bindingSet };
                gfxState.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
                gfxState.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
                if (hudDepth) {
                    nvrhi::Viewport hudVp(0.f, float(data.width), 0.f, float(data.height), 0.9f, 1.f);
                    gfxState.viewport.addViewport(hudVp);
                } else {
                    gfxState.viewport.addViewport(viewport);
                }
                gfxState.viewport.addScissorRect(scissor);
                cmdList->setGraphicsState(gfxState);
                cmdList->drawIndexed(
                    nvrhi::DrawArguments()
                        .setVertexCount(batch.indexCount)
                        .setStartIndexLocation(batch.startIndex)
                        .setStartVertexLocation(batch.baseVertex));
            };

            if (data.geometry) {
                const bool mdiActive = gpuCullMgr->IsSkinnedMDIEnabled() && gpuCullMgr->GetSkinnedObjectCount() > 0;
                if (mdiActive && data.passState->velocityMdiLayout) {
                    nvrhi::IBuffer* drawIndexBuffer = GetOrCreateDrawIndexBuffer("SkinnedVelocity", nvDevice);
                    auto& pools = gpuCullMgr->GetSkinnedPools();
                    VelocityTransformsCB vcb{};
                    vcb.m_W = Fidentity;
                    vcb.m_W_prev = Fidentity;
                    vcb.m_VP = data.viewProj;
                    vcb.m_VP_prev = data.prevViewProj;
                    cmdList->writeBuffer(velCB, &vcb, sizeof(vcb));
                    VelocitySkinCB scb{};
                    cmdList->writeBuffer(skinCB, &scb, sizeof(scb));

                    for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f) {
                        const auto& bucket = gpuCullMgr->GetSkinnedBucket(f);
                        if (bucket.count == 0)
                            continue;
                        const SkinningPipelineVariant* variant = selectVelMdi(f);
                        nvrhi::IBuffer* poolVB = pools.GetVertexBuffer(f);
                        nvrhi::IBuffer* poolIB = pools.GetIndexBuffer(f);
                        if (!variant || !variant->pipeline || !poolVB || !poolIB || !drawIndexBuffer)
                            continue;

                        const char* vsName =
                            f == VF_SKINNED_HQ4W ? "bindless_skinned_4w_mdi_velocity" :
                            f == VF_SKINNED_HQ3W ? "bindless_skinned_3w_mdi_velocity" :
                            f == VF_SKINNED_HQ2W ? "bindless_skinned_2w_mdi_velocity" :
                            f == VF_SKINNED_HQ1W ? "bindless_skinned_hq_mdi_velocity" :
                            "bindless_skinned_mdi_velocity";
                        auto* vsRefl = shaderLoader->GetCachedReflection(vsName, ".vs");
                        if (!vsRefl)
                            continue;

                        framegraph::BindingSetBuilder bsb(*vsRefl, *velPsRefl, nvDevice, "SkinnedVelocity.MDI");
                        bsb.ConstantBuffer("VelocityTransforms", velCB);
                        bsb.BufferSRV("g_BoneMatrices", currBones);
                        bsb.BufferSRV("g_PrevBoneMatrices", prevBones);
                        bsb.ConstantBuffer("VelocitySkinCB", skinCB);
                        bsb.BufferSRV("g_SkinnedRecords", bucket.recordsBuffer);
                        bsb.BufferSRV("g_SkinnedCompactIndices", bucket.compactBatchIndicesBuffer);
                        auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->velocityMdiLayout, nvDevice);
                        if (!bindingSet)
                            continue;

                        nvrhi::GraphicsState gfxState;
                        gfxState.pipeline = variant->pipeline;
                        gfxState.framebuffer = framebuffer;
                        gfxState.bindings = { bindingSet };
                        gfxState.vertexBuffers = { {poolVB, 0, 0}, {drawIndexBuffer, 1, 0} };
                        gfxState.indexBuffer = { poolIB, nvrhi::Format::R16_UINT, 0 };
                        gfxState.viewport.addViewport(viewport);
                        gfxState.viewport.addScissorRect(scissor);
                        gfxState.indirectParams = bucket.compactDrawArgsBuffer;
                        gfxState.indirectCountBuffer = bucket.compactCountBuffer;
                        cmdList->setGraphicsState(gfxState);
                        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, bucket.count);
                    }
                }

                for (const auto& batch : data.geometry->GetBatches()) {
                    if (!batch.isSkinned)
                        continue;
                    if (mdiActive) {
                        const u32 variantIdx = MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
                        const bool pooled = variantIdx == 0
                            && batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
                            && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
                        if (pooled)
                            continue;
                    }
                    drawResidual(batch, batch.worldMatrix, false);
                }
            }

            if (data.hudBatches) {
                for (const auto& batch : *data.hudBatches) {
                    if (IsScopeBatch(batch))
                        continue;
                    drawResidual(batch, ApplyHUDFOVAdjustment(batch.worldMatrix), true);
                }
            }
        }
    );

    return passData.motionVectors;
}

} // namespace xray::render::fg::passes
