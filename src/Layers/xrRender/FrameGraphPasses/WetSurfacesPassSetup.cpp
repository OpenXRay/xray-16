#include "stdafx.h"
#include "WetSurfacesPassSetup.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/TAAPassSetup.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"

extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API int ps_r_path_tracer;

namespace xray::render::fg::passes {

using namespace framegraph;

// Phase 2 CB — matrices for wet SSR + rain SM projection
struct alignas(16) WetConstantsGPU
{
    Fvector4 RainDensity; // x = y = rain density
    Fvector4 EyePos;
    Fvector4 SunColor;
    Fvector4 Timers; // x = time
    Fmatrix m_VP;
    Fmatrix m_RainSampleVP;
};
static_assert(sizeof(WetConstantsGPU) == 192, "WetConstantsGPU size mismatch");

struct WetPassData
{
    VirtualResourceHandle colorIn;
    VirtualResourceHandle normal;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle patched;
    VirtualResourceHandle colorOut;
    VirtualResourceHandle normalOut;
    VirtualResourceHandle rainSM;
    VirtualResourceHandle sceneReflection;
    u32 width = 0;
    u32 height = 0;
    WetSurfacesPassState* passState = nullptr;
    WetSurfacesExtras extras;
    bool hasRainSM = false;
    bool hasSceneReflection = false;
    bool hasBaseColor = false;
};

static void BlitColor(nvrhi::ICommandList* cmdList, nvrhi::ITexture* dst, nvrhi::ITexture* src)
{
    if (!cmdList || !dst || !src)
        return;
    cmdList->setTextureState(src, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
    cmdList->setTextureState(dst, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
    cmdList->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
    cmdList->setTextureState(dst, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
}

static WetConstantsGPU MakeWetConstants(const Fmatrix& rainSampleVP)
{
    WetConstantsGPU wet{};
    float rainDensity = g_pGamePersistent
        ? g_pGamePersistent->Environment().CurrentEnv.rain_density
        : 0.f;
    wet.RainDensity.set(rainDensity, rainDensity, 0.f, 0.f);
    const bool rtNoDistFade = (ps_r_rt_gi != 0) || (ps_r_path_tracer != 0);
    wet.EyePos.set(
        Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z,
        rtNoDistFade ? 1.f : 0.f);
    if (g_pGamePersistent)
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        wet.SunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 0.f);
    }
    const float t = Device.fTimeGlobal;
    wet.Timers.set(t, t * 10.f, t / 10.f, _sin(t));
    wet.m_VP = g_taa_unjittered_full_transform;
    wet.m_RainSampleVP = rainSampleVP;
    return wet;
}

constexpr u32 kWetPipeVersion = 25;

void InitializeWetSurfacesPass(nvrhi::IDevice* device, WetSurfacesPassState& state)
{
    if (!device)
        return;
    if (state.initialized && state.pipeVersion == kWetPipeVersion)
        return;
    state.initialized = false;

    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!loader)
    {
        state.initialized = true;
        return;
    }

    auto vs = loader->LoadVertexShader("fullscreen");
    auto patchPs = loader->LoadPixelShader("wet/rain_patch_normal");
    auto applyPs = loader->LoadPixelShader("wet/rain_apply");
    auto writePs = loader->LoadPixelShader("wet/rain_write_normal");

    // Apply is mandatory; patch/write are optional (Slang often crashes on patch).
    if (!vs.handle || !vs.reflection || !applyPs.handle || !applyPs.reflection)
    {
        Msg("! [WetSurfaces] Failed to load apply shader — wet disabled");
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "WetApply_v25_SpecUnion", *vs.reflection, *applyPs.reflection, device);

    if (patchPs.handle && patchPs.reflection)
    {
        state.patchLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "WetPatch_v24", *vs.reflection, *patchPs.reflection, device);
    }
    else
    {
        Msg("! [WetSurfaces] patch shader missing — apply-only mode");
    }

