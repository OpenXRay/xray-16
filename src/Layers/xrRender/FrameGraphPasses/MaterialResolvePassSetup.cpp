#include "stdafx.h"
#include "MaterialResolvePassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/GPUCullingManager.h"

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct MaterialResolvePassData {
    VirtualResourceHandle visId;
    VirtualResourceHandle depth;
    VirtualResourceHandle color;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle material;
    VirtualResourceHandle motionVectors;
    VirtualResourceHandle visDepth;
    VirtualResourceHandle skinnedDrawArgs;
    fg::RenderDevice* device = nullptr;
    MaterialCache* materialCache = nullptr;
    GPUCullingManager* gpuCulling = nullptr;
    nvrhi::IBuffer* splatBuffer = nullptr;
    MaterialResolvePassState* state = nullptr;
    ClusterDrawConfig config;
    Fmatrix prevView;
    Fmatrix prevProj;
    bool motionValid = false;
    u32 entryLimit = 0xFFFFFFFFu;
    u32 width = 0;
    u32 height = 0;
};

struct alignas(16) MaterialResolveParams {
    Fmatrix prevView;
    Fmatrix prevProj;
    u32 skinnedEntryBase;
    u32 motionValid;
    u32 entryLimit;
    u32 pad0;
};

}

bool EnsureMaterialResolveResources(fg::RenderDevice* device, MaterialResolvePassState& state)
{
    if (state.initialized)
        return true;
    if (state.failed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader) {
        state.failed = true;
        return false;
    }

    auto csResult = shaderLoader->LoadComputeShader("material_resolve", "main");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [MaterialResolve] Failed to load material_resolve compute shader");
        state.failed = true;
        return false;
    }
    state.shader = csResult.handle;

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("MaterialResolve", *csResult.reflection, nvDevice);
    if (!state.layout) {
        Msg("! [MaterialResolve] Failed to create binding layout");
        state.failed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.shader;
    pipeDesc.bindingLayouts = { state.layout };
    auto* backend = device->GetBackend();
    if (backend && backend->GetBindlessLayout())
        pipeDesc.addBindingLayout(backend->GetBindlessLayout());
    state.pipeline = cache.GetOrCreateComputePipeline("MaterialResolve", pipeDesc, nvDevice);
    if (!state.pipeline) {
        Msg("! [MaterialResolve] Failed to create compute pipeline");
        state.failed = true;
        return false;
    }

    state.initialized = true;
    Msg("* [MaterialResolve] Pipeline initialized");
    return true;
}

