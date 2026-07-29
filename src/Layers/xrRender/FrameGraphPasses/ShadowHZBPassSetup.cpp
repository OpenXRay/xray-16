#include "stdafx.h"
#include "ShadowHZBPassSetup.h"
#include "HiZBuildPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include <nvrhi/utils.h>

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct ShadowHZBCB
{
    u32 outputWidth;
    u32 outputHeight;
    u32 inputMipLevel;
    u32 isFirstMip;
};

void EnsureHZBTextures(nvrhi::IDevice* nv, ShadowHZBPassState& st, ShadowPassState& shadow)
{
    for (u32 c = 0; c < kCSMCascadeCount; ++c)
    {
        const u32 res = shadow.cascadeResolution[c] ? shadow.cascadeResolution[c] : GetCSMCascadeResolution(c);
        const u32 baseW = std::max(1u, res / 2);
        const u32 baseH = std::max(1u, res / 2);
        const u32 mips = CalculateHiZMipLevels(baseW, baseH);
        if (st.hzb[c] && st.cascadeRes[c] == res && st.mipLevels[c] == mips)
            continue;

        nvrhi::TextureDesc td;
        td.width = baseW;
        td.height = baseH;
        td.format = nvrhi::Format::R32_FLOAT;
        td.mipLevels = mips;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "rt_ShadowHZB_c%u", c);
        td.debugName = nameBuf;
        st.hzb[c] = nv->createTexture(td);
        st.cascadeRes[c] = res;
        st.mipLevels[c] = st.hzb[c] ? mips : 0;
        if (st.hzb[c])
        {
            nvrhi::CommandListHandle cmd = nv->createCommandList();
            cmd->open();
            cmd->clearTextureFloat(st.hzb[c], nvrhi::AllSubresources, nvrhi::Color(1.f));
            cmd->close();
            nv->executeCommandList(cmd);
        }
    }
}
} // namespace

