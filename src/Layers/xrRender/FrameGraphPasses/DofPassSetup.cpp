#include "stdafx.h"
#include "DofPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct alignas(16) DofCB
{
    Fvector4 dof_params;
    Fvector4 dof_kernel;
    Fmatrix m_InvVP;
    Fmatrix m_V;
};
static_assert(sizeof(DofCB) % 16 == 0);

void InitDof(nvrhi::IDevice* nv, DofPassState& st)
{
    if (st.initialized && st.pipeline && st.layout)
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
    auto ps = loader->LoadPixelShader("dof");
    if (!vs.handle || !ps.handle)
    {
        st.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    st.layout = cache.GetOrCreateBindingLayoutFromReflection("Dof_v1", *vs.reflection, *ps.reflection, nv);
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
        fb.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        st.pipeline = cache.GetOrCreatePipeline("Dof_v1", desc, fb, nv);
    }
    st.initialized = true;
}
} // namespace

VirtualResourceHandle setupDofPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    u32 width,
    u32 height,
    DofPassState& state)
{
    if (!device || !device->GetNVRHIDevice())
        return sceneColor;
    if (!ps_r2_ls_flags.test(R2FLAG_DOF))
        return sceneColor;

    InitDof(device->GetNVRHIDevice(), state);
    if (!state.pipeline || !state.layout || !depth.is_valid())
        return sceneColor;

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isTransient = false;
    outDesc.debugName = "rt_DOF";
    auto output = fg.CreateTexture("rt_DOF", outDesc);

    struct PassData
    {
        VirtualResourceHandle color, depth, output;
        DofPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        u32 width = 0, height = 0;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "DOF",
        [&, output](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.color = pb.read(sceneColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.output = pb.write(output, ResourceState::RenderTarget);
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* st = data.st;
            if (!cmd || !nv || !st || !st->pipeline)
                return;
            auto* color = graph.GetPhysicalTexture(data.color);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!color || !depthTex || !out)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("dof", ".ps");
            if (!vsR || !psR)
                return;

            auto* cb = cache.GetOrCreateVolatileCB("DOF", "DofParams", sizeof(DofCB), data.device, 8);
            if (!cb)
                return;

            DofCB params{};
            params.dof_params.set(ps_r2_dof.x, ps_r2_dof.y, ps_r2_dof.z, ps_r2_dof_sky);
            params.dof_kernel.set(0.5f / float(data.width), 0.5f / float(data.height), ps_r2_dof_kernel_size, 0.f);
            params.m_InvVP = Device.mInvFullTransform;
            params.m_V = Device.mView;
            cmd->writeBuffer(cb, &params, sizeof(params));

            BindingSetBuilder bsb(*vsR, *psR, nv, "DOF");
            bsb.ConstantBuffer("DofParams", cb);
            bsb.Texture("g_Color", color).Texture("g_Depth", depthTex);
            auto set = cache.GetOrCreateBindingSet(bsb.Build(), st->layout, nv);
            if (!set)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(out);
            auto fb = cache.GetOrCreateFramebuffer("DofFB", fbDesc, nv);

            nvrhi::GraphicsState gs;
            gs.pipeline = st->pipeline;
            gs.framebuffer = fb;
            gs.bindings = {set};
            gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(data.width), float(data.height)));
            cmd->setGraphicsState(gs);
            cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        });

    return pd.output;
}

} // namespace
