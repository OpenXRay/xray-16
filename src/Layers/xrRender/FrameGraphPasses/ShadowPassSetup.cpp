#include "stdafx.h"
#include "ShadowPassSetup.h"
#include "ShadowCascadeFit.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/ResourceManager/NativeRTFactory.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "xrEngine/IRenderBackend.h"
#include "Layers/xrRender/r__sector.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"

extern ENGINE_API float ps_r3_grass_blade_height;
extern ENGINE_API float psHUD_FOV;

namespace xray::render::fg
{
extern int ps_r__detail_gpu;
}

namespace xray::render::fg::passes
{

ShadowCascadeGPUData g_ShadowCascadeGPUData;

u32 GetCSMResolution()
{
    u32 s = ps_r2_smapsize;
    if (s < 512)
        s = 512;
    if (s > 8192)
        s = 8192;
    return s;
}

u32 GetCSMCascadeResolution(u32 cascade)
{
    u32 base = GetCSMResolution();
    u32 div = 1u << std::min(cascade, 2u); // 1, 2, 4
    u32 s = base / div;
    if (s < 512)
        s = 512;
    return s;
}

namespace
{

// Original r2 sun cascades (render_phase_sun.cpp): 20 / 40 / 160.
// Cascade ortho footprints are derived from r2_sun_near / r2_sun_far at setup time.

} // namespace

void InitializeShadowPass(fg::RenderDevice* device, ShadowPassState& state)
{
    if (!device)
        return;

    // Hot-upgrade: uniform Texture2DArray CSM → per-cascade Texture2D ladder
    if (state.initialized && state.enabled)
    {
        const bool sharedArray =
            state.shadowCascades[0] &&
            state.shadowCascades[0] == state.shadowCascades[1];
        const bool flatRes =
            state.cascadeResolution[0] > 0 &&
            state.cascadeResolution[0] == state.cascadeResolution[1] &&
            GetCSMCascadeResolution(0) != GetCSMCascadeResolution(1);
        if (sharedArray || flatRes)
            ShutdownShadowPass(device, state);
    }

    // Hot-upgrade: local shadow tiles need more volatile CB versions than CSM-only (16).
    constexpr u32 kCascadeCBVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    if (state.initialized && state.enabled && state.cascadeCB &&
        state.cascadeCBMaxVersions < kCascadeCBVersions)
    {
        nvrhi::IDevice* nv = device->GetNVRHIDevice();
        if (nv)
        {
            nvrhi::BufferDesc cbDesc;
            cbDesc.byteSize = sizeof(ShadowCascadeCB);
            cbDesc.isConstantBuffer = true;
            cbDesc.isVolatile = true;
            cbDesc.maxVersions = kCascadeCBVersions;
            cbDesc.debugName = "ShadowCascadeCB";
            state.cascadeCB = nv->createBuffer(cbDesc);
            state.cascadeCBMaxVersions = state.cascadeCB ? kCascadeCBVersions : 0;
            if (state.cascadeCB)
                Msg("* [ShadowPass] Upgraded ShadowCascadeCB maxVersions → %u", kCascadeCBVersions);
        }
    }

    // Hot-upgrade: older sessions may have CSM without billboard grass caster
    if (state.initialized && state.enabled && !state.billboardGrassPipeline)
    {
        nvrhi::IDevice* nv = device->GetNVRHIDevice();
        auto* loader = GEnv.Render->GetShaderLoader();
        if (nv && loader)
        {
            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IBindingLayout* bindlessLayout = nullptr;
            if (auto* backend = GEnv.Backend)
                bindlessLayout = backend->GetBindlessLayout();
            nvrhi::FramebufferInfoEx fbInfo;
            fbInfo.depthFormat = nvrhi::Format::D32;
            fbInfo.sampleCount = 1;
            auto bbVs = loader->LoadVertexShader("detail_billboard_shadow", "main");
            auto bbPs = loader->LoadPixelShader("detail_billboard_shadow", "main");
            if (bbVs.handle && bbVs.reflection && bbPs.handle && bbPs.reflection)
            {
                state.billboardGrassVs = bbVs.handle;
                state.billboardGrassPs = bbPs.handle;
                state.billboardGrassLayout = cache.GetOrCreateBindingLayoutFromReflection(
                    "ShadowCascadeBillboardGrass_v2", *bbVs.reflection, *bbPs.reflection, nv);
                if (state.billboardGrassLayout)
                {
                    nvrhi::GraphicsPipelineDesc bbDesc;
                    bbDesc.VS = state.billboardGrassVs;
                    bbDesc.PS = state.billboardGrassPs;
                    bbDesc.primType = nvrhi::PrimitiveType::TriangleList;
                    bbDesc.bindingLayouts = {state.billboardGrassLayout};
                    if (bindlessLayout)
                        bbDesc.bindingLayouts.push_back(bindlessLayout);
                    bbDesc.renderState.depthStencilState.setDepthTestEnable(true);
                    bbDesc.renderState.depthStencilState.setDepthWriteEnable(true);
                    bbDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                    bbDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                    bbDesc.renderState.rasterState.depthBias = 4;
                    bbDesc.renderState.rasterState.slopeScaledDepthBias = 3.0f;
                    state.billboardGrassPipeline = cache.GetOrCreatePipeline(
                        "ShadowCascadeBillboardGrass_v2", bbDesc, fbInfo, nv);
                }
                bbVs.reflection = nullptr;
                bbPs.reflection = nullptr;
            }
        }
        return;
    }
    if (state.initialized)
        return;

    // Always mark initialized so a failed init never retries every frame
    // (previously leaked a new 1024²×4 D32 CSM each frame → load hang / OOM).
    auto fail = [&](const char* reason) {
        Msg("! [ShadowPass] %s", reason);
        state.initialized = true;
        state.enabled = false;
    };

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (!nv)
    {
        fail("NVRHI device unavailable");
        return;
    }

    auto* resMgr = device->GetFGResourceManager();
    if (!resMgr || !resMgr->GetRTFactory() || !resMgr->GetTextureManager())
    {
        fail("Resource manager unavailable");
        return;
    }

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        fail("ShaderLoader unavailable");
        return;
    }

    auto vs = loader->LoadVertexShader("shadow\\shadow_cascade", "main");
    auto ps = loader->LoadPixelShader("shadow\\shadow_cascade", "main");
    if (!vs.handle || !ps.handle || !vs.reflection || !ps.reflection)
    {
        fail("Failed to load shadow_cascade shaders");
        return;
    }

    state.vs = vs.handle;
    state.ps = ps.handle;

