#include "stdafx.h"
#include "SSRPassSetup.h"
#include "ShaderConstants.h"
#include "PassCommon.h"
#include "IBLPrefilterPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API int ps_r_ssr_quality;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
constexpr u32 kSSRPipeVersion = 72;

struct SSRQualityParams
{
    float steps;
    float distance;
    float thickness;
    float refine;
    float blurIntensity;
    float blurKernel;
    float enableNearMarch;
    float enableBlurRefine;
};

SSRQualityParams QualityFromConsole()
{
    switch (std::clamp(ps_r_ssr_quality, 1, 4))
    {
    case 1: return {16.f, 80.f, 1.50f, 2.f, 1.25f, 3.f, 0.f, 0.f};
    case 2: return {24.f, 120.f, 1.25f, 4.f, 1.35f, 4.f, 0.f, 0.f};
    case 3: return {32.f, 160.f, 1.00f, 6.f, 1.45f, 5.f, 1.f, 1.f};
    default: return {48.f, 220.f, 0.85f, 8.f, 1.55f, 6.f, 1.f, 1.f};
    }
}

void EnsureSSRBuffer(nvrhi::IDevice* nv, SSRPassState& st, u32 fullW, u32 fullH, u32 traceW, u32 traceH)
{
    const bool needTrace = !st.history[0] || !st.history[1] || !st.blurTex ||
        st.historyW != traceW || st.historyH != traceH;
    const bool needOut = !st.outputTex || st.outputW != fullW || st.outputH != fullH;
    if (!needTrace && !needOut)
        return;

    nvrhi::TextureDesc td;
    td.format = nvrhi::Format::RGBA16_FLOAT;
    td.isRenderTarget = true;
    td.isShaderResource = true;
    td.initialState = nvrhi::ResourceStates::ShaderResource;
    td.keepInitialState = true;

    if (needOut)
    {
        td.width = fullW;
        td.height = fullH;
        td.debugName = "rt_SSR_Output";
        st.outputTex = nv->createTexture(td);
        st.outputW = fullW;
        st.outputH = fullH;
    }

    if (needTrace)
    {
        td.width = traceW;
        td.height = traceH;
        td.debugName = "rt_SSR_History0";
        st.history[0] = nv->createTexture(td);
        td.debugName = "rt_SSR_History1";
        st.history[1] = nv->createTexture(td);
        td.debugName = "rt_SSR_BlurPersist";
        st.blurTex = nv->createTexture(td);
        st.historyW = traceW;
        st.historyH = traceH;
        st.historyIndex = 0;
        st.historyNeedsClear = true;
        st.hasHistory = false;
    }
}

void DrawSSRFullscreen(
    nvrhi::ICommandList* cmd,
    nvrhi::IDevice* nv,
    nvrhi::GraphicsPipelineHandle pipe,
    nvrhi::BindingSetHandle set,
    nvrhi::ITexture* out,
    u32 w, u32 h,
    const char* fbName)
{
    if (!cmd || !pipe || !set || !out)
        return;
    auto& cache = GetPassResourceCache();
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(out);
    auto fb = cache.GetOrCreateFramebuffer(fbName, fbDesc, nv);
    nvrhi::GraphicsState gs;
    gs.pipeline = pipe;
    gs.framebuffer = fb;
    gs.bindings = {set};
    if (GEnv.Backend && GEnv.Backend->GetBindlessDescriptorTable())
        gs.addBindingSet(GEnv.Backend->GetBindlessDescriptorTable());
    gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(w), float(h)));
    cmd->setGraphicsState(gs);
    cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
}

