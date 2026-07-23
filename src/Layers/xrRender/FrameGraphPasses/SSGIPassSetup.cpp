#include "stdafx.h"
#include "SSGIPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API int ps_r_ssgi_quality;
extern ENGINE_API float ps_r_ssgi_intensity;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
void EnsureHistory(nvrhi::IDevice* nv, SSGIPassState& st, u32 w, u32 h)
{
    if (st.history[0] && st.historyW == w && st.historyH == h)
        return;
    for (int i = 0; i < 2; ++i)
    {
        nvrhi::TextureDesc td;
        td.width = w;
        td.height = h;
        td.format = nvrhi::Format::RGBA16_FLOAT;
        td.isRenderTarget = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = (i == 0) ? "rt_SSGI_Hist0" : "rt_SSGI_Hist1";
        st.history[i] = nv->createTexture(td);
    }
    st.historyW = w;
    st.historyH = h;
    st.hasHistory = false;
    st.historyIndex = 0;
}

void InitSSGI(nvrhi::IDevice* nv, SSGIPassState& st)
{
    if (st.initialized || !nv)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader) { st.initialized = true; return; }

    auto vs = loader->LoadVertexShader("fullscreen");
    auto psTrace = loader->LoadPixelShader("ssgi_trace");
    auto psBlur = loader->LoadPixelShader("ssgi_blur");
    auto psTemporal = loader->LoadPixelShader("ssgi_temporal");
    auto psApply = loader->LoadPixelShader("ssgi_apply");
    if (!vs.handle || !psTrace.handle || !psBlur.handle || !psTemporal.handle || !psApply.handle)
    {
        st.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();
    nvrhi::FramebufferInfoEx fb;
    fb.addColorFormat(nvrhi::Format::RGBA16_FLOAT);

    auto makePipe = [&](const char* name, auto& ps, nvrhi::BindingLayoutHandle& layout,
                        nvrhi::GraphicsPipelineHandle& pipe) {
        layout = cache.GetOrCreateBindingLayoutFromReflection(
            name, *vs.reflection, *ps.reflection, nv);
        if (!layout)
            return;
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(ps.handle);
        desc.addBindingLayout(layout);
        auto* backend = GEnv.Backend;
        if (backend && backend->GetBindlessLayout())
            desc.addBindingLayout(backend->GetBindlessLayout());
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        pipe = cache.GetOrCreatePipeline(name, desc, fb, nv);
    };

    makePipe("SSGI_Trace_v2", psTrace, st.traceLayout, st.tracePipeline);
    makePipe("SSGI_Blur_v2", psBlur, st.blurLayout, st.blurPipeline);
    makePipe("SSGI_Temporal_v2", psTemporal, st.temporalLayout, st.temporalPipeline);
    makePipe("SSGI_Apply_v2", psApply, st.applyLayout, st.applyPipeline);
    st.initialized = true;
}

void DrawFullscreen(
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
    gs.bindings = { set };
    if (GEnv.Backend && GEnv.Backend->GetBindlessDescriptorTable())
        gs.addBindingSet(GEnv.Backend->GetBindlessDescriptorTable());
    gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(w), float(h)));
    cmd->setGraphicsState(gs);
    cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
}
} // namespace

