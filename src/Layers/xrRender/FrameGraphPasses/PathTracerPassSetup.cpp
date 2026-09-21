#include "stdafx.h"
#include "PathTracerPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/ClusteredLightManager.h"
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
#include "xrEngine/xr_efflensflare.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace fg
{
extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

static nvrhi::ShaderHandle s_pathtrace_shader;
static nvrhi::IBuffer* s_cb = nullptr;
static nvrhi::ComputePipelineHandle s_pipeline;
static nvrhi::BindingLayoutHandle s_layout;
static nvrhi::SamplerHandle s_sampler;
static nvrhi::BufferHandle s_ptPlaceholderBuffer;
static bool s_initialized = false;
static LightingFallback s_readiness = LightingFallback::ResourcesUnavailable;

static void CreatePlaceholderBuffer(nvrhi::IDevice* nvDevice)
{
    if (s_ptPlaceholderBuffer)
        return;

    nvrhi::BufferDesc desc;
    desc.debugName = "PT_PlaceholderBuffer";
    desc.byteSize = 4;
    desc.canHaveRawViews = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    s_ptPlaceholderBuffer = nvDevice->createBuffer(desc);
}

static LightingFallback InitializeResources(RenderDevice* device)
{
    if (s_initialized)
        return s_readiness;

    auto* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* backend = device->GetBackend();
    if (!shaderLoader || !backend || !backend->GetBindlessLayout() || !backend->GetBindlessDescriptorTable())
        return s_readiness;

    s_initialized = true;
    auto csResult = shaderLoader->LoadComputeShader("rt_pathtrace");
    if (!csResult.handle || !csResult.reflection)
        return s_readiness = LightingFallback::ShaderUnavailable;
    s_pathtrace_shader = csResult.handle;

    auto& cache = GetPassResourceCache();
    s_cb = cache.GetOrCreateVolatileCB("PathTracer", "PathTracerCB", sizeof(PathTracerCB), device);
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    s_sampler = cache.GetOrCreateSampler("PathTracer", samplerDesc, nvDevice);
    CreatePlaceholderBuffer(nvDevice);
    if (!s_cb || !s_sampler || !s_ptPlaceholderBuffer)
    {
        s_initialized = false;
        return s_readiness;
    }

    s_layout = cache.GetOrCreateBindingLayoutFromReflection("PathTracer", *csResult.reflection, nvDevice);
    if (!s_layout)
        return s_readiness = LightingFallback::PipelineUnavailable;
    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = s_pathtrace_shader;
    pipeDesc.bindingLayouts = { s_layout, backend->GetBindlessLayout() };
    s_pipeline = nvDevice->createComputePipeline(pipeDesc);
    s_readiness = s_pipeline ? LightingFallback::None : LightingFallback::PipelineUnavailable;
    return s_readiness;
}

static void EnsureAccumulationBuffer(nvrhi::IDevice* nvDevice, u32 width, u32 height, PathTracerPassState& state)
{
    if (state.accumulation && state.width == width && state.height == height)
        return;

    nvrhi::TextureDesc desc;
    desc.debugName = "PT_Accumulation";
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA32_FLOAT;
    desc.isUAV = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;

    state.accumulation = nvDevice->createTexture(desc);
    state.history.valid = false;
    state.width = width;
    state.height = height;
}

LightingFallback EnsurePathTracerResources(RenderDevice* device, u32 width, u32 height, PathTracerPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !width || !height)
        return LightingFallback::ResourcesUnavailable;
    const auto readiness = InitializeResources(device);
    if (readiness != LightingFallback::None)
        return readiness;
    EnsureAccumulationBuffer(device->GetNVRHIDevice(), width, height, state);
    return state.accumulation ? LightingFallback::None : LightingFallback::ResourcesUnavailable;
}