void InitSSR(nvrhi::IDevice* nv, SSRPassState& st)
{
    if (!nv)
        return;
    if (st.initialized && st.pipeVersion == kSSRPipeVersion)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        st.pipeVersion = kSSRPipeVersion;
        return;
    }

    BindingSetBuilder::InvalidateReflectionCache();

    auto vs = loader->LoadVertexShader("fullscreen");
    auto ps = loader->LoadPixelShader("ssr_resolve");
    auto psBlur = loader->LoadPixelShader("ssr_blur");
    auto psTemp = loader->LoadPixelShader("ssr_temporal");
    auto psApply = loader->LoadPixelShader("ssr_apply");
    if (!vs.handle || !ps.handle || !psBlur.handle || !psTemp.handle || !psApply.handle ||
        !vs.reflection || !ps.reflection || !psBlur.reflection || !psTemp.reflection || !psApply.reflection)
    {
        Msg("! [SSR] Shader load failed (resolve=%d blur=%d temp=%d apply=%d) — will retry",
            !!ps.handle, !!psBlur.handle, !!psTemp.handle, !!psApply.handle);
        st.initialized = false;
        return;
    }

    auto& cache = GetPassResourceCache();
    st.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRResolve_v71", *vs.reflection, *ps.reflection, nv);
    st.blurLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRBlur_v71", *vs.reflection, *psBlur.reflection, nv);
    st.temporalLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRTemporal_v71", *vs.reflection, *psTemp.reflection, nv);
    st.applyLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRApply_v72_MetalWet", *vs.reflection, *psApply.reflection, nv);

    auto makeSingle = [&](auto& psSh, nvrhi::BindingLayoutHandle layout,
                          nvrhi::GraphicsPipelineHandle& pipe, const char* name) {
        if (!layout)
            return;
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(psSh.handle);
        desc.addBindingLayout(layout);
        auto* backend = GEnv.Backend;
        if (backend && backend->GetBindlessLayout())
            desc.addBindingLayout(backend->GetBindlessLayout());
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        nvrhi::FramebufferInfoEx fb;
        fb.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        pipe = cache.GetOrCreatePipeline(name, desc, fb, nv);
    };

    makeSingle(ps, st.layout, st.pipeline, "SSRResolve_v71");
    makeSingle(psBlur, st.blurLayout, st.blurPipeline, "SSRBlur_v71");
    makeSingle(psTemp, st.temporalLayout, st.temporalPipeline, "SSRTemporal_v71");
    makeSingle(psApply, st.applyLayout, st.applyPipeline, "SSRApply_v72_MetalWet");

    st.initialized = true;
    st.pipeVersion = kSSRPipeVersion;
    if (st.pipeline && st.blurPipeline && st.temporalPipeline && st.applyPipeline)
        Msg("* [SSR] initialized v72 (SSR∪RT metal/wet)");
    else
        Msg("! [SSR] Pipeline create failed (resolve=%d blur=%d temp=%d apply=%d)",
            !!st.pipeline, !!st.blurPipeline, !!st.temporalPipeline, !!st.applyPipeline);
}
} // namespace

