#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#if defined(XR_PLATFORM_WINDOWS)
#include "Layers/xrRender/Backend/D3D12Backend.h"
#endif
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

static nvrhi::BufferHandle s_placeholderBuffer;
static nvrhi::TextureHandle s_placeholderCube;

struct ReSTIRGICB {
    Fmatrix invViewProj;
    Fmatrix prevViewProj;
    Fvector4 cameraPos;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 frameIndex;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 pad[3];
};
static_assert(sizeof(ReSTIRGICB) == 224, "ReSTIRGICB must be 224 bytes");

struct TemporalCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 pad[3];
};
static_assert(sizeof(TemporalCB) == 112, "TemporalCB must be 112 bytes");

struct CompositeCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos;
    float screenWidth;
    float screenHeight;
    float giIntensity;
    u32 pad;
};
static_assert(sizeof(CompositeCB) == 96, "CompositeCB must be 96 bytes");

static void CreatePlaceholders(nvrhi::IDevice* nvDevice)
{
    if (!s_placeholderBuffer) {
        nvrhi::BufferDesc desc;
        desc.debugName = "RTGI_PlaceholderBuf";
        desc.byteSize = 4;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_placeholderBuffer = nvDevice->createBuffer(desc);
    }
    if (!s_placeholderCube) {
        nvrhi::TextureDesc desc;
        desc.debugName = "RTGI_PlaceholderCube";
        desc.width = 1;
        desc.height = 1;
        desc.dimension = nvrhi::TextureDimension::TextureCube;
        desc.arraySize = 6;
        desc.format = nvrhi::Format::RGBA8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        s_placeholderCube = nvDevice->createTexture(desc);
    }
}