PathTracerOutput setupPathTracerPass(FrameGraph& fg, fg::RenderDevice* device, RTAccelStructManager* accelMgr, VirtualResourceHandle sceneColorIn,
    const ClusterLightOutput& clusterLights, LightingFrameState& lighting, const PathTracerConfig& config,
    const Fmatrix& invViewProj, const Fvector& cameraPos, u32 width, u32 height,
    PathTracerPassState& state)
{
    state.pending.valid = false;
    const auto readiness = EnsurePathTracerResources(device, width, height, state);
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
    if (!sceneColorIn.is_valid() || !g_pGamePersistent)
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

    ResourceDesc accumulationDesc;
    accumulationDesc.width = width;
    accumulationDesc.height = height;
    accumulationDesc.format = nvrhi::Format::RGBA32_FLOAT;
    accumulationDesc.isUAV = true;
    accumulationDesc.isImported = true;
    accumulationDesc.isTransient = false;
    const auto accumulation = fg.ImportTexture("pt_Accumulation", state.accumulation, accumulationDesc);

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    resources::TextureManager* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1])
    {
        const shared_str& name0 = env.Current[0]->sky_texture_name;
        const shared_str& name1 = env.Current[1]->sky_texture_name;
        if (name0.size())
        {
            auto h = texManager->LoadTexture(name0.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
            if (t)
                sky0Tex = t;
        }
        if (name1.size())
        {
            auto h = texManager->LoadTexture(name1.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
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

    auto& lightManager = ClusteredLightManager::Instance();
    if (!lightManager.GetLightDataBuffer() ||
        (lightManager.GetLightCount() > 0 && (!clusterLights.active || !clusterLights.lightData.is_valid())))
    {
        lighting.Fail(LightingFallback::ResourcesUnavailable);
        return { sceneColorIn };
    }
    auto lightData = clusterLights.lightData;
    if (!lightData.is_valid())
    {
        auto* buffer = lightManager.GetLightDataBuffer();
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.structStride = buffer->getDesc().structStride;
        desc.isImported = true;
        desc.isTransient = false;
        lightData = fg.ImportBuffer("cluster_light_data", buffer, desc);
    }

    SunLightData sun = {};
    GetSunLightData(sun);
    const Fvector sunDir = sun.direction;
    const Fvector sc = sun.color;
    const float sunIntensity = std::max({ sc.x, sc.y, sc.z });
    Fvector sunColor;
    if (sunIntensity > 0.001f)
        sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
    else
        sunColor.set(0, 0, 0);

    PathTracerCB cbData = {};
    cbData.invViewProj = invViewProj;
    cbData.cameraPos_pad = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f };
    cbData.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    cbData.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
    cbData.screenWidth = static_cast<float>(width);
    cbData.screenHeight = static_cast<float>(height);
    cbData.sampleIndex = 0;
    cbData.maxBounces = config.maxBounces;

    const auto& batchCounts = accelMgr->GetBatchCounts();
    cbData.identityStaticCount = batchCounts.identityStatic;
    cbData.terrainBatchCount = batchCounts.terrain;
    cbData.transparentBatchCount = batchCounts.transparent;

    if (batchCounts.skinned > 0)
        cbData.skinnedBatchStart = batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal;
    else
        cbData.skinnedBatchStart = 0;

    if (batchCounts.grass > 0)
        cbData.grassBatchStart = batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned;
    else
        cbData.grassBatchStart = 0;

    cbData.detailAtlasIndex = accelMgr->GetDetailAtlasIndex();
    cbData.diffuseMode = config.diffuseMode;
    cbData.lightCount = lightManager.GetLightCount();

    state.pending.parameters = cbData;
    state.pending.sky0 = sky0Tex;
    state.pending.sky1 = sky1Tex;
    state.pending.sceneRevision = accelMgr->GetSceneRevision();
    state.pending.textureRevision = accelMgr->GetTextureRevision(sky0Tex, sky1Tex);
    state.pending.lightingSignature = lightManager.GetTransportSignature();
    state.pending.foliageSSS.set(ps_r_foliage_sss_tint.x, ps_r_foliage_sss_tint.y,
        ps_r_foliage_sss_tint.z, ps_r_foliage_sss_sigma);
    state.pending.foliageParams.set(ps_r_foliage_sss_blade, ps_r_foliage_sss_tuft,
        ps_r_foliage_sss_tree, ps_r_foliage_sss_ambient);
    state.pending.foliageParams2.set(ps_r_foliage_sss_forward, 0.0f, 0.0f, 0.0f);
    const auto& history = state.history;
    if (history.valid && history.sceneRevision == state.pending.sceneRevision &&
        history.textureRevision == state.pending.textureRevision &&
        history.lightingSignature == state.pending.lightingSignature &&
        memcmp(&history.foliageSSS, &state.pending.foliageSSS, sizeof(Fvector4)) == 0 &&
        memcmp(&history.foliageParams, &state.pending.foliageParams, sizeof(Fvector4)) == 0 &&
        memcmp(&history.foliageParams2, &state.pending.foliageParams2, sizeof(Fvector4)) == 0 &&
        history.sky0 == sky0Tex && history.sky1 == sky1Tex &&
        memcmp(&history.parameters, &cbData, sizeof(cbData)) == 0)
        cbData.sampleIndex = history.samples;
    lighting.historyUsed = cbData.sampleIndex != 0;

    auto& passData = fg.addCallbackPass<PathTracerData>(
        "Path Tracer",

        [&, width, height, cbData, sky0Tex, sky1Tex](FrameGraph& builder, PassHandle passHandle, PathTracerData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.lighting = &lighting;
            data.state = &state;
            data.scene = accelMgr->UseScene(builder, passBuilder);
            data.lightData = passBuilder.read(lightData, ResourceState::ShaderResource);
            data.width = width;
            data.height = height;
            data.cbData = cbData;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;

            data.accumulation = passBuilder.readWrite(accumulation, ResourceState::UnorderedAccess);
            data.outputTex = passBuilder.readWrite(sceneColorIn, ResourceState::UnorderedAccess);
        },

        [](const PathTracerData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            if (s_readiness != LightingFallback::None)
            {
                data.lighting->Fail(s_readiness);
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            nvrhi::ITexture* outTex = fg.GetPhysicalTexture(data.outputTex);
            auto* accumulationTex = fg.GetPhysicalTexture(data.accumulation);
            if (!outTex || !accumulationTex)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            cmdList->writeBuffer(s_cb, &data.cbData, sizeof(PathTracerCB));

            const auto scene = RTAccelStructManager::ResolveScene(fg, data.scene);
            if (!scene.tlas || !scene.batchInfo || !scene.vertices || !scene.indices || !scene.materials ||
                !scene.terrainMaterials || !scene.variants || !scene.textures)
            {
                data.lighting->Fail(LightingFallback::SceneUnavailable);
                return;
            }
            nvrhi::IBuffer* skinnedVB = scene.skinnedVertices;
            nvrhi::IBuffer* skinnedIB = scene.skinnedIndices;
            nvrhi::IBuffer* grassVB = scene.grassVertices;
            nvrhi::IBuffer* grassIB = scene.grassIndices;
            if (!skinnedVB)
                skinnedVB = s_ptPlaceholderBuffer.Get();
            if (!skinnedIB)
                skinnedIB = s_ptPlaceholderBuffer.Get();
            if (!grassVB)
                grassVB = s_ptPlaceholderBuffer.Get();
            if (!grassIB)
                grassIB = s_ptPlaceholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rt_pathtrace", ".cs");
            if (!csReflection)
            {
                data.lighting->Fail(LightingFallback::ShaderUnavailable);
                return;
            }

            auto* lightData = fg.GetPhysicalBuffer(data.lightData);
            auto* staticGlobals = GetPassResourceCache().GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            if (!lightData || !staticGlobals)
            {
                data.lighting->Fail(LightingFallback::ResourcesUnavailable);
                return;
            }

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "PathTracer");
            bsb.ConstantBuffer("PathTracerParams", s_cb);
            bsb.ConstantBuffer("static_globals", staticGlobals);
            bsb.BufferSRV("g_LightData", lightData);
            bsb.AccelStruct("g_SceneTLAS", scene.tlas);
            bsb.BufferSRV("g_BatchInfo", scene.batchInfo);
            bsb.BufferSRV("g_MegaVB", scene.vertices);
            bsb.BufferSRV("g_MegaIB", scene.indices);
            bsb.Texture("g_Sky0", data.sky0);
            bsb.Texture("g_Sky1", data.sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", scene.materials);
            bsb.BufferSRV("g_TerrainMaterials", scene.terrainMaterials);
            bsb.BufferSRV("g_Variants", scene.variants);
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.TextureUAV("g_Accumulation", accumulationTex);
            bsb.TextureUAV("g_Output", outTex);
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), s_layout);
            if (!bindingSet)
            {
                data.lighting->Fail(LightingFallback::BindingUnavailable);
                return;
            }

            nvrhi::ComputeState state;
            state.pipeline = s_pipeline;
            state.bindings = { bindingSet };

            if (scene.textures)
                state.addBindingSet(scene.textures);

            cmdList->setComputeState(state);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            data.state->pending.valid = true;
            data.state->pending.samples = data.cbData.sampleIndex + 1;
            data.lighting->recorded = true;
            data.lighting->recordedSamples = data.cbData.sampleIndex + 1;
        });

    return { passData.outputTex };
}

void ShutdownPathTracer()
{
    s_pathtrace_shader = nullptr;
    s_cb = nullptr;
    s_pipeline = nullptr;
    s_layout = nullptr;
    s_sampler = nullptr;
    s_ptPlaceholderBuffer = nullptr;
    s_initialized = false;
    s_readiness = LightingFallback::ResourcesUnavailable;
}
}