VirtualResourceHandle setupSSRPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle reflectionColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle worldPos,
    VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    SSRPassState& state)
{
    if (!device || !device->GetNVRHIDevice())
        return sceneColor;

    InitSSR(device->GetNVRHIDevice(), state);
    if (!state.pipeline || !state.layout || !state.blurPipeline || !state.blurLayout ||
        !state.temporalPipeline || !state.temporalLayout ||
        !state.applyPipeline || !state.applyLayout)
    {
        static bool s_loggedPipe = false;
        if (!s_loggedPipe)
        {
            Msg("! [SSR] Pipelines missing — passthrough");
            s_loggedPipe = true;
        }
        return sceneColor;
    }

    if (!reflectionColor.is_valid())
        reflectionColor = sceneColor;
    if (!depth.is_valid())
        return sceneColor;

    const int quality = std::clamp(ps_r_ssr_quality, 1, 4);
#if !defined(XR_PLATFORM_APPLE)
    const bool halfRes = quality < 4;
#else
    const bool halfRes = false;
#endif
    const u32 tw = halfRes ? std::max(1u, width / 2) : width;
    const u32 th = halfRes ? std::max(1u, height / 2) : height;

    EnsureSSRBuffer(device->GetNVRHIDevice(), state, width, height, tw, th);
    if (!state.outputTex || !state.blurTex || !state.history[0] || !state.history[1])
        return sceneColor;
    (void)hasPrevFrame;

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = false;
    outDesc.isImported = true;
    outDesc.debugName = "rt_SSR";
    auto output = fg.ImportTexture("rt_SSR", state.outputTex.Get(), outDesc);

    ResourceDesc blurDesc = outDesc;
    blurDesc.width = tw;
    blurDesc.height = th;
    blurDesc.debugName = "rt_SSR_Blur";
    auto blurRT = fg.ImportTexture("rt_SSR_Blur", state.blurTex.Get(), blurDesc);

    VirtualResourceHandle ssrBuf{};
    VirtualResourceHandle ssrHist{};
    {
        const u32 writeIdx = state.historyIndex & 1u;
        const u32 readIdx = (writeIdx ^ 1u);
        ResourceDesc ssrDesc = blurDesc;
        ssrDesc.debugName = "rt_SSR_Buffer";
        ssrBuf = fg.ImportTexture("rt_SSR_Buffer", state.history[writeIdx].Get(), ssrDesc);
        ssrDesc.debugName = "rt_SSR_History";
        ssrHist = fg.ImportTexture("rt_SSR_History", state.history[readIdx].Get(), ssrDesc);
    }

    struct PassData
    {
        VirtualResourceHandle color, reflection, depth, normal, base, worldPos, motion, ssr, hist, blur, output;
        u32 width, height, tw, th;
        int quality = 2;
        SSRPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        bool hasHistory = false;
        bool hasWorldPos = false;
        bool hasMotion = false;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "SSR",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.tw = tw;
            data.th = th;
            data.quality = quality;
            data.hasHistory = state.hasHistory && !state.historyNeedsClear;
            data.color = pb.read(sceneColor, ResourceState::ShaderResource);
            data.reflection = pb.read(reflectionColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.base = pb.read(baseColor, ResourceState::ShaderResource);
            data.hasWorldPos = worldPos.is_valid();
            if (data.hasWorldPos)
                data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.hasMotion = motionVectors.is_valid();
            if (data.hasMotion)
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            data.hist = pb.read(ssrHist, ResourceState::ShaderResource);
            data.ssr = pb.write(ssrBuf, ResourceState::RenderTarget);
            data.blur = pb.write(blurRT, ResourceState::RenderTarget);
            data.output = pb.write(output, ResourceState::RenderTarget);
            pb.sideEffects();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd->getDevice();
            auto* color = graph.GetPhysicalTexture(data.color);
            auto* reflection = graph.GetPhysicalTexture(data.reflection);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* normalTex = graph.GetPhysicalTexture(data.normal);
            auto* baseTex = graph.GetPhysicalTexture(data.base);
            auto* worldPosTex = data.hasWorldPos ? graph.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* motionTex = data.hasMotion ? graph.GetPhysicalTexture(data.motion) : nullptr;
            auto* histTex = graph.GetPhysicalTexture(data.hist);
            auto* out = graph.GetPhysicalTexture(data.output);
            auto* ssrTex = graph.GetPhysicalTexture(data.ssr);
            auto* blurTex = graph.GetPhysicalTexture(data.blur);
            if (!cmd || !nv || !color || !reflection || !depthTex || !normalTex || !baseTex ||
                !out || !ssrTex || !blurTex || !data.st)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            if (!loader)
                return;

            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("ssr_resolve", ".ps");
            auto* psBlurR = loader->GetCachedReflection("ssr_blur", ".ps");
            auto* psApplyR = loader->GetCachedReflection("ssr_apply", ".ps");
            auto* psTempR = loader->GetCachedReflection("ssr_temporal", ".ps");
            if (!vsR || !psR || !psBlurR || !psTempR || !psApplyR)
            {
                cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                cmd->copyTexture(out, nvrhi::TextureSlice(), color, nvrhi::TextureSlice());
                cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                return;
            }

#if defined(XR_PLATFORM_APPLE)
            const auto& rdesc = reflection->getDesc();
            if (!data.st->colorCopy ||
                data.st->colorCopy->getDesc().width != rdesc.width ||
                data.st->colorCopy->getDesc().height != rdesc.height)
            {
                nvrhi::TextureDesc td = rdesc;
                td.debugName = "SSR_ReflectionCopy";
                td.isRenderTarget = true;
                td.isShaderResource = true;
                td.initialState = nvrhi::ResourceStates::ShaderResource;
                td.keepInitialState = true;
                data.st->colorCopy = nv->createTexture(td);
            }
            if (data.st->colorCopy)
            {
                cmd->setTextureState(reflection, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                cmd->setTextureState(data.st->colorCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                cmd->copyTexture(data.st->colorCopy, nvrhi::TextureSlice(), reflection, nvrhi::TextureSlice());
                cmd->setTextureState(data.st->colorCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                cmd->setTextureState(reflection, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            const auto& cdesc = color->getDesc();
            if (!data.st->litCopy ||
                data.st->litCopy->getDesc().width != cdesc.width ||
                data.st->litCopy->getDesc().height != cdesc.height)
            {
                nvrhi::TextureDesc td = cdesc;
                td.debugName = "SSR_LitCopy";
                td.isRenderTarget = true;
                td.isShaderResource = true;
                td.initialState = nvrhi::ResourceStates::ShaderResource;
                td.keepInitialState = true;
                data.st->litCopy = nv->createTexture(td);
            }
            if (data.st->litCopy)
            {
                cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                cmd->setTextureState(data.st->litCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                cmd->copyTexture(data.st->litCopy, nvrhi::TextureSlice(), color, nvrhi::TextureSlice());
                cmd->setTextureState(data.st->litCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            {
                const auto& ddesc = depthTex->getDesc();
                if (!data.st->depthCopy ||
                    data.st->depthCopy->getDesc().width != ddesc.width ||
                    data.st->depthCopy->getDesc().height != ddesc.height)
                {
                    nvrhi::TextureDesc td{};
                    td.width = ddesc.width;
                    td.height = ddesc.height;
                    td.format = nvrhi::Format::D32;
                    td.debugName = "SSR_DepthCopy";
                    td.isShaderResource = true;
                    td.isTypeless = true;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    data.st->depthCopy = nv->createTexture(td);
                }
                if (data.st->depthCopy)
                {
                    cmd->setTextureState(depthTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(data.st->depthCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(data.st->depthCopy, nvrhi::TextureSlice(), depthTex, nvrhi::TextureSlice());
                    cmd->setTextureState(data.st->depthCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                }
            }
#else
            nvrhi::ITexture* sceneColorSrv = reflection;
            nvrhi::ITexture* litSrv = (color != out) ? color : nullptr;
            if (!litSrv)
            {
                const auto& cdesc = color->getDesc();
                if (!data.st->litCopy ||
                    data.st->litCopy->getDesc().width != cdesc.width ||
                    data.st->litCopy->getDesc().height != cdesc.height)
                {
                    nvrhi::TextureDesc td = cdesc;
                    td.debugName = "SSR_LitCopy";
                    td.isRenderTarget = true;
                    td.isShaderResource = true;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    data.st->litCopy = nv->createTexture(td);
                }
                if (data.st->litCopy)
                {
                    cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(data.st->litCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(data.st->litCopy, nvrhi::TextureSlice(), color, nvrhi::TextureSlice());
                    cmd->setTextureState(data.st->litCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    litSrv = data.st->litCopy;
                }
                else
                    litSrv = color;
            }
#endif

            cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(reflection, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(depthTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(normalTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(baseTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            if (staticGlobalsCB)
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmd->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            const SSRQualityParams q = QualityFromConsole();
            struct alignas(16) PCB
            {
                float cutoff, intensity, hasHistory, frameIndex;
                float marchSteps, marchDistance, marchThickness, refineSteps;
                float enableNearMarch, pad1, pad2, pad3;
            };
            PCB p{
                0.65f, 1.25f,
                data.hasHistory ? 1.f : 0.f,
                float(Device.dwFrame & 1023),
                q.steps, q.distance, q.thickness, q.refine,
                q.enableNearMarch, 0.f, 0.f, 0.f};
            auto* ssrCB = cache.GetOrCreateVolatileCB("SSR", "SSRParams_v71", sizeof(PCB), data.device);
            if (ssrCB)
                cmd->writeBuffer(ssrCB, &p, sizeof(p));

            static nvrhi::TextureHandle s_zeroMotion;
            if (!s_zeroMotion)
            {
                nvrhi::TextureDesc td;
                td.width = 1; td.height = 1;
                td.format = nvrhi::Format::RG16_FLOAT;
                td.initialState = nvrhi::ResourceStates::ShaderResource;
                td.keepInitialState = true;
                s_zeroMotion = nv->createTexture(td);
            }
            nvrhi::ITexture* motionSrv = motionTex ? motionTex : s_zeroMotion.Get();

            const auto& ibl = GetCurrentIBLBindResources();
            nvrhi::ITexture* sky0 = ibl.spec0 ? ibl.spec0 : cache.GetDummyCubeMap(nv);
            nvrhi::ITexture* sky1 = ibl.spec1 ? ibl.spec1 : sky0;

            nvrhi::ITexture* histSrv = histTex ? histTex : cache.GetDummyContactHistory(nv);
            if (data.st->historyNeedsClear && histSrv)
            {
                cmd->clearTextureFloat(histSrv, nvrhi::AllSubresources, nvrhi::Color(0.f));
                data.st->historyNeedsClear = false;
            }

#if defined(XR_PLATFORM_APPLE)
            nvrhi::ITexture* sceneColorSrv = data.st->colorCopy ? data.st->colorCopy.Get() : reflection;
            nvrhi::ITexture* litSrv = data.st->litCopy ? data.st->litCopy.Get() : color;
            nvrhi::ITexture* depthSrv = data.st->depthCopy ? data.st->depthCopy.Get() : depthTex;
#else
            nvrhi::ITexture* depthSrv = depthTex;
#endif

            cmd->clearTextureFloat(ssrTex, nvrhi::AllSubresources, nvrhi::Color(0.f));
            {
                BindingSetBuilder bsb(*vsR, *psR, nv, "SSR.Resolve");
                if (staticGlobalsCB)
                    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                if (ssrCB)
                    bsb.ConstantBuffer("SSRParams", ssrCB);
                static nvrhi::TextureHandle s_zeroWorldPos;
                if (!s_zeroWorldPos)
                {
                    nvrhi::TextureDesc td;
                    td.width = 1;
                    td.height = 1;
                    td.format = nvrhi::Format::RGBA16_FLOAT;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    td.debugName = "SSR_DummyWorldPos";
                    s_zeroWorldPos = nv->createTexture(td);
                }
                nvrhi::ITexture* wpSrv = worldPosTex ? worldPosTex : s_zeroWorldPos.Get();
                bsb.Texture("g_Color", litSrv)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_Base", baseTex)
                    .Texture("g_History", histSrv)
                    .Texture("g_WorldPos", wpSrv)
                    .Texture("g_Motion", motionSrv)
                    .Texture("g_SceneColor", sceneColorSrv)
                    .Texture("g_SceneDepth", depthSrv)
                    .Texture("s_env0", sky0)
                    .Texture("s_env1", sky1);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->layout, nv);
                if (!set)
                {
                    static bool s_loggedBind = false;
                    if (!s_loggedBind)
                    {
                        Msg("! [SSR] Resolve binding failed — scene passthrough");
                        s_loggedBind = true;
                    }
                    if (litSrv && out)
                    {
                        cmd->setTextureState(litSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmd->copyTexture(out, nvrhi::TextureSlice(), litSrv, nvrhi::TextureSlice());
                    }
                    cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    return;
                }
                DrawSSRFullscreen(cmd, nv, data.st->pipeline, set, ssrTex, data.tw, data.th, "SSR_Resolve_v71");
                cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            struct alignas(16) BlurCB { float texelX, texelY, blurIntensity, blurKernel; };
            BlurCB bcb{1.f / float(data.tw), 1.f / float(data.th), q.blurIntensity, q.blurKernel};
            auto* blurCB = cache.GetOrCreateVolatileCB("SSR", "BlurParams_v71", sizeof(BlurCB), data.device);
            if (blurCB)
                cmd->writeBuffer(blurCB, &bcb, sizeof(bcb));
            {
                BindingSetBuilder bsb(*vsR, *psBlurR, nv, "SSR.Blur");
                if (blurCB)
                    bsb.ConstantBuffer("SSRBlurParams", blurCB);
                bsb.Texture("g_SSR", ssrTex)
                    .Texture("g_Depth", depthSrv)
                    .Texture("g_Normal", normalTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->blurLayout, nv);
                if (set)
                    DrawSSRFullscreen(cmd, nv, data.st->blurPipeline, set, blurTex, data.tw, data.th, "SSR_Blur_v71");
                else
                {
                    cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(blurTex, nvrhi::TextureSlice(), ssrTex, nvrhi::TextureSlice());
                }
                cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            nvrhi::ITexture* ssrForApply = blurTex;
            {
                struct alignas(16) TempCB { float sx, sy, hasHistory, pad; };
                TempCB tcb{
                    float(data.tw), float(data.th),
                    data.hasHistory ? 1.f : 0.f, 0.f};
                auto* tempCB = cache.GetOrCreateVolatileCB("SSR", "TemporalParams_v71", sizeof(TempCB), data.device);
                if (tempCB)
                    cmd->writeBuffer(tempCB, &tcb, sizeof(tcb));
                BindingSetBuilder bsb(*vsR, *psTempR, nv, "SSR.Temporal");
                if (tempCB)
                    bsb.ConstantBuffer("SSRTemporalParams", tempCB);
                bsb.Texture("g_Current", blurTex)
                    .Texture("g_History", histSrv)
                    .Texture("g_Depth", depthSrv)
                    .Texture("g_Motion", motionSrv);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->temporalLayout, nv);
                if (set)
                {
                    DrawSSRFullscreen(cmd, nv, data.st->temporalPipeline, set, ssrTex, data.tw, data.th, "SSR_Temporal_v71");
                    cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    ssrForApply = ssrTex;
                }
                else
                {
                    cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(ssrTex, nvrhi::TextureSlice(), blurTex, nvrhi::TextureSlice());
                    cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    ssrForApply = ssrTex;
                }
                data.st->historyIndex ^= 1u;
                data.st->hasHistory = true;
            }

            if (q.enableBlurRefine > 0.5f && ssrForApply == ssrTex)
            {
                BlurCB bcb2{1.f / float(data.tw), 1.f / float(data.th), q.blurIntensity * 0.85f, q.blurKernel};
                auto* blurCB2 = cache.GetOrCreateVolatileCB("SSR", "BlurParams2_v71", sizeof(BlurCB), data.device);
                if (blurCB2)
                    cmd->writeBuffer(blurCB2, &bcb2, sizeof(bcb2));
                BindingSetBuilder bsb(*vsR, *psBlurR, nv, "SSR.BlurRefine");
                if (blurCB2)
                    bsb.ConstantBuffer("SSRBlurParams", blurCB2);
                bsb.Texture("g_SSR", ssrTex)
                    .Texture("g_Depth", depthSrv)
                    .Texture("g_Normal", normalTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->blurLayout, nv);
                if (set)
                {
                    DrawSSRFullscreen(cmd, nv, data.st->blurPipeline, set, blurTex, data.tw, data.th, "SSR_BlurRefine_v71");
                    cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    ssrForApply = blurTex;
                }
            }

            {
                BindingSetBuilder bsb(*vsR, *psApplyR, nv, "SSR.Apply");
                bsb.Texture("g_Color", litSrv)
                    .Texture("g_SSR", ssrForApply)
                    .Texture("g_Base", baseTex)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_WorldPos", worldPosTex ? worldPosTex : cache.GetDummyContactHistory(nv));
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->applyLayout, nv);
                if (set)
                    DrawSSRFullscreen(cmd, nv, data.st->applyPipeline, set, out, data.width, data.height, "SSR_Apply_v72");
                else if (litSrv && out)
                {
                    cmd->setTextureState(litSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(out, nvrhi::TextureSlice(), litSrv, nvrhi::TextureSlice());
                }
                cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }
        });

    return pd.output;
}

} // namespace
