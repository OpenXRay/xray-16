#include "stdafx.h"
#include "PostProcessPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/r4_rendertarget.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/ShadersExternalData.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct alignas(16) PostProcessCB
{
    Fvector4 dual_r0;
    Fvector4 dual_r1;
    Fvector4 dual_l0;
    Fvector4 dual_l1;
    Fvector4 noise0;
    Fvector4 noise1;
    Fvector4 color;
    Fvector4 gray;
    Fvector4 brightness;
    Fvector4 colormap;
    Fvector4 mode;
};

constexpr u32 kPostProcessPipeVersion = 4;

void EnsurePPPipeline(nvrhi::IDevice* nv, PostProcessPassState& st, nvrhi::Format outFmt)
{
    if (!nv)
        return;
    if (st.initialized && st.pipeline && st.pipelineFormat == outFmt && st.pipeVersion == kPostProcessPipeVersion)
        return;
    st.layout = nullptr;
    st.pipeline = nullptr;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("fullscreen");
    auto ps = loader->LoadPixelShader("postprocess");
    if (!vs.handle || !ps.handle)
    {
        st.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    if (!st.layout)
        st.layout = cache.GetOrCreateBindingLayoutFromReflection(
            "PostProcess_v3", *vs.reflection, *ps.reflection, nv);
    if (st.layout)
    {
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(ps.handle);
        desc.addBindingLayout(st.layout);
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        nvrhi::FramebufferInfoEx fb;
        fb.addColorFormat(outFmt);
        char pipeName[64];
        xr_sprintf(pipeName, "PostProcess_v3_fmt%u", (u32)outFmt);
        st.pipeline = cache.GetOrCreatePipeline(pipeName, desc, fb, nv);
        st.pipelineFormat = outFmt;
    }

    if (!st.noisePlaceholder)
    {
        nvrhi::TextureDesc td;
        td.width = 1;
        td.height = 1;
        td.format = nvrhi::Format::RGBA8_UNORM;
        td.debugName = "PP_NoisePlaceholder";
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        st.noisePlaceholder = nv->createTexture(td);
        nvrhi::CommandListHandle cmd = nv->createCommandList();
        cmd->open();
        u32 gray = 0xFF808080;
        cmd->writeTexture(st.noisePlaceholder, 0, 0, &gray, sizeof(gray));
        cmd->close();
        nv->executeCommandList(cmd);
    }

    st.initialized = true;
    st.pipeVersion = kPostProcessPipeVersion;
}
}