MaterialResolveOutput setupMaterialResolvePass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle visId,
    VirtualResourceHandle depth,
    VirtualResourceHandle color,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle material,
    VirtualResourceHandle skinnedDrawArgs,
    const ClusterDrawConfig& config,
    MaterialCache* materialCache,
    GPUCullingManager* gpuCulling,
    nvrhi::IBuffer* splatBuffer,
    const Fmatrix& prevView,
    const Fmatrix& prevProj,
    bool motionValid,
    u32 entryLimit,
    u32 width,
    u32 height,
    MaterialResolvePassState* state)
{
    ResourceDesc motionDesc;
    motionDesc.type = ResourceDesc::Type::Texture2D;
    motionDesc.debugName = "rt_MotionVectors";
    motionDesc.width = width;
    motionDesc.height = height;
    motionDesc.format = nvrhi::Format::RG16_FLOAT;
    motionDesc.isUAV = true;
    motionDesc.allowUAV = true;
    motionDesc.isTransient = true;
    VirtualResourceHandle motionHandle = fg.CreateTexture("rt_MotionVectors", motionDesc);

    ResourceDesc visDepthDesc = motionDesc;
    visDepthDesc.debugName = "rt_VisDepth";
    visDepthDesc.format = nvrhi::Format::R32_FLOAT;
    VirtualResourceHandle visDepthHandle = fg.CreateTexture("rt_VisDepth", visDepthDesc);

    auto& passData = fg.addCallbackPass<MaterialResolvePassData>(
        "Material Resolve",
        [&, visId, depth, color, normal, baseColor, material, skinnedDrawArgs, config, materialCache, gpuCulling, splatBuffer, prevView, prevProj, motionValid, entryLimit, width, height, state, motionHandle, visDepthHandle](FrameGraph& builder, PassHandle passHandle, MaterialResolvePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.materialCache = materialCache;
            data.gpuCulling = gpuCulling;
            data.splatBuffer = splatBuffer;
            data.state = state;
            data.config = config;
            data.prevView = prevView;
            data.prevProj = prevProj;
            data.motionValid = motionValid;
            data.entryLimit = entryLimit;
            data.width = width;
            data.height = height;
            data.visId = passBuilder.read(visId, ResourceState::ShaderResource);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            if (skinnedDrawArgs.is_valid())
                data.skinnedDrawArgs = passBuilder.read(skinnedDrawArgs, ResourceState::ShaderResource);
            data.color = passBuilder.readWrite(color, ResourceState::UnorderedAccess);
            data.normal = passBuilder.readWrite(normal, ResourceState::UnorderedAccess);
            data.baseColor = passBuilder.readWrite(baseColor, ResourceState::UnorderedAccess);
            data.material = passBuilder.readWrite(material, ResourceState::UnorderedAccess);
            data.motionVectors = passBuilder.write(motionHandle, ResourceState::UnorderedAccess);
            data.visDepth = passBuilder.write(visDepthHandle, ResourceState::UnorderedAccess);
        },
        [](const MaterialResolvePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ZoneScoped;
            ZoneName("MaterialResolvePass", 19);

            auto* visRT = fg.GetPhysicalTexture(data.visId);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = fg.GetPhysicalTexture(data.baseColor);
            auto* materialRT = fg.GetPhysicalTexture(data.material);
            auto* motionRT = fg.GetPhysicalTexture(data.motionVectors);
            auto* visDepthRT = fg.GetPhysicalTexture(data.visDepth);
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!visRT || !depthRT || !colorRT || !normalRT || !baseColorRT || !materialRT || !motionRT || !visDepthRT || !cmdList)
                return;

            cmdList->clearTextureFloat(normalRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            cmdList->clearTextureFloat(baseColorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));
            cmdList->clearTextureFloat(materialRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));

            const auto& config = data.config;
            if (!config.IsValid() || !config.UseMegaBuffers())
                return;

            if (data.materialCache) {
                data.materialCache->FinalizePendingMaterials(ctx);
                data.materialCache->FinalizePendingTerrainMaterials(ctx);
            }
            auto& matBuffer = bindless::MaterialBuffer::Instance();
            matBuffer.Upload(ctx);
            auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
            terrainMatBuffer.Upload(ctx);
            if (!matBuffer.GetBuffer() || !terrainMatBuffer.GetBuffer())
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("material_resolve", ".cs");
            if (!refl)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            nvrhi::IBuffer* terrainInstances = config.terrainInstanceBuffer ? config.terrainInstanceBuffer : config.instanceBuffer;

            GPUCullingManager* gpuCulling = data.skinnedDrawArgs.is_valid() ? data.gpuCulling : nullptr;
            const bool skinned = gpuCulling && gpuCulling->GetSkinnedEntryCount() > 0
                && gpuCulling->GetSkinnedEntryBuffer() && gpuCulling->GetSkinnedPreVertexBuffer()
                && gpuCulling->GetSkinnedPools().GetCombinedIndexBuffer() && gpuCulling->GetSkinnedRecordsBuffer()
                && gpuCulling->GetGlobalBoneBuffer();
            auto paramsCB = cache.GetOrCreateVolatileCB("MaterialResolve", "MaterialResolveParams", sizeof(MaterialResolveParams), data.device, 16);
            MaterialResolveParams params = {};
            params.prevView = data.prevView;
            params.prevProj = data.prevProj;
            params.skinnedEntryBase = skinned ? gpuCulling->GetClusterEntryCapacity() : 0xFFFFFFFFu;
            params.motionValid = data.motionValid ? 1u : 0u;
            params.entryLimit = data.entryLimit;
            cmdList->writeBuffer(paramsCB, &params, sizeof(params));

            BindingSetBuilder bsb(*refl, nvDevice, "MaterialResolve");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.ConstantBuffer("MaterialResolveParams", paramsCB);
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
            bsb.BufferSRV("g_Variants", bindless::VariantBuffer::Instance().GetBuffer());
            bsb.BufferSRV("g_TerrainMaterials", terrainMatBuffer.GetBuffer());
            bsb.BufferSRV("g_InstanceData", config.instanceBuffer);
            bsb.BufferSRV("g_TerrainInstanceData", terrainInstances);
            bsb.BufferSRV("g_DynamicInstanceData", config.dynamicInstanceBuffer ? config.dynamicInstanceBuffer : config.instanceBuffer);
            bsb.BufferSRV("g_DynamicPrevWorld", config.dynamicPrevWorldBuffer ? config.dynamicPrevWorldBuffer : config.megaVertexBuffer);
            bsb.BufferSRV("g_Entries", config.entryBuffer);
            bsb.BufferSRV("g_MegaVB", config.megaVertexBuffer);
            bsb.BufferSRV("g_MegaIB", config.megaIndexBuffer);
            bsb.BufferSRV("g_SkinnedEntries", skinned ? gpuCulling->GetSkinnedEntryBuffer() : config.entryBuffer);
            bsb.BufferSRV("g_SkinnedVB", skinned ? gpuCulling->GetSkinnedPreVertexBuffer() : config.megaVertexBuffer);
            bsb.BufferSRV("g_SkinnedIB", skinned ? gpuCulling->GetSkinnedPools().GetCombinedIndexBuffer() : config.megaIndexBuffer);
            bsb.BufferSRV("g_SkinnedPrevVB", skinned ? gpuCulling->GetSkinnedPrevVertexBuffer() : config.megaVertexBuffer);
            bsb.BufferSRV("g_SkinnedRecords", skinned ? gpuCulling->GetSkinnedRecordsBuffer() : config.entryBuffer);
            bsb.BufferSRV("g_BoneMatrices", skinned ? gpuCulling->GetGlobalBoneBuffer() : config.megaVertexBuffer);
            bsb.BufferSRV("g_PaintSplats", skinned && data.splatBuffer ? data.splatBuffer : config.megaVertexBuffer);
            bsb.Texture("g_VisID", visRT);
            bsb.Texture("g_Depth", depthRT);
            bsb.TextureUAV("g_OutNormal", normalRT);
            bsb.TextureUAV("g_OutBaseColor", baseColorRT);
            bsb.TextureUAV("g_OutMaterial", materialRT);
            bsb.TextureUAV("g_OutColor", colorRT);
            bsb.TextureUAV("g_OutMotion", motionRT);
            bsb.TextureUAV("g_OutVisDepth", visDepthRT);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->layout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->pipeline;
            cs.bindings = { bindingSet };
            if (auto* backend = data.device->GetBackend()) {
                if (auto* bindlessTable = backend->GetBindlessDescriptorTable())
                    cs.addBindingSet(bindlessTable);
            }
            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        });

    MaterialResolveOutput out;
    out.color = passData.color;
    out.normal = passData.normal;
    out.baseColor = passData.baseColor;
    out.material = passData.material;
    out.motionVectors = passData.motionVectors;
    out.visDepth = passData.visDepth;
    return out;
}

}
