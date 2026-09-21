#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace fg
{
extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

static nvrhi::BufferHandle s_rtgiPlaceholderBuffer;

static void CreatePlaceholders(nvrhi::IDevice* nvDevice)
{
    if (!s_rtgiPlaceholderBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "RTGI_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_rtgiPlaceholderBuffer = nvDevice->createBuffer(desc);
    }
}

static void InitializeResources(RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized)
        return;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* backend = device->GetBackend();
    if (!shaderLoader || !backend || !backend->GetBindlessLayout() || !backend->GetBindlessDescriptorTable())
        return;

    auto& cache = GetPassResourceCache();
    CreatePlaceholders(nvDevice);
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    state.sampler = cache.GetOrCreateSampler("RTGI", samplerDesc, nvDevice);
    state.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_CB", u32(std::max({ sizeof(ReSTIRGICB), sizeof(TemporalCB), sizeof(CompositeCB) })), device);
    if (!state.sampler || !state.cb || !s_rtgiPlaceholderBuffer)
        return;

    state.initialized = true;
    const auto createPipeline =
        [&](const char* shader, const char* name, bool rayTracing, nvrhi::BindingLayoutHandle& layout, nvrhi::ComputePipelineHandle& pipeline)
    {
        auto result = shaderLoader->LoadComputeShader(shader);
        if (!result.handle || !result.reflection)
            return LightingFallback::ShaderUnavailable;
        layout = cache.GetOrCreateBindingLayoutFromReflection(name, *result.reflection, nvDevice);
        if (!layout)
            return LightingFallback::PipelineUnavailable;
        nvrhi::ComputePipelineDesc desc;
        desc.CS = result.handle;
        desc.bindingLayouts = { layout };
        if (rayTracing)
            desc.bindingLayouts.push_back(backend->GetBindlessLayout());
        pipeline = nvDevice->createComputePipeline(desc);
        return pipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    };
    state.readiness = createPipeline("restir_gi_initial", "RTGI_Initial", true, state.initialLayout, state.initialPipeline);
    if (state.readiness != LightingFallback::None)
        return;
    state.readiness = createPipeline("restir_gi_temporal", "RTGI_Temporal", false, state.temporalLayout, state.temporalPipeline);
    if (state.readiness != LightingFallback::None)
        return;
    state.readiness = createPipeline("restir_gi_composite", "RTGI_Composite", false, state.compositeLayout, state.compositePipeline);
}

static void EnsurePersistentTextures(nvrhi::IDevice* nvDevice, ReSTIRGIPassState& state, u32 width, u32 height)
{
    if (state.reservoirA[0] &&
        state.reservoirA[1] &&
        state.reservoirB[0] &&
        state.reservoirB[1] &&
        state.directLighting &&
        state.texWidth == width &&
        state.texHeight == height)
        return;
    state.historyValid = false;
    state.currTemporalIdx = 0;

    for (int i = 0; i < 2; i++)
    {
        {
            nvrhi::TextureDesc desc;
            desc.debugName = i == 0 ? "RTGI_ReservoirA_0" : "RTGI_ReservoirA_1";
            desc.width = width;
            desc.height = height;
            desc.format = nvrhi::Format::RGBA32_FLOAT;
            desc.isUAV = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            state.reservoirA[i] = nvDevice->createTexture(desc);
        }
        {
            nvrhi::TextureDesc desc;
            desc.debugName = i == 0 ? "RTGI_ReservoirB_0" : "RTGI_ReservoirB_1";
            desc.width = width;
            desc.height = height;
            desc.format = nvrhi::Format::RGBA32_FLOAT;
            desc.isUAV = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            state.reservoirB[i] = nvDevice->createTexture(desc);
        }
    }

    {
        nvrhi::TextureDesc desc;
        desc.debugName = "RTGI_DirectLighting";
        desc.width = width;
        desc.height = height;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        state.directLighting = nvDevice->createTexture(desc);
    }

    state.texWidth = width;
    state.texHeight = height;
}