    if (writePs.handle && writePs.reflection)
    {
        state.writeNormalLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "WetWriteNormal_v24", *vs.reflection, *writePs.reflection, device);
    }

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);

    auto makePipe = [&](auto& ps, nvrhi::BindingLayoutHandle layout,
                        nvrhi::GraphicsPipelineHandle& pipe, const char* name) {
        if (!layout || !ps.handle)
            return;
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(ps.handle);
        desc.addBindingLayout(layout);
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        pipe = cache.GetOrCreatePipeline(name, desc, fbInfo, device);
    };

    makePipe(applyPs, state.layout, state.pipeline, "WetApply_v25_SpecUnion");
    makePipe(patchPs, state.patchLayout, state.patchPipeline, "WetPatch_v24");
    makePipe(writePs, state.writeNormalLayout, state.writeNormalPipeline, "WetWriteNormal_v24");

    state.initialized = true;
    state.pipeVersion = kWetPipeVersion;
    if (state.pipeline && state.layout)
    {
        Msg("* [WetSurfaces] Rebuild v25: apply=%d patch=%d write=%d",
            !!state.pipeline, !!state.patchPipeline, !!state.writeNormalPipeline);
    }
    else
        Msg("! [WetSurfaces] Pipeline create failed — wet disabled");
}

namespace
{
bool EnsureWetTarget(
    nvrhi::IDevice* nvDevice,
    nvrhi::TextureHandle& slot,
    u32 width,
    u32 height,
    const char* name)
{
    if (!nvDevice || width == 0 || height == 0)
        return false;

    if (slot)
    {
        const auto& d = slot->getDesc();
        if (d.width == width && d.height == height)
            return true;
    }

    nvrhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isRenderTarget = true;
    desc.isShaderResource = true;
    desc.debugName = name;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    slot = nvDevice->createTexture(desc);
    if (!slot)
    {
        Msg("! [WetSurfaces] Failed to create %s %ux%u RGBA16F", name, width, height);
        return false;
    }
    return true;
}
} // namespace