VirtualResourceHandle setupPostProcessPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle ldrInput,
    VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CRenderTarget* target,
    PostProcessPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !ldrInput.is_valid() || !target)
        return ldrInput;

    bool needNV = false;
    if (g_pGamePersistent && g_pGamePersistent->m_pGShaderConstants)
        needNV = g_pGamePersistent->m_pGShaderConstants->m_blender_mode.x > 0.5f;
    if (!target->u_need_PP() && !needNV)
        return ldrInput;

    nvrhi::Format outFmt = nvrhi::Format::RGBA8_UNORM;
    if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
        outFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;

    EnsurePPPipeline(device->GetNVRHIDevice(), state, outFmt);
    if (!state.pipeline || !state.layout)
        return ldrInput;

    const bool hasOutputTarget = outputTarget.is_valid();

    struct PassData
    {
        VirtualResourceHandle input, output;
        u32 width, height;
        PostProcessPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        CRenderTarget* target = nullptr;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "PostProcess",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.target = target;
            data.width = width;
            data.height = height;
            data.input = pb.read(ldrInput, ResourceState::ShaderResource);

            if (hasOutputTarget)
            {
                data.output = pb.write(outputTarget, ResourceState::RenderTarget);
            }
            else
            {
                ResourceDesc td;
                td.type = ResourceDesc::Type::Texture2D;
                td.width = width;
                td.height = height;
                td.format = outFmt;
                td.isRenderTarget = true;
                td.isTransient = true;
                td.debugName = "rt_PostProcess";
                data.output = pb.createTexture("rt_PostProcess", td);
            }
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* in = graph.GetPhysicalTexture(data.input);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !in || !out || !data.st || !data.target)
                return;

            nvrhi::Format fmt = out->getDesc().format;
            EnsurePPPipeline(nv, *data.st, fmt);
            if (!data.st->pipeline || !data.st->layout)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("postprocess", ".ps");
            if (!vsR || !psR)
                return;

            Fvector2 n0, n1, r0, r1, l0, l1;
            data.target->u_calc_tc_duality_ss(r0, r1, l0, l1);
            data.target->u_calc_tc_noise(n0, n1);

            int gblend = clampr(iFloor((1.f - data.target->get_gray()) * 255.f), 0, 255);
            int nblend = clampr(iFloor((1.f - data.target->get_noise()) * 255.f), 0, 255);
            u32 p_color = subst_alpha(data.target->get_color_base(), nblend);
            u32 p_gray = subst_alpha(data.target->get_color_gray(), gblend);
            const Fvector& bright = data.target->get_color_add();

            PostProcessCB cb{};
            cb.dual_r0.set(r0.x, r0.y, 0.f, 0.f);
            cb.dual_r1.set(r1.x, r1.y, 0.f, 0.f);
            cb.dual_l0.set(l0.x, l0.y, 0.f, 0.f);
            cb.dual_l1.set(l1.x, l1.y, 0.f, 0.f);
            cb.noise0.set(n0.x, n0.y, 0.f, 0.f);
            cb.noise1.set(n1.x, n1.y, 0.f, 0.f);
            cb.color.set(
                float(color_get_R(p_color)) / 255.f,
                float(color_get_G(p_color)) / 255.f,
                float(color_get_B(p_color)) / 255.f,
                float(color_get_A(p_color)) / 255.f);
            cb.gray.set(
                float(color_get_R(p_gray)) / 255.f,
                float(color_get_G(p_gray)) / 255.f,
                float(color_get_B(p_gray)) / 255.f,
                float(color_get_A(p_gray)) / 255.f);
            cb.brightness.set(bright.x, bright.y, bright.z, 0.f);
            const float cmWanted = data.target->get_cm_influence();
            nvrhi::ITexture* grad0 = data.target->get_cm_texture(0);
            nvrhi::ITexture* grad1 = data.target->get_cm_texture(1);
            const float cmInf = (grad0 && cmWanted > 0.001f) ? cmWanted : 0.f;
            cb.colormap.set(
                cmInf,
                data.target->get_cm_interpolate(),
                cmWanted > 0.001f ? 1.f : 0.f,
                0.f);

            float blenderMode = 0.f;
            float nvIntensity = 1.f;
            if (g_pGamePersistent && g_pGamePersistent->m_pGShaderConstants)
            {
                blenderMode = g_pGamePersistent->m_pGShaderConstants->m_blender_mode.x;
                nvIntensity = g_pGamePersistent->m_pGShaderConstants->hud_params.z;
            }
            cb.mode.set(blenderMode, nvIntensity, 0.f, 0.f);

            auto* pcb = cache.GetOrCreateVolatileCB("PostProcess", "PostProcessParams", sizeof(PostProcessCB), data.device);
            if (pcb)
                cmd->writeBuffer(pcb, &cb, sizeof(cb));

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmd->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            nvrhi::ITexture* noiseTex = data.st->noisePlaceholder.Get();
            if (!data.st->noiseTexture)
            {
                auto* resMgr = data.device->GetFGResourceManager();
                auto* texMgr = resMgr ? resMgr->GetTextureManager() : nullptr;
                if (texMgr)
                {
                    auto handle = texMgr->LoadTexture("fx\\fx_noise2");
                    data.st->noiseTexture = texMgr->GetNVRHITexture(handle);
                }
            }
            if (data.st->noiseTexture)
                noiseTex = data.st->noiseTexture;

            if (!grad0)
                grad0 = data.st->noisePlaceholder.Get();
            if (!grad1)
                grad1 = grad0 ? grad0 : data.st->noisePlaceholder.Get();

            BindingSetBuilder bsb(*vsR, *psR, nv, "PostProcess");
            if (pcb)
                bsb.ConstantBuffer("PostProcessParams", pcb);
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.Texture("s_base0", in);
            bsb.Texture("s_base1", in);
            bsb.Texture("s_noise", noiseTex);
            if (grad0)
                bsb.Texture("s_grad0", grad0);
            if (grad1)
                bsb.Texture("s_grad1", grad1);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->layout, nv);
            if (!set)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("PostProcess", fbDesc, nv);
            if (!fb)
                return;

            nvrhi::GraphicsState gs;
            gs.pipeline = data.st->pipeline;
            gs.framebuffer = fb;
            gs.bindings = {set};
            gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    return pd.output;
}

}
