#include "stdafx.h"
#include "ContactShadowsPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API int ps_r_contact_shadows;
extern ENGINE_API float ps_r_contact_shadows_length;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
void EnsureHistory(nvrhi::IDevice* nv, ContactShadowsPassState& st, u32 w, u32 h)
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
        td.debugName = (i == 0) ? "rt_ContactHist0" : "rt_ContactHist1";
        st.history[i] = nv->createTexture(td);
    }
    st.historyW = w;
    st.historyH = h;
    st.hasHistory = false;
    st.historyIndex = 0;
    if (st.history[0] && st.history[1])
    {
        nvrhi::CommandListHandle cmd = nv->createCommandList();
        cmd->open();
        cmd->clearTextureFloat(st.history[0], nvrhi::AllSubresources, nvrhi::Color(1.f, 0.f, 0.f, 0.f));
        cmd->clearTextureFloat(st.history[1], nvrhi::AllSubresources, nvrhi::Color(1.f, 0.f, 0.f, 0.f));
        cmd->close();
        nv->executeCommandList(cmd);
    }
}

void Init(nvrhi::IDevice* nv, ContactShadowsPassState& st)
{
    if (st.initialized || !nv)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader) { st.initialized = true; return; }

    auto vs = loader->LoadVertexShader("fullscreen");
    auto psMarch = loader->LoadPixelShader("contact_shadows");
    if (!vs.handle || !psMarch.handle)
    {
        st.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();
    st.marchLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "ContactMarch_v6", *vs.reflection, *psMarch.reflection, nv);
    if (st.marchLayout)
    {
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(psMarch.handle);
        desc.addBindingLayout(st.marchLayout);
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
        st.marchPipeline = cache.GetOrCreatePipeline("ContactMarch_v6", desc, fb, nv);
    }
    st.initialized = true;
}
} // namespace

nvrhi::ITexture* GetContactShadowHistory(ContactShadowsPassState& state)
{
    if (!state.hasHistory || !state.history[state.historyIndex])
        return nullptr;
    return state.history[state.historyIndex].Get();
}

VirtualResourceHandle setupContactShadowsPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle worldPos,
    VirtualResourceHandle motionVectors,
    u32 width,
    u32 height,
    bool hasPrevFrame,
    ContactShadowsPassState& state)
{
    if (!ps_r_contact_shadows || !device || !device->GetNVRHIDevice() || !worldPos.is_valid())
        return sceneColor;

    Init(device->GetNVRHIDevice(), state);
    if (!state.marchPipeline || !state.marchLayout)
        return sceneColor;

    EnsureHistory(device->GetNVRHIDevice(), state, width, height);
    if (!state.history[0] || !state.history[1])
        return sceneColor;

    const u32 readIdx = state.historyIndex;
    const u32 writeIdx = 1u - readIdx;

    ResourceDesc marchDesc;
    marchDesc.type = ResourceDesc::Type::Texture2D;
    marchDesc.width = width;
    marchDesc.height = height;
    marchDesc.format = nvrhi::Format::RGBA16_FLOAT;
    marchDesc.isRenderTarget = true;
    marchDesc.isTransient = true;
    marchDesc.debugName = "rt_ContactMarch";
    auto marchRT = fg.CreateTexture("rt_ContactMarch", marchDesc);

    ResourceDesc histDesc;
    histDesc.type = ResourceDesc::Type::Texture2D;
    histDesc.width = width;
    histDesc.height = height;
    histDesc.format = nvrhi::Format::RGBA16_FLOAT;
    histDesc.isImported = true;
    histDesc.debugName = "rt_ContactHistRead";
    auto histRead = fg.ImportTexture("rt_ContactHistRead", state.history[readIdx].Get(), histDesc);

    const bool useTemporal = motionVectors.is_valid() && hasPrevFrame && state.hasHistory;

    struct PassData
    {
        VirtualResourceHandle depth, worldPos, motion, hist, march;
        ContactShadowsPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        u32 width = 0, height = 0, writeIdx = 0;
        bool useTemporal = false;
    };

    fg.addCallbackPass<PassData>(
        "ContactShadows",
        [&, marchRT, histRead, useTemporal, writeIdx](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.writeIdx = writeIdx;
            data.useTemporal = useTemporal;
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.worldPos = pb.read(worldPos, ResourceState::ShaderResource);
            data.hist = pb.read(histRead, ResourceState::ShaderResource);
            if (useTemporal)
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            data.march = pb.write(marchRT, ResourceState::RenderTarget);
            pb.sideEffects();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* worldPosTex = graph.GetPhysicalTexture(data.worldPos);
            auto* hist = graph.GetPhysicalTexture(data.hist);
            auto* march = graph.GetPhysicalTexture(data.march);
            if (!cmd || !nv || !depthTex || !worldPosTex || !hist || !march || !data.st)
                return;

            nvrhi::ITexture* motionTex = nullptr;
            if (data.useTemporal && data.motion.is_valid())
                motionTex = graph.GetPhysicalTexture(data.motion);
            if (!motionTex)
            {
                static nvrhi::TextureHandle s_zero;
                if (!s_zero)
                {
                    nvrhi::TextureDesc td;
                    td.width = 1;
                    td.height = 1;
                    td.format = nvrhi::Format::RG16_FLOAT;
                    td.initialState = nvrhi::ResourceStates::ShaderResource;
                    td.keepInitialState = true;
                    td.debugName = "DummyZeroMV_Contact";
                    s_zero = nv->createTexture(td);
                    float z[2] = {0, 0};
                    cmd->writeTexture(s_zero, 0, 0, z, sizeof(z));
                }
                motionTex = s_zero;
            }

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psMarchR = loader->GetCachedReflection("contact_shadows", ".ps");
            if (!vsR || !psMarchR)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            struct alignas(16) MarchCB
            {
                float screenX, screenY, rayLen, hasHistory;
            };
            MarchCB mcb{
                float(data.width), float(data.height),
                ps_r_contact_shadows_length,
                data.useTemporal ? 1.f : 0.f};
            auto* marchCB = cache.GetOrCreateVolatileCB("Contact", "MarchParams", sizeof(MarchCB), data.device);
            if (marchCB)
                cmd->writeBuffer(marchCB, &mcb, sizeof(mcb));

            BindingSetBuilder bsb(*vsR, *psMarchR, nv, "Contact.March");
            if (staticGlobalsCB)
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            if (marchCB)
                bsb.ConstantBuffer("ContactParams", marchCB);
            bsb.Texture("g_Depth", depthTex)
                .Texture("g_WorldPos", worldPosTex)
                .Texture("g_Motion", motionTex)
                .Texture("g_History", hist);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->marchLayout, nv);
            if (!set || !data.st->marchPipeline)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(march);
            auto fb = cache.GetOrCreateFramebuffer("ContactMarch", fbDesc, nv);
            nvrhi::GraphicsState gs;
            gs.pipeline = data.st->marchPipeline;
            gs.framebuffer = fb;
            gs.bindings = {set};
            if (GEnv.Backend && GEnv.Backend->GetBindlessDescriptorTable())
                gs.addBindingSet(GEnv.Backend->GetBindlessDescriptorTable());
            gs.viewport.addViewportAndScissorRect(
                nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));

            if (data.st->history[data.writeIdx])
            {
                cmd->copyTexture(
                    data.st->history[data.writeIdx], nvrhi::TextureSlice(),
                    march, nvrhi::TextureSlice());
                data.st->historyIndex = data.writeIdx;
                data.st->hasHistory = true;
            }
        });

    return sceneColor;
}

} // namespace