    auto& cache = framegraph::GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "ShadowCascade_v2", *vs.reflection, *ps.reflection, nv);
    if (!state.layout)
    {
        fail("Failed to create binding layout");
        vs.reflection = nullptr;
        ps.reflection = nullptr;
        return;
    }

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.inputLayout = nv->createInputLayout(attrs, attrCount, state.vs);

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = state.inputLayout;
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.setDepthTestEnable(true);
    pipeDesc.renderState.depthStencilState.setDepthWriteEnable(true);
    pipeDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
    pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
    // Normal-offset (shadow_sampling.h) removes most acne, but on large flat surfaces
    // at grazing sun angles (building faces, seabed) it is not enough and depth
    // quantization shows up as parallel stripes. A moderate slope-scaled raster bias
    // kills that banding at render time; kept moderate so object shadows do not detach
    // (peter-panning). depthBiasClamp caps the worst-case slope contribution.
    pipeDesc.renderState.rasterState.depthBias = 2;
    pipeDesc.renderState.rasterState.slopeScaledDepthBias = 2.75f;
    pipeDesc.renderState.rasterState.depthBiasClamp = 0.0025f;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = {state.layout, bindlessLayout};
    else
        pipeDesc.bindingLayouts = {state.layout};

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.depthFormat = nvrhi::Format::D32;

    state.pipeline = cache.GetOrCreatePipeline("ShadowCascade_v2", pipeDesc, fbInfo, nv);
    if (!state.pipeline)
    {
        fail("Failed to create pipeline");
        vs.reflection = nullptr;
        ps.reflection = nullptr;
        return;
    }
    QueryBindingLayoutFromPipeline(state.pipeline, state.layout);

    // Foliage CSM caster — same VS/layout, alpha-clip PS. Trees/bushes are alpha-blended
    // (transparent set) so they never reach the opaque PS; this pipeline casts them as
    // alpha-tested cutouts (and skips non-foliage transparents like glass/water).
    {
        auto foliagePs = loader->LoadPixelShader("shadow\\shadow_cascade_foliage", "main");
        if (foliagePs.handle && foliagePs.reflection)
        {
            state.foliagePs = foliagePs.handle;
            nvrhi::GraphicsPipelineDesc foliageDesc = pipeDesc;
            foliageDesc.PS = state.foliagePs;
            state.foliagePipeline = cache.GetOrCreatePipeline(
                "ShadowCascadeFoliage_v2", foliageDesc, fbInfo, nv);
            foliagePs.reflection = nullptr;
        }
        if (!state.foliagePipeline)
            Msg("! [ShadowPass] Foliage CSM pipeline unavailable — trees will not cast");
    }

    // Terrain CSM caster — same VS, opaque PS (terrain mat IDs ≠ g_Materials)
    {
        auto terrainPs = loader->LoadPixelShader("shadow\\shadow_cascade_terrain", "main");
        if (terrainPs.handle && terrainPs.reflection)
        {
            state.terrainPs = terrainPs.handle;
            state.terrainLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "ShadowCascadeTerrain_v2", *vs.reflection, *terrainPs.reflection, nv);
            if (state.terrainLayout)
            {
                nvrhi::GraphicsPipelineDesc terrainDesc = pipeDesc;
                terrainDesc.PS = state.terrainPs;
                if (bindlessLayout)
                    terrainDesc.bindingLayouts = {state.terrainLayout, bindlessLayout};
                else
                    terrainDesc.bindingLayouts = {state.terrainLayout};
                state.terrainPipeline = cache.GetOrCreatePipeline(
                    "ShadowCascadeTerrain_v2", terrainDesc, fbInfo, nv);
            }
            terrainPs.reflection = nullptr;
        }
        if (!state.terrainPipeline)
            Msg("! [ShadowPass] Terrain CSM pipeline unavailable — terrain will not cast");
    }

    // Grass CSM caster (r2_sun_details) — blade geometry + alpha clip
    {
        auto grassVs = loader->LoadVertexShader("detail_gpu_shadow", "main");
        auto grassPs = loader->LoadPixelShader("detail_gpu_shadow", "main");
        if (grassVs.handle && grassVs.reflection && grassPs.handle && grassPs.reflection)
        {
            state.grassVs = grassVs.handle;
            state.grassPs = grassPs.handle;
            state.grassLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "ShadowCascadeGrass_v2", *grassVs.reflection, *grassPs.reflection, nv);
            if (state.grassLayout)
            {
                nvrhi::VertexAttributeDesc grassAttrs[] = {
                    nvrhi::VertexAttributeDesc()
                        .setName("POSITION")
                        .setFormat(nvrhi::Format::RGB32_FLOAT)
                        .setOffset(0)
                        .setElementStride(sizeof(FGDetailManager::BladeVertex)),
                    nvrhi::VertexAttributeDesc()
                        .setName("TEXCOORD")
                        .setFormat(nvrhi::Format::RG32_FLOAT)
                        .setOffset(12)
                        .setElementStride(sizeof(FGDetailManager::BladeVertex)),
                    nvrhi::VertexAttributeDesc()
                        .setName("COLOR")
                        .setFormat(nvrhi::Format::R32_FLOAT)
                        .setArraySize(2)
                        .setOffset(20)
                        .setElementStride(sizeof(FGDetailManager::BladeVertex)),
                };
                state.grassInputLayout = nv->createInputLayout(grassAttrs, 3, state.grassVs);

                nvrhi::GraphicsPipelineDesc grassDesc;
                grassDesc.VS = state.grassVs;
                grassDesc.PS = state.grassPs;
                grassDesc.inputLayout = state.grassInputLayout;
                grassDesc.primType = nvrhi::PrimitiveType::TriangleList;
                grassDesc.bindingLayouts = {state.grassLayout};
                if (bindlessLayout)
                    grassDesc.bindingLayouts.push_back(bindlessLayout);
                grassDesc.renderState.depthStencilState.setDepthTestEnable(true);
                grassDesc.renderState.depthStencilState.setDepthWriteEnable(true);
                grassDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                grassDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                grassDesc.renderState.rasterState.depthBias = 4;
                grassDesc.renderState.rasterState.slopeScaledDepthBias = 3.0f;

                state.grassPipeline = cache.GetOrCreatePipeline(
                    "ShadowCascadeGrass_v2", grassDesc, fbInfo, nv);
            }
            grassVs.reflection = nullptr;
            grassPs.reflection = nullptr;
        }
        if (!state.grassPipeline)
            Msg("! [ShadowPass] Grass CSM pipeline unavailable — grass will not cast");
    }

    // CoP billboard detail caster (r__detail_gpu 0 + r2_sun_details)
    {
        auto bbVs = loader->LoadVertexShader("detail_billboard_shadow", "main");
        auto bbPs = loader->LoadPixelShader("detail_billboard_shadow", "main");
        if (bbVs.handle && bbVs.reflection && bbPs.handle && bbPs.reflection)
        {
            state.billboardGrassVs = bbVs.handle;
            state.billboardGrassPs = bbPs.handle;
            state.billboardGrassLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "ShadowCascadeBillboardGrass_v2", *bbVs.reflection, *bbPs.reflection, nv);
            if (state.billboardGrassLayout)
            {
                nvrhi::GraphicsPipelineDesc bbDesc;
                bbDesc.VS = state.billboardGrassVs;
                bbDesc.PS = state.billboardGrassPs;
                bbDesc.primType = nvrhi::PrimitiveType::TriangleList;
                bbDesc.bindingLayouts = {state.billboardGrassLayout};
                if (bindlessLayout)
                    bbDesc.bindingLayouts.push_back(bindlessLayout);
                bbDesc.renderState.depthStencilState.setDepthTestEnable(true);
                bbDesc.renderState.depthStencilState.setDepthWriteEnable(true);
                bbDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                bbDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                bbDesc.renderState.rasterState.depthBias = 4;
                bbDesc.renderState.rasterState.slopeScaledDepthBias = 3.0f;
                state.billboardGrassPipeline = cache.GetOrCreatePipeline(
                    "ShadowCascadeBillboardGrass_v2", bbDesc, fbInfo, nv);
            }
            bbVs.reflection = nullptr;
            bbPs.reflection = nullptr;
        }
        if (!state.billboardGrassPipeline)
            Msg("! [ShadowPass] Billboard grass CSM pipeline unavailable");
    }

    // Actor/HUD skinned casters → near cascade (r__actor_shadow)
    {
        auto skVs = loader->LoadVertexShader("shadow\\shadow_cascade_skinned", "main");
        auto sk4Vs = loader->LoadVertexShader("shadow\\shadow_cascade_skinned_4w", "main");
        if (skVs.handle && skVs.reflection && ps.handle && ps.reflection)
        {
            state.skinnedVs = skVs.handle;
            state.skinnedLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "ShadowCascadeSkinned_v2", *skVs.reflection, *ps.reflection, nv);
            if (state.skinnedLayout)
            {
                nvrhi::VertexAttributeDesc attribs[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA16_SNORM).setOffset(0).setElementStride(24),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(8).setElementStride(24),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(12).setElementStride(24),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(24),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG16_SNORM).setOffset(20).setElementStride(24),
                };
                // Match SkinningPass nonHQ layout via shader reflection — createInputLayout from VS
                state.skinnedInputLayout = nv->createInputLayout(attribs, 5, state.skinnedVs);

                nvrhi::GraphicsPipelineDesc skDesc;
                skDesc.VS = state.skinnedVs;
                skDesc.PS = state.ps;
                skDesc.inputLayout = state.skinnedInputLayout;
                skDesc.primType = nvrhi::PrimitiveType::TriangleList;
                skDesc.bindingLayouts = {state.skinnedLayout};
                if (bindlessLayout)
                    skDesc.bindingLayouts.push_back(bindlessLayout);
                skDesc.renderState.depthStencilState.setDepthTestEnable(true);
                skDesc.renderState.depthStencilState.setDepthWriteEnable(true);
                skDesc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                skDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                skDesc.renderState.rasterState.depthBias = 4;
                skDesc.renderState.rasterState.slopeScaledDepthBias = 3.0f;
                state.skinnedPipeline = cache.GetOrCreatePipeline("ShadowCascadeSkinned_v2", skDesc, fbInfo, nv);
            }
            skVs.reflection = nullptr;
        }
        if (sk4Vs.handle && sk4Vs.reflection && ps.handle && ps.reflection)
        {
            state.skinned4wVs = sk4Vs.handle;
            state.skinned4wLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "ShadowCascadeSkinned4W_v2", *sk4Vs.reflection, *ps.reflection, nv);
            if (state.skinned4wLayout)
            {
                constexpr u32 stride4 = 40;
                nvrhi::VertexAttributeDesc attribs4[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride4),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride4),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride4),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride4),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride4),
                    nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(36).setElementStride(stride4),
                };
                state.skinned4wInputLayout = nv->createInputLayout(attribs4, 6, state.skinned4wVs);

                nvrhi::GraphicsPipelineDesc sk4Desc;
                sk4Desc.VS = state.skinned4wVs;
                sk4Desc.PS = state.ps;
                sk4Desc.inputLayout = state.skinned4wInputLayout;
                sk4Desc.primType = nvrhi::PrimitiveType::TriangleList;
                sk4Desc.bindingLayouts = {state.skinned4wLayout};
                if (bindlessLayout)
                    sk4Desc.bindingLayouts.push_back(bindlessLayout);
                sk4Desc.renderState.depthStencilState.setDepthTestEnable(true);
                sk4Desc.renderState.depthStencilState.setDepthWriteEnable(true);
                sk4Desc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                sk4Desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                sk4Desc.renderState.rasterState.depthBias = 4;
                sk4Desc.renderState.rasterState.slopeScaledDepthBias = 3.0f;
                state.skinned4wPipeline = cache.GetOrCreatePipeline("ShadowCascadeSkinned4W_v2", sk4Desc, fbInfo, nv);
            }
            sk4Vs.reflection = nullptr;
        }

        // Additional world-skinned formats so NPCs/mutants (which rarely use the
        // nonHQ/4W layouts) cast into every cascade. Layouts mirror SkinningPass.
        auto makeSkinnedShadowPipe = [&](const char* vsName, const char* dbgName,
                                         u32 stride, nvrhi::Format tcFormat,
                                         nvrhi::ShaderHandle& outVs, nvrhi::BindingLayoutHandle& outLayout,
                                         nvrhi::InputLayoutHandle& outIL, nvrhi::GraphicsPipelineHandle& outPipe) {
            auto v = loader->LoadVertexShader(vsName, "main");
            if (!v.handle || !v.reflection || !ps.handle || !ps.reflection)
                return;
            outVs = v.handle;
            outLayout = cache.GetOrCreateBindingLayoutFromReflection(dbgName, *v.reflection, *ps.reflection, nv);
            if (outLayout)
            {
                nvrhi::VertexAttributeDesc a[] = {
                    nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                    nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(tcFormat).setOffset(28).setElementStride(stride),
                };
                outIL = nv->createInputLayout(a, 5, outVs);
                nvrhi::GraphicsPipelineDesc d;
                d.VS = outVs;
                d.PS = state.ps;
                d.inputLayout = outIL;
                d.primType = nvrhi::PrimitiveType::TriangleList;
                d.bindingLayouts = {outLayout};
                if (bindlessLayout)
                    d.bindingLayouts.push_back(bindlessLayout);
                d.renderState.depthStencilState.setDepthTestEnable(true);
                d.renderState.depthStencilState.setDepthWriteEnable(true);
                d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
                d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                d.renderState.rasterState.depthBias = 4;
                d.renderState.rasterState.slopeScaledDepthBias = 3.0f;
                outPipe = cache.GetOrCreatePipeline(dbgName, d, fbInfo, nv);
            }
            v.reflection = nullptr;
        };

        makeSkinnedShadowPipe("shadow\\shadow_cascade_skinned_hq", "ShadowCascadeSkinnedHQ_v2",
            36, nvrhi::Format::RG32_FLOAT, state.skinnedHqVs, state.skinnedHqLayout,
            state.skinnedHqInputLayout, state.skinnedHqPipeline);
        makeSkinnedShadowPipe("shadow\\shadow_cascade_skinned_2w", "ShadowCascadeSkinned2W_v2",
            44, nvrhi::Format::RGBA32_FLOAT, state.skinned2wVs, state.skinned2wLayout,
            state.skinned2wInputLayout, state.skinned2wPipeline);
        makeSkinnedShadowPipe("shadow\\shadow_cascade_skinned_3w", "ShadowCascadeSkinned3W_v2",
            44, nvrhi::Format::RGBA32_FLOAT, state.skinned3wVs, state.skinned3wLayout,
            state.skinned3wInputLayout, state.skinned3wPipeline);

        auto initSkinnedMdi = [&](u32 fmt, const char* vsName, const char* dbgName,
                                  const nvrhi::VertexAttributeDesc* baseAttribs, u32 baseAttrCount) {
            auto v = loader->LoadVertexShader(vsName, "main");
            if (!v.handle || !v.reflection || !ps.handle || !ps.reflection)
                return;
            if (!state.skinnedMdiLayout)
            {
                state.skinnedMdiLayout = cache.GetOrCreateBindingLayoutFromReflection(
                    "ShadowCascadeSkinnedMDI_v1", *v.reflection, *ps.reflection, nv);
            }
            if (!state.skinnedMdiLayout)
            {
                v.reflection = nullptr;
                return;
            }
            nvrhi::VertexAttributeDesc attribs[8];
            for (u32 i = 0; i < baseAttrCount; ++i)
                attribs[i] = baseAttribs[i];
            attribs[baseAttrCount] = nvrhi::VertexAttributeDesc()
                .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
                .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true);
            state.skinnedMdiInputLayout[fmt] = nv->createInputLayout(attribs, baseAttrCount + 1, v.handle);
            nvrhi::GraphicsPipelineDesc d;
            d.VS = v.handle;
            d.PS = state.ps;
            d.inputLayout = state.skinnedMdiInputLayout[fmt];
            d.primType = nvrhi::PrimitiveType::TriangleList;
            d.bindingLayouts = {state.skinnedMdiLayout};
            if (bindlessLayout)
                d.bindingLayouts.push_back(bindlessLayout);
            d.renderState.depthStencilState.setDepthTestEnable(true);
            d.renderState.depthStencilState.setDepthWriteEnable(true);
            d.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual);
            d.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
            d.renderState.rasterState.depthBias = 4;
            d.renderState.rasterState.slopeScaledDepthBias = 3.0f;
            state.skinnedMdiPipeline[fmt] = cache.GetOrCreatePipeline(dbgName, d, fbInfo, nv);
            v.reflection = nullptr;
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
            initSkinnedMdi(VF_SKINNED_NONHQ, "shadow\\shadow_cascade_skinned_mdi",
                "ShadowCascadeSkinnedMDI_nonHQ_v1", attribs, 5);
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
            initSkinnedMdi(VF_SKINNED_HQ1W, "shadow\\shadow_cascade_skinned_hq_mdi",
                "ShadowCascadeSkinnedMDI_hq1w_v1", attribs, 5);
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
            initSkinnedMdi(VF_SKINNED_HQ4W, "shadow\\shadow_cascade_skinned_4w_mdi",
                "ShadowCascadeSkinnedMDI_hq4w_v1", attribs, 6);
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
            initSkinnedMdi(VF_SKINNED_HQ2W, "shadow\\shadow_cascade_skinned_2w_mdi",
                "ShadowCascadeSkinnedMDI_hq2w_v1", attribs, 5);
            initSkinnedMdi(VF_SKINNED_HQ3W, "shadow\\shadow_cascade_skinned_3w_mdi",
                "ShadowCascadeSkinnedMDI_hq3w_v1", attribs, 5);
        }

        if (!state.skinnedPipeline)
            Msg("! [ShadowPass] Actor skinned CSM pipeline unavailable");
    }

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(ShadowCascadeCB);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    // CSM (3) + HUD + rain + up to MAX_LOCAL_SHADOW_TILES writes/frame
    cbDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    cbDesc.debugName = "ShadowCascadeCB";
    state.cascadeCB = nv->createBuffer(cbDesc);
    if (!state.cascadeCB)
    {
        fail("Failed to create cascade CB");
        vs.reflection = nullptr;
        ps.reflection = nullptr;
        return;
    }
    state.cascadeCBMaxVersions = cbDesc.maxVersions;

    {
        nvrhi::BufferDesc grassCbDesc;
        grassCbDesc.byteSize = sizeof(GrassShadowCB);
        grassCbDesc.isConstantBuffer = true;
        grassCbDesc.isVolatile = true;
        grassCbDesc.maxVersions = 16;
        grassCbDesc.debugName = "GrassShadowCB";
        state.grassCB = nv->createBuffer(grassCbDesc);
    }

    // Resolution ladder: separate Texture2D maps (c0 full, c1 ½, c2 ¼).
    // Bind as g_ShadowMap0/1/2 — not a shared array — so each cascade keeps
    // its own resolution without layout/slot mismatches.
    static const char* kCascadeNames[kCSMCascadeCount] = {
        "rt_CascadedShadow0", "rt_CascadedShadow1", "rt_CascadedShadow2"};
    for (u32 i = 0; i < kCSMCascadeCount; ++i)
    {
        const u32 cres = GetCSMCascadeResolution(i);
        state.cascadeResolution[i] = cres;
        state.shadowCascadeHandle[i] = resMgr->GetRTFactory()->CreateShadowMap(
            cres, true, kCascadeNames[i]);
        state.shadowCascades[i] =
            resMgr->GetTextureManager()->GetNVRHITexture(state.shadowCascadeHandle[i]);
        if (!state.shadowCascades[i])
        {
            fail(make_string("Failed to create CSM cascade %u (%ux%u)", i, cres, cres).c_str());
            vs.reflection = nullptr;
            ps.reflection = nullptr;
            return;
        }
    }
    state.shadowArrayHandle = state.shadowCascadeHandle[0];
    state.shadowArray = state.shadowCascades[0];

    state.hudShadowHandle = resMgr->GetRTFactory()->CreateCascadedShadowMap(
        ShadowPassState::kHUDShadowResolution, 1, true, "rt_HUDShadow");
    state.hudShadowMap = resMgr->GetTextureManager()->GetNVRHITexture(state.hudShadowHandle);
    if (!state.hudShadowMap)
        Msg("! [ShadowPass] HUD shadow map create failed — HUD self-shadow disabled");

    state.initialized = true;
    state.enabled = true;
    Msg("* [ShadowPass] Init: OK (CSM ladder %u/%u/%u, HUD %ux%u)",
        state.cascadeResolution[0], state.cascadeResolution[1], state.cascadeResolution[2],
        ShadowPassState::kHUDShadowResolution, ShadowPassState::kHUDShadowResolution);

    vs.reflection = nullptr;
    ps.reflection = nullptr;
}

