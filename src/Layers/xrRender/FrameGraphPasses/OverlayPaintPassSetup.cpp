#include "stdafx.h"
#include "OverlayPaintPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Decals/OverlayManager.h"

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

    ref_cs paintCS;
    paintCS.create("overlay_paint");

    bool shaderOK = paintCS && paintCS->nvrhiShader;
    if (!shaderOK) {
        Msg("! [OverlayPaintPass] overlay_paint.cs not found");
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
        nvrhi::BindingLayoutItem::Texture_UAV(0)
    };
    state.layout = cache.GetOrCreateBindingLayout("OverlayPaint", layoutDesc, nvDevice);

    if (state.layout) {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = paintCS->nvrhiShader;
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

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, cbHandle),
                    nvrhi::BindingSetItem::Texture_UAV(0, overlay.colorOverlay)
                };
                auto bindSet = nvDevice->createBindingSet(bindDesc, ps->layout);

                ctx->SetComputePipeline(ps->pipeline.Get());
                ctx->SetComputeBindingSet(0, bindSet.Get());
                ctx->Dispatch(
                    (overlay.resolution + 7) / 8,
                    (overlay.resolution + 7) / 8,
                    1
                );
            }

            data.overlayMgr->ClearPendingPaints();
        }
    );
}

} // namespace xray::render::RENDER_NAMESPACE::passes