VirtualResourceHandle setupSSGIPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    SSGIPassState& state)
{
    if (!device || !device->GetNVRHIDevice())
        return sceneColor;
    InitSSGI(device->GetNVRHIDevice(), state);
    if (!state.tracePipeline || !state.blurPipeline || !state.applyPipeline)
        return sceneColor;

    const int quality = std::clamp(ps_r_ssgi_quality, 1, 3);
    const bool halfRes = quality < 3;
    const u32 tw = halfRes ? std::max(1u, width / 2) : width;
    const u32 th = halfRes ? std::max(1u, height / 2) : height;

    EnsureHistory(device->GetNVRHIDevice(), state, tw, th);
    const u32 readIdx = state.historyIndex;
    const u32 writeIdx = 1u - readIdx;
    const bool useTemporal = state.temporalPipeline && motionVectors.is_valid() &&
        hasPrevFrame && state.hasHistory && state.history[0];

    ResourceDesc giDesc;
    giDesc.type = ResourceDesc::Type::Texture2D;
    giDesc.width = tw;
    giDesc.height = th;
    giDesc.format = nvrhi::Format::RGBA16_FLOAT;
    giDesc.isRenderTarget = true;
    giDesc.isTransient = true;
    giDesc.debugName = "rt_SSGI";
    auto giTrace = fg.CreateTexture("rt_SSGI", giDesc);

    ResourceDesc blurDesc = giDesc;
    blurDesc.debugName = "rt_SSGI_Blur";
    auto giBlur = fg.CreateTexture("rt_SSGI_Blur", blurDesc);

    ResourceDesc tempDesc = giDesc;
    tempDesc.debugName = "rt_SSGI_Temporal";
    auto giTemporal = fg.CreateTexture("rt_SSGI_Temporal", tempDesc);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;
    outDesc.debugName = "rt_SSGI_Out";
    auto output = fg.CreateTexture("rt_SSGI_Out", outDesc);

    VirtualResourceHandle histRead{};
    if (useTemporal)
    {
        ResourceDesc histDesc = giDesc;
        histDesc.isTransient = false;
        histDesc.isImported = true;
        histDesc.debugName = "rt_SSGI_HistRead";
        histRead = fg.ImportTexture("rt_SSGI_HistRead", state.history[readIdx].Get(), histDesc);
    }

    struct PassData
    {
        VirtualResourceHandle color, depth, normal, base, motion, hist;
        VirtualResourceHandle giTrace, giBlur, giTemporal, output;
        u32 width, height, tw, th, writeIdx;
        int quality;
        float intensity;
        bool useTemporal;
        SSGIPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "SSGI",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.tw = tw;
            data.th = th;
            data.writeIdx = writeIdx;
            data.quality = quality;
            data.intensity = ps_r_ssgi_intensity;
            data.useTemporal = useTemporal;
            data.color = pb.read(sceneColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.base = pb.read(baseColor, ResourceState::ShaderResource);
            if (useTemporal)
            {
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
                data.hist = pb.read(histRead, ResourceState::ShaderResource);
            }
            data.giTrace = pb.write(giTrace, ResourceState::RenderTarget);
            data.giBlur = pb.write(giBlur, ResourceState::RenderTarget);
            data.giTemporal = pb.write(giTemporal, ResourceState::RenderTarget);
            data.output = pb.write(output, ResourceState::RenderTarget);
            pb.sideEffects();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd->getDevice();
            auto* color = graph.GetPhysicalTexture(data.color);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* normalTex = graph.GetPhysicalTexture(data.normal);
            auto* baseTex = graph.GetPhysicalTexture(data.base);
            auto* giT = graph.GetPhysicalTexture(data.giTrace);
            auto* giB = graph.GetPhysicalTexture(data.giBlur);
            auto* giTemp = graph.GetPhysicalTexture(data.giTemporal);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !color || !depthTex || !normalTex || !baseTex ||
                !giT || !giB || !giTemp || !out || !data.st)
                return;

            const auto& cdesc = color->getDesc();
            if (!data.st->colorCopy ||
                data.st->colorCopy->getDesc().width != cdesc.width ||
                data.st->colorCopy->getDesc().height != cdesc.height)
            {
                nvrhi::TextureDesc td = cdesc;
                td.debugName = "SSGI_ColorCopy";
                td.isRenderTarget = true;
                td.initialState = nvrhi::ResourceStates::ShaderResource;
                td.keepInitialState = true;
                data.st->colorCopy = nv->createTexture(td);
            }
            if (data.st->colorCopy)
                cmd->copyTexture(data.st->colorCopy, nvrhi::TextureSlice(), color, nvrhi::TextureSlice());

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psTraceR = loader->GetCachedReflection("ssgi_trace", ".ps");
            auto* psBlurR = loader->GetCachedReflection("ssgi_blur", ".ps");
            auto* psTempR = loader->GetCachedReflection("ssgi_temporal", ".ps");
            auto* psApplyR = loader->GetCachedReflection("ssgi_apply", ".ps");
            if (!vsR || !psTraceR || !psBlurR || !psApplyR)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            struct alignas(16) TraceCB
            {
                float intensity, maxDistance, thickness, rayCount;
            };
            float rays = (data.quality <= 1) ? 4.f : (data.quality == 2) ? 6.f : 8.f;
            float maxDist = (data.quality <= 1) ? 8.f : (data.quality == 2) ? 12.f : 18.f;
            TraceCB tcb{ data.intensity, maxDist, 0.85f, rays };
            auto* traceCB = cache.GetOrCreateVolatileCB("SSGI", "TraceParams", sizeof(TraceCB), data.device);
            if (traceCB)
                cmd->writeBuffer(traceCB, &tcb, sizeof(tcb));

            {
                BindingSetBuilder bsb(*vsR, *psTraceR, nv, "SSGI.Trace");
                if (staticGlobalsCB)
                    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                if (traceCB)
                    bsb.ConstantBuffer("SSGIParams", traceCB);
                bsb.Texture("g_Normal", normalTex)
                    .Texture("g_Base", baseTex)
                    .Texture("g_SceneColor", data.st->colorCopy)
                    .Texture("g_SceneDepth", depthTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->traceLayout, nv);
                DrawFullscreen(cmd, nv, data.st->tracePipeline, set, giT, data.tw, data.th, "SSGI_Trace");
                cmd->setTextureState(giT, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            struct alignas(16) BlurCB { float texelX, texelY, p2, p3; };
            BlurCB bcb{ 1.f / float(data.tw), 1.f / float(data.th), 0, 0 };
            auto* blurCB = cache.GetOrCreateVolatileCB("SSGI", "BlurParams", sizeof(BlurCB), data.device);
            if (blurCB)
                cmd->writeBuffer(blurCB, &bcb, sizeof(bcb));

            {
                BindingSetBuilder bsb(*vsR, *psBlurR, nv, "SSGI.Blur");
                if (blurCB)
                    bsb.ConstantBuffer("SSGIBlurParams", blurCB);
                bsb.Texture("g_SSGI", giT).Texture("g_Depth", depthTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->blurLayout, nv);
                DrawFullscreen(cmd, nv, data.st->blurPipeline, set, giB, data.tw, data.th, "SSGI_Blur");
                cmd->setTextureState(giB, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            nvrhi::ITexture* giForApply = giB;
            if (data.useTemporal && psTempR && data.st->temporalPipeline)
            {
                auto* hist = graph.GetPhysicalTexture(data.hist);
                auto* motionTex = graph.GetPhysicalTexture(data.motion);
                if (!motionTex)
                {
                    static nvrhi::TextureHandle s_zero;
                    if (!s_zero)
                    {
                        nvrhi::TextureDesc td;
                        td.width = 1; td.height = 1;
                        td.format = nvrhi::Format::RG16_FLOAT;
                        td.initialState = nvrhi::ResourceStates::ShaderResource;
                        td.keepInitialState = true;
                        s_zero = nv->createTexture(td);
                        float z[2] = {0, 0};
                        cmd->writeTexture(s_zero, 0, 0, z, sizeof(z));
                    }
                    motionTex = s_zero;
                }
                if (hist && motionTex)
                {
                    struct alignas(16) TempCB { float sx, sy, hasHistory, pad; };
                    TempCB tcb2{ float(data.tw), float(data.th), 1.f, 0.f };
                    auto* tempCB = cache.GetOrCreateVolatileCB("SSGI", "TemporalParams", sizeof(TempCB), data.device);
                    if (tempCB)
                        cmd->writeBuffer(tempCB, &tcb2, sizeof(tcb2));
                    BindingSetBuilder bsb(*vsR, *psTempR, nv, "SSGI.Temporal");
                    if (tempCB)
                        bsb.ConstantBuffer("SSGITemporalParams", tempCB);
                    bsb.Texture("g_Current", giB)
                        .Texture("g_History", hist)
                        .Texture("g_Motion", motionTex)
                        .Texture("g_Depth", depthTex);
                    auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->temporalLayout, nv);
                    DrawFullscreen(cmd, nv, data.st->temporalPipeline, set, giTemp, data.tw, data.th, "SSGI_Temporal");
                    cmd->setTextureState(giTemp, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    giForApply = giTemp;
                }
            }

            if (data.st->history[data.writeIdx] && giForApply)
            {
                cmd->copyTexture(
                    data.st->history[data.writeIdx], nvrhi::TextureSlice(),
                    giForApply, nvrhi::TextureSlice());
                data.st->historyIndex = data.writeIdx;
                data.st->hasHistory = true;
            }

            struct alignas(16) ApplyCB { float intensity, p1, p2, p3; };
            ApplyCB acb{ 1.0f, 0, 0, 0 };
            auto* applyCB = cache.GetOrCreateVolatileCB("SSGI", "ApplyParams", sizeof(ApplyCB), data.device);
            if (applyCB)
                cmd->writeBuffer(applyCB, &acb, sizeof(acb));

            {
                BindingSetBuilder bsb(*vsR, *psApplyR, nv, "SSGI.Apply");
                if (staticGlobalsCB)
                    bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                if (applyCB)
                    bsb.ConstantBuffer("SSGIApplyParams", applyCB);
                bsb.Texture("g_Color", data.st->colorCopy)
                    .Texture("g_SSGI", giForApply)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_Base", baseTex)
                    .Texture("g_Depth", depthTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->applyLayout, nv);
                DrawFullscreen(cmd, nv, data.st->applyPipeline, set, out, data.width, data.height, "SSGI_Apply");
            }
        });

    return pd.output;
}

} // namespace