void ShutdownShadowPass(fg::RenderDevice* device, ShadowPassState& state)
{
    if (device && device->GetFGResourceManager() && device->GetFGResourceManager()->GetRTFactory())
    {
        auto* factory = device->GetFGResourceManager()->GetRTFactory();
        bool releasedAlias = false;
        for (u32 i = 0; i < kCSMCascadeCount; ++i)
        {
            if (state.shadowCascadeHandle[i].IsValid())
            {
                if (state.shadowCascadeHandle[i] == state.shadowArrayHandle)
                    releasedAlias = true;
                factory->ReleaseRenderTarget(state.shadowCascadeHandle[i]);
            }
            state.shadowCascadeHandle[i] = {};
            state.shadowCascades[i] = nullptr;
            state.cascadeResolution[i] = 0;
        }
        // Legacy uniform-array path owned only shadowArrayHandle
        if (state.shadowArrayHandle.IsValid() && !releasedAlias)
            factory->ReleaseRenderTarget(state.shadowArrayHandle);
        state.shadowArrayHandle = {};
        if (state.hudShadowHandle.IsValid())
            factory->ReleaseRenderTarget(state.hudShadowHandle);
    }
    state.pipeline = nullptr;
    state.layout = nullptr;
    state.foliagePipeline = nullptr;
    state.terrainPipeline = nullptr;
    state.terrainLayout = nullptr;
    state.grassPipeline = nullptr;
    state.grassLayout = nullptr;
    state.billboardGrassPipeline = nullptr;
    state.billboardGrassLayout = nullptr;
    state.skinnedPipeline = nullptr;
    state.skinnedLayout = nullptr;
    state.skinned4wPipeline = nullptr;
    state.skinned4wLayout = nullptr;
    state.skinnedMdiLayout = nullptr;
    for (u32 i = 0; i < 6; ++i)
    {
        state.skinnedMdiPipeline[i] = nullptr;
        state.skinnedMdiInputLayout[i] = nullptr;
    }
    state.inputLayout = nullptr;
    state.grassInputLayout = nullptr;
    state.skinnedInputLayout = nullptr;
    state.skinned4wInputLayout = nullptr;
    state.vs = nullptr;
    state.ps = nullptr;
    state.terrainPs = nullptr;
    state.grassVs = nullptr;
    state.grassPs = nullptr;
    state.billboardGrassVs = nullptr;
    state.billboardGrassPs = nullptr;
    state.skinnedVs = nullptr;
    state.skinned4wVs = nullptr;
    state.cascadeCB = nullptr;
    state.cascadeCBMaxVersions = 0;
    state.grassCB = nullptr;
    state.shadowArray = nullptr;
    state.hudShadowMap = nullptr;
    state.initialized = false;
    state.enabled = false;
    g_ShadowCascadeGPUData.valid = false;
}

