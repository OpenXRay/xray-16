#include "stdafx.h"
#include "DlssFgPassSetup.h"
#include "StreamlineDLSS.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraphPasses/TAAPassSetup.h"

extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_upscale;

namespace xray::render::fg::passes {
using namespace framegraph;

namespace {

void DlssFgCopyMatrix(float dst[16], const Fmatrix& m)
{
    dst[0] = m._11; dst[1] = m._12; dst[2] = m._13; dst[3] = m._14;
    dst[4] = m._21; dst[5] = m._22; dst[6] = m._23; dst[7] = m._24;
    dst[8] = m._31; dst[9] = m._32; dst[10] = m._33; dst[11] = m._34;
    dst[12] = m._41; dst[13] = m._42; dst[14] = m._43; dst[15] = m._44;
}

}

void setupDlssFgPass(
    FrameGraph& fg,
    VirtualResourceHandle colorWithUI,
    VirtualResourceHandle hudlessColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle motionVectors,
    const Fmatrix& viewToClip,
    const Fmatrix& prevViewProj,
    const Fmatrix& currViewProj,
    u32 renderW,
    u32 renderH,
    u32 displayW,
    u32 displayH,
    bool reset)
{
    if (ps_r_upscale != 2 || !ps_r_dlss_fg || !Streamline_IsFGAvailable())
        return;
    if (!colorWithUI.is_valid() || !depth.is_valid() || !motionVectors.is_valid())
        return;

    struct PassData
    {
        VirtualResourceHandle color;
        VirtualResourceHandle hudless;
        VirtualResourceHandle depth;
        VirtualResourceHandle motion;
        Fmatrix viewToClip{};
        Fmatrix clipToView{};
        Fmatrix clipToPrev{};
        Fmatrix prevToClip{};
        u32 renderW = 0, renderH = 0;
        u32 displayW = 0, displayH = 0;
        bool reset = false;
        float jitterX = 0.f, jitterY = 0.f;
    };

    Fmatrix invCurr;
    invCurr.invert(currViewProj);
    Fmatrix clipToPrev;
    clipToPrev.mul(prevViewProj, invCurr);
    Fmatrix prevToClip;
    prevToClip.invert(clipToPrev);
    Fmatrix clipToView;
    clipToView.invert(viewToClip);

    fg.addCallbackPass<PassData>(
        "DLSS-FG",
        [&](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.color = pb.read(colorWithUI, ResourceState::ShaderResource);
            if (hudlessColor.is_valid())
                data.hudless = pb.read(hudlessColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            pb.sideEffects();
            data.viewToClip = viewToClip;
            data.clipToView = clipToView;
            data.clipToPrev = clipToPrev;
            data.prevToClip = prevToClip;
            data.renderW = renderW;
            data.renderH = renderH;
            data.displayW = displayW;
            data.displayH = displayH;
            data.reset = reset;
            data.jitterX = g_taa_jitter_px;
            data.jitterY = g_taa_jitter_py;
        },
        [](const PassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            if (!ctx)
                return;
            auto* color = fgGraph.GetPhysicalTexture(data.color);
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* mv = fgGraph.GetPhysicalTexture(data.motion);
            if (!color || !depthTex || !mv)
                return;

            DlssFgInputs in{};
            in.backbuffer = color;
            in.hudless = data.hudless.is_valid() ? fgGraph.GetPhysicalTexture(data.hudless) : nullptr;
            in.depth = depthTex;
            in.motionVectors = mv;
            in.displayWidth = data.displayW;
            in.displayHeight = data.displayH;
            in.renderWidth = data.renderW;
            in.renderHeight = data.renderH;
            in.jitterX = data.jitterX;
            in.jitterY = data.jitterY;
            in.reset = data.reset;
            DlssFgCopyMatrix(in.viewToClip, data.viewToClip);
            DlssFgCopyMatrix(in.clipToView, data.clipToView);
            DlssFgCopyMatrix(in.clipToPrevClip, data.clipToPrev);
            DlssFgCopyMatrix(in.prevClipToClip, data.prevToClip);
            in.cameraPos[0] = Device.vCameraPosition.x;
            in.cameraPos[1] = Device.vCameraPosition.y;
            in.cameraPos[2] = Device.vCameraPosition.z;
            in.cameraUp[0] = Device.vCameraTop.x;
            in.cameraUp[1] = Device.vCameraTop.y;
            in.cameraUp[2] = Device.vCameraTop.z;
            in.cameraFwd[0] = Device.vCameraDirection.x;
            in.cameraFwd[1] = Device.vCameraDirection.y;
            in.cameraFwd[2] = Device.vCameraDirection.z;
            Fvector right;
            right.crossproduct(Device.vCameraTop, Device.vCameraDirection);
            right.normalize_safe();
            in.cameraRight[0] = right.x;
            in.cameraRight[1] = right.y;
            in.cameraRight[2] = right.z;
            in.cameraNear = VIEWPORT_NEAR;
            in.cameraFar = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 600.f;
            in.cameraFOV = deg2rad(Device.fFOV);
            in.cameraAspect = (data.displayH > 0) ? ((float)data.displayW / (float)data.displayH) : 1.f;

            Streamline_EvaluateDLSSG(ctx->GetCommandList(), in);
        });
}

}
