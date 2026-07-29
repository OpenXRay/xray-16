#include "stdafx.h"
#include "CameraModelPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"

extern ECORE_API float ps_r2_mblur;
extern ENGINE_API int ps_r_upscale;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
bool UpscaleOwnsTemporal()
{
    return ps_r_upscale != 0;
}

void EnsureCameraPipelines(nvrhi::IDevice* nv, CameraModelPassState& st, nvrhi::Format outFmt)
{
    if (!nv)
        return;
    if (st.initialized && st.cameraPipeline && st.pipelineFormat == outFmt)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("fullscreen");
    auto camPs = loader->LoadPixelShader("camera_model");
    auto blurPs = loader->LoadPixelShader("motion_blur");
    if (!vs.handle || !camPs.handle)
    {
        st.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();
    st.cameraLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "CameraModel_v1", *vs.reflection, *camPs.reflection, nv);
    if (st.cameraLayout)
    {
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(camPs.handle);
        desc.addBindingLayout(st.cameraLayout);
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        nvrhi::FramebufferInfoEx fb;
        fb.addColorFormat(outFmt);
        char pipeName[64];
        xr_sprintf(pipeName, "CameraModel_v1_fmt%u", (u32)outFmt);
        st.cameraPipeline = cache.GetOrCreatePipeline(pipeName, desc, fb, nv);
    }

    if (blurPs.handle)
    {
        st.mblurLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "MotionBlur_v1", *vs.reflection, *blurPs.reflection, nv);
        if (st.mblurLayout)
        {
            nvrhi::GraphicsPipelineDesc desc;
            desc.setVertexShader(vs.handle);
            desc.setPixelShader(blurPs.handle);
            desc.addBindingLayout(st.mblurLayout);
            desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
            desc.renderState.blendState.targets[0].setBlendEnable(false);
            desc.renderState.depthStencilState.setDepthTestEnable(false);
            desc.renderState.depthStencilState.setDepthWriteEnable(false);
            desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
            nvrhi::FramebufferInfoEx fb;
            fb.addColorFormat(outFmt);
            char pipeName[64];
            xr_sprintf(pipeName, "MotionBlur_v1_fmt%u", (u32)outFmt);
            st.mblurPipeline = cache.GetOrCreatePipeline(pipeName, desc, fb, nv);
        }
    }

    st.pipelineFormat = outFmt;
    st.initialized = true;
}

float CameraDistortAmount()
{
    return (ps_r_camera && ps_r_camera_distort_enable) ? ps_r_camera_distort : 0.f;
}
float CameraCAAmount()
{
    return (ps_r_camera && ps_r_camera_ca_enable) ? ps_r_camera_ca : 0.f;
}
float CameraVignetteAmount()
{
    return (ps_r_camera && ps_r_camera_vignette_enable) ? ps_r_camera_vignette : 0.f;
}
float CameraGrainAmount()
{
    if (UpscaleOwnsTemporal())
        return 0.f;
    return (ps_r_camera && ps_r_camera_grain_enable) ? ps_r_camera_grain : 0.f;
}
float CameraMblurAmount()
{
    if (UpscaleOwnsTemporal())
        return 0.f;
    return (ps_r_camera && ps_r_camera_mblur_enable) ? ps_r2_mblur : 0.f;
}

bool CameraEffectsActive()
{
    if (!ps_r_camera)
        return false;
    return CameraDistortAmount() > 1e-4f
        || CameraCAAmount() > 1e-4f
        || CameraVignetteAmount() > 1e-4f
        || CameraGrainAmount() > 1e-4f
        || CameraMblurAmount() > 1e-4f;
}
} // namespace