ShadowCascadeOutputs setupCascadedShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    MaterialCache* materialCache,
    fg::FGDetailManager* detailManager,
    const Fvector& sunDirection,
    const xr_vector<xray::render::GeometryBatch>* hudBatches,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches,
    fg::GPUCullingManager* gpuCulling,
    ShadowPassState& state,
    framegraph::VirtualResourceHandle skinnedDrawArgs)
{
    using namespace framegraph;

    ShadowCascadeOutputs outputs;
    outputs.valid = false;

    if (!device)
        return outputs;

    InitializeShadowPass(device, state);
    if (!state.enabled || !state.shadowArray)
        return outputs;

    const bool sunOn = ps_r2_ls_flags.test(R2FLAG_SUN);

    // Cascade split distances driven by r2_sun_near / r2_sun_far (not hardcoded 20/40/160).
    // Mid sits ~35% of the way from near→far so raising sun_far actually extends coverage.
    const float nearDist = std::max(ps_r2_sun_near, 5.f);
    const float farDist = std::max(ps_r2_sun_far, nearDist + 20.f);
    const float midDist = std::clamp(
        nearDist + (farDist - nearDist) * 0.35f,
        nearDist + 5.f,
        farDist - 5.f);

    Fvector4 splits;
    // .w = UV rim blend width for fine→coarse handoff
    splits.set(nearDist, midDist, farDist, 0.15f);
    if (ps_r2_sun_near_border > 0.5f && ps_r2_sun_near_border < 1.f)
        splits.w = std::max(0.08f, 0.55f * (1.f - ps_r2_sun_near_border));
    else
        splits.w = 0.18f;

    Fvector sunDir = sunDirection;
    if (sunDir.magnitude() < 1e-4f)
        sunDir.set(0.3f, 0.8f, 0.2f);
    sunDir.normalize_safe();

    // Nested frustum coverage: each coarser cascade includes the near volume so
    // UV handoff (fine→coarse) always has a valid parent map (classic stencil defer).
    const float viewNear = VIEWPORT_NEAR;
    const float zFar[kCSMCascadeCount] = {splits.x, splits.y, splits.z};
    // Ortho footprint ≈ split distance so r2_sun_* scales both selection AND map coverage.
    const float mapSize[kCSMCascadeCount] = {splits.x, splits.y, splits.z};

    const u32 smapRes = GetCSMResolution();
    for (u32 i = 0; i < kCSMCascadeCount; ++i)
    {
        const float sliceNear = viewNear;
        const float sliceFar = zFar[i];
        ComputeFrustumFitCascadeMatrices(
            sunDir, mapSize[i], sliceNear, sliceFar, i, state.cascadeResolution[i],
            state.cascadeClipVP[i], state.cascadeSampleVP[i]);
    }
    state.cascadeSplits = splits;

    // Tight HUD/weapon light frustum (~2.5m ortho around camera near volume)
    ComputeFrustumFitCascadeMatrices(
        sunDir, 2.5f, VIEWPORT_NEAR, 4.0f, 0,
        ShadowPassState::kHUDShadowResolution,
        state.hudClipVP, state.hudSampleVP);

    g_ShadowCascadeGPUData.valid = sunOn;
    g_ShadowCascadeGPUData.splits = splits;
    g_ShadowCascadeGPUData.mapSize = float(smapRes);
    for (u32 i = 0; i < 4; ++i)
        g_ShadowCascadeGPUData.matrices[i].identity();
    for (u32 i = 0; i < kCSMCascadeCount; ++i)
        g_ShadowCascadeGPUData.matrices[i] = state.cascadeSampleVP[i];
    // Slot 3 = dedicated HUD shadow sample matrix
    g_ShadowCascadeGPUData.matrices[3] = state.hudSampleVP;

    // view_shadow_proj: camera forward in far-cascade UV (accum_sun_far).
    // When looking steeply up/down the UV delta is unreliable — disable directional
    // gate so only radial rim fade applies (otherwise shadows wash out).
    {
        const Fmatrix& farM = state.cascadeSampleVP[kCSMCascadeCount - 1];
        Fvector2 vsp;
        vsp.set(0.f, 0.f);
        if (_abs(Device.vCameraDirection.y) < 0.85f)
        {
            Fvector p0 = Device.vCameraPosition;
            Fvector p1;
            p1.mad(p0, Device.vCameraDirection, 50.f);
            Fvector4 u0, u1;
            farM.transform(u0, p0);
            farM.transform(u1, p1);
            vsp.set(u1.x - u0.x, u1.y - u0.y);
            float len = _sqrt(vsp.x * vsp.x + vsp.y * vsp.y);
            if (len > 1e-5f)
            {
                vsp.x /= len;
                vsp.y /= len;
            }
            else
                vsp.set(0.f, 0.f);
        }
        g_ShadowCascadeGPUData.viewShadowProj = vsp;
    }

    ResourceDesc desc[kCSMCascadeCount];
    VirtualResourceHandle shadowHandles[kCSMCascadeCount];
    static const char* kImportNames[kCSMCascadeCount] = {
        "rt_CascadedShadow0", "rt_CascadedShadow1", "rt_CascadedShadow2"};
    for (u32 i = 0; i < kCSMCascadeCount; ++i)
    {
        desc[i].type = ResourceDesc::Type::Texture2D;
        desc[i].width = state.cascadeResolution[i];
        desc[i].height = state.cascadeResolution[i];
        desc[i].depth = 1;
        desc[i].arraySize = 1;
        desc[i].format = nvrhi::Format::D32;
        desc[i].isDepthStencil = true;
        desc[i].isImported = true;
        desc[i].debugName = kImportNames[i];
        shadowHandles[i] = fg.ImportTexture(kImportNames[i], state.shadowCascades[i], desc[i]);
    }

    struct PassData
    {
        VirtualResourceHandle shadowCascades[kCSMCascadeCount];
        VirtualResourceHandle skinnedDrawArgs;
        ShadowPassState* passState = nullptr;
        BindlessForwardConfig bindlessConfig;
        MaterialCache* materialCache = nullptr;
        FGDetailManager* detailManager = nullptr;
        const xr_vector<xray::render::GeometryBatch>* hudBatches = nullptr;
        const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches = nullptr;
        GPUCullingManager* gpuCulling = nullptr;
        fg::RenderDevice* device = nullptr;
        bool sunOn = false;
        bool sunDetails = false;
        bool actorShadow = false;
        bool hudShadow = false;
    };

    const bool sunDetails = sunOn && ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS);
    const bool actorShadow = ps_r__common_flags.test(RFLAG_ACTOR_SHADOW);
    const bool hudShadow = sunOn && state.hudShadowMap != nullptr;

    auto& passData = fg.addCallbackPass<PassData>(
        "CascadedShadows",
        [&, sunOn, sunDetails, actorShadow, hudShadow, detailManager, hudBatches, worldSkinnedBatches, gpuCulling, skinnedDrawArgs](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            for (u32 i = 0; i < kCSMCascadeCount; ++i)
                data.shadowCascades[i] = pb.write(shadowHandles[i], ResourceState::DepthStencilWrite);
            if (skinnedDrawArgs.is_valid())
                data.skinnedDrawArgs = pb.read(skinnedDrawArgs, ResourceState::IndirectArgument);
            data.passState = &state;
            data.bindlessConfig = bindlessConfig;
            data.materialCache = materialCache;
            data.detailManager = detailManager;
            data.hudBatches = hudBatches;
            data.worldSkinnedBatches = worldSkinnedBatches;
            data.gpuCulling = gpuCulling;
            data.device = device;
            data.sunOn = sunOn;
            data.sunDetails = sunDetails;
            data.actorShadow = actorShadow;
            data.hudShadow = hudShadow;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.passState || !data.passState->enabled)
                return;

            nvrhi::ITexture* shadowTex[kCSMCascadeCount] = {};
            for (u32 i = 0; i < kCSMCascadeCount; ++i)
            {
                shadowTex[i] = graph.GetPhysicalTexture(data.shadowCascades[i]);
                if (!shadowTex[i])
                    shadowTex[i] = data.passState->shadowCascades[i];
            }
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !shadowTex[0])
                return;

            ShadowPassState& st = *data.passState;

            // Far cascade (c=2): skip clear+redraw every other frame when camera is calm.
            static Fvector s_prevCamPos = {0, 0, 0};
            static Fvector s_prevCamDir = {0, 0, 1};
            static bool s_farCascadeValid = false;
            const float camMove = Device.vCameraPosition.distance_to(s_prevCamPos);
            const float camTurn = 1.f - _abs(Device.vCameraDirection.dotproduct(s_prevCamDir));
            const bool cameraCalm = camMove < 0.35f && camTurn < 0.02f;
            const bool skipFarCascade = s_farCascadeValid && cameraCalm && ((Device.dwFrame % 3u) != 0);
            s_prevCamPos = Device.vCameraPosition;
            s_prevCamDir = Device.vCameraDirection;

            for (u32 c = 0; c < kCSMCascadeCount; ++c)
            {
                if (!shadowTex[c])
                    continue;
                if (c == 2 && skipFarCascade)
                    continue;
                cmd->clearDepthStencilTexture(
                    shadowTex[c], nvrhi::TextureSubresourceSet(0, 1, 0, 1), true, 1.0f, false, 0);
            }

            if (!data.sunOn)
            {
                s_farCascadeValid = false;
                return;
            }

            if (data.materialCache)
                data.materialCache->FinalizePendingMaterials(ctx);

            auto& matBuffer = bindless::MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto& cache = framegraph::GetPassResourceCache();
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* backend = GEnv.Backend;
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // HUD uses dedicated map (viewport set per-HUD res below)
            // World cascades use per-cascade resolution ladder viewports.

            // Dedicated HUD/weapon shadow map (FOV-adjusted, matches HUD forward VS)
            if (data.hudShadow && data.hudBatches && data.gpuCulling && st.hudShadowMap &&
                (st.skinnedPipeline || st.skinned4wPipeline))
            {
                auto* boneBuf = data.gpuCulling->GetGlobalBoneBuffer();
                auto* skVsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned", ".vs");
                auto* sk4VsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_4w", ".vs");
                auto* psReflHud = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
                if (boneBuf && psReflHud)
                {
                    cmd->clearDepthStencilTexture(st.hudShadowMap, nvrhi::TextureSubresourceSet(0, 1, 0, 1), true, 1.0f, false, 0);

                    ShadowCascadeCB cbData{};
                    cbData.lightVP = st.hudClipVP;
                    cmd->writeBuffer(st.cascadeCB, &cbData, sizeof(cbData));

                    nvrhi::FramebufferDesc fbDesc;
                    nvrhi::TextureSubresourceSet depthSub;
                    depthSub.baseArraySlice = 0;
                    depthSub.numArraySlices = 1;
                    fbDesc.setDepthAttachment(st.hudShadowMap, depthSub);
                    auto fb = cache.GetOrCreateFramebuffer("ShadowHUD", fbDesc, nv);

                    auto dynCB = cache.GetOrCreateVolatileCB(
                        "ShadowCascade_v2", "HUDDyn", sizeof(DynamicTransforms), data.device, 256);
                    auto matCB = cache.GetOrCreateVolatileCB(
                        "ShadowCascade_v2", "HUDMat", sizeof(SkinnedMaterialCB), data.device, 256);

                    const u32 hudRes = ShadowPassState::kHUDShadowResolution;
                    nvrhi::Viewport hudVp(0.f, float(hudRes), 0.f, float(hudRes), 0.f, 1.f);

                    // Match SkinningPassSetup HUD FOV so cast/receive share the same space
                    auto applyHudFov = [](const Fmatrix& worldMatrix) -> Fmatrix {
                        const float fovScale = 1.0f / psHUD_FOV;
                        Fmatrix viewMatrix = Device.mView;
                        Fmatrix invView;
                        invView.invert(viewMatrix);
                        Fmatrix fovScaleMatrix;
                        fovScaleMatrix.identity();
                        fovScaleMatrix._11 = fovScale;
                        fovScaleMatrix._22 = fovScale;
                        fovScaleMatrix._33 = 1.0f;
                        Fmatrix t1, t2, result;
                        t1.mul(viewMatrix, worldMatrix);
                        t2.mul(fovScaleMatrix, t1);
                        result.mul(invView, t2);
                        return result;
                    };

                    if (fb && dynCB && matCB)
                    {
                        for (const auto& batch : *data.hudBatches)
                        {
                            if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
                                continue;

                            CKinematics* parent = nullptr;
                            const u32 visualType = batch.visual ? batch.visual->getType() : 0;
                            if (visualType == MT_SKELETON_GEOMDEF_ST)
                                parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                            else if (visualType == MT_SKELETON_GEOMDEF_PM)
                                parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                            if (!parent)
                                continue;

                            const u32 boneOffset = data.gpuCulling->GetOrUploadSkeleton(cmd, parent);

                            const bool use4w = (batch.vertexStride == 40) ||
                                (batch.skinningRenderMode == 9) || (batch.skinningRenderMode == 10);
                            nvrhi::IGraphicsPipeline* pipe =
                                use4w ? st.skinned4wPipeline.Get() : st.skinnedPipeline.Get();
                            nvrhi::IBindingLayout* layout =
                                use4w ? st.skinned4wLayout.Get() : st.skinnedLayout.Get();
                            nvrhi::IInputLayout* il =
                                use4w ? st.skinned4wInputLayout.Get() : st.skinnedInputLayout.Get();
                            auto* vsRefl = use4w ? sk4VsRefl : skVsRefl;
                            if (!pipe || !layout || !il || !vsRefl)
                                continue;

                            DynamicTransforms dyn{};
                            FillDynamicTransforms(dyn, applyHudFov(batch.worldMatrix));
                            cmd->writeBuffer(dynCB, &dyn, sizeof(dyn));

                            SkinnedMaterialCB matId{};
                            matId.materialID = batch.bindlessMaterialID;
                            matId.skeletonBoneOffset = boneOffset;
                            cmd->writeBuffer(matCB, &matId, sizeof(matId));

                            framegraph::BindingSetBuilder bsb(*vsRefl, *psReflHud, nv, "ShadowHUD");
                            bsb.ConstantBuffer("dynamic_transforms", dynCB);
                            bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                            bsb.ConstantBuffer("SkinnedMaterialCB", matCB);
                            bsb.BufferSRV("g_BoneMatrices", boneBuf);
                            BindBindlessMaterialTables(bsb);
                            BindPaintSplatBuffer(bsb, nv);
                            auto set = cache.GetOrCreateBindingSet(bsb.Build(), layout, nv);
                            if (!set)
                                continue;

                            nvrhi::GraphicsState gs;
                            gs.pipeline = pipe;
                            gs.framebuffer = fb;
                            gs.bindings = {set};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {{batch.vertexBuffer, 0, 0}};
                            gs.indexBuffer = {batch.indexBuffer, nvrhi::Format::R16_UINT, 0};
                            gs.viewport.addViewport(hudVp);
                            gs.viewport.addScissorRect(nvrhi::Rect(hudRes, hudRes));
                            cmd->setGraphicsState(gs);
                            cmd->drawIndexed(
                                nvrhi::DrawArguments()
                                    .setVertexCount(batch.indexCount)
                                    .setStartIndexLocation(batch.startIndex)
                                    .setStartVertexLocation(batch.baseVertex));
                        }
                    }
                }
            }

            if (!data.bindlessConfig.UseGPUCulling() || !data.bindlessConfig.UseMegaBuffers())
                return;

            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("ShadowCascade_v2", nv);
            auto* vsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".vs");
            auto* psRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            if (!vsRefl || !psRefl || !drawIndexBuffer || !st.pipeline)
                return;

            const bool drawGrass =
                data.sunDetails &&
                data.detailManager &&
                psDeviceFlags.is(rsDrawDetails) &&
                data.detailManager->generatedInstancesBuffer;

            if (drawGrass && st.grassCB)
            {
                GrassShadowCB gcb{};
                gcb.grassBladeHeight = ps_r3_grass_blade_height;
                gcb.buildDetailsIndex = data.detailManager->buildDetailsBindlessIndex;
                cmd->writeBuffer(st.grassCB, &gcb, sizeof(gcb));
            }

            // ── World skinned casters (NPCs, mutants, corpses) → every cascade ──
            // Reuses the forward skeleton bone upload (deduped per frame) so the CSM
            // matches the animated pose. Drawn per-cascade so distant characters keep
            // casting; count is small so per-draw overhead is negligible.
            const bool drawWorldSkinned =
                (ps_r_skinned_shadows != 0) && data.worldSkinnedBatches && data.gpuCulling &&
                (st.skinnedPipeline || st.skinned4wPipeline || st.skinnedHqPipeline ||
                 st.skinned2wPipeline || st.skinned3wPipeline);
            nvrhi::IBuffer* skBoneBuf =
                drawWorldSkinned ? data.gpuCulling->GetGlobalBoneBuffer() : nullptr;
            auto* skPsRefl = shaderLoader->GetCachedReflection("shadow\\shadow_cascade", ".ps");
            auto* skVsReflNonHQ = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned", ".vs");
            auto* skVsRefl4W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_4w", ".vs");
            auto* skVsReflHQ = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_hq", ".vs");
            auto* skVsRefl2W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_2w", ".vs");
            auto* skVsRefl3W = shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_3w", ".vs");
            nvrhi::IBuffer* skDynCB = nullptr;
            nvrhi::IBuffer* skMatCB = nullptr;
            const bool worldSkinnedReady = drawWorldSkinned && skBoneBuf && skPsRefl;
            if (worldSkinnedReady)
            {
                skDynCB = cache.GetOrCreateVolatileCB(
                    "ShadowCascade_v2", "WorldSkinDyn", sizeof(DynamicTransforms), data.device, 4096);
                skMatCB = cache.GetOrCreateVolatileCB(
                    "ShadowCascade_v2", "WorldSkinMat", sizeof(SkinnedMaterialCB), data.device, 4096);
            }

            for (u32 c = 0; c < kCSMCascadeCount; ++c)
            {
                // Indoor: few portal-visible sectors → only near cascade
                if (ps_r_portal_cull != 0 && ps_r_shadow_indoor_near_only != 0 && c > 0 &&
                    PortalTraverser.r_sectors.size() > 0 && PortalTraverser.r_sectors.size() <= 8)
                    continue;

                if (!shadowTex[c])
                    continue;

                // Far cascade cadence: reuse previous contents when camera is calm
                if (c == 2 && skipFarCascade)
                    continue;

                const u32 cres = st.cascadeResolution[c] ? st.cascadeResolution[c] : GetCSMCascadeResolution(c);
                nvrhi::Viewport cascadeVp(0.f, float(cres), 0.f, float(cres), 0.f, 1.f);

                // Per-cascade light-frustum caster cull (replaces cast_all for near/mid)
                const bool lightCull = (ps_r_shadow_light_cull != 0) && data.gpuCulling;
                if (lightCull)
                {
                    const float inflate = (c == 0) ? 1.35f : 1.15f;
                    data.gpuCulling->BuildLightFrustumCasters(cmd, nv, st.cascadeClipVP[c], inflate);
                }

                ShadowCascadeCB cbData{};
                cbData.lightVP = st.cascadeClipVP[c];
                cmd->writeBuffer(st.cascadeCB, &cbData, sizeof(cbData));

                nvrhi::FramebufferDesc fbDesc;
                fbDesc.setDepthAttachment(shadowTex[c]);
                auto fb = cache.GetOrCreateFramebuffer(
                    make_string("ShadowCascade_%u_%u", c, cres).c_str(), fbDesc, nv);
                if (!fb)
                    continue;

                auto drawSet = [&](const BindlessDrawSet& set, nvrhi::IGraphicsPipeline* pipeline) {
                    if (!pipeline)
                        return;

                    // Resolve light-cull buffers from manager (created during BuildLightFrustumCasters)
                    nvrhi::IBuffer* lcArgs = nullptr;
                    nvrhi::IBuffer* lcIdx = nullptr;
                    nvrhi::IBuffer* lcMat = nullptr;
                    nvrhi::IBuffer* lcCnt = nullptr;
                    if (lightCull && data.gpuCulling)
                    {
                        if (set.instanceBuffer == data.gpuCulling->GetStaticInstanceBuffer())
                        {
                            lcArgs = data.gpuCulling->GetStaticShadowCompactDrawArgsBuffer();
                            lcIdx = data.gpuCulling->GetStaticShadowIndicesBuffer();
                            lcMat = data.gpuCulling->GetStaticShadowCompactMaterialIDBuffer();
                            lcCnt = data.gpuCulling->GetStaticShadowCountBuffer();
                        }
                        else if (set.instanceBuffer == data.gpuCulling->GetDynamicInstanceBuffer())
                        {
                            lcArgs = data.gpuCulling->GetDynamicShadowCompactDrawArgsBuffer();
                            lcIdx = data.gpuCulling->GetDynamicShadowIndicesBuffer();
                            lcMat = data.gpuCulling->GetDynamicShadowCompactMaterialIDBuffer();
                            lcCnt = data.gpuCulling->GetDynamicShadowCountBuffer();
                        }
                        else if (set.instanceBuffer == data.gpuCulling->GetTransparentInstanceBuffer())
                        {
                            lcArgs = data.gpuCulling->GetTransparentShadowCompactDrawArgsBuffer();
                            lcIdx = data.gpuCulling->GetTransparentShadowIndicesBuffer();
                            lcMat = data.gpuCulling->GetTransparentShadowCompactMaterialIDBuffer();
                            lcCnt = data.gpuCulling->GetTransparentShadowCountBuffer();
                        }
                    }
                    const bool useLightCull = lightCull && lcArgs && lcIdx && lcMat && lcCnt;
                    const bool castAll = !useLightCull && (ps_r_shadow_cast_all != 0) &&
                        set.IsCastAllValid() && (c < 1);
                    if (!useLightCull && !castAll && !set.IsValid())
                        return;

                    nvrhi::IBuffer* batchIndices = useLightCull ? lcIdx
                        : (castAll ? set.castAllBatchIndicesBuffer : set.compactBatchIndicesBuffer);
                    nvrhi::IBuffer* materialIDs = useLightCull ? lcMat
                        : (castAll ? set.castAllMaterialIDBuffer : set.compactMaterialIDBuffer);
                    nvrhi::IBuffer* drawArgs = useLightCull ? lcArgs
                        : (castAll ? set.castAllDrawArgsBuffer : set.compactDrawArgsBuffer);
                    nvrhi::IBuffer* countBuffer = useLightCull ? lcCnt
                        : (castAll ? set.castAllCountBuffer : set.compactCountBuffer);

                    framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "ShadowCascade_v2");
                    bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                    BindBindlessMaterialTables(bsb);
                    bsb.BufferSRV("g_InstanceData", set.instanceBuffer);
                    bsb.BufferSRV("g_CompactBatchIndices", batchIndices);
                    bsb.BufferSRV("g_CompactMaterialIDs", materialIDs);
                    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), st.layout, nv);
                    if (!bindingSet)
                        return;

                    nvrhi::GraphicsState gs;
                    gs.pipeline = pipeline;
                    gs.framebuffer = fb;
                    gs.bindings = {bindingSet};
                    if (bindlessTable)
                        gs.addBindingSet(bindlessTable);
                    gs.vertexBuffers = {
                        {data.bindlessConfig.megaVertexBuffer, 0, 0},
                        {drawIndexBuffer, 1, 0}};
                    gs.indexBuffer = {data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                    gs.viewport.addViewport(cascadeVp);
                    gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                    gs.indirectParams = drawArgs;
                    gs.indirectCountBuffer = countBuffer;

                    cmd->setGraphicsState(gs);
                    DrawIndexedIndirectCountOrFallback(cmd, 0, 0, set.totalObjectCount);
                };

                {
                    static bool s_drawDbgDone = false;
                    if (!s_drawDbgDone) {
                        const auto& sS = data.bindlessConfig.staticSet;
                        const auto& dS = data.bindlessConfig.dynamicSet;
                        const auto& tS = data.bindlessConfig.transparentCasterSet;
                        Msg("* [ShadowDbg] draw cast_all=%d | static: obj=%u castAll=%d | dyn: obj=%u castAll=%d | trans: obj=%u castAll=%d foliagePipe=%d",
                            ps_r_shadow_cast_all,
                            sS.totalObjectCount, sS.IsCastAllValid() ? 1 : 0,
                            dS.totalObjectCount, dS.IsCastAllValid() ? 1 : 0,
                            tS.totalObjectCount, tS.IsCastAllValid() ? 1 : 0,
                            st.foliagePipeline ? 1 : 0);
                        s_drawDbgDone = true;
                    }
                }

                drawSet(data.bindlessConfig.staticSet, st.pipeline);
                const bool shaftsNeedFarCasters = ps_r_sun_shafts > 0;
                if (c < 2 || shaftsNeedFarCasters)
                    drawSet(data.bindlessConfig.dynamicSet, st.pipeline);
                if (c < 2 || shaftsNeedFarCasters)
                    drawSet(data.bindlessConfig.transparentCasterSet, st.foliagePipeline);

                if (data.gpuCulling && data.gpuCulling->GetTessObjectCount() > 0)
                {
                    nvrhi::IBuffer* tessInst = data.gpuCulling->GetTessInstanceBuffer();
                    nvrhi::IBuffer* tessMat = data.gpuCulling->GetTessMaterialIDBuffer();
                    nvrhi::IBuffer* tessIdx = data.gpuCulling->GetTessBatchIndicesBuffer();
                    nvrhi::IBuffer* tessArgs = data.gpuCulling->GetTessDrawArgsBuffer();
                    const u32 tessCount = data.gpuCulling->GetTessObjectCount();
                    if (tessInst && tessMat && tessIdx && tessArgs && tessCount > 0)
                    {
                        framegraph::BindingSetBuilder tsb(*vsRefl, *psRefl, nv, "ShadowCascade.Tess");
                        tsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                        BindBindlessMaterialTables(tsb);
                        tsb.BufferSRV("g_InstanceData", tessInst);
                        tsb.BufferSRV("g_CompactBatchIndices", tessIdx);
                        tsb.BufferSRV("g_CompactMaterialIDs", tessMat);
                        auto tessSet = cache.GetOrCreateBindingSet(tsb.Build(), st.layout, nv);
                        if (tessSet)
                        {
                            nvrhi::GraphicsState gs;
                            gs.pipeline = st.pipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {tessSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {data.bindlessConfig.megaVertexBuffer, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {
                                data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                            gs.viewport.addViewport(cascadeVp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            gs.indirectParams = tessArgs;
                            cmd->setGraphicsState(gs);
                            cmd->drawIndexedIndirect(0, tessCount);
                        }
                    }
                }

                // Terrain cast into near+mid (far cascade too coarse / expensive)
                if (st.terrainPipeline && st.terrainLayout &&
                    data.bindlessConfig.HasTerrain() &&
                    data.bindlessConfig.UseTerrainCompaction() &&
                    c < 2)
                {
                    if (data.materialCache)
                        data.materialCache->FinalizePendingTerrainMaterials(ctx);
                    auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
                    terrainMatBuffer.Upload(ctx);

                    auto* terrainPsRefl = shaderLoader->GetCachedReflection(
                        "shadow\\shadow_cascade_terrain", ".ps");
                    if (terrainPsRefl && terrainMatBuffer.GetBuffer())
                    {
                        framegraph::BindingSetBuilder tbsb(*vsRefl, *terrainPsRefl, nv, "ShadowCascade.Terrain");
                        tbsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                        BindBindlessMaterialTables(tbsb);
                        tbsb.BufferSRV("g_InstanceData", data.bindlessConfig.terrainInstanceBuffer);
                        tbsb.BufferSRV("g_CompactBatchIndices", data.bindlessConfig.terrainCompactBatchIndicesBuffer);
                        tbsb.BufferSRV("g_CompactMaterialIDs", data.bindlessConfig.terrainCompactMaterialIDBuffer);
                        auto terrainSet = cache.GetOrCreateBindingSet(tbsb.Build(), st.terrainLayout, nv);
                        if (terrainSet)
                        {
                            nvrhi::GraphicsState gs;
                            gs.pipeline = st.terrainPipeline;
                            gs.framebuffer = fb;
                            gs.bindings = {terrainSet};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {
                                {data.bindlessConfig.megaVertexBuffer, 0, 0},
                                {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {
                                data.bindlessConfig.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                            gs.viewport.addViewport(cascadeVp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            gs.indirectParams = data.bindlessConfig.terrainCompactDrawArgsBuffer;
                            gs.indirectCountBuffer = data.bindlessConfig.terrainCompactCountBuffer;
                            cmd->setGraphicsState(gs);
                            DrawIndexedIndirectCountOrFallback(
                                cmd, 0, 0, data.bindlessConfig.terrainObjectCount);
                        }
                    }
                }

                // Grass cast into near cascade only (r2_sun_details) — mid/far too coarse
                if (drawGrass && c < 1 && st.grassCB)
                {
                    auto* dm = data.detailManager;
                    const bool billboardMode = !ps_r__detail_gpu;

                    if (billboardMode &&
                        st.billboardGrassPipeline && st.billboardGrassLayout &&
                        dm->visibleBillboardInstancesBuffer &&
                        dm->billboardDrawArgsBuffer &&
                        dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0 &&
                        dm->detailModelsBuffer && dm->pulledVertexBuffer)
                    {
                        auto* bbVsRefl = shaderLoader->GetCachedReflection("detail_billboard_shadow", ".vs");
                        auto* bbPsRefl = shaderLoader->GetCachedReflection("detail_billboard_shadow", ".ps");
                        if (bbVsRefl && bbPsRefl)
                        {
                            framegraph::BindingSetBuilder gsb(
                                *bbVsRefl, *bbPsRefl, nv, "ShadowCascade.BillboardGrass");
                            gsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                            gsb.ConstantBuffer("GrassShadowCB", st.grassCB);
                            // detail_billboard_shadow.ps includes bindless_common.h → t8/t9/t10
                            BindBindlessMaterialTables(gsb);
                            gsb.BufferSRV("visible_indices", dm->visibleBillboardInstancesBuffer);
                            gsb.BufferSRV("detail_models", dm->detailModelsBuffer);
                            gsb.BufferSRV("pulled_vertices", dm->pulledVertexBuffer);
                            gsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                            auto grassSet = cache.GetOrCreateBindingSet(gsb.Build(), st.billboardGrassLayout, nv);
                            if (grassSet)
                            {
                                nvrhi::GraphicsState gs;
                                gs.pipeline = st.billboardGrassPipeline;
                                gs.framebuffer = fb;
                                gs.bindings = {grassSet};
                                if (bindlessTable)
                                    gs.addBindingSet(bindlessTable);
                                gs.indexBuffer = {dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0};
                                gs.viewport.addViewport(cascadeVp);
                                gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                                gs.indirectParams = dm->billboardDrawArgsBuffer;
                                cmd->setGraphicsState(gs);
                                cmd->drawIndexedIndirect(0);
                            }
                        }
                    }
                    else if (!billboardMode && st.grassPipeline && st.grassLayout)
                    {
                        auto* grassVsRefl = shaderLoader->GetCachedReflection("detail_gpu_shadow", ".vs");
                        auto* grassPsRefl = shaderLoader->GetCachedReflection("detail_gpu_shadow", ".ps");
                        if (grassVsRefl && grassPsRefl)
                        {
                            for (u32 lod = 0; lod < 2 && lod < FGDetailManager::LOD_COUNT; ++lod)
                            {
                                if (!dm->visibleInstancesBuffer[lod] || !dm->drawArgsBuffer[lod] ||
                                    !dm->bladeVertexBuffer[lod] || !dm->bladeIndexBuffer[lod])
                                    continue;

                                framegraph::BindingSetBuilder gsb(
                                    *grassVsRefl, *grassPsRefl, nv, "ShadowCascade.Grass");
                                gsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                                gsb.ConstantBuffer("GrassShadowCB", st.grassCB);
                                gsb.BufferSRV("visible_indices", dm->visibleInstancesBuffer[lod]);
                                gsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                                auto grassSet = cache.GetOrCreateBindingSet(gsb.Build(), st.grassLayout, nv);
                                if (!grassSet)
                                    continue;

                                nvrhi::GraphicsState gs;
                                gs.pipeline = st.grassPipeline;
                                gs.framebuffer = fb;
                                gs.bindings = {grassSet};
                                if (bindlessTable)
                                    gs.addBindingSet(bindlessTable);
                                gs.vertexBuffers = {{dm->bladeVertexBuffer[lod], 0, 0}};
                                gs.indexBuffer = {dm->bladeIndexBuffer[lod], nvrhi::Format::R16_UINT, 0};
                                gs.viewport.addViewport(cascadeVp);
                                gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                                gs.indirectParams = dm->drawArgsBuffer[lod];
                                cmd->setGraphicsState(gs);
                                cmd->drawIndexedIndirect(0);
                            }
                        }
                    }
                }

                if (worldSkinnedReady && c < 2)
                {
                    const bool mdiActive = data.gpuCulling->IsSkinnedMDIEnabled() &&
                        st.skinnedMdiLayout && skBoneBuf;
                    auto* mdiVsRefl = mdiActive
                        ? shaderLoader->GetCachedReflection("shadow\\shadow_cascade_skinned_mdi", ".vs")
                        : nullptr;
                    auto drawIndexBuffer = mdiActive
                        ? GetOrCreateDrawIndexBuffer("ShadowCascade_SkinnedMDI", nv)
                        : nullptr;

                    if (mdiActive && mdiVsRefl && drawIndexBuffer)
                    {
                        auto& pools = data.gpuCulling->GetSkinnedPools();
                        for (u32 f = SkinnedGeometryPools::FIRST_FORMAT; f < SkinnedGeometryPools::FORMAT_COUNT; ++f)
                        {
                            const auto& bucket = data.gpuCulling->GetSkinnedBucket(f);
                            if (bucket.count == 0 || !st.skinnedMdiPipeline[f])
                                continue;
                            nvrhi::IBuffer* poolVB = pools.GetVertexBuffer(f);
                            nvrhi::IBuffer* poolIB = pools.GetIndexBuffer(f);
                            if (!poolVB || !poolIB || !bucket.compactDrawArgsBuffer ||
                                !bucket.compactCountBuffer || !bucket.recordsBuffer ||
                                !bucket.compactBatchIndicesBuffer || !bucket.compactMaterialIDBuffer)
                                continue;

                            framegraph::BindingSetBuilder bsb(*mdiVsRefl, *skPsRefl, nv, "ShadowCascade.WorldSkinMDI");
                            bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                            bsb.BufferSRV("g_BoneMatrices", skBoneBuf);
                            BindBindlessMaterialTables(bsb);
                            BindPaintSplatBuffer(bsb, nv);
                            bsb.BufferSRV("g_SkinnedRecords", bucket.recordsBuffer);
                            bsb.BufferSRV("g_SkinnedCompactIndices", bucket.compactBatchIndicesBuffer);
                            bsb.BufferSRV("g_SkinnedCompactMaterialIDs", bucket.compactMaterialIDBuffer);
                            auto set = cache.GetOrCreateBindingSet(bsb.Build(), st.skinnedMdiLayout, nv);
                            if (!set)
                                continue;

                            nvrhi::GraphicsState gs;
                            gs.pipeline = st.skinnedMdiPipeline[f];
                            gs.framebuffer = fb;
                            gs.bindings = {set};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {{poolVB, 0, 0}, {drawIndexBuffer, 1, 0}};
                            gs.indexBuffer = {poolIB, nvrhi::Format::R16_UINT, 0};
                            gs.viewport.addViewport(cascadeVp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            gs.indirectParams = bucket.compactDrawArgsBuffer;
                            gs.indirectCountBuffer = bucket.compactCountBuffer;
                            cmd->setGraphicsState(gs);
                            DrawIndexedIndirectCountOrFallback(cmd, 0, 0, bucket.count);
                        }
                    }

                    if (skDynCB && skMatCB)
                    {
                        for (const auto& batch : *data.worldSkinnedBatches)
                        {
                            if (!batch.isSkinned || !batch.vertexBuffer || !batch.indexBuffer)
                                continue;

                            if (mdiActive)
                            {
                                const u32 variantIdx = bindless::MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
                                const bool pooled = variantIdx == 0
                                    && batch.skinnedPoolFormat >= SkinnedGeometryPools::FIRST_FORMAT
                                    && batch.skinnedPoolFormat < SkinnedGeometryPools::FORMAT_COUNT;
                                if (pooled)
                                    continue;
                            }

                            CKinematics* parent = nullptr;
                            const u32 visualType = batch.visual ? batch.visual->getType() : 0;
                            if (visualType == MT_SKELETON_GEOMDEF_ST)
                                parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
                            else if (visualType == MT_SKELETON_GEOMDEF_PM)
                                parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
                            if (!parent)
                                continue;

                            const u16 rm = batch.skinningRenderMode;
                            const u32 stride = batch.vertexStride;
                            nvrhi::IGraphicsPipeline* pipe = nullptr;
                            nvrhi::IBindingLayout* layout = nullptr;
                            nvrhi::IInputLayout* il = nullptr;
                            auto* vsRefl = skVsReflNonHQ;
                            auto pick3w = [&] { pipe = st.skinned3wPipeline.Get(); layout = st.skinned3wLayout.Get(); il = st.skinned3wInputLayout.Get(); vsRefl = skVsRefl3W; };
                            auto pick2w = [&] { pipe = st.skinned2wPipeline.Get(); layout = st.skinned2wLayout.Get(); il = st.skinned2wInputLayout.Get(); vsRefl = skVsRefl2W; };
                            auto pick4w = [&] { pipe = st.skinned4wPipeline.Get(); layout = st.skinned4wLayout.Get(); il = st.skinned4wInputLayout.Get(); vsRefl = skVsRefl4W; };
                            auto pickHq = [&] { pipe = st.skinnedHqPipeline.Get(); layout = st.skinnedHqLayout.Get(); il = st.skinnedHqInputLayout.Get(); vsRefl = skVsReflHQ; };
                            auto pickNonHq = [&] { pipe = st.skinnedPipeline.Get(); layout = st.skinnedLayout.Get(); il = st.skinnedInputLayout.Get(); vsRefl = skVsReflNonHQ; };
                            if (rm == 7 || rm == 8) pick3w();
                            else if (rm == 5 || rm == 6) pick2w();
                            else if (rm == 9 || rm == 10) pick4w();
                            else if (rm == 4 || rm == 2) pickHq();
                            else if (rm == 3 || rm == 1) pickNonHq();
                            else if (stride == 36) pickHq();
                            else if (stride == 40) pick4w();
                            else if (stride == 44) pick2w();
                            else pickNonHq();

                            if (!pipe || !layout || !il || !vsRefl)
                                continue;

                            const u32 boneOffset = data.gpuCulling->GetOrUploadSkeleton(cmd, parent);

                            DynamicTransforms dyn{};
                            FillDynamicTransforms(dyn, batch.worldMatrix);
                            cmd->writeBuffer(skDynCB, &dyn, sizeof(dyn));

                            SkinnedMaterialCB matId{};
                            matId.materialID = batch.bindlessMaterialID;
                            matId.skeletonBoneOffset = boneOffset;
                            cmd->writeBuffer(skMatCB, &matId, sizeof(matId));

                            framegraph::BindingSetBuilder bsb(*vsRefl, *skPsRefl, nv, "ShadowCascade.WorldSkin");
                            bsb.ConstantBuffer("dynamic_transforms", skDynCB);
                            bsb.ConstantBuffer("ShadowCascadeCB", st.cascadeCB);
                            bsb.ConstantBuffer("SkinnedMaterialCB", skMatCB);
                            bsb.BufferSRV("g_BoneMatrices", skBoneBuf);
                            BindBindlessMaterialTables(bsb);
                            BindPaintSplatBuffer(bsb, nv);
                            auto set = cache.GetOrCreateBindingSet(bsb.Build(), layout, nv);
                            if (!set)
                                continue;

                            nvrhi::GraphicsState gs;
                            gs.pipeline = pipe;
                            gs.framebuffer = fb;
                            gs.bindings = {set};
                            if (bindlessTable)
                                gs.addBindingSet(bindlessTable);
                            gs.vertexBuffers = {{batch.vertexBuffer, 0, 0}};
                            gs.indexBuffer = {batch.indexBuffer, nvrhi::Format::R16_UINT, 0};
                            gs.viewport.addViewport(cascadeVp);
                            gs.viewport.addScissorRect(nvrhi::Rect(cres, cres));
                            cmd->setGraphicsState(gs);
                            cmd->drawIndexed(
                                nvrhi::DrawArguments()
                                    .setVertexCount(batch.indexCount)
                                    .setStartIndexLocation(batch.startIndex)
                                    .setStartVertexLocation(batch.baseVertex));
                        }
                    }
                }

                if (c == 2)
                    s_farCascadeValid = true;
            }
        });

    outputs.shadowArray = passData.shadowCascades[0];
    outputs.valid = true;
    return outputs;
}

} // namespace xray::render::fg::passes
