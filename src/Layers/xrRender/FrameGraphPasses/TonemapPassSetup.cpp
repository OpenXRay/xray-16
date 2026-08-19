#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/device.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

extern ENGINE_API int ps_r_hdr10;
extern ENGINE_API float ps_r_hdr10_hud;
extern ENGINE_API float ps_r_hdr10_paper_white;
extern ENGINE_API float ps_r_hdr10_peak;
extern ENGINE_API int ps_r_hdr_debug;
extern ENGINE_API float ps_r_hdr_exposure_bias;
extern ENGINE_API float ps_r_hdr_contrast;
extern ENGINE_API float ps_r_hdr_saturation;
extern ENGINE_API float ps_r_hdr_white;
extern ENGINE_API float ps_r_hdr_lift;
extern ENGINE_API float ps_r_hdr_gamma;
extern ENGINE_API float ps_r_hdr_gain;
extern ENGINE_API float ps_r_hdr_temp;
extern ENGINE_API float ps_r_hdr_tint;
extern ENGINE_API float ps_r_hdr_bloom;

namespace xray::render::fg {
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

namespace {
constexpr u32 kTonemapPipeVersion = 32;

struct TonemapHDRCB {
    float hdr10, paperWhite, peakNits, hudNits;
    Fvector4 dofParams;
    Fvector4 dofMblur;
    Fvector4 eyePos;
    Fmatrix invVP;
    Fmatrix prevVP;
    float exposureBias, contrast, saturation, whitePoint;
    float lift, gamma, gain, bloomScale;
    float temp, tint, pad0, pad1;
    Fvector4 padGrade;
};
static_assert(sizeof(TonemapHDRCB) == 256);

struct BloomCB {
    float srcW, srcH, dstW, dstH;
    float threshold, intensity, dirX, dirY;
};
static_assert(sizeof(BloomCB) == 32);
}

void InitializeTonemapPass(nvrhi::IDevice* device, TonemapPassState& state) {
    const u32 hdr10 = (GEnv.Backend && GEnv.Backend->IsHdr10()) ? 1u : 0u;
    if (state.initialized && state.pipeVersion == kTonemapPipeVersion && state.pipeline && state.hdr10 == hdr10)
        return;

    state.initialized = false;
    state.pipeVersion = 0;
    state.pipeline = nullptr;
    state.bindingLayout = nullptr;

    if (!device)
        return;

    if (!state.fallbackExposureTexture)
    {
        nvrhi::TextureDesc texDesc;
        texDesc.debugName = "FallbackExposure";
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.format = nvrhi::Format::R32_FLOAT;
        texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        texDesc.keepInitialState = true;

        state.fallbackExposureTexture = device->createTexture(texDesc);
        nvrhi::TextureDesc depthFb;
        depthFb.debugName = "FallbackTonemapDepth";
        depthFb.width = 1;
        depthFb.height = 1;
        depthFb.format = nvrhi::Format::R32_FLOAT;
        depthFb.initialState = nvrhi::ResourceStates::ShaderResource;
        depthFb.keepInitialState = true;
        state.fallbackDepthTexture = device->createTexture(depthFb);
        nvrhi::TextureDesc bloomFb;
        bloomFb.debugName = "FallbackBloom";
        bloomFb.width = 1;
        bloomFb.height = 1;
        bloomFb.format = nvrhi::Format::RGBA16_FLOAT;
        bloomFb.initialState = nvrhi::ResourceStates::ShaderResource;
        bloomFb.keepInitialState = true;
        state.fallbackBloom = device->createTexture(bloomFb);

        nvrhi::CommandListHandle cmdList = device->createCommandList();
        cmdList->open();
        float defaultExposure = 1.0f;
        cmdList->writeTexture(state.fallbackExposureTexture, 0, 0, &defaultExposure, sizeof(float));
        float defaultDepth = 1.0f;
        cmdList->writeTexture(state.fallbackDepthTexture, 0, 0, &defaultDepth, sizeof(float));
        cmdList->close();
        device->executeCommandList(cmdList);
    }

    if (GEnv.Render->GetShaderLoader()) {
        framegraph::BindingSetBuilder::InvalidateReflectionCache();
        auto& cache = framegraph::GetPassResourceCache();
        auto vsResult = GEnv.Render->GetShaderLoader()->LoadVertexShader("tonemap");
        auto psResult = GEnv.Render->GetShaderLoader()->LoadPixelShader("tonemap");
        if (vsResult.handle && psResult.handle) {
            state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TonemapPass_CoP_v29", *vsResult.reflection, *psResult.reflection, device);

            if (state.bindingLayout) {
                nvrhi::GraphicsPipelineDesc pipeDesc;
                pipeDesc.setVertexShader(vsResult.handle);
                pipeDesc.setPixelShader(psResult.handle);
                pipeDesc.addBindingLayout(state.bindingLayout);
                pipeDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
                pipeDesc.renderState.blendState.targets[0].setBlendEnable(false);
                pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
                pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);
                pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

                nvrhi::FramebufferInfoEx fbInfo;
                nvrhi::Format fbFmt = nvrhi::Format::RGBA8_UNORM;
                if (hdr10)
                    fbFmt = nvrhi::Format::RGBA16_FLOAT;
                else if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
                    fbFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;
                fbInfo.addColorFormat(fbFmt);

                state.pipeline = cache.GetOrCreatePipeline("TonemapPass_CoP_v29", pipeDesc, fbInfo, device);
            }
        }

        auto extract = GEnv.Render->GetShaderLoader()->LoadComputeShader("bloom_extract");
        auto blur = GEnv.Render->GetShaderLoader()->LoadComputeShader("bloom_blur");
        if (extract.handle && extract.reflection) {
            state.bloomExtractLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "BloomExtract_v9", *extract.reflection, device);
            nvrhi::ComputePipelineDesc desc;
            desc.CS = extract.handle;
            desc.bindingLayouts = { state.bloomExtractLayout };
            state.bloomExtractPipeline = device->createComputePipeline(desc);
        }
        if (blur.handle && blur.reflection) {
            state.bloomBlurLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "BloomBlur_v2", *blur.reflection, device);
            nvrhi::ComputePipelineDesc desc;
            desc.CS = blur.handle;
            desc.bindingLayouts = { state.bloomBlurLayout };
            state.bloomBlurPipeline = device->createComputePipeline(desc);
        }
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(BloomCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;
        cbDesc.debugName = "BloomCB";
        state.bloomCB = device->createBuffer(cbDesc);
        cbDesc.byteSize = sizeof(TonemapHDRCB);
        cbDesc.debugName = "TonemapHDRCB";
        state.hdrCB = device->createBuffer(cbDesc);

        auto encVs = GEnv.Render->GetShaderLoader()->LoadVertexShader("tonemap");
        auto encPs = GEnv.Render->GetShaderLoader()->LoadPixelShader("hdr10_encode");
        if (encVs.handle && encPs.handle) {
            state.encodeLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "Hdr10Encode_v4", *encVs.reflection, *encPs.reflection, device);
            if (state.encodeLayout) {
                nvrhi::GraphicsPipelineDesc encDesc;
                encDesc.setVertexShader(encVs.handle);
                encDesc.setPixelShader(encPs.handle);
                encDesc.addBindingLayout(state.encodeLayout);
                encDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
                encDesc.renderState.blendState.targets[0].setBlendEnable(false);
                encDesc.renderState.depthStencilState.setDepthTestEnable(false);
                encDesc.renderState.depthStencilState.setDepthWriteEnable(false);
                encDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
                nvrhi::FramebufferInfoEx encFb;
                nvrhi::Format encFmt = nvrhi::Format::R10G10B10A2_UNORM;
                if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
                    encFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;
                encFb.addColorFormat(encFmt);
                state.encodePipeline = cache.GetOrCreatePipeline("Hdr10Encode_v4", encDesc, encFb, device);
            }
        }
    }

    state.initialized = true;
    state.pipeVersion = kTonemapPipeVersion;
    state.hdr10 = hdr10;
}