static void InitializeResources(fg::RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized) return;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    CreatePlaceholders(nvDevice);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    state.sampler = cache.GetOrCreateSampler("RTGI", samplerDesc, nvDevice);

    state.cb = cache.GetOrCreateVolatileCB("RTGI", "RTGI_CB",
        (u32)std::max({ sizeof(ReSTIRGICB), sizeof(TemporalCB), sizeof(CompositeCB) }), device);

    // --- Initial pass layout (RT + bindless) ---
    {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_initial");
        if (csResult.handle) {
            state.initialLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Initial", *csResult.reflection, nvDevice);
#if defined(XR_PLATFORM_WINDOWS)
            auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
#else
            nvrhi::IBindingLayout* bindlessLayout = nullptr;
#endif

            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.initialLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.initialLayout };
            state.initialPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    // --- Temporal pass layout ---
    {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_temporal");
        if (csResult.handle) {
            state.temporalLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Temporal", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            pipeDesc.bindingLayouts = { state.temporalLayout };
            state.temporalPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    // --- Composite pass layout ---
    {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_gi_composite");
        if (csResult.handle) {
            state.compositeLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGI_Composite", *csResult.reflection, nvDevice);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = csResult.handle;
            pipeDesc.bindingLayouts = { state.compositeLayout };
            state.compositePipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    state.enabled = state.initialPipeline && state.temporalPipeline && state.compositePipeline;
    state.initialized = true;

    if (state.enabled)
        Msg("* [ReSTIR GI] All pipelines created successfully");
    else
        Msg("! [ReSTIR GI] Pipeline creation failed (initial=%s temporal=%s composite=%s)",
            state.initialPipeline ? "ok" : "FAIL",
            state.temporalPipeline ? "ok" : "FAIL",
            state.compositePipeline ? "ok" : "FAIL");
}

static void EnsurePersistentTextures(nvrhi::IDevice* nvDevice, ReSTIRGIPassState& state, u32 width, u32 height)
{
    if (state.reservoirA[0] && state.texWidth == width && state.texHeight == height)
        return;

    for (int i = 0; i < 2; i++) {
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

struct InitialPassData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    ReSTIRGICB cbData;
    u32 width, height;
    nvrhi::ITexture* sky0;
    nvrhi::ITexture* sky1;
    u32 writeIdx;
};

struct TemporalPassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle prevNormals;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle prevWorldPos;
    VirtualResourceHandle motionVectors;
    TemporalCB cbData;
    u32 width, height;
    u32 readIdx;
    u32 writeIdx;
};

struct CompositePassData {
    fg::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle sceneColorIn;
    VirtualResourceHandle sceneColor;
    CompositeCB cbData;
    u32 width, height;
    u32 reservoirIdx;
};

ReSTIRGIOutput setupReSTIRGIPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle worldPos,
    VirtualResourceHandle prevNormals,
    VirtualResourceHandle prevWorldPos,
    VirtualResourceHandle motionVectors,
    VirtualResourceHandle sceneColorIn,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    const Fvector& cameraPos,
    float giIntensity,
    u32 width, u32 height,
    ReSTIRGIPassState& state,
    bool hasPrevFrameData)
{
    InitializeResources(device, state);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.debugName = "rtgi_SceneColor";
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isUAV = true;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;
    VirtualResourceHandle outHandle = fg.CreateTexture("rtgi_SceneColor", outDesc);

    if (!state.enabled || !accelMgr || !accelMgr->IsReady()) {
        auto& passData = fg.addCallbackPass<CompositePassData>(
            "ReSTIR GI (disabled)",
            [&](FrameGraph& builder, PassHandle passHandle, CompositePassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.sceneColor = pb.write(outHandle, ResourceState::UnorderedAccess);
            },
            [](const CompositePassData&, const FrameGraph&, fg::RenderContext*) {}
        );
        return { passData.sceneColor };
    }

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    EnsurePersistentTextures(nvDevice, state, width, height);

    u32 writeIdx = state.currTemporalIdx;
    u32 readIdx = 1 - writeIdx;

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    auto* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = s_placeholderCube.Get();
    nvrhi::ITexture* sky1Tex = s_placeholderCube.Get();
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1]) {
        if (env.Current[0]->sky_texture_name.size()) {
            auto h0 = texManager->LoadTexture(env.Current[0]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h0);
            if (t) sky0Tex = t;
        }
        if (env.Current[1]->sky_texture_name.size()) {
            auto h1 = texManager->LoadTexture(env.Current[1]->sky_texture_name.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h1);
            if (t) sky1Tex = t;
        }
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
    initialCB.skinnedBatchStart = batchCounts.skinned > 0
        ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal
        : 0;
    initialCB.grassBatchStart = batchCounts.grass > 0
        ? batchCounts.identityStatic + batchCounts.terrain + batchCounts.transparent + batchCounts.instancedTotal + batchCounts.skinned
        : 0;
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
        [&, sky0Tex, sky1Tex, initialCB, writeIdx, fgDirectLighting, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, InitialPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            pb.write(fgDirectLighting, ResourceState::UnorderedAccess);
            pb.write(fgResA, ResourceState::UnorderedAccess);
            pb.write(fgResB, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.device = device;
            data.accelMgr = accelMgr;
            data.state = &state;
            data.cbData = initialCB;
            data.width = width;
            data.height = height;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;
            data.writeIdx = writeIdx;
        },
        [](const InitialPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            if (!depthTex || !normalTex || !baseColorTex || !worldPosTex) {
                Msg("! [RTGI Initial] Null FG texture: depth=%d normal=%d baseColor=%d", !!depthTex, !!normalTex, !!baseColorTex);
                return;
            }

            nvrhi::ITexture* sky0 = data.sky0;
            nvrhi::ITexture* sky1 = data.sky1;
            nvrhi::ITexture* directLit = data.state->directLighting.Get();
            nvrhi::ITexture* resA = data.state->reservoirA[data.writeIdx].Get();
            nvrhi::ITexture* resB = data.state->reservoirB[data.writeIdx].Get();
            auto* tlas = data.accelMgr->GetTLAS();
            auto* batchInfo = data.accelMgr->GetBatchInfoBuffer();
            auto* megaVB = data.accelMgr->GetMegaVB();
            auto* megaIB = data.accelMgr->GetMegaIB();
            auto* matBuf = data.accelMgr->GetMaterialBuffer();
            auto* terrainBuf = data.accelMgr->GetTerrainMaterialBuffer();

            if (!sky0 || !sky1 || !directLit || !resA || !resB ||
                !tlas || !batchInfo || !megaVB || !megaIB || !matBuf || !terrainBuf) {
                Msg("! [RTGI Initial] Null binding: sky0=%d sky1=%d directLit=%d resA=%d resB=%d tlas=%d batch=%d megaVB=%d megaIB=%d mat=%d terrain=%d",
                    !!sky0, !!sky1, !!directLit, !!resA, !!resB, !!tlas, !!batchInfo, !!megaVB, !!megaIB, !!matBuf, !!terrainBuf);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            ReSTIRGICB cb = data.cbData;
            const auto& bc = data.accelMgr->GetBatchCounts();
            cb.identityStaticCount = bc.identityStatic;
            cb.terrainBatchCount = bc.terrain;
            cb.skinnedBatchStart = bc.skinned > 0
                ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal
                : 0;
            cb.grassBatchStart = bc.grass > 0
                ? bc.identityStatic + bc.terrain + bc.transparent + bc.instancedTotal + bc.skinned
                : 0;
            cb.detailAtlasIndex = data.accelMgr->GetDetailAtlasIndex();
            cmdList->writeBuffer(data.state->cb, &cb, sizeof(ReSTIRGICB));

            nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
            nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!skinnedVB) skinnedVB = s_placeholderBuffer.Get();
            if (!skinnedIB) skinnedIB = s_placeholderBuffer.Get();
            if (!grassVB) grassVB = s_placeholderBuffer.Get();
            if (!grassIB) grassIB = s_placeholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_initial", ".cs");
            if (!csReflection) return;

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
            bsb.Texture("t_WorldPos", worldPosTex);
            bsb.TextureUAV("u_DirectLighting", directLit);
            bsb.TextureUAV("u_ReservoirA", resA);
            bsb.TextureUAV("u_ReservoirB", resB);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->initialLayout, nvDevice);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->initialPipeline;
            cs.bindings = { bindingSet };

#if defined(XR_PLATFORM_WINDOWS)
            auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    cs.addBindingSet(bindlessTable);
            }
#endif

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    // ============================================
    //  PASS 2: Temporal Resampling
    // ============================================
    if (hasPrevFrameData && motionVectors.is_valid()) {
        TemporalCB temporalCB;
        temporalCB.invViewProj = invViewProj;
        temporalCB.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0 };
        temporalCB.screenWidth = (float)width;
        temporalCB.screenHeight = (float)height;
        temporalCB.invScreenWidth = 1.0f / width;
        temporalCB.invScreenHeight = 1.0f / height;
        temporalCB.frameIndex = Device.dwFrame;
        temporalCB.pad[0] = temporalCB.pad[1] = temporalCB.pad[2] = 0;

        fg.addCallbackPass<TemporalPassData>(
            "ReSTIR GI Temporal",
            [&, temporalCB, readIdx, writeIdx, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, TemporalPassData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.depth = pb.read(depth, ResourceState::ShaderResource);
                data.normal = pb.read(normal, ResourceState::ShaderResource);
                if (prevNormals.is_valid())
                    data.prevNormals = pb.read(prevNormals, ResourceState::ShaderResource);
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
                if (prevWorldPos.is_valid())
                    data.prevWorldPos = pb.read(prevWorldPos, ResourceState::ShaderResource);
                data.motionVectors = pb.read(motionVectors, ResourceState::ShaderResource);
                pb.readWrite(fgResA, ResourceState::UnorderedAccess);
                pb.readWrite(fgResB, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.device = device;
                data.state = &state;
                data.cbData = temporalCB;
                data.width = width;
                data.height = height;
                data.readIdx = readIdx;
                data.writeIdx = writeIdx;
            },
            [](const TemporalPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* prevNormalsTex = data.prevNormals.is_valid() ? fg.GetPhysicalTexture(data.prevNormals) : normalTex;
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                auto* prevWorldPosTex = data.prevWorldPos.is_valid() ? fg.GetPhysicalTexture(data.prevWorldPos) : worldPosTex;
                auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !mvTex) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalCB));

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_temporal", ".cs");
                if (!csReflection) return;

                framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Temporal");
                bsb.ConstantBuffer("ReSTIRTemporalParams", data.state->cb);
                bsb.Texture("t_PrevReservoirA", data.state->reservoirA[data.readIdx]);
                bsb.Texture("t_PrevReservoirB", data.state->reservoirB[data.readIdx]);
                bsb.Texture("t_MotionVectors", mvTex);
                bsb.Texture("t_Depth", depthTex);
                bsb.Texture("t_Normal", normalTex);
                bsb.Texture("t_PrevNormal", prevNormalsTex);
                bsb.Texture("t_BaseColor", baseColorTex);
                bsb.Texture("t_WorldPos", worldPosTex);
                bsb.Texture("t_PrevWorldPos", prevWorldPosTex);
                bsb.TextureUAV("u_ReservoirA", data.state->reservoirA[data.writeIdx]);
                bsb.TextureUAV("u_ReservoirB", data.state->reservoirB[data.writeIdx]);
                auto& cache = GetPassResourceCache();
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->temporalLayout, nvDevice);
                if (!bindingSet) return;

                nvrhi::ComputeState cs;
                cs.pipeline = data.state->temporalPipeline;
                cs.bindings = { bindingSet };

                cmdList->setComputeState(cs);
                cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
            }
        );
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
        [&, compositeCB, writeIdx, outHandle, fgDirectLighting, fgResA, fgResB](FrameGraph& builder, PassHandle passHandle, CompositePassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.sceneColorIn = pb.read(sceneColorIn, ResourceState::ShaderResource);
            pb.read(fgDirectLighting, ResourceState::ShaderResource);
            pb.read(fgResA, ResourceState::ShaderResource);
            pb.read(fgResB, ResourceState::ShaderResource);
            data.sceneColor = pb.write(outHandle, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.cbData = compositeCB;
            data.width = width;
            data.height = height;
            data.reservoirIdx = writeIdx;
        },
        [](const CompositePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            auto* sceneColorInTex = fg.GetPhysicalTexture(data.sceneColorIn);
            auto* outTex = fg.GetPhysicalTexture(data.sceneColor);
            if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !sceneColorInTex || !outTex) return;

            nvrhi::ITexture* directLit = data.state->directLighting.Get();
            nvrhi::ITexture* resA = data.state->reservoirA[data.reservoirIdx].Get();
            nvrhi::ITexture* resB = data.state->reservoirB[data.reservoirIdx].Get();
            if (!directLit || !resA || !resB) {
                Msg("! [RTGI Composite] Null persistent texture: directLit=%d resA=%d resB=%d idx=%d",
                    !!directLit, !!resA, !!resB, data.reservoirIdx);
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(CompositeCB));

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("restir_gi_composite", ".cs");
            if (!csReflection) return;

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "ReSTIRGI.Spatial");
            bsb.ConstantBuffer("CompositeParams", data.state->cb);
            bsb.Texture("t_DirectLighting", directLit);
            bsb.Texture("t_ReservoirA", resA);
            bsb.Texture("t_ReservoirB", resB);
            bsb.Texture("t_Depth", depthTex);
            bsb.Texture("t_Normal", normalTex);
            bsb.Texture("t_BaseColor", baseColorTex);
            bsb.Texture("t_SceneColorIn", sceneColorInTex);
            bsb.Texture("t_WorldPos", worldPosTex);
            bsb.TextureUAV("u_SceneColor", outTex);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->compositeLayout, nvDevice);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->compositePipeline;
            cs.bindings = { bindingSet };

            cmdList->setComputeState(cs);
            cmdList->dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    state.currTemporalIdx ^= 1;

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
    for (int i = 0; i < 2; i++) {
        state.reservoirA[i] = nullptr;
        state.reservoirB[i] = nullptr;
    }
    state.directLighting = nullptr;
    s_placeholderBuffer = nullptr;
    s_placeholderCube = nullptr;
    state.initialized = false;
    state.enabled = false;
}

}
