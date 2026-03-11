// xrRender/FrameGraphPasses/SkinningPassSetup.cpp
// Consolidated skinned mesh rendering pass with World and HUD phases
// Uses GPU-driven global bone buffer for efficient skinning
#include "stdafx.h"
#include "SkinningPassSetup.h"
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
#include "xrCore/FMesh.hpp"

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {
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

void InitializeSkinningResources(ng::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, SkinningPassState& state)
{
    if (state.initialized)
        return;

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
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("SkinningPass", *skinnedVsForReflection.reflection, *skinnedPsResult.reflection, nvDevice);

    auto hudPsResult = shaderLoader->LoadPixelShader("bindless_skinned_hud", "main");
    if (hudPsResult.handle) {
        state.hudPS = hudPsResult.handle;
        state.hudLayout = cache.GetOrCreateBindingLayoutFromReflection("SkinningPass_HUD", *skinnedVsForReflection.reflection, *hudPsResult.reflection, nvDevice);
    }
    if (!state.hudLayout)
        state.hudLayout = state.layout;

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
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
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
        hudVariant.pipeline = cache.GetOrCreatePipeline(cacheName, pipeDesc, fbInfo, nvDevice);
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
        initVariant(state.nonHQ, "bindless_skinned", "SkinningPass_nonHQ", attribs, 5);
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
        initVariant(state.hq1w, "bindless_skinned_hq", "SkinningPass_hq1w", attribs, 5);
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
        initVariant(state.hq4w, "bindless_skinned_4w", "SkinningPass_hq4w", attribs, 6);
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
        initVariant(state.hq2w, "bindless_skinned_2w", "SkinningPass_hq2w", attribs, 5);
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
        initVariant(state.hq3w, "bindless_skinned_3w", "SkinningPass_hq3w", attribs, 5);
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

static nvrhi::IGraphicsPipeline* SelectSkinnedPipeline(const SkinningPassState& state, u32 vertexStride, u16 renderMode)
{
    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ)
        return state.hq3w.pipeline.Get();
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ)
        return state.hq2w.pipeline.Get();
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ)
        return state.hq4w.pipeline.Get();
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ)
        return state.hq1w.pipeline.Get();
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE)
        return state.nonHQ.pipeline.Get();

    if (vertexStride == 36)
        return state.hq1w.pipeline.Get();
    if (vertexStride == 40)
        return state.hq4w.pipeline.Get();
    if (vertexStride == 44)
        return state.hq2w.pipeline.Get();
    if (vertexStride == 24)
        return state.nonHQ.pipeline.Get();

    if (vertexStride >= 36)
        return state.hq1w.pipeline.Get();
    return state.nonHQ.pipeline.Get();
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
static void RenderSkinnedBatch(
    const SkinningPassState& state,
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::IBuffer* dynTransformsCB,
    nvrhi::IBuffer* shaderParamsCB,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* materialIdCB,
    nvrhi::IBuffer* globalBoneBuffer,
    nvrhi::IBuffer* terrainMaterialsSB,
    nvrhi::IDescriptorTable* bindlessTable,
    nvrhi::IBindingLayout* bindlessLayout,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    const GeometryBatch& batch,
    const Fmatrix& worldMatrix,
    u32 skeletonBoneOffset,
    nvrhi::IBuffer* splatBuffer,
    decals::OverlayManager::SplatRange splatRange = {0, 0},
    bool isHUD = false)
{
    using namespace RENDER_NAMESPACE;
    using namespace RENDER_NAMESPACE::bindless;

    if (!batch.vertexBuffer || !batch.indexBuffer)
        return;

    DynamicTransforms dynTransData = {};
    FillDynamicTransforms(dynTransData, worldMatrix);
    cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

    SkinnedMaterialCB matIdData = {};
    matIdData.materialID = batch.bindlessMaterialID;
    matIdData.skeletonBoneOffset = skeletonBoneOffset;
    matIdData.splatOffset = splatRange.offset;
    matIdData.splatCount = splatRange.count;
    cmdList->writeBuffer(materialIdCB, &matIdData, sizeof(matIdData));

    if (!globalBoneBuffer)
        return;

    auto& matBuffer = MaterialBuffer::Instance();
    auto& cache = framegraph::GetPassResourceCache();

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsReflection = shaderLoader->GetCachedReflection("bindless_skinned", ".vs");
    const char* psShaderName = isHUD ? "bindless_skinned" : "bindless_skinned";
    auto* psReflection = shaderLoader->GetCachedReflection(psShaderName, ".ps");
    auto activeLayout = isHUD ? state.hudLayout : state.layout;

    framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, isHUD ? "Skinning.HUD" : "Skinning");
    bsb.ConstantBuffer("dynamic_transforms", dynTransformsCB);
    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
    bsb.BufferSRV("g_BoneMatrices", globalBoneBuffer);
    bsb.ConstantBuffer("SkinnedMaterialCB", materialIdCB);
    bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
    bsb.BufferSRV("g_PaintSplats", splatBuffer);

    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), activeLayout, nvDevice);
    if (!bindingSet)
        return;

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
                nvDevice, framebuffer, variantIdx, *variant, p, fmt,
                GetSkinnedInputLayout(state, fmt), state.layout, bindlessLayout);
        } else {
            pipeline = SelectSkinnedPipeline(state, batch.vertexStride, batch.skinningRenderMode);
        }
        if (!pipeline)
            continue;

        nvrhi::GraphicsState gfxState;
        gfxState.pipeline = pipeline;
        gfxState.framebuffer = framebuffer;
        gfxState.bindings = { bindingSet };
        if (bindlessTable)
            gfxState.addBindingSet(bindlessTable);
        gfxState.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
        gfxState.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
        gfxState.viewport.addViewport(viewport);
        gfxState.viewport.addScissorRect(scissor);

        cmdList->setGraphicsState(gfxState);
        cmdList->drawIndexed(
            nvrhi::DrawArguments()
                .setVertexCount(batch.indexCount)
                .setStartIndexLocation(batch.startIndex)
                .setStartVertexLocation(batch.baseVertex));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNING PASS SETUP