void ShutdownTonemapPass(TonemapPassState& state) {
    state.fallbackExposureTexture = nullptr;
    state.fallbackDepthTexture = nullptr;
    state.fallbackBloom = nullptr;
    state.bloom0 = nullptr;
    state.bloom1 = nullptr;
    state.pipeline = nullptr;
    state.bindingLayout = nullptr;
    state.bloomExtractPipeline = nullptr;
    state.bloomExtractLayout = nullptr;
    state.bloomBlurPipeline = nullptr;
    state.bloomBlurLayout = nullptr;
    state.bloomCB = nullptr;
    state.hdrCB = nullptr;
    state.encodePipeline = nullptr;
    state.encodeLayout = nullptr;
    state.bloomW = 0;
    state.bloomH = 0;
    state.initialized = false;
    state.pipeVersion = 0;
}

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    TonemapPassState& tonemapState,
    const ExposurePassState* exposureState,
    framegraph::VirtualResourceHandle depthTexture,
    framegraph::VirtualResourceHandle worldPosTexture)
{
    using namespace framegraph;

    if (device && device->GetNVRHIDevice())
        InitializeTonemapPass(device->GetNVRHIDevice(), tonemapState);

    bool hasExposure = exposureTexture.is_valid();
    bool hasOutputTarget = outputTarget.is_valid();
    bool hasDepth = depthTexture.is_valid();
    bool hasWorldPos = worldPosTexture.is_valid();

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        [hdrInput, exposureTexture, outputTarget, depthTexture, worldPosTexture, hasExposure, hasOutputTarget, hasDepth, hasWorldPos, width, height, &tonemapState, exposureState, device](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.hasExposure = hasExposure;
            data.hasDepth = hasDepth;
            data.hasWorldPos = hasWorldPos;
            data.passState = &tonemapState;
            data.exposurePassState = exposureState;
            data.device = device;

            data.hdrInput = passBuilder.read(hdrInput, ResourceState::ShaderResource);

            if (hasExposure) {
                data.exposureInput = passBuilder.read(exposureTexture, ResourceState::ShaderResource);
            }
            if (hasDepth) {
                data.depthInput = passBuilder.read(depthTexture, ResourceState::ShaderResource);
            }
            if (hasWorldPos) {
                data.worldPosInput = passBuilder.read(worldPosTexture, ResourceState::ShaderResource);
            }

            if (hasOutputTarget) {
                data.ldrOutput = passBuilder.write(outputTarget, ResourceState::RenderTarget);
            } else {
                nvrhi::Format ldrFmt = nvrhi::Format::RGBA8_UNORM;
                if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
                    ldrFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;

                framegraph::ResourceDesc ldrDesc;
                ldrDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                ldrDesc.width = width;
                ldrDesc.height = height;
                ldrDesc.format = ldrFmt;
                ldrDesc.isRenderTarget = true;
                ldrDesc.isTransient = false;
                ldrDesc.debugName = "rt_Final";

                data.ldrOutput = passBuilder.createTexture("rt_Final", ldrDesc);
            }
        },

        [](const TonemapPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            auto* ps = data.passState;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* device = cmdList->getDevice();

            if (!ps->pipeline || !ps->bindingLayout)
                return;

            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);
            if (!hdrTexture || !ldrTexture)
                return;

            nvrhi::ITexture* exposureTex = ps->fallbackExposureTexture;
            if (data.hasExposure && data.exposureInput.is_valid())
            {
                if (auto* e = fg.GetPhysicalTexture(data.exposureInput))
                    exposureTex = e;
            }
            if (!exposureTex && data.exposurePassState)
                exposureTex = GetExposureTexture(*data.exposurePassState);
            if (!exposureTex)
                exposureTex = ps->fallbackExposureTexture;
            if (!exposureTex)
                return;

            auto& cache = framegraph::GetPassResourceCache();

            nvrhi::ITexture* depthTex = ps->fallbackDepthTexture;
            if (data.hasDepth && data.depthInput.is_valid())
            {
                if (auto* d = fg.GetPhysicalTexture(data.depthInput))
                    depthTex = d;
            }
            if (!depthTex)
                depthTex = ps->fallbackDepthTexture;

            nvrhi::ITexture* bloomTex = ps->fallbackBloom;
            const u32 bw = std::max(1u, data.width / 4);
            const u32 bh = std::max(1u, data.height / 4);
            if (ps->bloomExtractPipeline && ps->bloomBlurPipeline && ps->bloomCB) {
                if (!ps->bloom0 || ps->bloomW != bw || ps->bloomH != bh) {
                    nvrhi::TextureDesc td;
                    td.debugName = "Bloom0";
                    td.width = bw;
                    td.height = bh;
                    td.format = nvrhi::Format::RGBA16_FLOAT;
                    td.isUAV = true;
                    td.initialState = nvrhi::ResourceStates::UnorderedAccess;
                    td.keepInitialState = true;
                    ps->bloom0 = device->createTexture(td);
                    td.debugName = "Bloom1";
                    ps->bloom1 = device->createTexture(td);
                    ps->bloomW = bw;
                    ps->bloomH = bh;
                }
                if (ps->bloom0 && ps->bloom1) {
                    BloomCB cb{};
                    cb.srcW = float(data.width);
                    cb.srcH = float(data.height);
                    cb.dstW = float(bw);
                    cb.dstH = float(bh);
                    cb.threshold = ps_r2_ls_bloom_threshold;
                    cb.intensity = ps_r2_ls_bloom_kernel_scale * std::max(ps_r2_ls_bloom_kernel_b, 0.25f);
                    cb.dirX = 0.f;
                    cb.dirY = 0.f;
                    cmdList->writeBuffer(ps->bloomCB, &cb, sizeof(cb));

                    auto* exRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("bloom_extract", ".cs");
                    if (exRefl) {
                        framegraph::BindingSetBuilder bsb(*exRefl, device, "Bloom.Extract");
                        bsb.ConstantBuffer("BloomParams", ps->bloomCB);
                        bsb.Texture("t_Hdr", hdrTexture);
                        bsb.Texture("t_exposure", exposureTex);
                        if (depthTex)
                            bsb.TextureSlot(2, depthTex);
                        nvrhi::ITexture* worldPosTex = nullptr;
                        if (data.hasWorldPos && data.worldPosInput.is_valid())
                            worldPosTex = fg.GetPhysicalTexture(data.worldPosInput);
                        if (!worldPosTex)
                            worldPosTex = cache.GetDummyContactHistory(device);
                        if (worldPosTex)
                            bsb.Texture("t_WorldPos", worldPosTex);
                        bsb.TextureUAV("u_Bloom", ps->bloom0);
                        auto bs = cache.GetOrCreateBindingSet(bsb.Build(), ps->bloomExtractLayout, device);
                        if (bs) {
                            nvrhi::ComputeState cs;
                            cs.pipeline = ps->bloomExtractPipeline;
                            cs.bindings = { bs };
                            cmdList->setComputeState(cs);
                            cmdList->dispatch((bw + 7) / 8, (bh + 7) / 8, 1);
                        }
                    }

                    auto dispatchBlur = [&](nvrhi::ITexture* src, nvrhi::ITexture* dst, float dx, float dy) {
                        BloomCB bcb = cb;
                        bcb.srcW = float(bw);
                        bcb.srcH = float(bh);
                        bcb.intensity = ps_r2_ls_bloom_kernel_scale;
                        bcb.dirX = dx;
                        bcb.dirY = dy;
                        cmdList->writeBuffer(ps->bloomCB, &bcb, sizeof(bcb));
                        auto* blRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("bloom_blur", ".cs");
                        if (!blRefl)
                            return;
                        framegraph::BindingSetBuilder bsb(*blRefl, device, "Bloom.Blur");
                        bsb.ConstantBuffer("BloomParams", ps->bloomCB);
                        bsb.Texture("t_In", src);
                        bsb.TextureUAV("u_Out", dst);
                        auto bs = cache.GetOrCreateBindingSet(bsb.Build(), ps->bloomBlurLayout, device);
                        if (!bs)
                            return;
                        nvrhi::ComputeState cs;
                        cs.pipeline = ps->bloomBlurPipeline;
                        cs.bindings = { bs };
                        cmdList->setComputeState(cs);
                        cmdList->dispatch((bw + 7) / 8, (bh + 7) / 8, 1);
                    };
                    dispatchBlur(ps->bloom0, ps->bloom1, 1.f, 0.f);
                    dispatchBlur(ps->bloom1, ps->bloom0, 0.f, 1.f);
                    bloomTex = ps->bloom0;
                }
            }

            auto* vsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("tonemap", ".vs");
            auto* psRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("tonemap", ".ps");
            if (!vsRefl || !psRefl)
                return;

            if (ps->hdrCB) {
                TonemapHDRCB hcb{};
                hcb.hdr10 = (GEnv.Backend && GEnv.Backend->IsHdr10()) ? 1.f : 0.f;
                hcb.paperWhite = ps_r_hdr10_paper_white;
                hcb.peakNits = ps_r_hdr10_peak;
                hcb.hudNits = ps_r_hdr10_hud;
                Fvector3 dof = ps_r2_dof;
                if (g_pGamePersistent)
                    g_pGamePersistent->GetCurrentDof(dof);
                hcb.dofParams.set(dof.x, dof.y, dof.z, ps_r2_dof_sky);
                const bool dofOn = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF);
                const bool mblurOn = ps_r2_mblur > 0.001f;
                hcb.dofMblur.set(ps_r2_dof_kernel_size, ps_r2_mblur, dofOn ? 1.f : 0.f, mblurOn ? 1.f : 0.f);
                hcb.eyePos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.f);
                hcb.invVP = Device.mInvFullTransform;
                hcb.prevVP = Device.mFullTransformSaved;
                hcb.exposureBias = ps_r_hdr_exposure_bias;
                hcb.contrast = ps_r_hdr_contrast;
                hcb.saturation = ps_r_hdr_saturation;
                hcb.whitePoint = ps_r_hdr_white;
                hcb.lift = ps_r_hdr_lift;
                hcb.gamma = ps_r_hdr_gamma;
                hcb.gain = ps_r_hdr_gain;
                hcb.bloomScale = ps_r_hdr_bloom;
                hcb.temp = ps_r_hdr_temp;
                hcb.tint = ps_r_hdr_tint;
                cmdList->writeBuffer(ps->hdrCB, &hcb, sizeof(hcb));
            }

            framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, device, "Tonemap");
            bsb.Texture("t_hdr", hdrTexture);
            bsb.Texture("t_exposure", exposureTex);
            bsb.Texture("t_bloom", bloomTex);
            if (depthTex)
                bsb.TextureSlot(3, depthTex);
            if (ps->hdrCB)
                bsb.ConstantBuffer("TonemapHDR", ps->hdrCB);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->bindingLayout, device);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(ldrTexture);
            auto framebuffer = cache.GetOrCreateFramebuffer("TonemapPass_CoP", fbDesc, device);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            nvrhi::GraphicsState state;
            state.pipeline = ps->pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );

    return passData.ldrOutput;
}

