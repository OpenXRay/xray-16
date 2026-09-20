#include "stdafx.h"
#include "DetailResolvePassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"

extern ENGINE_API int ps_r3_grass_interaction_debug;

namespace xray::render::fg::passes {

using namespace framegraph;

namespace {

struct DetailResolvePassData {
    VirtualResourceHandle visId;
    VirtualResourceHandle depth;
    VirtualResourceHandle color;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle material;
    VirtualResourceHandle motionVectors;
    VirtualResourceHandle visDepth;
    fg::RenderDevice* device = nullptr;
    fg::FGDetailManager* detailManager = nullptr;
    DetailResolvePassState* state = nullptr;
    std::shared_ptr<const FGDetailManager::VisibilityFrame> detailFrame;
    Fmatrix prevView;
    Fmatrix prevProj;
    bool motionValid = false;
    float prevTime = 0.0f;
    u32 width = 0;
    u32 height = 0;
};

struct alignas(16) DetailResolveParams {
    Fmatrix prevView;
    Fmatrix prevProj;
    u32 entryBase;
    u32 motionValid;
    float prevTime;
    u32 veinIndex;
    u32 segments[4];
    u32 interactionDebug;
    u32 preparedCapacity[3];
};

}

bool EnsureDetailResolveResources(fg::RenderDevice* device, DetailResolvePassState& state,
    const DetailPassResources& details)
{
    if (!details.HasSource())
        return false;
    if (state.initialized && state.sourceLayout == details.frame->source->bindingLayout)
        return true;
    if (state.failed)
        return false;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader) {
        state.failed = true;
        return false;
    }

    auto csResult = shaderLoader->LoadComputeShader("detail_resolve", "main");
    if (!csResult.handle || !csResult.reflection) {
        Msg("! [DetailResolve] Failed to load detail_resolve compute shader");
        state.failed = true;
        return false;
    }
    state.shader = csResult.handle;

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("DetailResolve", *csResult.reflection, nvDevice);
    if (!state.layout) {
        Msg("! [DetailResolve] Failed to create binding layout");
        state.failed = true;
        return false;
    }

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.shader;
    pipeDesc.bindingLayouts = { state.layout, device->GetBackend()->GetBindlessLayout(), details.frame->source->bindingLayout };
    state.pipeline = nvDevice->createComputePipeline(pipeDesc);
    if (!state.pipeline) {
        Msg("! [DetailResolve] Failed to create compute pipeline");
        state.failed = true;
        return false;
    }

    state.sourceLayout = details.frame->source->bindingLayout;
    state.initialized = true;
    Msg("* [DetailResolve] Pipeline initialized");
    return true;
}

