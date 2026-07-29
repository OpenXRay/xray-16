#include "stdafx.h"
#include "ShadowMaskPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "Layers/xrRender/xrRender_console.h"
#include <nvrhi/utils.h>

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct ShadowMaskCB
{
    u32 maskW, maskH;
    u32 fullW, fullH;
};

void EnsureMask(nvrhi::IDevice* nv, ShadowMaskPassState& st, u32 w, u32 h)
{
    if (st.mask && st.width == w && st.height == h)
        return;
    nvrhi::TextureDesc td;
    td.width = w;
    td.height = h;
    td.format = nvrhi::Format::RGBA8_UNORM;
    td.isUAV = true;
    td.isShaderResource = true;
    td.initialState = nvrhi::ResourceStates::ShaderResource;
    td.keepInitialState = true;
    td.debugName = "rt_ShadowMask";
    st.mask = nv->createTexture(td);
    st.width = st.mask ? w : 0;
    st.height = st.mask ? h : 0;
    if (st.mask)
    {
        nvrhi::CommandListHandle cmd = nv->createCommandList();
        cmd->open();
        cmd->clearTextureFloat(st.mask, nvrhi::AllSubresources, nvrhi::Color(1.f, 1.f, 1.f, 1.f));
        cmd->close();
        nv->executeCommandList(cmd);
    }
}
} // namespace

