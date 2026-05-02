#include "stdafx.h"
#include "PathTracerPassSetup.h"
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
#include "xrEngine/xr_efflensflare.h"
#include "xrEngine/IGame_Persistent.h"
#include <nvrhi/utils.h>

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

static nvrhi::ShaderHandle s_pathtrace_shader;
static nvrhi::IBuffer* s_cb = nullptr;
static nvrhi::ComputePipelineHandle s_pipeline;
static nvrhi::BindingLayoutHandle s_layout;
static nvrhi::SamplerHandle s_sampler;
static nvrhi::TextureHandle s_accumBuffer;
static nvrhi::TextureHandle s_placeholderCube;
static nvrhi::BufferHandle s_placeholderBuffer;
static u32 s_accumWidth = 0;
static u32 s_accumHeight = 0;
static bool s_initialized = false;
static bool s_enabled = false;

struct PathTracerCB {
    Fmatrix invViewProj;
    Fvector4 cameraPos_pad;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    u32 sampleIndex;
    u32 maxBounces;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 transparentBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 pad[2];
};
static_assert(sizeof(PathTracerCB) == 160, "PathTracerCB must be 160 bytes");

static void CreatePlaceholderCubemap(nvrhi::IDevice* nvDevice)
{
    if (s_placeholderCube) return;

    nvrhi::TextureDesc desc;
    desc.debugName = "PT_PlaceholderCube";
    desc.width = 1;
    desc.height = 1;
    desc.dimension = nvrhi::TextureDimension::TextureCube;
    desc.arraySize = 6;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.isRenderTarget = false;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    s_placeholderCube = nvDevice->createTexture(desc);
}

static void CreatePlaceholderBuffer(nvrhi::IDevice* nvDevice)
{
    if (s_placeholderBuffer) return;

    nvrhi::BufferDesc desc;
    desc.debugName = "PT_PlaceholderBuffer";
    desc.byteSize = 4;
    desc.canHaveRawViews = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    s_placeholderBuffer = nvDevice->createBuffer(desc);
}

