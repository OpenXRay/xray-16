#include "stdafx.h"
#include "BloomPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
static constexpr u32 kBloomCBVersions = 64;

// Classic r2 threshold (~1e-5) is for LDR combine space. HDR forward needs a
// real bright-pass floor or the whole frame enters the bloom pyramid.
static float BloomThresholdHDR()
{
    const float t = ps_r2_ls_bloom_threshold;
    return (t < 0.05f) ? 1.25f : t;
}

static float BloomIntensity()
{
    return std::clamp(ps_r2_ls_bloom_kernel_scale * 0.55f, 0.1f, 1.25f);
}

nvrhi::GraphicsPipelineHandle MakeFSPipe(
    const char* name, nvrhi::IDevice* nv,
    nvrhi::ShaderHandle vs, nvrhi::ShaderHandle ps,
    nvrhi::BindingLayoutHandle layout, nvrhi::Format fmt, bool additive)
{
    auto& cache = GetPassResourceCache();
    nvrhi::GraphicsPipelineDesc desc;
    desc.setVertexShader(vs);
    desc.setPixelShader(ps);
    desc.addBindingLayout(layout);
    desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    auto& blend = desc.renderState.blendState.targets[0];
    if (additive)
    {
        blend.setBlendEnable(true);
        blend.setSrcBlend(nvrhi::BlendFactor::One);
        blend.setDestBlend(nvrhi::BlendFactor::One);
        blend.setSrcBlendAlpha(nvrhi::BlendFactor::One);
        blend.setDestBlendAlpha(nvrhi::BlendFactor::One);
    }
    else
    {
        blend.setBlendEnable(false);
    }
    desc.renderState.depthStencilState.setDepthTestEnable(false);
    desc.renderState.depthStencilState.setDepthWriteEnable(false);
    desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
    nvrhi::FramebufferInfoEx fb;
    fb.addColorFormat(fmt);
    return cache.GetOrCreatePipeline(name, desc, fb, nv);
}

void InitBloom(nvrhi::IDevice* nv, BloomPassState& st)
{
    if (st.initialized && st.extractPipe && st.downPipe && st.upPipe && st.compPipe)
        return;
    st.initialized = false;

    if (!nv)
    {
        st.initialized = true;
        return;
    }
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("fullscreen");
    auto ex = loader->LoadPixelShader("bloom\\extract");
    auto dn = loader->LoadPixelShader("bloom\\downsample");
    auto up = loader->LoadPixelShader("bloom\\upsample");
    auto cp = loader->LoadPixelShader("bloom\\composite");
    if (!vs.handle || !ex.handle || !dn.handle || !up.handle || !cp.handle)
    {
        st.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    st.extractLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomExtract_v3", *vs.reflection, *ex.reflection, nv);
    st.downLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomDown_v3", *vs.reflection, *dn.reflection, nv);
    st.upLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomUp_v3", *vs.reflection, *up.reflection, nv);
    st.compLayout = cache.GetOrCreateBindingLayoutFromReflection("BloomComp_v4", *vs.reflection, *cp.reflection, nv);
    if (st.extractLayout)
        st.extractPipe = MakeFSPipe("BloomExtract_v3", nv, vs.handle, ex.handle, st.extractLayout, nvrhi::Format::RGBA16_FLOAT, false);
    if (st.downLayout)
        st.downPipe = MakeFSPipe("BloomDown_v3", nv, vs.handle, dn.handle, st.downLayout, nvrhi::Format::RGBA16_FLOAT, false);
    if (st.upLayout)
        st.upPipe = MakeFSPipe("BloomUp_v3", nv, vs.handle, up.handle, st.upLayout, nvrhi::Format::RGBA16_FLOAT, false);
    if (st.compLayout)
        st.compPipe = MakeFSPipe("BloomCompAdd_v4", nv, vs.handle, cp.handle, st.compLayout, nvrhi::Format::RGBA16_FLOAT, true);
    st.initialized = true;
}

bool DrawFS(
    nvrhi::ICommandList* cmd, nvrhi::IDevice* nv,
    nvrhi::GraphicsPipelineHandle pipe, nvrhi::BindingSetHandle set,
    nvrhi::ITexture* rt, u32 w, u32 h, const char* fbName)
{
    if (!cmd || !nv || !pipe || !set || !rt)
        return false;
    // Guard against pool/alias size mismatches (viewport write into corner).
    const auto& d = rt->getDesc();
    if (d.width < w || d.height < h)
    {
        Msg("! [Bloom] RT '%s' too small (%ux%u < %ux%u) — skip", fbName, d.width, d.height, w, h);
        return false;
    }
    auto& cache = GetPassResourceCache();
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(rt);
    auto fb = cache.GetOrCreateFramebuffer(fbName, fbDesc, nv);
    if (!fb)
        return false;
    nvrhi::GraphicsState gs;
    gs.pipeline = pipe;
    gs.framebuffer = fb;
    gs.bindings = {set};
    gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(w), float(h)));
    cmd->setGraphicsState(gs);
    cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
    return true;
}
} // namespace

