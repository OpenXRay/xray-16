#include "stdafx.h"
#include "OverlayPaintPassSetup.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Decals/OverlayManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

struct alignas(16) OverlayPaintCB {
    Fvector2 hitUV;
    float radius;
    float opacity;
    Fvector4 tintColor;
    u32 overlayWidth;
    u32 overlayHeight;
    u32 flags;
    u32 pad;
};

void InitializeOverlayPaintResources(ng::RenderDevice* device, OverlayPaintPassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        state.initialized = true;
        return;
    }

    auto csResult = RImplementation.m_shaderLoader->LoadComputeShader("overlay_paint");

    if (!csResult.handle) {
        Msg("! [OverlayPaintPass] overlay_paint.cs not found");
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("OverlayPaint", *csResult.reflection, nvDevice);

    if (state.layout) {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = csResult.handle;
        pipeDesc.bindingLayouts = { state.layout };
        state.pipeline = cache.GetOrCreateComputePipeline("OverlayPaint", pipeDesc, nvDevice);
    }

    state.enabled = state.pipeline && state.layout;
    state.initialized = true;

    Msg("* [OverlayPaintPass] Initialized (enabled=%s)", state.enabled ? "yes" : "no");
}

void setupOverlayPaintPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    decals::OverlayManager* overlayMgr,
    OverlayPaintPassState& state)
{
    if (!overlayMgr)
        return;

    const auto& paints = overlayMgr->GetPendingPaints();
    if (paints.empty())
        return;

    InitializeOverlayPaintResources(device, state);
    if (!state.enabled)
        return;

    struct OverlayPaintPassData {
        ng::RenderDevice* device;
        decals::OverlayManager* overlayMgr;
        xr_vector<decals::PaintCommand> commands;
        OverlayPaintPassState* passState;
    };

    fg.addCallbackPass<OverlayPaintPassData>(
        "OverlayPaint",

        [&](FrameGraph& builder, PassHandle passHandle, OverlayPaintPassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            pb.sideEffects();

            data.device = device;
            data.overlayMgr = overlayMgr;
            data.commands = paints;
            data.passState = &state;
            overlayMgr->ClearPendingPaints();
        },

        [](const OverlayPaintPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            auto& cache = GetPassResourceCache();
            auto* ps = data.passState;

            auto cbHandle = cache.GetOrCreateVolatileCB(
                "OverlayPaint", "PaintCB", sizeof(OverlayPaintCB), 64, nvDevice);

            for (const auto& cmd : data.commands) {
                auto& overlay = data.overlayMgr->GetOrCreateOverlay(cmd.target);

                if (overlay.dirty) {
                    cmdList->clearTextureFloat(overlay.colorOverlay, nvrhi::AllSubresources, nvrhi::Color(0));
                    overlay.dirty = false;
                }

                OverlayPaintCB cb;
                cb.hitUV = cmd.hitUV;
                cb.radius = cmd.uvRadius;
                cb.opacity = 1.0f;
                cb.tintColor = cmd.tintColor;
                cb.overlayWidth = overlay.resolution;
                cb.overlayHeight = overlay.resolution;
                cb.flags = cmd.flags;
                cb.pad = 0;

                cmdList->writeBuffer(cbHandle, &cb, sizeof(cb));

                auto* paintRefl = RImplementation.m_shaderLoader->GetCachedReflection("overlay_paint", ".cs");
                BindingSetBuilder bsb(*paintRefl, nvDevice, "OverlayPaint");
                bsb.ConstantBuffer("PaintParams", cbHandle)
                   .TextureUAV("g_Overlay", overlay.colorOverlay);
                auto bindDesc = bsb.Build();
                auto bindSet = nvDevice->createBindingSet(bindDesc, ps->layout);

                ctx->SetComputePipeline(ps->pipeline.Get());
                ctx->SetComputeBindingSet(0, bindSet.Get());
                ctx->Dispatch(
                    (overlay.resolution + 7) / 8,
                    (overlay.resolution + 7) / 8,
                    1
                );
            }

        }
    );
}

} // namespace xray::render::RENDER_NAMESPACE::passes