DefaultOutputLayout setupWetSurfacesPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& inputs,
    u32 width,
    u32 height,
    WetSurfacesPassState& passState,
    const WetSurfacesExtras& extras)
{
    DefaultOutputLayout outputs = inputs;
    static bool s_loggedNoFlag = false;
    static bool s_loggedMissingInputs = false;
    static bool s_loggedNoPipelines = false;
    static bool s_loggedNoTargets = false;

    if (!ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF))
    {
        if (!s_loggedNoFlag)
        {
            Msg("! [WetSurfaces] Disabled by r3_dynamic_wet_surfaces flag");
            s_loggedNoFlag = true;
        }
        return outputs;
    }

    const float rainDensity = g_pGamePersistent
        ? g_pGamePersistent->Environment().CurrentEnv.rain_density
        : 0.f;
    if (rainDensity < 0.001f)
        return outputs;

    if (!inputs.albedo.is_valid() || !inputs.normal.is_valid() ||
        !inputs.worldPos.is_valid())
    {
        if (!s_loggedMissingInputs)
        {
            Msg("! [WetSurfaces] Missing inputs: albedo=%d normal=%d worldPos=%d",
                inputs.albedo.is_valid(), inputs.normal.is_valid(),
                inputs.worldPos.is_valid());
            s_loggedMissingInputs = true;
        }
        return outputs;
    }

    if (!device || !device->GetNVRHIDevice())
        return outputs;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    InitializeWetSurfacesPass(nvDevice, passState);

    if (!passState.pipeline || !passState.layout)
    {
        if (!s_loggedNoPipelines)
        {
            Msg("! [WetSurfaces] Missing apply pipeline/layout (patch=%d write=%d)",
                !!passState.patchPipeline, !!passState.writeNormalPipeline);
            s_loggedNoPipelines = true;
        }
        return outputs;
    }

    const bool doWriteNormal = passState.writeNormalPipeline && passState.writeNormalLayout
        && passState.patchPipeline && passState.patchLayout;

    const bool needResize = passState.texWidth != width || passState.texHeight != height;
    if (needResize)
    {
        passState.patched = nullptr;
        passState.color = nullptr;
        passState.normal = nullptr;
        passState.texWidth = 0;
        passState.texHeight = 0;
    }

    if (!EnsureWetTarget(nvDevice, passState.patched, width, height, "rt_WetPatched") ||
        !EnsureWetTarget(nvDevice, passState.color, width, height, "rt_WetColor") ||
        (doWriteNormal && !EnsureWetTarget(nvDevice, passState.normal, width, height, "rt_WetNormal")))
    {
        passState.patched = nullptr;
        passState.color = nullptr;
        passState.normal = nullptr;
        passState.texWidth = 0;
        passState.texHeight = 0;
        if (!s_loggedNoTargets)
        {
            Msg("! [WetSurfaces] RT alloc failed — wet disabled this frame");
            s_loggedNoTargets = true;
        }
        return outputs;
    }
    passState.texWidth = width;
    passState.texHeight = height;
    s_loggedNoTargets = false;

    ResourceDesc texDesc;
    texDesc.type = ResourceDesc::Type::Texture2D;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.format = nvrhi::Format::RGBA16_FLOAT;
    texDesc.isRenderTarget = true;
    texDesc.isTransient = false;

    texDesc.debugName = "rt_WetPatched";
    auto patched = fg.ImportTexture("rt_WetPatched", passState.patched.Get(), texDesc);
    texDesc.debugName = "rt_WetColor";
    auto outColor = fg.ImportTexture("rt_WetColor", passState.color.Get(), texDesc);

    VirtualResourceHandle outNormal{};
    if (doWriteNormal)
    {
        texDesc.debugName = "rt_WetNormal";
        outNormal = fg.ImportTexture("rt_WetNormal", passState.normal.Get(), texDesc);
    }

    const bool hasRainSM = extras.rainSMValid && extras.rainSM.is_valid();
    const bool hasSceneReflection = extras.sceneReflectionValid && extras.sceneReflection.is_valid();

    (void)fg.addCallbackPass<WetPassData>(
        "WetSurfaces",
        [inputs, patched, outColor, outNormal, doWriteNormal, hasRainSM, hasSceneReflection,
         width, height, &passState, extras](
            FrameGraph& builder, PassHandle passHandle, WetPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.passState = &passState;
            data.extras = extras;
            data.hasRainSM = hasRainSM;
            data.hasSceneReflection = hasSceneReflection;
            data.colorIn = passBuilder.read(inputs.albedo, ResourceState::ShaderResource);
            data.normal = passBuilder.read(inputs.normal, ResourceState::ShaderResource);
            data.worldPos = passBuilder.read(inputs.worldPos, ResourceState::ShaderResource);
            data.hasBaseColor = inputs.baseColor.is_valid();
            if (data.hasBaseColor)
                data.baseColor = passBuilder.read(inputs.baseColor, ResourceState::ShaderResource);
            if (hasRainSM)
                data.rainSM = passBuilder.read(extras.rainSM, ResourceState::ShaderResource);
            if (hasSceneReflection)
                data.sceneReflection = passBuilder.read(extras.sceneReflection, ResourceState::ShaderResource);
            data.patched = passBuilder.write(patched, ResourceState::RenderTarget);
            data.colorOut = passBuilder.write(outColor, ResourceState::RenderTarget);
            if (doWriteNormal && outNormal.is_valid())
                data.normalOut = passBuilder.write(outNormal, ResourceState::RenderTarget);
            passBuilder.sideEffects();
        },
        [](const WetPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx)
        {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto* ps = data.passState;

            auto* colorIn = fgGraph.GetPhysicalTexture(data.colorIn);
            auto* colorOut = fgGraph.GetPhysicalTexture(data.colorOut);
            BlitColor(cmdList, colorOut, colorIn);

            if (!ps || !ps->pipeline || !ps->layout)
                return;

            auto* normalTex = fgGraph.GetPhysicalTexture(data.normal);
            auto* worldPosTex = fgGraph.GetPhysicalTexture(data.worldPos);
            auto* patchedTex = fgGraph.GetPhysicalTexture(data.patched);
            auto* baseTex = data.hasBaseColor ? fgGraph.GetPhysicalTexture(data.baseColor) : nullptr;
            if (!colorIn || !normalTex || !worldPosTex || !patchedTex || !colorOut)
                return;

            if (colorIn == colorOut)
            {
                static bool s_loggedAlias = false;
                if (!s_loggedAlias)
                {
                    Msg("! [WetSurfaces] colorIn == colorOut — wet cannot composite");
                    s_loggedAlias = true;
                }
                return;
            }

            auto& cache = GetPassResourceCache();
            auto* rd = ctx->GetDevice();
            auto wetCB = cache.GetOrCreateVolatileCB(
                "WetSurfaces", "WetConstants_v1n_RainVP", sizeof(WetConstantsGPU), rd);
            WetConstantsGPU wet = MakeWetConstants(data.extras.rainSampleVP);
            cmdList->writeBuffer(wetCB, &wet, sizeof(wet));

            static int s_wetExecLog = 0;
            if ((s_wetExecLog++ % 120) == 0)
            {
                Msg("* [WetSurfaces] exec rain=%.3f ssrSrc=%d in=%p out=%p",
                    wet.RainDensity.x, data.hasSceneReflection,
                    (void*)colorIn, (void*)colorOut);
            }

            auto* vsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("fullscreen", ".vs");
            auto* applyRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("wet/rain_apply", ".ps");
            if (!vsRefl || !applyRefl)
                return;
            auto* patchRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("wet/rain_patch_normal", ".ps");
            auto* writeRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("wet/rain_write_normal", ".ps");

            nvrhi::ITexture* rainSM = data.hasRainSM
                ? fgGraph.GetPhysicalTexture(data.rainSM)
                : nullptr;
            if (!rainSM)
                rainSM = data.extras.rainSMTex;
            if (!rainSM)
                rainSM = cache.GetDummyShadowMap2D(nvDevice);

            auto* texMgr = rd && rd->GetFGResourceManager()
                ? rd->GetFGResourceManager()->GetTextureManager()
                : nullptr;
            auto loadTex = [&](const char* name) -> nvrhi::ITexture* {
                if (!texMgr)
                    return nullptr;
                auto h = texMgr->LoadTexture(name);
                return texMgr->GetNVRHITexture(h);
            };
            nvrhi::ITexture* waterRipple = loadTex("fx" DELIMITER "water_sbumpvolume");
            if (!waterRipple)
                waterRipple = loadTex("fx" DELIMITER "rain_splash");
            if (!waterRipple)
                waterRipple = loadTex("water" DELIMITER "water_sbumpvolume");
            nvrhi::ITexture* waterFall = loadTex("fx" DELIMITER "water_normal");
            if (!waterFall)
                waterFall = loadTex("water" DELIMITER "water_normal");
            nvrhi::ITexture* puddlesPerlin = loadTex("fx" DELIMITER "puddles_perlin");
            if (!waterRipple)
                waterRipple = cache.GetDummyContactHistory(nvDevice);
            if (!waterFall)
                waterFall = waterRipple;
            if (!puddlesPerlin)
                puddlesPerlin = waterRipple;
            if (!baseTex)
                baseTex = cache.GetDummyContactHistory(nvDevice);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            // 1) Patch normals + wetness (optional)
            bool havePatched = false;
            if (ps->patchPipeline && ps->patchLayout && patchRefl)
            {
                BindingSetBuilder bsb(*vsRefl, *patchRefl, nvDevice, "WetPatch");
                bsb.ConstantBuffer("WetConstants", wetCB)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_WorldPos", worldPosTex)
                    .Texture("g_RainShadow", rainSM)
                    .Texture("g_Water", waterRipple)
                    .Texture("g_WaterFall", waterFall)
                    .Texture("g_PuddlesPerlin", puddlesPerlin)
                    .Texture("g_Base", baseTex);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->patchLayout, nvDevice);
                if (bindingSet)
                {
                    nvrhi::FramebufferDesc fbDesc;
                    fbDesc.addColorAttachment(patchedTex);
                    auto framebuffer = cache.GetOrCreateFramebuffer("WetPatch_v1d", fbDesc, nvDevice);
                    if (framebuffer)
                    {
                        nvrhi::GraphicsState state;
                        state.pipeline = ps->patchPipeline;
                        state.framebuffer = framebuffer;
                        state.viewport.addViewportAndScissorRect(viewport);
                        state.addBindingSet(bindingSet);
                        cmdList->setGraphicsState(state);
                        cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
                        cmdList->setTextureState(patchedTex, nvrhi::AllSubresources,
                            nvrhi::ResourceStates::ShaderResource);
                        havePatched = true;
                    }
                }
            }

            // Apply-only: feed scene normals; wet amount comes from rain in PS
            nvrhi::ITexture* patchedForApply = havePatched ? patchedTex : normalTex;

            // 2) Write normals for AO (optional)
            if (ps->writeNormalPipeline && ps->writeNormalLayout && writeRefl
                && data.normalOut.is_valid() && havePatched)
            {
                auto* normalOut = fgGraph.GetPhysicalTexture(data.normalOut);
                if (normalOut)
                {
                BindingSetBuilder bsb(*vsRefl, *writeRefl, nvDevice, "WetWriteNormal");
                bsb.Texture("g_Normal", normalTex).Texture("g_Patched", patchedTex);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->writeNormalLayout, nvDevice);
                if (bindingSet)
                {
                    nvrhi::FramebufferDesc fbDesc;
                    fbDesc.addColorAttachment(normalOut);
                    auto framebuffer = cache.GetOrCreateFramebuffer("WetWriteNormal_v1f", fbDesc, nvDevice);
                    if (framebuffer)
                    {
                        nvrhi::GraphicsState state;
                        state.pipeline = ps->writeNormalPipeline;
                        state.framebuffer = framebuffer;
                        state.viewport.addViewportAndScissorRect(viewport);
                        state.addBindingSet(bindingSet);
                        cmdList->setGraphicsState(state);
                        cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
                        cmdList->setTextureState(normalOut, nvrhi::AllSubresources,
                            nvrhi::ResourceStates::ShaderResource);
                    }
                }
                }
            }

            // 3) Apply darken + streaks + wet SSR
            {
                nvrhi::ITexture* sceneRefl = data.hasSceneReflection
                    ? fgGraph.GetPhysicalTexture(data.sceneReflection)
                    : nullptr;
                if (!sceneRefl)
                    sceneRefl = colorIn; // fallback: still safe (no write feedback to colorIn)

                cmdList->setTextureState(colorIn, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
                cmdList->setTextureState(patchedForApply, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
                cmdList->setTextureState(worldPosTex, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
                cmdList->setTextureState(sceneRefl, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
                cmdList->setTextureState(colorOut, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::RenderTarget);

                BindingSetBuilder bsb(*vsRefl, *applyRefl, nvDevice, "WetApply");
                bsb.ConstantBuffer("WetConstants", wetCB)
                    .Texture("g_Color", colorIn)
                    .Texture("g_Patched", patchedForApply)
                    .Texture("g_WorldPos", worldPosTex)
                    .Texture("g_WaterFall", waterFall)
                    .Texture("g_SceneReflection", sceneRefl)
                    .Texture("g_Base", baseTex);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->layout, nvDevice);
                if (!bindingSet)
                {
                    static bool s_loggedApplyBind = false;
                    if (!s_loggedApplyBind)
                    {
                        Msg("! [WetSurfaces] Apply binding failed — passthrough blit kept");
                        s_loggedApplyBind = true;
                    }
                    return;
                }

                nvrhi::FramebufferDesc fbDesc;
                fbDesc.addColorAttachment(colorOut);
                auto framebuffer = cache.GetOrCreateFramebuffer("WetApply_v7", fbDesc, nvDevice);
                if (!framebuffer)
                    return;

                nvrhi::GraphicsState state;
                state.pipeline = ps->pipeline;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(viewport);
                state.addBindingSet(bindingSet);
                cmdList->setGraphicsState(state);
                cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
                cmdList->setTextureState(colorOut, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
            }
        });

    outputs.albedo = outColor;
    if (doWriteNormal && outNormal.is_valid())
        outputs.normal = outNormal;
    return outputs;
}

} // namespace xray::render::fg::passes