framegraph::VirtualResourceHandle setupHdr10EncodePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle src,
    framegraph::VirtualResourceHandle dst,
    u32 width,
    u32 height,
    TonemapPassState& tonemapState)
{
    using namespace framegraph;
    if (device && device->GetNVRHIDevice())
        InitializeTonemapPass(device->GetNVRHIDevice(), tonemapState);

    struct EncodeData {
        VirtualResourceHandle src;
        VirtualResourceHandle dst;
        TonemapPassState* state = nullptr;
        u32 width = 0;
        u32 height = 0;
    };

    auto& passData = fg.addCallbackPass<EncodeData>(
        "HDR10 Encode",
        [src, dst, width, height, &tonemapState](FrameGraph& builder, PassHandle passHandle, EncodeData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.src = pb.read(src, ResourceState::ShaderResource);
            data.dst = pb.write(dst, ResourceState::RenderTarget);
            data.state = &tonemapState;
            data.width = width;
            data.height = height;
        },
        [](const EncodeData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* ps = data.state;
            if (!ps || !ps->encodePipeline || !ps->encodeLayout || !ps->hdrCB)
                return;
            auto* srcTex = fgGraph.GetPhysicalTexture(data.src);
            auto* dstTex = fgGraph.GetPhysicalTexture(data.dst);
            if (!srcTex || !dstTex)
                return;
            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            nvrhi::IDevice* nv = cmd->getDevice();
            TonemapHDRCB hcb{};
            hcb.hdr10 = 1.f;
            hcb.paperWhite = ps_r_hdr10_paper_white;
            hcb.peakNits = ps_r_hdr10_peak;
            hcb.hudNits = ps_r_hdr10_hud;
            cmd->writeBuffer(ps->hdrCB, &hcb, sizeof(hcb));
            auto* vsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("tonemap", ".vs");
            auto* psRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("hdr10_encode", ".ps");
            if (!vsRefl || !psRefl)
                return;
            auto& cache = framegraph::GetPassResourceCache();
            framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "Hdr10Encode");
            bsb.Texture("t_src", srcTex);
            bsb.ConstantBuffer("TonemapHDR", ps->hdrCB);
            auto bs = cache.GetOrCreateBindingSet(bsb.Build(), ps->encodeLayout, nv);
            if (!bs)
                return;
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(dstTex);
            auto fb = cache.GetOrCreateFramebuffer("Hdr10Encode", fbDesc, nv);
            nvrhi::Viewport vp;
            vp.maxX = float(data.width);
            vp.maxY = float(data.height);
            vp.maxZ = 1.f;
            nvrhi::GraphicsState gs;
            gs.pipeline = ps->encodePipeline;
            gs.framebuffer = fb;
            gs.viewport.addViewportAndScissorRect(vp);
            gs.addBindingSet(bs);
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );
    return passData.dst;
}

