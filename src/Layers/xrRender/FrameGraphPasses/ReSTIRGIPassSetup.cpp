#include "stdafx.h"
#include "ReSTIRGIPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace xray::render::RENDER_NAMESPACE::passes {

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
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    u32 frameIndex;
    u32 pad[3];
};
static_assert(sizeof(TemporalCB) == 96, "TemporalCB must be 96 bytes");

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

static void InitializeResources(ng::RenderDevice* device, ReSTIRGIPassState& state)
{
    if (state.initialized) return;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    CreatePlaceholders(nvDevice);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    state.sampler = cache.GetOrCreateSampler("RTGI", samplerDesc, nvDevice);

    nvrhi::BufferDesc cbDesc;
    cbDesc.debugName = "RTGI_CB";
    cbDesc.byteSize = std::max({ sizeof(ReSTIRGICB), sizeof(TemporalCB), sizeof(CompositeCB) });
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.keepInitialState = true;
    cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    state.cb = nvDevice->createBuffer(cbDesc);

    // --- Initial pass layout (RT + bindless) ---
    {
        nvrhi::BindingLayoutDesc desc;
        desc.visibility = nvrhi::ShaderType::Compute;
        desc.bindings = {
            nvrhi::BindingLayoutItem::RayTracingAccelStruct(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(3),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(4),
            nvrhi::BindingLayoutItem::Texture_SRV(5),
            nvrhi::BindingLayoutItem::Texture_SRV(6),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(7),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(9),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(11),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(12),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(13),
            nvrhi::BindingLayoutItem::Texture_SRV(14),
            nvrhi::BindingLayoutItem::Texture_SRV(15),
            nvrhi::BindingLayoutItem::Texture_SRV(16),
            nvrhi::BindingLayoutItem::Texture_SRV(17),
            nvrhi::BindingLayoutItem::Texture_UAV(0),
            nvrhi::BindingLayoutItem::Texture_UAV(1),
            nvrhi::BindingLayoutItem::Texture_UAV(2),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::Sampler(0),
        };
        state.initialLayout = cache.GetOrCreateBindingLayout("RTGI_Initial", desc, nvDevice);

        ref_cs shader;
        shader.create("restir_gi_initial");
        if (shader && shader->nvrhiShader) {
            auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = shader->nvrhiShader;
            if (bindlessLayout)
                pipeDesc.bindingLayouts = { state.initialLayout, bindlessLayout };
            else
                pipeDesc.bindingLayouts = { state.initialLayout };
            state.initialPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    // --- Temporal pass layout ---
    {
        nvrhi::BindingLayoutDesc desc;
        desc.visibility = nvrhi::ShaderType::Compute;
        desc.bindings = {
            nvrhi::BindingLayoutItem::Texture_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Texture_SRV(2),
            nvrhi::BindingLayoutItem::Texture_SRV(3),
            nvrhi::BindingLayoutItem::Texture_SRV(4),
            nvrhi::BindingLayoutItem::Texture_SRV(5),
            nvrhi::BindingLayoutItem::Texture_SRV(6),
            nvrhi::BindingLayoutItem::Texture_SRV(7),
            nvrhi::BindingLayoutItem::Texture_SRV(8),
            nvrhi::BindingLayoutItem::Texture_UAV(0),
            nvrhi::BindingLayoutItem::Texture_UAV(1),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
        };
        state.temporalLayout = cache.GetOrCreateBindingLayout("RTGI_Temporal_v2", desc, nvDevice);

        ref_cs shader;
        shader.create("restir_gi_temporal");
        if (shader && shader->nvrhiShader) {
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = shader->nvrhiShader;
            pipeDesc.bindingLayouts = { state.temporalLayout };
            state.temporalPipeline = nvDevice->createComputePipeline(pipeDesc);
        }
    }

    // --- Composite pass layout ---
    {
        nvrhi::BindingLayoutDesc desc;
        desc.visibility = nvrhi::ShaderType::Compute;
        desc.bindings = {
            nvrhi::BindingLayoutItem::Texture_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Texture_SRV(2),
            nvrhi::BindingLayoutItem::Texture_SRV(3),
            nvrhi::BindingLayoutItem::Texture_SRV(4),
            nvrhi::BindingLayoutItem::Texture_SRV(5),
            nvrhi::BindingLayoutItem::Texture_SRV(6),
            nvrhi::BindingLayoutItem::Texture_SRV(7),
            nvrhi::BindingLayoutItem::Texture_UAV(0),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
        };
        state.compositeLayout = cache.GetOrCreateBindingLayout("RTGI_Composite_v3", desc, nvDevice);

        ref_cs shader;
        shader.create("restir_gi_composite");
        if (shader && shader->nvrhiShader) {
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = shader->nvrhiShader;
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
    ng::RenderDevice* device;
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
    ng::RenderDevice* device;
    ReSTIRGIPassState* state;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle motionVectors;
    TemporalCB cbData;
    u32 width, height;
    u32 readIdx;
    u32 writeIdx;
};

struct CompositePassData {
    ng::RenderDevice* device;
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
    ng::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle worldPos,
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
            [](const CompositePassData&, const FrameGraph&, ng::RenderContext*) {}
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
        [](const InitialPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
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

            cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(ReSTIRGICB));

            nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
            nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!skinnedVB) skinnedVB = s_placeholderBuffer.Get();
            if (!skinnedIB) skinnedIB = s_placeholderBuffer.Get();
            if (!grassVB) grassVB = s_placeholderBuffer.Get();
            if (!grassIB) grassIB = s_placeholderBuffer.Get();

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::RayTracingAccelStruct(1, tlas),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(2, batchInfo),
                nvrhi::BindingSetItem::RawBuffer_SRV(3, megaVB),
                nvrhi::BindingSetItem::RawBuffer_SRV(4, megaIB),
                nvrhi::BindingSetItem::Texture_SRV(5, sky0),
                nvrhi::BindingSetItem::Texture_SRV(6, sky1),
                nvrhi::BindingSetItem::RawBuffer_SRV(7, skinnedVB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuf),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(9, terrainBuf),
                nvrhi::BindingSetItem::RawBuffer_SRV(11, skinnedIB),
                nvrhi::BindingSetItem::RawBuffer_SRV(12, grassVB),
                nvrhi::BindingSetItem::RawBuffer_SRV(13, grassIB),
                nvrhi::BindingSetItem::Texture_SRV(14, depthTex),
                nvrhi::BindingSetItem::Texture_SRV(15, normalTex),
                nvrhi::BindingSetItem::Texture_SRV(16, baseColorTex),
                nvrhi::BindingSetItem::Texture_SRV(17, worldPosTex),
                nvrhi::BindingSetItem::Texture_UAV(0, directLit),
                nvrhi::BindingSetItem::Texture_UAV(1, resA),
                nvrhi::BindingSetItem::Texture_UAV(2, resB),
                nvrhi::BindingSetItem::ConstantBuffer(5, data.state->cb),
                nvrhi::BindingSetItem::Sampler(0, data.state->sampler),
            };
            auto bindingSet = nvDevice->createBindingSet(bindDesc, data.state->initialLayout);
            if (!bindingSet) return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.state->initialPipeline;
            cs.bindings = { bindingSet };

            auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    cs.addBindingSet(bindlessTable);
            }

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
                data.baseColor = pb.read(baseColor, ResourceState::ShaderResource);
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
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
            [](const TemporalPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
                auto* depthTex = fg.GetPhysicalTexture(data.depth);
                auto* normalTex = fg.GetPhysicalTexture(data.normal);
                auto* baseColorTex = fg.GetPhysicalTexture(data.baseColor);
                auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
                auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
                if (!depthTex || !normalTex || !baseColorTex || !worldPosTex || !mvTex) return;

                nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
                nvrhi::ICommandList* cmdList = ctx->GetCommandList();

                cmdList->writeBuffer(data.state->cb, &data.cbData, sizeof(TemporalCB));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::Texture_SRV(0, data.state->reservoirA[data.readIdx]),
                    nvrhi::BindingSetItem::Texture_SRV(1, data.state->reservoirB[data.readIdx]),
                    nvrhi::BindingSetItem::Texture_SRV(2, mvTex),
                    nvrhi::BindingSetItem::Texture_SRV(3, depthTex),
                    nvrhi::BindingSetItem::Texture_SRV(4, normalTex),
                    nvrhi::BindingSetItem::Texture_SRV(5, depthTex),
                    nvrhi::BindingSetItem::Texture_SRV(6, normalTex),
                    nvrhi::BindingSetItem::Texture_SRV(7, baseColorTex),
                    nvrhi::BindingSetItem::Texture_SRV(8, worldPosTex),
                    nvrhi::BindingSetItem::Texture_UAV(0, data.state->reservoirA[data.writeIdx]),
                    nvrhi::BindingSetItem::Texture_UAV(1, data.state->reservoirB[data.writeIdx]),
                    nvrhi::BindingSetItem::ConstantBuffer(5, data.state->cb),
                };
                auto bindingSet = nvDevice->createBindingSet(bindDesc, data.state->temporalLayout);
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
        [](const CompositePassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
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

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::Texture_SRV(0, directLit),
                nvrhi::BindingSetItem::Texture_SRV(1, resA),
                nvrhi::BindingSetItem::Texture_SRV(2, resB),
                nvrhi::BindingSetItem::Texture_SRV(3, depthTex),
                nvrhi::BindingSetItem::Texture_SRV(4, normalTex),
                nvrhi::BindingSetItem::Texture_SRV(5, baseColorTex),
                nvrhi::BindingSetItem::Texture_SRV(6, sceneColorInTex),
                nvrhi::BindingSetItem::Texture_SRV(7, worldPosTex),
                nvrhi::BindingSetItem::Texture_UAV(0, outTex),
                nvrhi::BindingSetItem::ConstantBuffer(5, data.state->cb),
            };
            auto bindingSet = nvDevice->createBindingSet(bindDesc, data.state->compositeLayout);
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
