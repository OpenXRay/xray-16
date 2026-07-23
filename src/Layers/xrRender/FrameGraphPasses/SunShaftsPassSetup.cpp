#include "stdafx.h"
#include "SunShaftsPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{

using namespace framegraph;

struct alignas(16) SunShaftsCBGPU
{
    Fvector4 shaft_params; // intensity, quality, samples, scale
};
static_assert(sizeof(SunShaftsCBGPU) == 16);

struct alignas(16) SunShaftsCombineCBGPU
{
    Fvector4 combine_params; // xy = half-res texel size, zw = unused
};
static_assert(sizeof(SunShaftsCombineCBGPU) == 16);

struct SunShaftsPassData
{
    VirtualResourceHandle depth;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle shadowMap;
    VirtualResourceHandle output; // half-res shafts
    nvrhi::ITexture* shadowMapArray = nullptr;
    nvrhi::ITexture* shadowCascades[3] = {};
    u32 width = 0;  // march (half) resolution
    u32 height = 0;
    float intensity = 0.f;
    u32 quality = 0;
    SunShaftsPassState* passState = nullptr;
};

struct SunShaftsCombineData
{
    VirtualResourceHandle sceneColor;
    VirtualResourceHandle shafts; // half-res
    VirtualResourceHandle output; // full-res scene + shafts
    u32 width = 0;  // full resolution
    u32 height = 0;
    u32 halfWidth = 0;
    u32 halfHeight = 0;
    SunShaftsPassState* passState = nullptr;
};

void InitializeSunShaftsPass(nvrhi::IDevice* device, SunShaftsPassState& state)
{
    if (state.initialized || !device)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        state.initialized = true;
        return;
    }

    auto vs = loader->LoadVertexShader("fullscreen");
    auto ps = loader->LoadPixelShader("volumetric/sunshafts");
    if (!vs.handle || !ps.handle || !ps.reflection)
    {
        Msg("! [SunShafts] Failed to load shaders");
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "SunShafts_CSMLadder", *vs.reflection, *ps.reflection, device);

    nvrhi::GraphicsPipelineDesc desc;
    desc.setVertexShader(vs.handle);
    desc.setPixelShader(ps.handle);
    desc.addBindingLayout(state.layout);
    desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    desc.renderState.blendState.targets[0].setBlendEnable(false);
    desc.renderState.depthStencilState.setDepthTestEnable(false);
    desc.renderState.depthStencilState.setDepthWriteEnable(false);
    desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
    state.pipeline = cache.GetOrCreatePipeline("SunShafts", desc, fbInfo, device);

    // Full-res combine pipeline: upsample + blur half-res shafts, add to scene.
    auto combinePs = loader->LoadPixelShader("volumetric/sunshafts_combine");
    if (combinePs.handle && combinePs.reflection)
    {
        state.combineLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "SunShaftsCombine", *vs.reflection, *combinePs.reflection, device);

        nvrhi::GraphicsPipelineDesc cdesc;
        cdesc.setVertexShader(vs.handle);
        cdesc.setPixelShader(combinePs.handle);
        cdesc.addBindingLayout(state.combineLayout);
        cdesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        cdesc.renderState.blendState.targets[0].setBlendEnable(false);
        cdesc.renderState.depthStencilState.setDepthTestEnable(false);
        cdesc.renderState.depthStencilState.setDepthWriteEnable(false);
        cdesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

        nvrhi::FramebufferInfoEx cfbInfo;
        cfbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        state.combinePipeline = cache.GetOrCreatePipeline("SunShaftsCombine", cdesc, cfbInfo, device);
    }
    else
    {
        Msg("! [SunShafts] Failed to load combine shader");
    }

    state.initialized = true;
    Msg("* [SunShafts] Pass initialized (march=%d combine=%d)",
        state.pipeline ? 1 : 0, state.combinePipeline ? 1 : 0);
}