VirtualResourceHandle setupBloomPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    u32 width,
    u32 height,
    BloomPassState& state)
{
    if (!device || !device->GetNVRHIDevice())
        return sceneColor;
    InitBloom(device->GetNVRHIDevice(), state);
    if (!state.extractPipe || !state.downPipe || !state.upPipe || !state.compPipe)
        return sceneColor;
    if (!depth.is_valid())
        return sceneColor;

    // Non-transient: never alias bloom pyramids onto unrelated full-res RTs.
    auto makeRT = [&](const char* name, u32 w, u32 h) {
        ResourceDesc d;
        d.type = ResourceDesc::Type::Texture2D;
        d.width = w;
        d.height = h;
        d.format = nvrhi::Format::RGBA16_FLOAT;
        d.isRenderTarget = true;
        d.isTransient = false;
        d.debugName = name;
        return fg.CreateTexture(name, d);
    };

    u32 mipW[kBloomMips], mipH[kBloomMips];
    VirtualResourceHandle mips[kBloomMips];
    mipW[0] = std::max(1u, width / 2);
    mipH[0] = std::max(1u, height / 2);
    mips[0] = makeRT("rt_Bloom0", mipW[0], mipH[0]);
    for (u32 i = 1; i < kBloomMips; ++i)
    {
        mipW[i] = std::max(1u, mipW[i - 1] / 2);
        mipH[i] = std::max(1u, mipH[i - 1] / 2);
        char name[32];
        xr_sprintf(name, "rt_Bloom%u", i);
        mips[i] = makeRT(name, mipW[i], mipH[i]);
    }

    VirtualResourceHandle upMips[kBloomMips - 1];
    for (u32 i = 0; i < kBloomMips - 1; ++i)
    {
        char name[32];
        xr_sprintf(name, "rt_BloomUp%u", i);
        upMips[i] = makeRT(name, mipW[kBloomMips - 2 - i], mipH[kBloomMips - 2 - i]);
    }

    auto output = makeRT("rt_BloomComposite", width, height);

    struct PassData
    {
        VirtualResourceHandle scene;
        VirtualResourceHandle depth;
        VirtualResourceHandle mips[kBloomMips];
        VirtualResourceHandle upMips[kBloomMips - 1];
        VirtualResourceHandle output;
        u32 mipW[kBloomMips], mipH[kBloomMips];
        u32 width, height;
        BloomPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "Bloom",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.scene = pb.read(sceneColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            for (u32 i = 0; i < kBloomMips; ++i)
            {
                data.mips[i] = pb.write(mips[i], ResourceState::RenderTarget);
                data.mipW[i] = mipW[i];
                data.mipH[i] = mipH[i];
            }
            for (u32 i = 0; i < kBloomMips - 1; ++i)
                data.upMips[i] = pb.write(upMips[i], ResourceState::RenderTarget);
            data.output = pb.write(output, ResourceState::RenderTarget);
            pb.sideEffects();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* st = data.st;
            if (!cmd || !nv || !st)
                return;
            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* exR = loader->GetCachedReflection("bloom\\extract", ".ps");
            auto* dnR = loader->GetCachedReflection("bloom\\downsample", ".ps");
            auto* upR = loader->GetCachedReflection("bloom\\upsample", ".ps");
            auto* cpR = loader->GetCachedReflection("bloom\\composite", ".ps");
            if (!vsR || !exR || !dnR || !upR || !cpR)
                return;

            auto* scene = graph.GetPhysicalTexture(data.scene);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            nvrhi::ITexture* mipTex[kBloomMips];
            for (u32 i = 0; i < kBloomMips; ++i)
                mipTex[i] = graph.GetPhysicalTexture(data.mips[i]);
            if (!scene || !depthTex || !mipTex[0])
                return;

            struct alignas(16) P4 { float x, y, z, w; };
            auto* cb = cache.GetOrCreateVolatileCB(
                "Bloom", "BloomParams", sizeof(P4), data.device, kBloomCBVersions);
            if (!cb)
                return;

            const float thr = BloomThresholdHDR();
            const float intensity = BloomIntensity();

            // Extract → mip0
            {
                P4 p{thr, 0.5f, 0, 0};
                cmd->writeBuffer(cb, &p, sizeof(p));
                BindingSetBuilder bsb(*vsR, *exR, nv, "Bloom.Extract");
                bsb.ConstantBuffer("BloomParams", cb);
                bsb.Texture("g_Color", scene);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), st->extractLayout, nv);
                DrawFS(cmd, nv, st->extractPipe, set, mipTex[0], data.mipW[0], data.mipH[0], "BloomExtractFB");
                cmd->setTextureState(mipTex[0], nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            // Downsample chain
            for (u32 i = 1; i < kBloomMips; ++i)
            {
                if (!mipTex[i] || !mipTex[i - 1])
                    break;
                P4 p{1.f / float(data.mipW[i - 1]), 1.f / float(data.mipH[i - 1]), 0, 0};
                cmd->writeBuffer(cb, &p, sizeof(p));
                BindingSetBuilder bsb(*vsR, *dnR, nv, "Bloom.Down");
                bsb.ConstantBuffer("BloomParams", cb);
                bsb.Texture("g_Color", mipTex[i - 1]);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), st->downLayout, nv);
                char fbName[32];
                xr_sprintf(fbName, "BloomDown%u", i);
                DrawFS(cmd, nv, st->downPipe, set, mipTex[i], data.mipW[i], data.mipH[i], fbName);
                cmd->setTextureState(mipTex[i], nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            }

            // Upsample + accumulate toward larger mips
            nvrhi::ITexture* bloomFull = mipTex[kBloomMips - 1];
            for (int i = int(kBloomMips) - 2; i >= 0; --i)
            {
                auto* upRT = graph.GetPhysicalTexture(data.upMips[kBloomMips - 2 - i]);
                auto* highMip = mipTex[i];
                if (!upRT || !bloomFull || !highMip)
                    break;
                P4 p{1.f / float(data.mipW[i + 1]), 1.f / float(data.mipH[i + 1]), 0, 0};
                cmd->writeBuffer(cb, &p, sizeof(p));
                BindingSetBuilder bsb(*vsR, *upR, nv, "Bloom.Up");
                bsb.ConstantBuffer("BloomParams", cb);
                bsb.Texture("g_Low", bloomFull).Texture("g_High", highMip);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), st->upLayout, nv);
                char fbName[32];
                xr_sprintf(fbName, "BloomUp%u", i);
                if (!DrawFS(cmd, nv, st->upPipe, set, upRT, data.mipW[i], data.mipH[i], fbName))
                    break;
                cmd->setTextureState(upRT, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                bloomFull = upRT;
            }

            auto* out = graph.GetPhysicalTexture(data.output);
            if (!out || !bloomFull)
                return;

            // Preserve scene: copy HDR, then add bloom (additive PSO).
            cmd->setTextureState(scene, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
            cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
            cmd->copyTexture(out, nvrhi::TextureSlice(), scene, nvrhi::TextureSlice());
            cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
            cmd->setTextureState(bloomFull, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

            P4 p{intensity, 1e-7f, 0, 0};
            cmd->writeBuffer(cb, &p, sizeof(p));
            BindingSetBuilder bsb(*vsR, *cpR, nv, "Bloom.Comp");
            bsb.ConstantBuffer("BloomParams", cb);
            bsb.Texture("g_Bloom", bloomFull);
            bsb.Texture("g_Depth", depthTex);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), st->compLayout, nv);
            DrawFS(cmd, nv, st->compPipe, set, out, data.width, data.height, "BloomCompFB");
            cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
        });

    return pd.output;
}

} // namespace
