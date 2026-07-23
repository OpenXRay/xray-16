#include "stdafx.h"
#include "TAAPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API float ps_r_taa_sharpness;
extern ENGINE_API int ps_r_taa;

namespace xray::render::fg::passes
{
using namespace framegraph;

float g_taa_jitter_px = 0.f;
float g_taa_jitter_py = 0.f;
float g_taa_jitter_prev_px = 0.f;
float g_taa_jitter_prev_py = 0.f;
Fmatrix g_taa_unjittered_full_transform;
Fmatrix g_taa_unjittered_inv_full_transform;

void ApplyTAAJitter()
{
    // Keep temporal reprojection in the unjittered camera space.
    g_taa_unjittered_full_transform.mul(Device.mProject, Device.mView);
    g_taa_unjittered_inv_full_transform.invert_44(g_taa_unjittered_full_transform);

    g_taa_jitter_prev_px = g_taa_jitter_px;
    g_taa_jitter_prev_py = g_taa_jitter_py;
    g_taa_jitter_px = 0.f;
    g_taa_jitter_py = 0.f;
    // Full projection jitter currently shifts the whole frame in this pipeline.
    // Keep TAA temporal accumulation enabled, but leave the camera projection stable.

    Device.mFullTransform = g_taa_unjittered_full_transform;
    Device.mInvFullTransform = g_taa_unjittered_inv_full_transform;
    if (GEnv.Render)
        GEnv.Render->SetCacheXform(Device.mView, Device.mProject);
}

namespace
{
void EnsureHistory(nvrhi::IDevice* nv, TAAPassState& state, u32 w, u32 h)
{
    if (state.history[0] && state.historyW == w && state.historyH == h)
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
        td.debugName = (i == 0) ? "rt_TAA_History0" : "rt_TAA_History1";
        state.history[i] = nv->createTexture(td);
    }
    state.historyW = w;
    state.historyH = h;
    state.hasHistory = false;
    state.historyIndex = 0;

    // Clear so first temporal frame never blends garbage (nested-frame look)
    if (state.history[0] && state.history[1])
    {
        nvrhi::CommandListHandle cmd = nv->createCommandList();
        cmd->open();
        cmd->clearTextureFloat(state.history[0], nvrhi::AllSubresources, nvrhi::Color(0.f));
        cmd->clearTextureFloat(state.history[1], nvrhi::AllSubresources, nvrhi::Color(0.f));
        cmd->close();
        nv->executeCommandList(cmd);
    }
}

void InitializeTAA(nvrhi::IDevice* device, TAAPassState& state)
{
    // Hot-reload path: bump layout/PSO name when shader binding changes
    if (state.initialized && state.pipeline && state.layout)
        return;
    if (state.initialized && (!state.pipeline || !state.layout))
        state.initialized = false;
    if (state.initialized || !device)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        state.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("fullscreen");
    auto ps = loader->LoadPixelShader("taa");
    if (!vs.handle || !ps.handle)
    {
        state.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "TAA_v4", *vs.reflection, *ps.reflection, device);
    if (state.layout)
    {
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(ps.handle);
        desc.addBindingLayout(state.layout);
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        nvrhi::FramebufferInfoEx fb;
        fb.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        state.pipeline = cache.GetOrCreatePipeline("TAA_v4", desc, fb, device);
    }
    state.initialized = true;
}
} // namespace