ShadowHZBOutput setupShadowHZBPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    ShadowPassState& shadowState,
    ShadowHZBPassState& state,
    VirtualResourceHandle shadowMapHandle)
{
    ShadowHZBOutput out{};
    if (!device || ps_r_shadow_hzb == 0 || !shadowState.enabled)
        return out;

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (!nv)
        return out;

    if (!state.initialized)
    {
        auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader("shadow_hzb_build");
        if (!cs.handle || !cs.reflection)
        {
            Msg("! [ShadowHZB] shadow_hzb_build.cs missing");
            state.initialized = true;
            state.computeEnabled = false;
            return out;
        }
        auto& cache = GetPassResourceCache();
        state.layout = cache.GetOrCreateBindingLayoutFromReflection(
            "ShadowHZBPass", *cs.reflection, nv);
        if (state.layout)
        {
            nvrhi::ComputePipelineDesc pd;
            pd.CS = cs.handle;
            pd.bindingLayouts = {state.layout};
            state.pipeline = cache.GetOrCreateComputePipeline("ShadowHZBPass", pd, nv);
        }
        state.computeEnabled = (state.pipeline != nullptr);
        state.initialized = true;
        if (state.computeEnabled)
            Msg("* [ShadowHZB] pipeline ready");
        cs.reflection = nullptr;
    }

    if (!state.computeEnabled)
        return out;

    EnsureHZBTextures(nv, state, shadowState);
    for (u32 c = 0; c < kCSMCascadeCount; ++c)
    {
        if (!state.hzb[c] || !shadowState.shadowCascades[c])
            return out;
    }

    VirtualResourceHandle hzbHandles[kCSMCascadeCount];
    for (u32 c = 0; c < kCSMCascadeCount; ++c)
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2D;
        desc.width = state.hzb[c]->getDesc().width;
        desc.height = state.hzb[c]->getDesc().height;
        desc.format = nvrhi::Format::R32_FLOAT;
        desc.mipLevels = state.mipLevels[c];
        desc.isUAV = true;
        desc.isImported = true;
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "rt_ShadowHZB_c%u", c);
        desc.debugName = nameBuf;
        hzbHandles[c] = fg.ImportTexture(nameBuf, state.hzb[c], desc);
    }

    struct PassData
    {
        ShadowHZBPassState* st = nullptr;
        ShadowPassState* shadow = nullptr;
        fg::RenderDevice* device = nullptr;
        VirtualResourceHandle hzbHandles[kCSMCascadeCount];
    };

    fg.addCallbackPass<PassData>(
        "ShadowHZB",
        [&, hzbHandles, shadowMapHandle](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.st = &state;
            data.shadow = &shadowState;
            data.device = device;
            if (shadowMapHandle.is_valid())
                pb.read(shadowMapHandle, ResourceState::ShaderResource);
            for (u32 c = 0; c < kCSMCascadeCount; ++c)
            {
                data.hzbHandles[c] = pb.write(hzbHandles[c], ResourceState::UnorderedAccess);
            }
        },
        [](const PassData& data, const FrameGraph&, fg::RenderContext* ctx) {
            if (!data.st || !data.shadow || !data.st->pipeline)
                return;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = data.device->GetNVRHIDevice();
            if (!cmd || !nv)
                return;

            auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("shadow_hzb_build", ".cs");
            if (!refl)
                return;

            auto cb = GetPassResourceCache().GetOrCreateVolatileCB(
                "ShadowHZB", "HZBParams", sizeof(ShadowHZBCB), data.device, 256);
            if (!cb)
                return;

            constexpr u32 kMaxShadowHZBMips = 7;
            for (u32 c = 0; c < kCSMCascadeCount; ++c)
            {
                nvrhi::ITexture* src = data.shadow->shadowCascades[c];
                nvrhi::ITexture* hzb = data.st->hzb[c];
                if (!src || !hzb)
                    continue;

                const u32 baseW = hzb->getDesc().width;
                const u32 baseH = hzb->getDesc().height;
                const u32 mips = std::min(data.st->mipLevels[c], kMaxShadowHZBMips);

                for (u32 mip = 0; mip < mips; ++mip)
                {
                    const u32 outW = std::max(1u, baseW >> mip);
                    const u32 outH = std::max(1u, baseH >> mip);
                    ShadowHZBCB pcb{};
                    pcb.outputWidth = outW;
                    pcb.outputHeight = outH;
                    pcb.inputMipLevel = 0;
                    pcb.isFirstMip = (mip == 0) ? 1u : 0u;
                    cmd->writeBuffer(cb, &pcb, sizeof(pcb));

                    nvrhi::TextureSubresourceSet inSub;
                    inSub.baseMipLevel = (mip > 0) ? mip - 1 : 0;
                    inSub.numMipLevels = 1;
                    nvrhi::TextureSubresourceSet outSub;
                    outSub.baseMipLevel = mip;
                    outSub.numMipLevels = 1;

                    BindingSetBuilder bsb(*refl, nv, "ShadowHZB");
                    if (mip == 0)
                        bsb.Texture("g_input_depth", src);
                    else
                        bsb.Texture("g_input_depth", hzb, nvrhi::Format::R32_FLOAT, inSub);
                    bsb.TextureUAV("g_output_hiz", hzb, nvrhi::Format::R32_FLOAT, outSub);
                    bsb.ConstantBuffer("ShadowHZBParams", cb);
                    auto set = GetPassResourceCache().GetOrCreateBindingSet(
                        bsb.Build(), data.st->layout, nv);
                    if (!set)
                        continue;

                    nvrhi::ComputeState cs;
                    cs.pipeline = data.st->pipeline;
                    cs.bindings = {set};
                    cmd->setComputeState(cs);
                    cmd->dispatch((outW + 7) / 8, (outH + 7) / 8, 1);
                    if (mip + 1 < mips)
                        nvrhi::utils::TextureUavBarrier(cmd, hzb);
                }
                cmd->setTextureState(hzb, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }
        });

    out.valid = true;
    for (u32 c = 0; c < kCSMCascadeCount; ++c)
    {
        out.hzb[c] = state.hzb[c];
        out.handles[c] = hzbHandles[c];
        out.mipLevels[c] = state.mipLevels[c];
    }
    return out;
}

} // namespace xray::render::fg::passes
