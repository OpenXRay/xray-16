#include "stdafx.h"
#include "CASPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API float ps_r_cas_sharpness;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
void EnsureCASPipeline(nvrhi::IDevice* nv, CASPassState& st, nvrhi::Format outFmt)
{
    if (!nv)
        return;
    if (st.initialized && st.pipeline && st.pipelineFormat == outFmt)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("fullscreen");
    auto ps = loader->LoadPixelShader("cas");
    if (!vs.handle || !ps.handle)
    {
        st.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    if (!st.layout)
        st.layout = cache.GetOrCreateBindingLayoutFromReflection("CAS_v1", *vs.reflection, *ps.reflection, nv);
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
        // Unique cache key per format
        char pipeName[64];
        xr_sprintf(pipeName, "CAS_v1_fmt%u", (u32)outFmt);
        st.pipeline = cache.GetOrCreatePipeline(pipeName, desc, fb, nv);
        st.pipelineFormat = outFmt;
    }
    st.initialized = true;
}
} // namespace

VirtualResourceHandle setupCASPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle ldrInput,
    VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    CASPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !outputTarget.is_valid())
        return ldrInput;

    nvrhi::Format outFmt = nvrhi::Format::RGBA8_UNORM;
    if (GEnv.Backend && GEnv.Backend->GetBackBuffer())
        outFmt = GEnv.Backend->GetBackBuffer()->getDesc().format;

    EnsureCASPipeline(device->GetNVRHIDevice(), state, outFmt);
    if (!state.pipeline || !state.layout)
        return ldrInput;

    struct PassData
    {
        VirtualResourceHandle input, output;
        u32 width, height;
        CASPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        nvrhi::Format outFmt = nvrhi::Format::RGBA8_UNORM;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "CAS",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.outFmt = outFmt;
            data.input = pb.read(ldrInput, ResourceState::ShaderResource);
            data.output = pb.write(outputTarget, ResourceState::RenderTarget);
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* in = graph.GetPhysicalTexture(data.input);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !in || !out || !data.st)
                return;

            // Prefer actual output texture format (handles swapchain BGRA etc.)
            nvrhi::Format fmt = out->getDesc().format;
            EnsureCASPipeline(nv, *data.st, fmt);
            if (!data.st->pipeline || !data.st->layout)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("cas", ".ps");
            if (!vsR || !psR) return;

            struct alignas(16) PCB { float sx, sy, sharp, pad; };
            auto* cb = cache.GetOrCreateVolatileCB("CAS", "CASParams", sizeof(PCB), data.device);
            PCB p{float(data.width), float(data.height), ps_r_cas_sharpness, 0};
            if (cb) cmd->writeBuffer(cb, &p, sizeof(p));

            BindingSetBuilder bsb(*vsR, *psR, nv, "CAS");
            if (cb) bsb.ConstantBuffer("CASParams", cb);
            bsb.Texture("g_Color", in);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->layout, nv);
            if (!set) return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("CAS", fbDesc, nv);
            if (!fb) return;

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

} // namespace