LightingFallback EnsureReSTIRGIResources(RenderDevice* device, ReSTIRGIPassState& state, u32 width, u32 height)
{
    if (!device || !device->GetNVRHIDevice() || !width || !height)
        return LightingFallback::ResourcesUnavailable;
    InitializeResources(device, state);
    if (state.readiness != LightingFallback::None)
        return state.readiness;
    EnsurePersistentTextures(device->GetNVRHIDevice(), state, width, height);
    if (!state.directLighting || !state.reservoirA[0] || !state.reservoirA[1] || !state.reservoirB[0] || !state.reservoirB[1])
        return LightingFallback::ResourcesUnavailable;
    return LightingFallback::None;
}

ReSTIRGIOutput setupReSTIRGIPass(FrameGraph& fg, fg::RenderDevice* device, RTAccelStructManager* accelMgr, VirtualResourceHandle depth,
    VirtualResourceHandle normal, VirtualResourceHandle baseColor, VirtualResourceHandle prevNormals, VirtualResourceHandle prevDepth,
    VirtualResourceHandle motionVectors, VirtualResourceHandle sceneColorIn, const Fmatrix& invViewProj, const Fmatrix& prevViewProj, const Fvector& cameraPos,
    float giIntensity, u32 width, u32 height, ReSTIRGIPassState& state, bool hasPrevFrameData, LightingFrameState& lighting)
{
    const auto readiness = EnsureReSTIRGIResources(device, state, width, height);
    if (readiness != LightingFallback::None)
    {
        lighting.Fail(readiness);
        return { sceneColorIn };
    }
    if (!accelMgr || !accelMgr->IsReady())
    {
        lighting.Fail(LightingFallback::SceneUnavailable);
        return { sceneColorIn };
    }
    if (!sceneColorIn.is_valid() || !depth.is_valid() || !normal.is_valid() || !baseColor.is_valid() || !g_pGamePersistent)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sceneColorIn };
    }
    const auto& outputDesc = fg.GetResourceDesc(sceneColorIn);
    if ((!outputDesc.isUAV && !outputDesc.allowUAV) ||
        outputDesc.format != nvrhi::Format::RGBA16_FLOAT ||
        outputDesc.width != width ||
        outputDesc.height != height ||
        outputDesc.sampleCount != 1)
    {
        lighting.Fail(LightingFallback::InputsUnavailable);
        return { sceneColorIn };
    }
    const bool useHistory = state.historyValid && hasPrevFrameData;
    state.historyValid = false;
    state.initialRecorded = false;

    u32 writeIdx = state.currTemporalIdx;
    u32 readIdx = 1 - writeIdx;

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    auto* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1])
    {
        if (env.Current[0]->sky_texture_name.size())
        {
            auto h0 = texManager->LoadTexture(env.Current[0]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h0);
            if (t)
                sky0Tex = t;
        }
        if (env.Current[1]->sky_texture_name.size())
        {
            auto h1 = texManager->LoadTexture(env.Current[1]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h1);
            if (t)
                sky1Tex = t;
        }
    }
    if (!sky0Tex ||
        !sky1Tex ||
        sky0Tex->getDesc().dimension != nvrhi::TextureDimension::TextureCube ||
        sky1Tex->getDesc().dimension != nvrhi::TextureDimension::TextureCube)
    {
        lighting.Fail(LightingFallback::EnvironmentUnavailable);
        return { sceneColorIn };
    }

    Fvector sunDir = env.CurrentEnv.sun_dir;
    Fvector3 sc = { env.CurrentEnv.sun_color.x, env.CurrentEnv.sun_color.y, env.CurrentEnv.sun_color.z };
    float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    Fvector sunColor;
    if (sunIntensity > 0.001f)
        sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
    else
        sunColor.set(0, 0, 0);

    const auto& batchCounts = accelMgr->GetBatchCounts();

    ReSTIRGICB initialCB;
    initialCB.invViewProj = invViewProj;
    initialCB.prevViewProj = prevViewProj;
    initialCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    initialCB.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    initialCB.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
    initialCB.screenWidth = (float)width;
    initialCB.screenHeight = (float)height;
    initialCB.giIntensity = giIntensity;
    initialCB.frameIndex = Device.dwFrame;
    initialCB.identityStaticCount = batchCounts.identityStatic;
    initialCB.terrainBatchCount = batchCounts.terrain;
    initialCB.skinnedBatchStart =
        batchCounts.skinned > 0 ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal : 0;
    initialCB.grassBatchStart = batchCounts.grass > 0 ?
        batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned :
        0;
    initialCB.detailAtlasIndex = accelMgr->GetDetailAtlasIndex();
    initialCB.pad[0] = initialCB.pad[1] = initialCB.pad[2] = 0;

    ResourceDesc persistDesc;
    persistDesc.type = ResourceDesc::Type::Texture2D;
    persistDesc.width = width;
    persistDesc.height = height;
    persistDesc.isImported = true;
    persistDesc.isTransient = false;
    persistDesc.isUAV = true;

    auto dlDesc = persistDesc;
    dlDesc.format = nvrhi::Format::RGBA16_FLOAT;
    VirtualResourceHandle fgDirectLighting = fg.ImportTexture("rtgi_DirectLighting", state.directLighting.Get(), dlDesc);

    auto resDesc = persistDesc;
    resDesc.format = nvrhi::Format::RGBA32_FLOAT;
    VirtualResourceHandle fgResA = fg.ImportTexture("rtgi_ResA_W", state.reservoirA[writeIdx].Get(), resDesc);
    VirtualResourceHandle fgResB = fg.ImportTexture("rtgi_ResB_W", state.reservoirB[writeIdx].Get(), resDesc);

    fg.GetRTRegistry().RegisterRT("rt_DirectLighting", fgDirectLighting);
    fg.GetRTRegistry().RegisterRT("rt_GI_ReservoirA", fgResA);
    fg.GetRTRegistry().RegisterRT("rt_GI_ReservoirB", fgResB);

    // ============================================
    //  PASS 1: Initial Sample (RT shadow + bounce)
    // ============================================
    fg.addCallbackPass<InitialPassData>(
        "ReSTIR GI Initial",
        [&, sky0Tex, sky1Tex, initialCB, fgDirectLighting, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, InitialPassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.directLighting = pb.write(fgDirectLighting, ResourceState::UnorderedAccess);
            data.reservoirA = pb.write(fgResA, ResourceState::UnorderedAccess);
            data.reservoirB = pb.write(fgResB, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.scene = accelMgr->UseScene(builder, pb);
            data.state = &state;
            data.lighting = &lighting;
            data.cbData = initialCB;
            data.width = width;
            data.height = height;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
        },
        [](const InitialPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            if (!depthTex || !normalTex || !baseColorTex)
            {
                Msg("! [RTGI Initial] Null FG texture: depth=%d normal=%d baseColor=%d", !!depthTex, !!normalTex, !!baseColorTex);
                data.lighting->Fail(LightingFallback::InputsUnavailable);
                return;
            }

            nvrhi::ITexture* sky0 = data.sky0;
            nvrhi::ITexture* sky1 = data.sky1;
            nvrhi::ITexture* directLit = fg.GetPhysicalTexture(data.directLighting);
            nvrhi::ITexture* resA = fg.GetPhysicalTexture(data.reservoirA);
            nvrhi::ITexture* resB = fg.GetPhysicalTexture(data.reservoirB);
            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            auto* tlas = scene.tlas;
            auto* batchInfo = scene.batchInfo;
            auto* megaVB = scene.vertices;
            auto* megaIB = scene.indices;
            auto* matBuf = scene.materials;
            auto* terrainBuf = scene.terrainMaterials;

            if (!sky0 || !sky1 || !directLit || !resA || !resB || !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf || !scene.textures)
            {
                Msg("! [RTGI Initial] Null binding: sky0=%d sky1=%d directLit=%d resA=%d resB=%d tlas=%d batch=%d megaVB=%d megaIB=%d mat=%d terrain=%d",
                    !!sky0, !!sky1, !!directLit, !!resA, !!resB, !!tlas, !!batchInfo, !!megaVB, !!megaIB, !!matBuf, !!terrainBuf);
                data.lighting->Fail(LightingFallback::SceneUnavailable);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(ReSTIRGICB));

            nvrhi::IBuffer* skinnedVB = scene.skinnedVertices;
            nvrhi::IBuffer* skinnedIB = scene.skinnedIndices;
            nvrhi::IBuffer* grassVB = scene.grassVertices;
            nvrhi::IBuffer* grassIB = scene.grassIndices;
            if (!skinnedVB)
                skinnedVB = s_rtgiPlaceholderBuffer.Get();
            if (!skinnedIB)
                skinnedIB = s_rtgiPlaceholderBuffer.Get();
            if (!grassVB)
                grassVB = s_rtgiPlaceholderBuffer.Get();
            if (!grassIB)
                grassIB = s_rtgiPlaceholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_initial", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Initial");
            bsb.ConstantBuffer("ReSTIRGIParams", data.state->cb);
            bsb.AccelStruct("g_SceneTLAS", tlas);
            bsb.BufferSRV("g_BatchInfo", batchInfo);
            bsb.BufferSRV("g_MegaVB", megaVB);
            bsb.BufferSRV("g_MegaIB", megaIB);
            bsb.Texture("g_Sky0", sky0);
            bsb.Texture("g_Sky1", sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", matBuf);
            bsb.BufferSRV("g_TerrainMaterials", terrainBuf);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.TextureUAV("u_DirectLighting", directLit);
            bsb.TextureUAV("u_ReservoirA", resA);
            bsb.TextureUAV("u_ReservoirB", resB);
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), data.state->initialLayout);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->initialPipeline;
            cs.bindings = { bindingSet };

            if (scene.textures)
                cs.addBindingSet(scene.textures);

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.state->initialRecorded = true;
        });

    // ============================================
    //  PASS 2: Temporal Resampling
    // ============================================
    if (useHistory && motionVectors.is_valid())
    {
        const auto previousA = fg.ImportTexture("rtgi_ResA_R", state.reservoirA[readIdx], resDesc);
        const auto previousB = fg.ImportTexture("rtgi_ResB_R", state.reservoirB[readIdx], resDesc);
        TemporalCB temporalCB;
        temporalCB.invViewProj = invViewProj;
        temporalCB.prevInvViewProj.invert_44(prevViewProj);
        temporalCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        temporalCB.screenWidth = (float)width;
        temporalCB.screenHeight = (float)height;
        temporalCB.invScreenWidth = 1.0f / width;
        temporalCB.invScreenHeight = 1.0f / height;
        temporalCB.frameIndex = Device.dwFrame;
        temporalCB.pad[0] = temporalCB.pad[1] = temporalCB.pad[2] = 0;

        fg.addCallbackPass<TemporalPassData>(
            "ReSTIR GI Temporal",
            [&, temporalCB, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, TemporalPassData& data)
            {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                if (prevDepth.is_valid())
                    data.prevDepth = pb.read(prevDepth, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                data.previousA = pb.read(previousA, ResourceState::ShaderResource);
                data.previousB = pb.read(previousB, ResourceState::ShaderResource);
                data.reservoirA = pb.readWrite(fgResA, ResourceState::UnorderedAccess);
                data.reservoirB = pb.readWrite(fgResB, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.lighting = &lighting;
                data.cbData = temporalCB;
                data.width = width;
                data.height = height;
            },
            [](const TemporalPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
            {
                if (!data.state->initialRecorded || data.lighting->effective != LightingMode::RTGI)
                    return;
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fg.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* prevDepthTex = data.prevDepth.is_valid() ? fg.GetPhysicalTexture(data.prevDepth) : depthTex;
                auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
                auto* previousA = fg.GetPhysicalTexture(data.previousA);
                auto* previousB = fg.GetPhysicalTexture(data.previousB);
                auto* reservoirA = fg.GetPhysicalTexture(data.reservoirA);
                auto* reservoirB = fg.GetPhysicalTexture(data.reservoirB);
                if (!depthTex || !normalTex || !prevNormalsTex || !prevDepthTex || !baseColorTex || !mvTex || !previousA || !previousB || !reservoirA || !reservoirB)
                {
                    data.lighting->Fail(LightingFallback::InputsUnavailable);
                    return;
                }

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalCB));

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_temporal", ".cs");
                if (!csReflection)
                {
                    data.lighting->Fail(LightingFallback::ShaderUnavailable);
                    return;
                }

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Temporal");
                bsb.ConstantBuffer("ReSTIRTemporalParams", data.state->cb);
                bsb.Texture("t_PrevReservoirA", previousA);
                bsb.Texture("t_PrevReservoirB", previousB);
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_PrevDepth", prevDepthTex);
                bsb.TextureUAV("u_ReservoirA", reservoirA);
                bsb.TextureUAV("u_ReservoirB", reservoirB);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->temporalLayout, nvDevice);
                if (!bindingSet)
                {
                    data.lighting->Fail(LightingFallback::BindingUnavailable);
                    return;
                }

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalPipeline;
                cs.bindings = { bindingSet };

                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            });
    }

    // ============================================
    //  PASS 3: Composite (direct + indirect → scene)
    // ============================================
    CompositeCB compositeCB;
    compositeCB.invViewProj = invViewProj;
    compositeCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
    compositeCB.screenWidth = (float)width;
    compositeCB.screenHeight = (float)height;
    compositeCB.giIntensity = giIntensity;
    compositeCB.pad = 0;

    auto& compositeData = fg.addCallbackPass<CompositePassData>(
        "ReSTIR GI Composite",
        [&, compositeCB, writeIdx, fgDirectLighting, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, CompositePassData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.directLighting = pb.read(fgDirectLighting, ResourceState::ShaderResource);
            data.reservoirA = pb.read(fgResA, ResourceState::ShaderResource);
            data.reservoirB = pb.read(fgResB, ResourceState::ShaderResource);
            data.sceneColor = pb.readWrite(sceneColorIn, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.lighting = &lighting;
            data.cbData = compositeCB;
            data.width = width;
            data.height = height;
            data.reservoirIdx = writeIdx;
        },
        [](const CompositePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (!data.state->initialRecorded || data.lighting->effective != LightingMode::RTGI)
                return;
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* outTex = fg.GetPhysicalTexture(data.sceneColor);
            if (!depthTex || !normalTex || !baseColorTex || !outTex)
            {
                data.lighting->Fail(LightingFallback::InputsUnavailable);
                return;
            }

            nvrhi::ITexture* directLit = fg.GetPhysicalTexture(data.directLighting);
            nvrhi::ITexture* resA = fg.GetPhysicalTexture(data.reservoirA);
            nvrhi::ITexture* resB = fg.GetPhysicalTexture(data.reservoirB);
            if (!directLit || !resA || !resB)
            {
                Msg("! [RTGI Composite] Null persistent texture: directLit=%d resA=%d resB=%d idx=%d", !!directLit, !!resA, !!resB, data.reservoirIdx);
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(CompositeCB));

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_composite", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Spatial");
            bsb.ConstantBuffer("CompositeParams", data.state->cb);
            bsb.Texture("t_DirectLighting", directLit);
            bsb.Texture("t_ReservoirA", resA);
            bsb.Texture("t_ReservoirB", resB);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.TextureUAV("u_SceneColor", outTex);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->compositeLayout, nvDevice);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->compositePipeline;
            cs.bindings = { bindingSet };

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.state->currTemporalIdx = 1 - data.reservoirIdx;
            data.state->historyValid = true;
            data.lighting->recorded = true;
        });

    return { compositeData.sceneColor };
}

void ShutdownReSTIRGI(ReSTIRGIPassState& state)
{
    state.initialPipeline = nullptr;
    state.initialLayout = nullptr;
    state.temporalPipeline = nullptr;
    state.temporalLayout = nullptr;
    state.compositePipeline = nullptr;
    state.compositeLayout = nullptr;
    state.cb = nullptr;
    state.sampler = nullptr;
    for (int i = 0; i < 2; i++)
    {
        state.reservoirA[i] = nullptr;
        state.reservoirB[i] = nullptr;
    }
    state.directLighting = nullptr;
    s_rtgiPlaceholderBuffer = nullptr;
    state.initialized = false;
    state.readiness = LightingFallback::ResourcesUnavailable;
    state.historyValid = false;
    state.initialRecorded = false;
    state.currTemporalIdx = 0;
    state.texWidth = 0;
    state.texHeight = 0;
}
}