MaterialResolveOutput setupDetailResolvePass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle visId,
    VirtualResourceHandle depth,
    const MaterialResolveOutput& inputs,
    fg::FGDetailManager* detailManager,
    const DetailPassResources& details,
    const Fmatrix& prevView,
    const Fmatrix& prevProj,
    bool motionValid,
    float prevTime,
    u32 width,
    u32 height,
    DetailResolvePassState* state)
{
    auto& passData = fg.addCallbackPass<DetailResolvePassData>(
        "Detail Resolve",
        [&, visId, depth, inputs, detailManager, prevView, prevProj, motionValid, prevTime, width, height, state](FrameGraph& builder, PassHandle passHandle, DetailResolvePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.detailManager = detailManager;
            data.state = state;
            data.detailFrame = details.frame;
            details.Read(passBuilder, false, false);
            data.prevView = prevView;
            data.prevProj = prevProj;
            data.motionValid = motionValid;
            data.prevTime = prevTime;
            data.width = width;
            data.height = height;
            data.visId = passBuilder.read(visId, ResourceState::ShaderResource);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            data.color = passBuilder.readWrite(inputs.color, ResourceState::UnorderedAccess);
            data.normal = passBuilder.readWrite(inputs.normal, ResourceState::UnorderedAccess);
            data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::UnorderedAccess);
            data.material = passBuilder.readWrite(inputs.material, ResourceState::UnorderedAccess);
            data.motionVectors = passBuilder.readWrite(inputs.motionVectors, ResourceState::UnorderedAccess);
            data.visDepth = passBuilder.readWrite(inputs.visDepth, ResourceState::UnorderedAccess);
        },
        [](const DetailResolvePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            ZoneScoped;
            ZoneName("DetailResolvePass", 17);

            auto* dm = data.detailManager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!dm || !cmdList || !data.state->initialized)
                return;
            const auto& frame = *data.detailFrame;
            const auto& source = *frame.source;
            R_ASSERT2(source.bindingLayout == data.state->sourceLayout, "[DetailManager] detail resolve source layout mismatch");

            auto* visRT = fg.GetPhysicalTexture(data.visId);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* baseColorRT = fg.GetPhysicalTexture(data.baseColor);
            auto* materialRT = fg.GetPhysicalTexture(data.material);
            auto* motionRT = fg.GetPhysicalTexture(data.motionVectors);
            auto* visDepthRT = fg.GetPhysicalTexture(data.visDepth);
            if (!visRT || !depthRT || !colorRT || !normalRT || !baseColorRT || !materialRT || !motionRT || !visDepthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("detail_resolve", ".cs");
            if (!refl)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto detailGlobalsCB = cache.GetOrCreateVolatileCB("Detail", "DetailGlobals", sizeof(FGDetailManager::DetailFrameConstants), data.device);
            FGDetailManager::DetailFrameConstants frameConstants;
            dm->FillFrameConstants(frameConstants);
            cmdList->writeBuffer(detailGlobalsCB, &frameConstants, sizeof(frameConstants));

            auto paramsCB = cache.GetOrCreateVolatileCB("DetailResolve", "DetailResolveParams", sizeof(DetailResolveParams), data.device, 16);
            DetailResolveParams params = {};
            params.prevView = data.prevView;
            params.prevProj = data.prevProj;
            params.entryBase = frame.entryBase;
            params.motionValid = data.motionValid ? 1u : 0u;
            params.prevTime = data.prevTime;
            params.veinIndex = 0;
            for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
                params.segments[lod] = FGDetailManager::LOD_SEGMENTS[lod];
            params.segments[3] = 0;
            params.interactionDebug = ps_r3_grass_interaction_debug ? 1u : 0u;
            for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
                params.preparedCapacity[lod] = frame.preparedCapacity[lod];
            cmdList->writeBuffer(paramsCB, &params, sizeof(params));

            BindingSetBuilder bsb(*refl, nvDevice, "DetailResolve");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
            bsb.ConstantBuffer("DetailResolveParams", paramsCB);
            bsb.BufferSRV("g_VisibleLod0", frame.visible[0]);
            bsb.BufferSRV("g_VisibleLod1", frame.visible[1]);
            bsb.BufferSRV("g_VisibleLod2", frame.visible[2]);
            bsb.BufferSRV("g_PreparedLod0", frame.prepared[0]);
            bsb.BufferSRV("g_PreparedLod1", frame.prepared[1]);
            bsb.BufferSRV("g_PreparedLod2", frame.prepared[2]);
            bsb.BufferSRV("g_VisibleMesh", frame.visible[FGDetailManager::VIS_KIND_MESH]);
            bsb.BufferSRV("g_VisibleDecal", frame.visible[FGDetailManager::VIS_KIND_DECAL]);
            bsb.BufferSRV("detail_models", source.models);
            bsb.BufferSRV("pulled_vertices", source.pulledVertices);
            bsb.BufferSRV("grass_object_tints", dm->cachedGrassTintsBuffer);
            bsb.BufferSRV("g_DetailPackets", frame.packets);
            bsb.Texture("g_Perlin4D", dm->perlin4dTexture);
            bsb.Texture("g_Interaction", dm->interactionTexture[dm->interactionCurrent]);
            bsb.Texture("g_InteractionPrev", dm->interactionTexture[dm->interactionCurrent ^ 1u]);
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
            cs.bindings = { bindingSet, data.device->GetBackend()->GetBindlessDescriptorTable(), source.descriptorTable };
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