void RenderHdrDebugUI(const ExposurePassState* exposure)
{
    if (!ps_r_hdr_debug)
        return;
    if (!Device.GetImGuiContext())
        return;
    ImGui::SetCurrentContext(Device.GetImGuiContext());
    ImGui::SetNextWindowSize(ImVec2(420, 620), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("HDR Debug"))
    {
        ImGui::End();
        return;
    }

    ImGui::SliderFloat("Exposure bias", &ps_r_hdr_exposure_bias, -4.f, 4.f, "%.2f");
    ImGui::SliderFloat("Contrast", &ps_r_hdr_contrast, 0.2f, 3.f, "%.2f");
    ImGui::SliderFloat("Saturation", &ps_r_hdr_saturation, 0.f, 3.f, "%.2f");
    ImGui::SliderFloat("White point", &ps_r_hdr_white, 0.4f, 8.f, "%.2f");
    ImGui::SliderFloat("Lift", &ps_r_hdr_lift, -0.5f, 0.5f, "%.3f");
    ImGui::SliderFloat("Gamma", &ps_r_hdr_gamma, 0.3f, 2.6f, "%.2f");
    ImGui::SliderFloat("Gain", &ps_r_hdr_gain, 0.2f, 3.f, "%.2f");
    ImGui::SliderFloat("Temperature", &ps_r_hdr_temp, -1.f, 1.f, "%.2f");
    ImGui::SliderFloat("Tint", &ps_r_hdr_tint, -1.f, 1.f, "%.2f");
    ImGui::SliderFloat("Bloom", &ps_r_hdr_bloom, 0.f, 4.f, "%.2f");
    ImGui::Separator();
    ImGui::SliderFloat("Paper white", &ps_r_hdr10_paper_white, 80.f, 1000.f, "%.0f");
    ImGui::SliderFloat("Peak nits", &ps_r_hdr10_peak, 200.f, 10000.f, "%.0f");
    ImGui::SliderFloat("HUD nits", &ps_r_hdr10_hud, 80.f, 1000.f, "%.0f");
    ImGui::SliderFloat("Middle gray", &ps_r2_tonemap_middlegray, 0.f, 2.f, "%.3f");
    ImGui::SliderFloat("Adapt", &ps_r2_tonemap_adaptation, 0.01f, 10.f, "%.2f");
    ImGui::SliderFloat("Low lum", &ps_r2_tonemap_low_lum, 0.0001f, 1.f, "%.4f");
    ImGui::SliderFloat("TM amount", &ps_r2_tonemap_amount, 0.f, 1.f, "%.3f");
    ImGui::SliderFloat("Bloom thresh", &ps_r2_ls_bloom_threshold, 0.f, 1.f, "%.4f");
    ImGui::SliderFloat("Bloom scale", &ps_r2_ls_bloom_kernel_scale, 0.5f, 2.f, "%.2f");

    if (ImGui::Button("Reset grade"))
    {
        ps_r_hdr_exposure_bias = 0.f;
        ps_r_hdr_contrast = 1.f;
        ps_r_hdr_saturation = 1.f;
        ps_r_hdr_white = 1.7f;
        ps_r_hdr_lift = 0.f;
        ps_r_hdr_gamma = 1.f;
        ps_r_hdr_gain = 1.f;
        ps_r_hdr_temp = 0.f;
        ps_r_hdr_tint = 0.f;
        ps_r_hdr_bloom = 1.f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy cvars"))
    {
        char buf[768];
        snprintf(buf, sizeof(buf),
            "r_hdr_exposure_bias %.3f\n"
            "r_hdr_contrast %.3f\n"
            "r_hdr_saturation %.3f\n"
            "r_hdr_white %.3f\n"
            "r_hdr_lift %.3f\n"
            "r_hdr_gamma %.3f\n"
            "r_hdr_gain %.3f\n"
            "r_hdr_temp %.3f\n"
            "r_hdr_tint %.3f\n"
            "r_hdr_bloom %.3f\n"
            "r_hdr10_paper_white %.1f\n"
            "r_hdr10_peak %.1f\n"
            "r_hdr10_hud %.1f\n"
            "r2_tonemap_middlegray %.3f\n"
            "r2_tonemap_adaptation %.3f\n"
            "r2_tonemap_lowlum %.4f\n"
            "r2_tonemap_amount %.3f\n"
            "r2_ls_bloom_threshold %.4f\n"
            "r2_ls_bloom_kernel_scale %.3f\n",
            ps_r_hdr_exposure_bias, ps_r_hdr_contrast, ps_r_hdr_saturation, ps_r_hdr_white,
            ps_r_hdr_lift, ps_r_hdr_gamma, ps_r_hdr_gain, ps_r_hdr_temp, ps_r_hdr_tint, ps_r_hdr_bloom,
            ps_r_hdr10_paper_white, ps_r_hdr10_peak, ps_r_hdr10_hud,
            ps_r2_tonemap_middlegray, ps_r2_tonemap_adaptation, ps_r2_tonemap_low_lum, ps_r2_tonemap_amount,
            ps_r2_ls_bloom_threshold, ps_r2_ls_bloom_kernel_scale);
        ImGui::SetClipboardText(buf);
    }

    if (exposure)
    {
        ImGui::Separator();
        ImGui::Text("Exposure: %.3f", exposure->currentExposure);
        float hist[64];
        float peak = 1.f;
        for (int i = 0; i < 64; i++)
        {
            hist[i] = (float)exposure->histBins[i];
            peak = std::max(peak, hist[i]);
        }
        for (int i = 0; i < 64; i++)
            hist[i] = log2f(1.f + hist[i]) / log2f(1.f + peak);
        ImGui::Text("Luminance histogram (log2, -10 .. +4)");
        ImGui::PlotHistogram("##hdrhist", hist, 64, 0, nullptr, 0.f, 1.f, ImVec2(-1, 120));
    }

    ImGui::End();
}

} // namespace xray::render::fg::passes