static void InitializeResources(fg::RenderDevice* device)
{
    if (s_initialized) return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto& cache = GetPassResourceCache();

    auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_pathtrace");
    if (!csResult.handle) {
        Msg("! [PathTracer] Failed to load rt_pathtrace shader");
        s_initialized = true;
        return;
    }
    s_pathtrace_shader = csResult.handle;

    s_cb = cache.GetOrCreateVolatileCB("PathTracer", "PathTracerCB", sizeof(PathTracerCB), device);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    s_sampler = cache.GetOrCreateSampler("PathTracer", samplerDesc, nvDevice);

    CreatePlaceholderCubemap(nvDevice);
    CreatePlaceholderBuffer(nvDevice);

    s_layout = cache.GetOrCreateBindingLayoutFromReflection("PathTracer", *csResult.reflection, nvDevice);

#if defined(XR_PLATFORM_WINDOWS)
    auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
#else
    nvrhi::IBindingLayout* bindlessLayout = nullptr;
#endif

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = s_pathtrace_shader;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { s_layout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { s_layout };
    s_pipeline = nvDevice->createComputePipeline(pipeDesc);

    s_enabled = s_pipeline != nullptr;
    s_initialized = true;

    if (s_enabled)
        Msg("* [PathTracer] Pipeline created successfully");
    else
        Msg("! [PathTracer] Pipeline creation failed");
}

static void EnsureAccumulationBuffer(nvrhi::IDevice* nvDevice, u32 width, u32 height)
{
    if (s_accumBuffer && s_accumWidth == width && s_accumHeight == height)
        return;

    nvrhi::TextureDesc desc;
    desc.debugName = "PT_Accumulation";
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA32_FLOAT;
    desc.isUAV = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;

    s_accumBuffer = nvDevice->createTexture(desc);
    s_accumWidth = width;
    s_accumHeight = height;
}

struct PathTracerData {
    fg::RenderDevice* device;
    RTAccelStructManager* accelMgr;
    VirtualResourceHandle outputTex;
    PathTracerCB cbData;
    u32 width, height;
    nvrhi::ITexture* sky0;
    nvrhi::ITexture* sky1;
};

PathTracerOutput setupPathTracerPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    RTAccelStructManager* accelMgr,
    const PathTracerConfig& config,
    const Fmatrix& invViewProj,
    const Fvector& cameraPos,
    u32 width,
    u32 height)
{
    InitializeResources(device);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.debugName = "pt_SceneColor";
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isUAV = true;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;

    VirtualResourceHandle outHandle = fg.CreateTexture("pt_SceneColor", outDesc);

    if (!s_enabled || !accelMgr || !accelMgr->IsReady()) {
        auto& passData = fg.addCallbackPass<PathTracerData>(
            "Path Tracer (disabled)",
            [&](FrameGraph& builder, PassHandle passHandle, PathTracerData& data) {
                RenderPassBuilder passBuilder(builder, passHandle);
                data.outputTex = passBuilder.write(outHandle, ResourceState::UnorderedAccess);
            },
            [](const PathTracerData&, const FrameGraph&, fg::RenderContext*) {}
        );
        return { passData.outputTex };
    }

    EnsureAccumulationBuffer(device->GetNVRHIDevice(), width, height);

    CEnvironment& env = g_pGamePersistent->Environment();
    auto* resourceManager = device->GetFGResourceManager();
    resources::TextureManager* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

    nvrhi::ITexture* sky0Tex = s_placeholderCube.Get();
    nvrhi::ITexture* sky1Tex = s_placeholderCube.Get();
    float skyWeight = env.CurrentEnv.weight;

    if (texManager && env.Current[0] && env.Current[1]) {
        const shared_str& name0 = env.Current[0]->sky_texture_name;
        const shared_str& name1 = env.Current[1]->sky_texture_name;
        if (name0.size()) {
            auto h = texManager->LoadTexture(name0.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
            if (t) sky0Tex = t;
        }
        if (name1.size()) {
            auto h = texManager->LoadTexture(name1.c_str());
            nvrhi::ITexture* t = texManager->GetNVRHITexture(h);
            if (t) sky1Tex = t;
        }
    }

    Fvector sunDir = { 0, -1, 0 };
    Fvector sunColor = { 1, 1, 1 };
    float sunIntensity = 1.0f;
        sunDir = env.CurrentEnv.sun_dir;
        Fvector3 sc;
        sc.x = env.CurrentEnv.sun_color.x;
        sc.y = env.CurrentEnv.sun_color.y;
        sc.z = env.CurrentEnv.sun_color.z;
        sunIntensity = std::max({ sc.x, sc.y, sc.z });
        if (sunIntensity > 0.001f)
            sunColor.set(sc.x / sunIntensity, sc.y / sunIntensity, sc.z / sunIntensity);
        else
            sunColor.set(0, 0, 0);

    PathTracerCB cbData;
    cbData.invViewProj = invViewProj;
    cbData.cameraPos_pad = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f };
    cbData.sunDir_intensity = { sunDir.x, sunDir.y, sunDir.z, sunIntensity };
    cbData.sunColor_skyWeight = { sunColor.x, sunColor.y, sunColor.z, skyWeight };
    cbData.screenWidth = static_cast<float>(width);
    cbData.screenHeight = static_cast<float>(height);
    cbData.sampleIndex = config.sampleIndex;
    cbData.maxBounces = config.maxBounces;

    const auto& batchCounts = accelMgr->GetBatchCounts();
    cbData.identityStaticCount = batchCounts.identityStatic;
    cbData.terrainBatchCount = batchCounts.terrain;
    cbData.transparentBatchCount = batchCounts.transparent;

    if (batchCounts.skinned > 0)
        cbData.skinnedBatchStart = batchCounts.identityStatic + batchCounts.terrain +
                                   batchCounts.transparent + batchCounts.instancedTotal;
    else
        cbData.skinnedBatchStart = 0;

    if (batchCounts.grass > 0)
        cbData.grassBatchStart = batchCounts.identityStatic + batchCounts.terrain +
                                 batchCounts.transparent + batchCounts.instancedTotal +
                                 batchCounts.skinned;
    else
        cbData.grassBatchStart = 0;

    cbData.detailAtlasIndex = accelMgr->GetDetailAtlasIndex();
    cbData.pad[0] = 0;
    cbData.pad[1] = 0;

    auto& passData = fg.addCallbackPass<PathTracerData>(
        "Path Tracer",

        [&, width, height, cbData, sky0Tex, sky1Tex](FrameGraph& builder, PassHandle passHandle, PathTracerData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.accelMgr = accelMgr;
            data.width = width;
            data.height = height;
            data.cbData = cbData;
            data.sky0 = sky0Tex;
            data.sky1 = sky1Tex;

            data.outputTex = passBuilder.write(outHandle, ResourceState::UnorderedAccess);
        },

        [](const PathTracerData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (!s_enabled) return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            nvrhi::ITexture* outTex = fg.GetPhysicalTexture(data.outputTex);
            if (!outTex || !s_accumBuffer) return;

            cmdList->writeBuffer(s_cb, &data.cbData, sizeof(PathTracerCB));

            nvrhi::IBuffer* skinnedVB = data.accelMgr->GetSkinnedOutputVB();
            nvrhi::IBuffer* skinnedIB = data.accelMgr->GetSkinnedIB();
            nvrhi::IBuffer* grassVB = data.accelMgr->GetGrassOutputVB();
            nvrhi::IBuffer* grassIB = data.accelMgr->GetGrassIB();
            if (!skinnedVB) skinnedVB = s_placeholderBuffer.Get();
            if (!skinnedIB) skinnedIB = s_placeholderBuffer.Get();
            if (!grassVB) grassVB = s_placeholderBuffer.Get();
            if (!grassIB) grassIB = s_placeholderBuffer.Get();

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* csReflection = shaderLoader->GetCachedReflection("rt_pathtrace", ".cs");
            if (!csReflection) return;

            framegraph::BindingSetBuilder bsb(*csReflection, nvDevice, "PathTracer");
            bsb.ConstantBuffer("PathTracerParams", s_cb);
            bsb.AccelStruct("g_SceneTLAS", data.accelMgr->GetTLAS());
            bsb.BufferSRV("g_BatchInfo", data.accelMgr->GetBatchInfoBuffer());
            bsb.BufferSRV("g_MegaVB", data.accelMgr->GetMegaVB());
            bsb.BufferSRV("g_MegaIB", data.accelMgr->GetMegaIB());
            bsb.Texture("g_Sky0", data.sky0);
            bsb.Texture("g_Sky1", data.sky1);
            bsb.BufferSRV("g_SkinnedVB", skinnedVB);
            bsb.BufferSRV("g_Materials", data.accelMgr->GetMaterialBuffer());
            bsb.BufferSRV("g_TerrainMaterials", data.accelMgr->GetTerrainMaterialBuffer());
            bsb.BufferSRV("g_SkinnedIB", skinnedIB);
            bsb.BufferSRV("g_GrassVB", grassVB);
            bsb.BufferSRV("g_GrassIB", grassIB);
            bsb.TextureUAV("g_Accumulation", s_accumBuffer);
            bsb.TextureUAV("g_Output", outTex);
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), s_layout, nvDevice);

            nvrhi::ComputeState state;
            state.pipeline = s_pipeline;
            state.bindings = { bindingSet };

#if defined(XR_PLATFORM_WINDOWS)
            auto* backend = dynamic_cast<D3D12Backend*>(GEnv.Backend);
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
            }
#endif

            cmdList->setComputeState(state);
            cmdList->dispatch(
                (data.width + 7) / 8,
                (data.height + 7) / 8,
                1
            );
        }
    );

    return { passData.outputTex };
}

void ShutdownPathTracer()
{
    s_pathtrace_shader = nullptr;
    s_cb = nullptr;
    s_pipeline = nullptr;
    s_layout = nullptr;
    s_sampler = nullptr;
    s_accumBuffer = nullptr;
    s_placeholderCube = nullptr;
    s_placeholderBuffer = nullptr;
    s_accumWidth = 0;
    s_accumHeight = 0;
    s_initialized = false;
    s_enabled = false;
}

}