VirtualResourceHandle setupCameraModelPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle ldrInput,
    VirtualResourceHandle motionVectors,
    VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CameraModelPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !ldrInput.is_valid())
        return ldrInput;
    if (!CameraEffectsActive())
        return ldrInput;

    nvrhi::Format outFmt = nvrhi::Format::RGBA8_UNORM;
    if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
        outFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;

    EnsureCameraPipelines(device->GetNVRHIDevice(), state, outFmt);
    if (!state.cameraPipeline || !state.cameraLayout)
        return ldrInput;

    const bool wantMblur = CameraMblurAmount() > 1e-4f && motionVectors.is_valid() && state.mblurPipeline && state.mblurLayout;

    VirtualResourceHandle cameraOut = outputTarget;
    if (!cameraOut.is_valid() || wantMblur)
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = outFmt;
        desc.isRenderTarget = true;
        desc.isTransient = true;
        desc.debugName = "rt_CameraModel";
        cameraOut = fg.CreateTexture("rt_CameraModel", desc);
    }

    struct CamData
    {
        VirtualResourceHandle input, output;
        u32 width, height;
        CameraModelPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
    };

    auto& camPass = fg.addCallbackPass<CamData>(
        "CameraModel",
        [&](FrameGraph& b, PassHandle ph, CamData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.input = pb.read(ldrInput, ResourceState::ShaderResource);
            data.output = pb.write(cameraOut, ResourceState::RenderTarget);
        },
        [](const CamData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* in = graph.GetPhysicalTexture(data.input);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !in || !out || !data.st)
                return;

            EnsureCameraPipelines(nv, *data.st, out->getDesc().format);
            if (!data.st->cameraPipeline || !data.st->cameraLayout)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("camera_model", ".ps");
            if (!vsR || !psR)
                return;

            struct alignas(16) PCB
            {
                float sx, sy, distort, ca;
                float vignette, grain, time, pad;
            };
            auto* cb = cache.GetOrCreateVolatileCB("CameraModel", "CameraModelParams", sizeof(PCB), data.device);
            PCB p{
                float(data.width), float(data.height),
                CameraDistortAmount(), CameraCAAmount(),
                CameraVignetteAmount(), CameraGrainAmount(),
                Device.fTimeGlobal, 0.f
            };
            if (cb)
                cmd->writeBuffer(cb, &p, sizeof(p));

            BindingSetBuilder bsb(*vsR, *psR, nv, "CameraModel");
            if (cb)
                bsb.ConstantBuffer("CameraModelParams", cb);
            bsb.Texture("g_Color", in);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->cameraLayout, nv);
            if (!set)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("CameraModel", fbDesc, nv);
            if (!fb)
                return;

            nvrhi::GraphicsState gs;
            gs.pipeline = data.st->cameraPipeline;
            gs.framebuffer = fb;
            gs.bindings = { set };
            gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    VirtualResourceHandle result = camPass.output;
    if (!wantMblur)
    {
        if (outputTarget.is_valid() && result != outputTarget)
        {
            // already writing to outputTarget when !wantMblur && output valid
        }
        return result;
    }

    VirtualResourceHandle mblurOut = outputTarget;
    if (!mblurOut.is_valid())
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = outFmt;
        desc.isRenderTarget = true;
        desc.isTransient = true;
        desc.debugName = "rt_MotionBlur";
        mblurOut = fg.CreateTexture("rt_MotionBlur", desc);
    }

    struct BlurData
    {
        VirtualResourceHandle input, motion, output;
        u32 width, height;
        CameraModelPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
    };

    auto& blurPass = fg.addCallbackPass<BlurData>(
        "MotionBlur",
        [&](FrameGraph& b, PassHandle ph, BlurData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.input = pb.read(result, ResourceState::ShaderResource);
            data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            data.output = pb.write(mblurOut, ResourceState::RenderTarget);
        },
        [](const BlurData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* in = graph.GetPhysicalTexture(data.input);
            auto* motion = graph.GetPhysicalTexture(data.motion);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !in || !motion || !out || !data.st)
                return;
            if (!data.st->mblurPipeline || !data.st->mblurLayout)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("motion_blur", ".ps");
            if (!vsR || !psR)
                return;

            struct alignas(16) PCB { float sx, sy, intensity, pad; };
            auto* cb = cache.GetOrCreateVolatileCB("MotionBlur", "MotionBlurParams", sizeof(PCB), data.device);
            PCB p{ float(data.width), float(data.height), CameraMblurAmount(), 0.f };
            if (cb)
                cmd->writeBuffer(cb, &p, sizeof(p));

            BindingSetBuilder bsb(*vsR, *psR, nv, "MotionBlur");
            if (cb)
                bsb.ConstantBuffer("MotionBlurParams", cb);
            bsb.Texture("g_Color", in);
            bsb.Texture("g_Motion", motion);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->mblurLayout, nv);
            if (!set)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("MotionBlur", fbDesc, nv);
            if (!fb)
                return;

            nvrhi::GraphicsState gs;
            gs.pipeline = data.st->mblurPipeline;
            gs.framebuffer = fb;
            gs.bindings = { set };
            gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    return blurPass.output;
}

} // namespace