// ═══════════════════════════════════════════════════════════════════════════

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const SkinnedVisibilityData& visibilityData,
    SkinningPassState* state,
    decals::OverlayManager* overlayMgr)
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
        [&, width, height, visibilityData, state, overlayMgr](FrameGraph& builder, PassHandle passHandle, SkinningPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;
            data.visibilityData = visibilityData;
            data.passState = state;
            data.overlayMgr = overlayMgr;

            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.normal = passBuilder.readWrite(inputs.normal, ResourceState::RenderTarget);
            if (inputs.baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::RenderTarget);
            if (inputs.worldPos.is_valid())
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
        [](const SkinningPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            using namespace RENDER_NAMESPACE;

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
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            if (worldPosRT)
                fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = framegraph::GetPassResourceCache().GetOrCreateFramebuffer("SkinningPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            const auto& rtDesc = colorRT->getDesc();

            // Get GPUCullingManager for bone buffer (passed via visibility data userData)
            GPUCullingManager* gpuCullMgr = data.visibilityData.visibilityByVisualUserData
                ? static_cast<GPUCullingManager*>(data.visibilityData.visibilityByVisualUserData)
                : nullptr;

            if (!gpuCullMgr || !gpuCullMgr->GetGlobalBoneBuffer()) {
                Msg("! [SkinningPass] GPUCullingManager bone buffer not available!");
                return;
            }

            // Begin frame - reset skeleton allocations
            gpuCullMgr->BeginSkinnedFrame();

            // Get global bone buffer for shader binding
            nvrhi::IBuffer* globalBoneBuffer = gpuCullMgr->GetGlobalBoneBuffer();

            auto& cache = framegraph::GetPassResourceCache();
            auto dynTransformsCB = cache.GetOrCreateVolatileCB("SkinningPass", "DynTransforms", sizeof(DynamicTransforms), data.device, 1024 * 8);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto shaderParamsCB = cache.GetOrCreateVolatileCB("SkinningPass", "ShaderParams", sizeof(ShaderParams), data.device, 512);
            auto materialIdCB = cache.GetOrCreateVolatileCB("SkinningPass", "MaterialId", sizeof(SkinnedMaterialCB), data.device, 1024);

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
                    0.0f, 1.0f  // Normal depth range
                );

                // Use GPU culling visibility if available (visual-based lookup)
                const bool useGPUCulling = data.visibilityData.enabled &&
                                           data.visibilityData.visibilityByVisualCallback != nullptr;

                if (useGPUCulling) {
                    // Render using visibility data (GPU-culled path)
                    // Visual-based lookup handles batch reordering correctly
                    // NOTE: We use data from N-2 frames ago (double-buffer readback delay)

                    u32 renderedCount = 0;
                    u32 culledCount = 0;

                    // Iterate over current frame's batches
                    for (const auto& batch : data.geometry->GetBatches()) {
                        if (!batch.isSkinned)
                            continue;

                        // Check visibility using visual pointer lookup
                        u32 visibilityValue = batch.visual
                            ? data.visibilityData.visibilityByVisualCallback(
                                batch.visual, data.visibilityData.visibilityByVisualUserData)
                            : 0;

                        if (visibilityValue == 0) {
                            culledCount++;
                            continue;
                        }

                        u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                        auto sr = GetSplatRange(batch, data.overlayMgr);

                        RenderSkinnedBatch(
                            *data.passState,
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB,
                            globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                            bindlessTable, bindlessLayout, worldViewport, scissor,
                            batch, batch.worldMatrix, boneOffset, splatBuffer, sr
                        );
                        renderedCount++;
                    }

                    // Report stats via callback
                    if (data.visibilityData.statsCallback) {
                        data.visibilityData.statsCallback(renderedCount, culledCount,
                            data.visibilityData.statsUserData);
                    }
                } else {
                    // Render all skinned batches (no GPU culling)
                    u32 drawCount = 0;
                    for (const auto& batch : data.geometry->GetBatches()) {
                        if (!batch.isSkinned)
                            continue;

                        u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);
                        auto sr = GetSplatRange(batch, data.overlayMgr);

                        RenderSkinnedBatch(
                            *data.passState,
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB,
                            globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                            bindlessTable, bindlessLayout, worldViewport, scissor,
                            batch, batch.worldMatrix, boneOffset, splatBuffer, sr
                        );
                        drawCount++;
                    }
                }
            }

            // ═══════════════════════════════════════════════════════
            //  PHASE 2: HUD SKINNED MESHES (depth [0.0, 0.1])
            // ═══════════════════════════════════════════════════════
            if (hasHUDSkinned) {
                nvrhi::Viewport hudViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 0.1f  // Compressed depth range for HUD
                );

                for (const auto& batch : *data.hudBatches) {
                    Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);
                    u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);

                    RenderSkinnedBatch(
                        *data.passState,
                        cmdList, nvDevice, framebuffer,
                        dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB,
                        globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                        bindlessTable, bindlessLayout, hudViewport, scissor,
                        batch, adjustedWorldMatrix, boneOffset, splatBuffer, {0, 0}, true
                    );
                }

            }
        }
    );

    // Return outputs
    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.worldPos = passData.worldPos;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
