#include "stdafx.h"
#include "SSSSSPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

extern ENGINE_API int ps_r_sssss;
extern ENGINE_API float ps_r_sssss_width;
extern ENGINE_API float ps_r_sssss_strength;
extern ENGINE_API int ps_r_sssss_debug;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct alignas(16) SSSSSCB
{
    float texelX, texelY, dirX, dirY;
    float widthScale, strength, debugMode, pad0;
};
static_assert(sizeof(SSSSSCB) % 16 == 0);

void InitSSSSS(nvrhi::IDevice* nv, SSSSSPassState& st)
{
    if (st.initialized && st.blurPipeline && st.blurLayout && st.pipeVersion == 3)
        return;
    st.initialized = false;
    st.pipeVersion = 0;
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
    auto ps = loader->LoadPixelShader("sssss_blur");
    if (!vs.handle || !ps.handle || !vs.reflection || !ps.reflection)
    {
        st.initialized = true;
        return;
    }
    auto& cache = GetPassResourceCache();
    st.blurLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "SSSSSBlur_v3", *vs.reflection, *ps.reflection, nv);
    if (st.blurLayout)
    {
        nvrhi::GraphicsPipelineDesc desc;
        desc.setVertexShader(vs.handle);
        desc.setPixelShader(ps.handle);
        desc.addBindingLayout(st.blurLayout);
        desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        desc.renderState.blendState.targets[0].setBlendEnable(false);
        desc.renderState.depthStencilState.setDepthTestEnable(false);
        desc.renderState.depthStencilState.setDepthWriteEnable(false);
        desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
        nvrhi::FramebufferInfoEx fb;
        fb.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        st.blurPipeline = cache.GetOrCreatePipeline("SSSSSBlur_v3", desc, fb, nv);
    }
    st.initialized = true;
    st.pipeVersion = 3;
}

void DrawSSSSSFullscreen(
    nvrhi::ICommandList* cmd,
    nvrhi::IDevice* nv,
    nvrhi::GraphicsPipelineHandle pipe,
    nvrhi::BindingSetHandle set,
    nvrhi::ITexture* out,
    u32 w, u32 h,
    const char* fbName)
{
    if (!cmd || !pipe || !set || !out)
        return;
    auto& cache = GetPassResourceCache();
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(out);
    auto fb = cache.GetOrCreateFramebuffer(fbName, fbDesc, nv);
    nvrhi::GraphicsState gs;
    gs.pipeline = pipe;
    gs.framebuffer = fb;
    gs.bindings = { set };
    gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(w), float(h)));
    cmd->setGraphicsState(gs);
    cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
}
} // namespace

VirtualResourceHandle setupSSSSSPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    u32 width,
    u32 height,
    SSSSSPassState& state)
{
    if (!ps_r_sssss || !device || !device->GetNVRHIDevice())
        return sceneColor;
    if (!sceneColor.is_valid() || !depth.is_valid() || !normal.is_valid() || !baseColor.is_valid())
        return sceneColor;

    InitSSSSS(device->GetNVRHIDevice(), state);
    if (!state.blurPipeline || !state.blurLayout)
        return sceneColor;

    ResourceDesc tmpDesc;
    tmpDesc.type = ResourceDesc::Type::Texture2D;
    tmpDesc.width = width;
    tmpDesc.height = height;
    tmpDesc.format = nvrhi::Format::RGBA16_FLOAT;
    tmpDesc.isRenderTarget = true;
    tmpDesc.isTransient = true;
    tmpDesc.debugName = "rt_SSSSS_Temp";
    auto tempRT = fg.CreateTexture("rt_SSSSS_Temp", tmpDesc);

    ResourceDesc outDesc = tmpDesc;
    outDesc.debugName = "rt_SSSSS_Out";
    auto output = fg.CreateTexture("rt_SSSSS_Out", outDesc);

    struct PassData
    {
        VirtualResourceHandle color, depth, normal, base, temp, output;
        u32 width = 0, height = 0;
        SSSSSPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        float widthScale = 1.0f;
        float strength = 1.0f;
        float debugMode = 0.0f;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "SSSSS",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.widthScale = ps_r_sssss_width;
            data.strength = ps_r_sssss_strength;
            data.debugMode = ps_r_sssss_debug ? 1.f : 0.f;
            data.color = pb.read(sceneColor, ResourceState::ShaderResource);
            data.depth = pb.read(depth, ResourceState::ShaderResource);
            data.normal = pb.read(normal, ResourceState::ShaderResource);
            data.base = pb.read(baseColor, ResourceState::ShaderResource);
            data.temp = pb.write(tempRT, ResourceState::RenderTarget);
            data.output = pb.write(output, ResourceState::RenderTarget);
            pb.sideEffects();
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            auto* color = graph.GetPhysicalTexture(data.color);
            auto* depthTex = graph.GetPhysicalTexture(data.depth);
            auto* normalTex = graph.GetPhysicalTexture(data.normal);
            auto* baseTex = graph.GetPhysicalTexture(data.base);
            auto* tempTex = graph.GetPhysicalTexture(data.temp);
            auto* out = graph.GetPhysicalTexture(data.output);
            if (!cmd || !nv || !color || !depthTex || !normalTex || !baseTex || !tempTex || !out || !data.st)
                return;

            auto& cache = GetPassResourceCache();
            auto* loader = GEnv.Render->GetShaderLoader();
            if (!loader)
                return;
            auto* vsR = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psR = loader->GetCachedReflection("sssss_blur", ".ps");
            if (!vsR || !psR)
                return;

            auto blurPass = [&](nvrhi::ITexture* src, nvrhi::ITexture* dst, float dirX, float dirY, const char* name) {
                SSSSSCB cb{
                    1.f / float(data.width), 1.f / float(data.height),
                    dirX, dirY,
                    data.widthScale, data.strength,
                    data.debugMode, 0.f};
                auto* cbuf = cache.GetOrCreateVolatileCB("SSSSS", name, sizeof(SSSSSCB), data.device);
                if (!cbuf)
                    return;
                cmd->writeBuffer(cbuf, &cb, sizeof(cb));
                BindingSetBuilder bsb(*vsR, *psR, nv, name);
                bsb.ConstantBuffer("SSSSSParams", cbuf)
                    .Texture("g_Color", src)
                    .Texture("g_Depth", depthTex)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_Base", baseTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->blurLayout, nv);
                if (set)
                    DrawSSSSSFullscreen(cmd, nv, data.st->blurPipeline, set, dst, data.width, data.height, name);
            };

            blurPass(color, tempTex, 1.f, 0.f, "SSSSS_H_v3");
            cmd->setTextureState(tempTex, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
            blurPass(tempTex, out, 0.f, 1.f, "SSSSS_V_v3");
            cmd->setTextureState(out, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
        });

    return pd.output;
}

} // namespace