void ShutdownTAAPass(TAAPassState& state)
{
    state.pipeline = nullptr;
    state.layout = nullptr;
    state.history[0] = nullptr;
    state.history[1] = nullptr;
    state.hasHistory = false;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupTAAPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    TAAPassState& state)
{
    if (!device || !device->GetNVRHIDevice())
        return sceneColor;

    InitializeTAA(device->GetNVRHIDevice(), state);
    if (!state.pipeline || !state.layout)
        return sceneColor;

    EnsureHistory(device->GetNVRHIDevice(), state, width, height);
    if (!state.history[0] || !state.history[1])
        return sceneColor;

    const u32 readIdx = state.historyIndex;
    const u32 writeIdx = 1u - readIdx;

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = true;
    outDesc.debugName = "rt_TAA";
    auto output = fg.CreateTexture("rt_TAA", outDesc);

    ResourceDesc histDesc;
    histDesc.type = ResourceDesc::Type::Texture2D;
    histDesc.width = width;
    histDesc.height = height;
    histDesc.format = nvrhi::Format::RGBA16_FLOAT;
    histDesc.isImported = true;
    histDesc.debugName = "rt_TAA_HistoryRead";
    auto historyRead = fg.ImportTexture("rt_TAA_HistoryRead", state.history[readIdx].Get(), histDesc);

    const bool useTemporal = motionVectors.is_valid() && hasPrevFrame && state.hasHistory;

    struct PassData
    {
        VirtualResourceHandle current, history, motion, depth, output;
        TAAPassState* passState = nullptr;
        fg::RenderDevice* device = nullptr;
        u32 width = 0, height = 0;
        u32 writeIdx = 0;
        bool useTemporal = false;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "TAA",
        [&, output, historyRead, useTemporal, writeIdx](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.passState = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.writeIdx = writeIdx;
            data.useTemporal = useTemporal;
            data.current = pb.read(sceneColor, ResourceState::ShaderResource);
            data.history = pb.read(historyRead, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            if (useTemporal)
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            data.output = pb.write(output, ResourceState::RenderTarget);
            pb.sideEffects(); // history copy after resolve
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* cur = graph.GetPhysicalTexture(data.current);
            auto* hist = graph.GetPhysicalTexture(data.history);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !cur || !hist || !depthTex || !out || !data.passState)
                return;

            TAAPassState& st = *data.passState;
            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("taa", ".ps");
            if (!vsR || !psR || !st.pipeline || !st.layout)
                return;

            nvrhi::ITexture* motionTex = nullptr;
            if (data.useTemporal && data.motion.is_valid())
                motionTex = graph.GetPhysicalTexture(data.motion);
            if (!motionTex)
            {
                // Zero motion → treat as first frame (history unused effectively via high blend)
                static nvrhi::TextureHandle s_zeroMotion;
                if (!s_zeroMotion)
                {
                    nvrhi::TextureDesc td;
                    td.width = 1;
                    td.height = 1;
                    td.format = nvrhi::Format::RG16_FLOAT;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    td.debugName = "DummyZeroMotion";
                    s_zeroMotion = nv->createTexture(td);
                    float zeros[2] = {0.f, 0.f};
                    cmd->writeTexture(s_zeroMotion, 0, 0, zeros, sizeof(zeros));
                }
                motionTex = s_zeroMotion;
            }

            auto* cb = cache.GetOrCreateVolatileCB("TAA", "TAAParams_v3", sizeof(TAAParamsCB), data.device);
            TAAParamsCB params{};
            params.screenSizeX = float(data.width);
            params.screenSizeY = float(data.height);
            params.blendAlpha = data.useTemporal ? 0.38f : 1.0f;
            params.sharpness = ps_r_taa_sharpness;
            params.jitterX = g_taa_jitter_px;
            params.jitterY = g_taa_jitter_py;
            params.prevJitterX = g_taa_jitter_prev_px;
            params.prevJitterY = g_taa_jitter_prev_py;
            if (cb)
                cmd->writeBuffer(cb, &params, sizeof(params));

            BindingSetBuilder bsb(*vsR, *psR, nv, "TAA");
            if (cb)
                bsb.ConstantBuffer("TAAParams", cb);
            bsb.Texture("g_Current", cur)
                .Texture("g_History", hist)
                .Texture("g_Motion", motionTex)
                .Texture("g_Depth", depthTex);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), st.layout, nv);
            if (!set)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("TAA", fbDesc, nv);

            nvrhi::GraphicsState gs;
            gs.pipeline = st.pipeline;
            gs.framebuffer = fb;
            gs.bindings = {set};
            gs.viewport.addViewportAndScissorRect(
                nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));

            // Persist resolved frame into history[writeIdx]
            if (st.history[data.writeIdx])
            {
                cmd->copyTexture(
                    st.history[data.writeIdx], nvrhi::TextureSlice(),
                    out, nvrhi::TextureSlice());
                st.historyIndex = data.writeIdx;
                st.hasHistory = true;
            }
        });

    return passData.output;
}

} // namespace xray::render::fg::passes
