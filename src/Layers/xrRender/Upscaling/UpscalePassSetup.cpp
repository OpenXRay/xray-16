#include "stdafx.h"
#include "UpscalePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API int ps_r_fsr_fg;
extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_dlss_rr;
extern ENGINE_API int ps_r_dlss_auto_exposure;
extern ENGINE_API float ps_r_fsr_sharpness;
extern ENGINE_API int ps_r_upscale;

namespace xray::render::fg::passes {
using namespace framegraph;

namespace {

struct alignas(16) DlssRrGuideCB
{
    float cameraPos[4];
    float screenSize[2];
    float pad[2];
};

void EnsureDisplayColor(nvrhi::IDevice* nv, UpscalePassState& state, u32 w, u32 h)
{
    if (state.displayColor && state.displayW == w && state.displayH == h)
        return;
    nvrhi::TextureDesc td;
    td.width = w;
    td.height = h;
    td.format = nvrhi::Format::RGBA16_FLOAT;
    td.isUAV = true;
    td.isShaderResource = true;
    td.isRenderTarget = true;
    td.initialState = nvrhi::ResourceStates::UnorderedAccess;
    td.keepInitialState = true;
    td.debugName = "rt_Upscale_DisplayColor";
    state.displayColor = nv->createTexture(td);
    state.displayW = w;
    state.displayH = h;
}

void EnsureRRGuideResources(fg::RenderDevice* device, UpscalePassState& state, u32 w, u32 h)
{
    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    if (!state.rrGuidePipeline)
    {
        auto* loader = RImplementation.GetShaderLoader();
        if (!loader)
            return;
        auto cs = loader->LoadComputeShader("dlss_rr_guides");
        if (!cs.handle || !cs.reflection)
            return;
        auto& cache = GetPassResourceCache();
        state.rrGuideLayout = cache.GetOrCreateBindingLayoutFromReflection("DlssRrGuides", *cs.reflection, nv);
        nvrhi::ComputePipelineDesc pd;
        pd.CS = cs.handle;
        pd.bindingLayouts = { state.rrGuideLayout };
        state.rrGuidePipeline = cache.GetOrCreateComputePipeline("DlssRrGuides", pd, nv);
        state.rrGuideCB = cache.GetOrCreateVolatileCB("Upscale", "DlssRrGuideCB", sizeof(DlssRrGuideCB), device);
    }

    if (state.rrDiffuseAlbedo && state.rrGuideW == w && state.rrGuideH == h)
        return;

    auto make = [&](const char* name, nvrhi::Format fmt) {
        nvrhi::TextureDesc td;
        td.width = w;
        td.height = h;
        td.format = fmt;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::UnorderedAccess;
        td.keepInitialState = true;
        td.debugName = name;
        return nv->createTexture(td);
    };
    state.rrDiffuseAlbedo = make("rt_DlssRr_DiffuseAlbedo", nvrhi::Format::RGBA16_FLOAT);
    state.rrSpecularAlbedo = make("rt_DlssRr_SpecularAlbedo", nvrhi::Format::RGBA16_FLOAT);
    state.rrSpecularHitDist = make("rt_DlssRr_SpecularHitDist", nvrhi::Format::R16_FLOAT);
    state.rrGuideW = w;
    state.rrGuideH = h;
}

static void UpscaleCopyMatrix(float dst[16], const Fmatrix& m)
{
    dst[0] = m._11; dst[1] = m._12; dst[2] = m._13; dst[3] = m._14;
    dst[4] = m._21; dst[5] = m._22; dst[6] = m._23; dst[7] = m._24;
    dst[8] = m._31; dst[9] = m._32; dst[10] = m._33; dst[11] = m._34;
    dst[12] = m._41; dst[13] = m._42; dst[14] = m._43; dst[15] = m._44;
}

}

framegraph::VirtualResourceHandle setupUpscaleOrResolvePass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle motionVectors,
    VirtualResourceHandle exposure,
    const UpscaleState& upscaleState,
    IUpscaleBackend* backend,
    u32 renderW,
    u32 renderH,
    UpscalePassState& state,
    const UpscaleRRGuides* rrGuides)
{
    if (!backend || !backend->IsAvailable())
        return sceneColor;
    if (!upscaleState.upscaleActive && !NeedsResolveToDisplay(upscaleState))
        return sceneColor;

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    EnsureDisplayColor(nv, state, upscaleState.displayWidth, upscaleState.displayHeight);

    const bool wantRR = rrGuides && ps_r_upscale == 2 && ps_r_dlss_rr != 0 && backend->SupportsRR();
    if (wantRR)
        EnsureRRGuideResources(device, state, renderW, renderH);

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = upscaleState.displayWidth;
    outDesc.height = upscaleState.displayHeight;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isUAV = true;
    outDesc.isRenderTarget = true;
    outDesc.isImported = true;
    outDesc.isTransient = false;
    VirtualResourceHandle outHandle = fg.ImportTexture("rt_Upscale_DisplayColor", state.displayColor.Get(), outDesc);

    VirtualResourceHandle rrDiffHandle{}, rrSpecHandle{}, rrHitHandle{}, rrNoisySpecHandle{};
    if (wantRR && state.rrDiffuseAlbedo)
    {
        ResourceDesc gd;
        gd.type = ResourceDesc::Type::Texture2D;
        gd.width = renderW;
        gd.height = renderH;
        gd.format = nvrhi::Format::RGBA16_FLOAT;
        gd.isUAV = true;
        gd.isImported = true;
        gd.isTransient = false;
        rrDiffHandle = fg.ImportTexture("rt_DlssRr_DiffuseAlbedo", state.rrDiffuseAlbedo.Get(), gd);
        rrSpecHandle = fg.ImportTexture("rt_DlssRr_SpecularAlbedo", state.rrSpecularAlbedo.Get(), gd);
        gd.format = nvrhi::Format::R16_FLOAT;
        gd.isUAV = true;
        rrHitHandle = fg.ImportTexture("rt_DlssRr_SpecularHitDist", state.rrSpecularHitDist.Get(), gd);
        if (rrGuides->noisySpecular)
        {
            ResourceDesc ns = gd;
            ns.format = nvrhi::Format::RGBA16_FLOAT;
            ns.isUAV = false;
            rrNoisySpecHandle = fg.ImportTexture("rt_DlssRr_NoisySpecularIn", rrGuides->noisySpecular, ns);
        }
    }

    if (wantRR && state.rrGuidePipeline && rrDiffHandle.is_valid() && rrNoisySpecHandle.is_valid())
    {
        struct PackData
        {
            VirtualResourceHandle baseColor, normals, worldPos, depth, noisySpec;
            VirtualResourceHandle outDiff, outSpec, outHit;
            UpscalePassState* state = nullptr;
            u32 w = 0, h = 0;
        };
        fg.addCallbackPass<PackData>(
            "DLSS-RR Guides",
            [&](FrameGraph& builder, PassHandle passHandle, PackData& data) {
                RenderPassBuilder pb(builder, passHandle);
                data.baseColor = pb.read(rrGuides->baseColor, ResourceState::ShaderResource);
                data.normals = pb.read(rrGuides->normals, ResourceState::ShaderResource);
                data.worldPos = pb.read(rrGuides->worldPos, ResourceState::ShaderResource);
                data.depth = pb.read(rrGuides->depth.is_valid() ? rrGuides->depth : depth, ResourceState::ShaderResource);
                data.noisySpec = pb.read(rrNoisySpecHandle, ResourceState::ShaderResource);
                data.outDiff = pb.write(rrDiffHandle, ResourceState::UnorderedAccess);
                data.outSpec = pb.write(rrSpecHandle, ResourceState::UnorderedAccess);
                data.outHit = pb.write(rrHitHandle, ResourceState::UnorderedAccess);
                pb.sideEffects();
                data.state = &state;
                data.w = renderW;
                data.h = renderH;
            },
            [](const PackData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
                if (!data.state || !data.state->rrGuidePipeline || !ctx)
                    return;
                auto* base = fgGraph.GetPhysicalTexture(data.baseColor);
                auto* nrm = fgGraph.GetPhysicalTexture(data.normals);
                auto* wpos = fgGraph.GetPhysicalTexture(data.worldPos);
                auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
                auto* noisySpec = fgGraph.GetPhysicalTexture(data.noisySpec);
                auto* outD = data.state->rrDiffuseAlbedo.Get();
                auto* outS = data.state->rrSpecularAlbedo.Get();
                auto* outH = data.state->rrSpecularHitDist.Get();
                if (!base || !nrm || !wpos || !depthTex || !noisySpec || !outD || !outS || !outH)
                    return;

                auto* cmd = ctx->GetCommandList();
                auto* nvDev = cmd->getDevice();
                auto& cache = GetPassResourceCache();
                auto* refl = RImplementation.GetShaderLoader()->GetCachedReflection("dlss_rr_guides", ".cs");
                if (!refl || !data.state->rrGuideCB)
                    return;

                DlssRrGuideCB cb{};
                cb.cameraPos[0] = Device.vCameraPosition.x;
                cb.cameraPos[1] = Device.vCameraPosition.y;
                cb.cameraPos[2] = Device.vCameraPosition.z;
                cb.cameraPos[3] = 1.f;
                cb.screenSize[0] = (float)data.w;
                cb.screenSize[1] = (float)data.h;
                cmd->writeBuffer(data.state->rrGuideCB, &cb, sizeof(cb));

                BindingSetBuilder bsb(*refl, nvDev, "DlssRrGuides");
                bsb.ConstantBuffer("DlssRrGuideParams", data.state->rrGuideCB)
                   .Texture("g_BaseColor", base)
                   .Texture("g_Normal", nrm)
                   .Texture("g_WorldPos", wpos)
                   .Texture("g_NoisySpecular", noisySpec)
                   .Texture("g_Depth", depthTex, nvrhi::Format::R32_FLOAT)
                   .TextureUAV("u_DiffuseAlbedo", outD)
                   .TextureUAV("u_SpecularAlbedo", outS)
                   .TextureUAV("u_SpecularHitDist", outH);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.state->rrGuideLayout, nvDev);
                nvrhi::ComputeState cs;
                cs.pipeline = data.state->rrGuidePipeline;
                cs.bindings = { set };
                cmd->setComputeState(cs);
                cmd->dispatch((data.w + 7) / 8, (data.h + 7) / 8, 1);
            });
    }

    struct PassData {
        VirtualResourceHandle src;
        VirtualResourceHandle dst;
        VirtualResourceHandle depth;
        VirtualResourceHandle motion;
        VirtualResourceHandle exposure;
        VirtualResourceHandle normals;
        VirtualResourceHandle diffuseAlbedo;
        VirtualResourceHandle specularAlbedo;
        VirtualResourceHandle specularHit;
        IUpscaleBackend* backend;
        UpscalePassState* state;
        UpscaleState upscale;
        u32 renderW, renderH;
        nvrhi::ITexture* noisyDiffuse = nullptr;
        nvrhi::ITexture* noisySpecular = nullptr;
        nvrhi::ITexture* hitDistance = nullptr;
        bool wantRR = false;
    };

    auto& pass = fg.addCallbackPass<PassData>(
        "Upscale / Resolve",
        [&](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.src = pb.read(sceneColor, ResourceState::ShaderResource);
            if (depth.is_valid())
                data.depth = pb.read(depth, ResourceState::ShaderResource);
            if (motionVectors.is_valid())
                data.motion = pb.read(motionVectors, ResourceState::ShaderResource);
            if (exposure.is_valid())
                data.exposure = pb.read(exposure, ResourceState::ShaderResource);
            if (wantRR && rrGuides && rrDiffHandle.is_valid() && rrSpecHandle.is_valid() &&
                rrHitHandle.is_valid() && rrNoisySpecHandle.is_valid() && state.rrGuidePipeline)
            {
                if (rrGuides->normals.is_valid())
                    data.normals = pb.read(rrGuides->normals, ResourceState::ShaderResource);
                data.diffuseAlbedo = pb.read(rrDiffHandle, ResourceState::ShaderResource);
                data.specularAlbedo = pb.read(rrSpecHandle, ResourceState::ShaderResource);
                data.specularHit = pb.read(rrHitHandle, ResourceState::ShaderResource);
                data.noisyDiffuse = rrGuides->noisyDiffuse;
                data.noisySpecular = rrGuides->noisySpecular;
                data.hitDistance = rrGuides->hitDistance;
                data.wantRR = true;
            }
            data.dst = pb.write(outHandle, ResourceState::UnorderedAccess);
            pb.sideEffects();
            data.backend = backend;
            data.state = &state;
            data.upscale = upscaleState;
            data.renderW = renderW;
            data.renderH = renderH;
        },
        [](const PassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* src = fgGraph.GetPhysicalTexture(data.src);
            auto* dst = data.state ? data.state->displayColor.Get() : nullptr;
            if (!src || !dst || !ctx)
                return;

            nvrhi::ICommandList* cmd = ctx->GetCommandList();
            UpscaleInputs in;
            in.color = src;
            in.depth = data.depth.is_valid() ? fgGraph.GetPhysicalTexture(data.depth) : nullptr;
            in.motionVectors = data.motion.is_valid() ? fgGraph.GetPhysicalTexture(data.motion) : nullptr;
            in.exposure = data.exposure.is_valid() ? fgGraph.GetPhysicalTexture(data.exposure) : nullptr;
            in.normals = data.normals.is_valid() ? fgGraph.GetPhysicalTexture(data.normals) : nullptr;
            in.diffuseAlbedo = data.diffuseAlbedo.is_valid() ? fgGraph.GetPhysicalTexture(data.diffuseAlbedo) : nullptr;
            in.specularAlbedo = data.specularAlbedo.is_valid() ? fgGraph.GetPhysicalTexture(data.specularAlbedo) : nullptr;
            in.specularHitDistance = data.specularHit.is_valid() ? fgGraph.GetPhysicalTexture(data.specularHit) : nullptr;
            in.diffuseHitDistance = data.hitDistance;
            in.roughness = in.normals;
            in.noisyDiffuse = data.noisyDiffuse;
            in.noisySpecular = data.noisySpecular;
            in.output = dst;
            in.renderWidth = data.renderW;
            in.renderHeight = data.renderH;
            in.displayWidth = data.upscale.displayWidth;
            in.displayHeight = data.upscale.displayHeight;
            in.jitterX = data.upscale.jitterX;
            in.jitterY = data.upscale.jitterY;
            in.sharpness = ps_r_fsr_sharpness;
            in.frameIndex = Device.dwFrame;
            in.frameTimeMs = Device.fTimeDelta > 1e-5f ? Device.fTimeDelta * 1000.f : 16.f;
            in.reset = data.upscale.resetHistory;
            in.enableFG = (ps_r_upscale == 1 && ps_r_fsr_fg) || (ps_r_upscale == 2 && ps_r_dlss_fg);
            in.enableRR = data.wantRR && (ps_r_upscale == 2 && ps_r_dlss_rr);
            if (ps_r_upscale == 2 && ps_r_dlss_auto_exposure)
                in.exposure = nullptr;
            UpscaleCopyMatrix(in.worldToView, Device.mView);
            UpscaleCopyMatrix(in.viewToClip, Device.mProject);

            bool ok = false;
            if (data.backend && data.backend->IsAvailable())
                ok = data.backend->Evaluate(cmd, in);

            if (!ok) {
                static bool s_failLogged = false;
                if (!s_failLogged)
                {
                    s_failLogged = true;
                    Msg("! [Upscale] Evaluate failed — render %ux%u → display %ux%u",
                        data.renderW, data.renderH, data.upscale.displayWidth, data.upscale.displayHeight);
                }
                if (data.renderW == data.upscale.displayWidth &&
                    data.renderH == data.upscale.displayHeight)
                {
                    nvrhi::TextureSlice slice;
                    cmd->copyTexture(dst, slice, src, slice);
                }
                else
                {
                    cmd->clearTextureFloat(dst, nvrhi::TextureSubresourceSet(0, 1, 0, 1),
                        nvrhi::Color(0.f));
                }
            }
        });

    return pass.dst;
}

}