static bool NeedSunShafts(float& outIntensity, u32& outQuality)
{
    outIntensity = 0.f;
    outQuality = ps_r_sun_shafts;
    if (!ps_r_sun_shafts)
        return false;
    if (!g_pGamePersistent)
        return false;
    const auto& env = g_pGamePersistent->Environment().CurrentEnv;
    outIntensity = env.m_fSunShaftsIntensity;
    if (outIntensity < 0.0001f)
        return false;
    return true;
}

VirtualResourceHandle setupSunShaftsPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle worldPos,
    nvrhi::ITexture* shadowMapArray,
    VirtualResourceHandle shadowMapHandle,
    u32 width,
    u32 height,
    SunShaftsPassState& passState,
    nvrhi::ITexture* const* shadowCascades)
{
    if (device && device->GetNVRHIDevice())
        InitializeSunShaftsPass(device->GetNVRHIDevice(), passState);

    float intensity = 0.f;
    u32 quality = 0;
    if (!NeedSunShafts(intensity, quality) || !passState.pipeline ||
        !passState.combinePipeline || !shadowMapArray)
        return sceneColor;

    // Original renders the volumetric sun into a reduced-res buffer, then combines.
    const u32 halfW = (width > 1) ? (width / 2) : 1;
    const u32 halfH = (height > 1) ? (height / 2) : 1;

    ResourceDesc halfDesc;
    halfDesc.type = ResourceDesc::Type::Texture2D;
    halfDesc.width = halfW;
    halfDesc.height = halfH;
    halfDesc.format = nvrhi::Format::RGBA16_FLOAT;
    halfDesc.isRenderTarget = true;
    halfDesc.isTransient = true;
    halfDesc.debugName = "rt_SunShafts_half";
    auto shaftsHalf = fg.CreateTexture("rt_SunShafts_half", halfDesc);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;
    outDesc.debugName = "rt_SunShafts";
    auto output = fg.CreateTexture("rt_SunShafts", outDesc);

    // ── Pass 1: half-res ray march (shafts term only) ─────────────────────────
    fg.addCallbackPass<SunShaftsPassData>(
        "SunShaftsMarch",
        [=, &passState](FrameGraph& builder, PassHandle passHandle, SunShaftsPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.width = halfW;
            data.height = halfH;
            data.intensity = intensity;
            data.quality = quality;
            data.passState = &passState;
            data.shadowMapArray = shadowMapArray;
            for (int i = 0; i < 3; ++i)
                data.shadowCascades[i] = (shadowCascades && shadowCascades[i]) ? shadowCascades[i] : shadowMapArray;
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            if (shadowMapHandle.is_valid())
                data.shadowMap = pb.read(shadowMapHandle, ResourceState::ShaderResource);
            data.output = pb.write(shaftsHalf, ResourceState::RenderTarget);
        },
        [](const SunShaftsPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* ps = data.passState;
            if (!ps || !ps->pipeline || !ps->layout)
                return;

            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd->getDevice();
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            auto* outTex = fg.GetPhysicalTexture(data.output);
            if (!depthTex || !worldPosTex || !outTex || !data.shadowMapArray)
                return;

            auto& cache = GetPassResourceCache();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), ctx->GetDevice());
            auto shaftCB = cache.GetOrCreateVolatileCB(
                "SunShafts", "SunShaftsCB", sizeof(SunShaftsCBGPU), ctx->GetDevice());

            StaticGlobals sg = BuildStaticGlobals();
            cmd->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));

            SunShaftsCBGPU cb{};
            const float samples = (data.quality >= 3) ? 40.f : 20.f;
            const float scale = ps_r_sun_shafts_scale > 0.f ? ps_r_sun_shafts_scale : 1.f;
            cb.shaft_params.set(data.intensity, float(data.quality), samples, scale);
            cmd->writeBuffer(shaftCB, &cb, sizeof(cb));

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psRefl = loader->GetCachedReflection("volumetric/sunshafts", ".ps");
            if (!vsRefl || !psRefl)
                return;

            BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "SunShaftsMarch");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
                .ConstantBuffer("SunShaftsCB", shaftCB)
                .Texture("g_Depth", depthTex)
                .Texture("g_WorldPos", worldPosTex);
            {
                static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
                auto& smpCache = GetPassResourceCache();
                nvrhi::ITexture* dummy2D = smpCache.GetDummyShadowMap2D(nv);
                for (u32 i = 0; i < 3; ++i)
                {
                    nvrhi::ITexture* t = data.shadowCascades[i] ? data.shadowCascades[i]
                        : (i == 0 && data.shadowMapArray ? data.shadowMapArray : dummy2D);
                    if (t)
                        bsb.Texture(kNames[i], t);
                }
            }
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->layout, nv);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outTex);
            auto fb = cache.GetOrCreateFramebuffer("SunShaftsMarch", fbDesc, nv);

            nvrhi::Viewport vp(0.f, float(data.width), 0.f, float(data.height), 0.f, 1.f);
            nvrhi::GraphicsState state;
            state.pipeline = ps->pipeline;
            state.framebuffer = fb;
            state.viewport.addViewportAndScissorRect(vp);
            state.addBindingSet(bindingSet);
            cmd->setGraphicsState(state);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    // ── Pass 2: full-res combine (upsample + blur + add to scene) ─────────────
    auto& combineData = fg.addCallbackPass<SunShaftsCombineData>(
        "SunShaftsCombine",
        [=, &passState](FrameGraph& builder, PassHandle passHandle, SunShaftsCombineData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.width = width;
            data.height = height;
            data.halfWidth = halfW;
            data.halfHeight = halfH;
            data.passState = &passState;
            data.sceneColor = pb.read(sceneColor, ResourceState::ShaderResource);
            data.shafts = pb.read(shaftsHalf, ResourceState::ShaderResource);
            data.output = pb.write(output, ResourceState::RenderTarget);
        },
        [](const SunShaftsCombineData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* ps = data.passState;
            if (!ps || !ps->combinePipeline || !ps->combineLayout)
                return;

            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd->getDevice();
            auto* colorTex = fg.GetPhysicalTexture(data.sceneColor);
            auto* shaftsTex = fg.GetPhysicalTexture(data.shafts);
            auto* outTex = fg.GetPhysicalTexture(data.output);
            if (!colorTex || !shaftsTex || !outTex)
                return;

            auto& cache = GetPassResourceCache();
            auto combineCB = cache.GetOrCreateVolatileCB(
                "SunShafts", "SunShaftsCombineCB", sizeof(SunShaftsCombineCBGPU), ctx->GetDevice());

            SunShaftsCombineCBGPU cb{};
            cb.combine_params.set(
                1.f / float(data.halfWidth), 1.f / float(data.halfHeight), 0.f, 0.f);
            cmd->writeBuffer(combineCB, &cb, sizeof(cb));

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psRefl = loader->GetCachedReflection("volumetric/sunshafts_combine", ".ps");
            if (!vsRefl || !psRefl)
                return;

            BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "SunShaftsCombine");
            bsb.ConstantBuffer("SunShaftsCombineCB", combineCB)
                .Texture("g_Color", colorTex)
                .Texture("g_Shafts", shaftsTex);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->combineLayout, nv);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outTex);
            auto fb = cache.GetOrCreateFramebuffer("SunShaftsCombine", fbDesc, nv);

            nvrhi::Viewport vp(0.f, float(data.width), 0.f, float(data.height), 0.f, 1.f);
            nvrhi::GraphicsState state;
            state.pipeline = ps->combinePipeline;
            state.framebuffer = fb;
            state.viewport.addViewportAndScissorRect(vp);
            state.addBindingSet(bindingSet);
            cmd->setGraphicsState(state);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    return combineData.output;
}

} // namespace xray::render::fg::passes
