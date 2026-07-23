#include "stdafx.h"
#include "SSRPassSetup.h"
#include "ShaderConstants.h"
#include "PassCommon.h"
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
constexpr u32 kSSRPipeVersion = 43;

struct SSRQualityParams
{
    float steps;
    float distance;
    float thickness;
    float refine;
    float blurIntensity;
};

SSRQualityParams QualityFromConsole()
{
    switch (std::clamp(ps_r_ssr_quality, 1, 4))
    {
    case 1: return {16.f, 80.f, 1.50f, 2.f, 0.70f};
    case 2: return {24.f, 120.f, 1.25f, 4.f, 0.85f};
    case 3: return {32.f, 160.f, 1.00f, 6.f, 1.00f};
    default: return {48.f, 220.f, 0.85f, 8.f, 1.00f};
    }
}

void EnsureSSRBuffer(nvrhi::IDevice* nv, SSRPassState& st, u32 w, u32 h)
{
    if (st.history[0] && st.history[1] && st.outputTex && st.blurTex &&
        st.historyW == w && st.historyH == h)
        return;
    nvrhi::TextureDesc td;
    td.width = w;
    td.height = h;
    td.format = nvrhi::Format::RGBA16_FLOAT;
    td.isRenderTarget = true;
    td.isShaderResource = true;
    td.initialState = nvrhi::ResourceStates::ShaderResource;
    td.keepInitialState = true;
    td.debugName = "rt_SSR_History0";
    st.history[0] = nv->createTexture(td);
    td.debugName = "rt_SSR_History1";
    st.history[1] = nv->createTexture(td);
    td.debugName = "rt_SSR_Output";
    st.outputTex = nv->createTexture(td);
    td.debugName = "rt_SSR_BlurPersist";
    st.blurTex = nv->createTexture(td);
    st.historyW = w;
    st.historyH = h;
    st.historyIndex = 0;
    st.historyNeedsClear = true;
    st.hasHistory = false;
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
    auto psApply = loader->LoadPixelShader("ssr_apply");
    if (!vs.handle || !ps.handle || !psBlur.handle || !psApply.handle ||
        !vs.reflection || !ps.reflection || !psBlur.reflection || !psApply.reflection)
    {
        Msg("! [SSR] Shader load failed (resolve=%d blur=%d apply=%d) — will retry",
            !!ps.handle, !!psBlur.handle, !!psApply.handle);
        st.initialized = false;
        return;
    }

    auto& cache = GetPassResourceCache();
    st.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRResolve_v45", *vs.reflection, *ps.reflection, nv);
    st.blurLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRBlur_v45", *vs.reflection, *psBlur.reflection, nv);
    st.applyLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSRApply_v45", *vs.reflection, *psApply.reflection, nv);

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

    makeSingle(ps, st.layout, st.pipeline, "SSRResolve_v45");
    makeSingle(psBlur, st.blurLayout, st.blurPipeline, "SSRBlur_v45");
    makeSingle(psApply, st.applyLayout, st.applyPipeline, "SSRApply_v45");

    st.initialized = true;
    st.pipeVersion = kSSRPipeVersion;
    if (st.pipeline && st.blurPipeline && st.applyPipeline)
        Msg("* [SSR] initialized v45 (SSR dim sky + NPC face sheen)");
    else
        Msg("! [SSR] Pipeline create failed (resolve=%d blur=%d apply=%d)",
            !!st.pipeline, !!st.blurPipeline, !!st.applyPipeline);
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

    EnsureSSRBuffer(device->GetNVRHIDevice(), state, width, height);
    if (!state.outputTex || !state.blurTex || !state.history[0] || !state.history[1])
        return sceneColor;
    (void)hasPrevFrame;
    (void)motionVectors;
    (void)worldPos;

    // Persistent imported RTs — never FG-aliased (CreateTexture non-transient was not enough).
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
    blurDesc.debugName = "rt_SSR_Blur";
    auto blurRT = fg.ImportTexture("rt_SSR_Blur", state.blurTex.Get(), blurDesc);

    VirtualResourceHandle ssrBuf{};
    VirtualResourceHandle ssrHist{};
    {
        const u32 writeIdx = state.historyIndex & 1u;
        const u32 readIdx = (writeIdx ^ 1u);
        ResourceDesc ssrDesc = outDesc;
        ssrDesc.debugName = "rt_SSR_Buffer";
        ssrBuf = fg.ImportTexture("rt_SSR_Buffer", state.history[writeIdx].Get(), ssrDesc);
        ssrDesc.debugName = "rt_SSR_History";
        ssrHist = fg.ImportTexture("rt_SSR_History", state.history[readIdx].Get(), ssrDesc);
    }

    struct PassData
    {
        VirtualResourceHandle color, reflection, depth, normal, base, ssr, hist, blur, output;
        u32 width, height;
        SSRPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        bool hasHistory = false;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "SSR",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.hasHistory = state.hasHistory && !state.historyNeedsClear;
            data.color = pb.read(sceneColor, ResourceState::ShaderResource);
            data.reflection = pb.read(reflectionColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.base = pb.read(baseColor, ResourceState::ShaderResource);
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

            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("ssr_resolve", ".ps");
            auto* psBlurR = loader->GetCachedReflection("ssr_blur", ".ps");
            auto* psApplyR = loader->GetCachedReflection("ssr_apply", ".ps");

            // Lit scene copy for apply SRV (must NOT be the same texture as `out`)
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

            // Depth → typeless SRV copy (same as water SSR on Metal/MoltenVK)
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

            // Fallback: scene already in out if resolve/apply fail
            cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
            cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
            cmd->copyTexture(out, nvrhi::TextureSlice(), color, nvrhi::TextureSlice());
            cmd->setTextureState(color, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(depthTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(normalTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(baseTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

            if (!vsR || !psR || !psBlurR || !psApplyR)
            {
                cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                return;
            }

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
                float pad0, pad1, pad2, pad3;
            };
            PCB p{
                0.65f, 0.95f,
                data.hasHistory ? 1.f : 0.f,
                float(Device.dwFrame & 1023),
                q.steps, q.distance, q.thickness, q.refine,
                0.f, 0.f, 0.f, 0.f};
            auto* ssrCB = cache.GetOrCreateVolatileCB("SSR", "SSRParams_v45", sizeof(PCB), data.device);
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

            nvrhi::ITexture* sky0 = nullptr;
            nvrhi::ITexture* sky1 = nullptr;
            ResolveEnvSkyCubes(data.device, sky0, sky1);
            if (!sky0) sky0 = cache.GetDummyCubeMap(nv);
            if (!sky1) sky1 = sky0;

            nvrhi::ITexture* histSrv = histTex ? histTex : cache.GetDummyContactHistory(nv);
            if (data.st->historyNeedsClear && histSrv)
            {
                cmd->clearTextureFloat(histSrv, nvrhi::AllSubresources, nvrhi::Color(0.f));
                data.st->historyNeedsClear = false;
            }

            static int s_ssrLog = 0;
            if ((s_ssrLog++ % 300) == 0)
                Msg("* [SSR] exec v45 resolve→blur→apply");

            // 1) Resolve
            cmd->clearTextureFloat(ssrTex, nvrhi::AllSubresources, nvrhi::Color(0.f));
            {
                nvrhi::ITexture* depthSrv = data.st->depthCopy ? data.st->depthCopy.Get() : depthTex;
                BindingSetBuilder bsb(*vsR, *psR, nv, "SSR.Resolve");
                if (staticGlobalsCB)
                    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                if (ssrCB)
                    bsb.ConstantBuffer("SSRParams", ssrCB);
                bsb.Texture("g_Color", data.st->litCopy ? data.st->litCopy.Get() : color)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_Base", baseTex)
                    .Texture("g_History", histSrv)
                    .Texture("g_Motion", s_zeroMotion)
                    .Texture("g_SceneColor", data.st->colorCopy)
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
                    // Never leave rt_SSR unwritten (transient-era footgun).
                    nvrhi::ITexture* litSrc = data.st->litCopy ? data.st->litCopy.Get() : color;
                    if (litSrc && out)
                    {
                        cmd->setTextureState(litSrc, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmd->copyTexture(out, nvrhi::TextureSlice(), litSrc, nvrhi::TextureSlice());
                    }
                    cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    return;
                }
                DrawSSRFullscreen(cmd, nv, data.st->pipeline, set, ssrTex, data.width, data.height, "SSR_Resolve_v45");
                cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            // 2) Bilateral denoise
            struct alignas(16) BlurCB { float texelX, texelY, blurIntensity, pad; };
            BlurCB bcb{1.f / float(data.width), 1.f / float(data.height), q.blurIntensity, 0.f};
            auto* blurCB = cache.GetOrCreateVolatileCB("SSR", "BlurParams_v45", sizeof(BlurCB), data.device);
            if (blurCB)
                cmd->writeBuffer(blurCB, &bcb, sizeof(bcb));
            {
                nvrhi::ITexture* depthSrv = data.st->depthCopy ? data.st->depthCopy.Get() : depthTex;
                BindingSetBuilder bsb(*vsR, *psBlurR, nv, "SSR.Blur");
                if (blurCB)
                    bsb.ConstantBuffer("SSRBlurParams", blurCB);
                bsb.Texture("g_SSR", ssrTex)
                    .Texture("g_Depth", depthSrv)
                    .Texture("g_Normal", normalTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->blurLayout, nv);
                if (set)
                    DrawSSRFullscreen(cmd, nv, data.st->blurPipeline, set, blurTex, data.width, data.height, "SSR_Blur_v45");
                else
                {
                    cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(blurTex, nvrhi::TextureSlice(), ssrTex, nvrhi::TextureSlice());
                }
                cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            // Store blurred SSR into history write slot for next frame temporal hold
            {
                cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                cmd->copyTexture(ssrTex, nvrhi::TextureSlice(), blurTex, nvrhi::TextureSlice());
                cmd->setTextureState(ssrTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                cmd->setTextureState(blurTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                data.st->historyIndex ^= 1u;
                data.st->hasHistory = true;
            }

            // 3) Apply: read litCopy, write out — never the same texture
            {
                nvrhi::ITexture* litSrc = data.st->litCopy ? data.st->litCopy.Get() : color;
                BindingSetBuilder bsb(*vsR, *psApplyR, nv, "SSR.Apply");
                bsb.Texture("g_Color", litSrc).Texture("g_SSR", blurTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->applyLayout, nv);
                if (set)
                    DrawSSRFullscreen(cmd, nv, data.st->applyPipeline, set, out, data.width, data.height, "SSR_Apply_v45");
                else if (litSrc && out)
                {
                    cmd->setTextureState(litSrc, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                    cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                    cmd->copyTexture(out, nvrhi::TextureSlice(), litSrc, nvrhi::TextureSlice());
                }
                cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }
        });

    return pd.output;
}

} // namespace