ShadowMaskOutput setupShadowMaskPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthInput,
    u32 fullWidth,
    u32 fullHeight,
    const BindlessForwardConfig& bindlessConfig,
    const ShadowHZBOutput& hzb,
    ShadowMaskPassState& state)
{
    ShadowMaskOutput out{};
    if (!device || ps_r_shadow_mask == 0 || !depthInput.is_valid())
        return out;

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (!nv)
        return out;

    if (!state.initialized)
    {
        auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader("shadow_mask");
        if (!cs.handle || !cs.reflection)
        {
            Msg("! [ShadowMask] shadow_mask.cs missing");
            state.initialized = true;
            state.computeEnabled = false;
            return out;
        }
        auto& cache = GetPassResourceCache();
        state.layout = cache.GetOrCreateBindingLayoutFromReflection(
            "ShadowMaskPass", *cs.reflection, nv);
        if (state.layout)
        {
            nvrhi::ComputePipelineDesc pd;
            pd.CS = cs.handle;
            pd.bindingLayouts = {state.layout};
            state.pipeline = cache.GetOrCreateComputePipeline("ShadowMaskPass", pd, nv);
        }
        state.computeEnabled = (state.pipeline != nullptr);
        state.initialized = true;
        if (state.computeEnabled)
            Msg("* [ShadowMask] pipeline ready");
        cs.reflection = nullptr;
    }
    if (!state.computeEnabled)
        return out;

    const u32 maskW = std::max(1u, fullWidth);
    const u32 maskH = std::max(1u, fullHeight);
    EnsureMask(nv, state, maskW, maskH);
    if (!state.mask)
        return out;

    {
        static bool s_once = false;
        if (!s_once)
        {
            Msg("* [ShadowMask] full-res %ux%u", maskW, maskH);
            s_once = true;
        }
    }

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = maskW;
    desc.height = maskH;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.isUAV = true;
    desc.isImported = true;
    desc.debugName = "rt_ShadowMask";
    auto maskHandle = fg.ImportTexture("rt_ShadowMask", state.mask, desc);

    struct PassData
    {
        VirtualResourceHandle depth;
        VirtualResourceHandle mask;
        ShadowMaskPassState* st = nullptr;
        BindlessForwardConfig config;
        ShadowHZBOutput hzb;
        fg::RenderDevice* device = nullptr;
        u32 fullW = 0, fullH = 0;
        u32 maskW = 0, maskH = 0;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "ShadowMask",
        [&, maskHandle, depthInput, fullWidth, fullHeight, maskW, maskH, bindlessConfig, hzb](
            FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.depth = pb.read(depthInput, ResourceState::ShaderResource);
            data.mask = pb.write(maskHandle, ResourceState::UnorderedAccess);
            if (hzb.valid)
            {
                for (u32 i = 0; i < 3; ++i)
                {
                    if (hzb.handles[i].is_valid())
                        pb.read(hzb.handles[i], ResourceState::ShaderResource);
                }
            }
            data.st = &state;
            data.config = bindlessConfig;
            data.hzb = hzb;
            data.device = device;
            data.fullW = fullWidth;
            data.fullH = fullHeight;
            data.maskW = maskW;
            data.maskH = maskH;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            if (!data.st || !data.st->pipeline)
                return;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* maskTex = graph.GetPhysicalTexture(data.mask);
            if (!cmd || !nv || !depthTex || !maskTex)
                return;

            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("shadow_mask", ".cs");
            if (!refl)
                return;

            auto& cache = GetPassResourceCache();
            auto* staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "ShadowMask", "StaticGlobals", sizeof(StaticGlobals), data.device, 8);
            if (!staticGlobalsCB)
                return;
            StaticGlobals globals = BuildStaticGlobals();
            cmd->writeBuffer(staticGlobalsCB, &globals, sizeof(globals));

            auto paramsCB = cache.GetOrCreateVolatileCB(
                "ShadowMask", "Params", sizeof(ShadowMaskCB), data.device, 8);
            if (!paramsCB)
                return;
            ShadowMaskCB pcb{data.maskW, data.maskH, data.fullW, data.fullH};
            cmd->writeBuffer(paramsCB, &pcb, sizeof(pcb));

            auto& clm = ClusteredLightManager::Instance();
            nvrhi::ITexture* dummy2D = cache.GetDummyShadowMap2D(nv);
            nvrhi::ITexture* dummyHZB = cache.GetDummyContactDepth(nv);
            nvrhi::ITexture* contactHist = data.config.contactHistory
                ? data.config.contactHistory
                : cache.GetDummyContactHistory(nv);
            nvrhi::ITexture* localAtlas = data.config.localShadowAtlas
                ? data.config.localShadowAtlas
                : cache.GetDummyShadowMap(nv);

            BindingSetBuilder bsb(*refl, nv, "ShadowMask");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.ConstantBuffer("ShadowMaskParams", paramsCB);
            BindBindlessMaterialTables(bsb);
            bsb.Texture("g_SceneDepth", depthTex);
            bsb.TextureUAV("g_OutMask", maskTex);
            static const char* kSm[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
            static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
            for (u32 i = 0; i < 3; ++i)
            {
                nvrhi::ITexture* sm = data.config.shadowCascades[i]
                    ? data.config.shadowCascades[i]
                    : dummy2D;
                bsb.Texture(kSm[i], sm);
                nvrhi::ITexture* hz = (data.hzb.valid && data.hzb.hzb[i])
                    ? data.hzb.hzb[i]
                    : dummyHZB;
                bsb.Texture(kHzb[i], hz);
            }
            if (contactHist)
                bsb.Texture("g_ContactHistory", contactHist);
            if (localAtlas)
                bsb.Texture("g_LocalShadowAtlas", localAtlas);
            nvrhi::ITexture* localEsm = data.config.localShadowESM
                ? data.config.localShadowESM
                : cache.GetDummyLocalShadowESM(nv);
            if (localEsm)
                bsb.Texture("g_LocalShadowESM", localEsm);
            if (clm.GetLightDataBuffer())
                bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            if (clm.GetShadowDataBuffer())
                bsb.BufferSRV("g_ShadowData", clm.GetShadowDataBuffer());
            if (clm.GetClusterGridBuffer())
                bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            if (clm.GetLightIndexListBuffer())
                bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());

            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->layout, nv);
            if (!set)
                return;

            nvrhi::ComputeState cs;
            cs.pipeline = data.st->pipeline;
            cs.bindings = {set};
            cmd->setComputeState(cs);
            cmd->dispatch((data.maskW + 7) / 8, (data.maskH + 7) / 8, 1);
            cmd->setTextureState(maskTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
        });

    out.mask = state.mask;
    out.handle = passData.mask;
    out.valid = true;
    return out;
}

} // namespace xray::render::fg::passes
