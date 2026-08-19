#include "stdafx.h"
#include "GlowPassSetup.h"
#include "PassVertexFormats.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render::fg::passes {
using namespace framegraph;

namespace {
xr_vector<void*> g_glowRegistry;
GlowCollectFn g_glowCollect = nullptr;

void EnsureGlowResources(nvrhi::IDevice* nv, GlowPassState& state)
{
    if (state.initialized)
        return;

    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
        return;
    auto vsResult = shaderLoader->LoadVertexShader("glow_forward");
    auto psResult = shaderLoader->LoadPixelShader("glow_forward");
    if (!vsResult.handle || !psResult.handle)
        return;
    state.vs = vsResult.handle;
    state.ps = psResult.handle;

    auto& cache = GetPassResourceCache();
    state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "GlowBillboard_v7", *vsResult.reflection, *psResult.reflection, nv);
    if (!state.bindingLayout)
        return;

    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(SunVertex, position))
            .setElementStride(sizeof(SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setOffset(offsetof(SunVertex, color))
            .setElementStride(sizeof(SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(SunVertex, u))
            .setElementStride(sizeof(SunVertex)),
    };
    state.inputLayout = cache.GetOrCreateInputLayout("GlowBillboard_v7", attribs, std::size(attribs), state.vs, nv);

    nvrhi::RenderState rs;
    rs.blendState.targets[0].enableBlend();
    rs.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
    rs.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::One);
    rs.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
    rs.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
    rs.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::One);
    rs.depthStencilState.setDepthTestEnable(true);
    rs.depthStencilState.setDepthWriteEnable(false);
    rs.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::GreaterOrEqual);
    rs.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::GraphicsPipelineDesc pso;
    pso.inputLayout = state.inputLayout;
    pso.VS = state.vs;
    pso.PS = state.ps;
    pso.bindingLayouts = { state.bindingLayout };
    pso.renderState = rs;
    pso.primType = nvrhi::PrimitiveType::TriangleList;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.depthFormat = nvrhi::Format::D32;
    state.pipeline = cache.GetOrCreatePipeline("GlowBillboard_v7", pso, fbInfo, nv);

    u16 indices[6] = { 0, 1, 2, 2, 1, 3 };
    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = sizeof(indices);
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    ibDesc.debugName = "GlowBillboardIB";
    state.ib = nv->createBuffer(ibDesc);
    {
        auto cmd = nv->createCommandList();
        cmd->open();
        cmd->writeBuffer(state.ib, indices, sizeof(indices));
        cmd->close();
        nv->executeCommandList(cmd);
    }

    nvrhi::TextureDesc td;
    td.width = 1;
    td.height = 1;
    td.format = nvrhi::Format::RGBA8_UNORM;
    td.initialState = nvrhi::ResourceStates::ShaderResource;
    td.keepInitialState = true;
    td.debugName = "GlowPlaceholder";
    state.placeholderTex = nv->createTexture(td);
    const u32 white = 0xFFFFFFFFu;
    {
        auto cmd = nv->createCommandList();
        cmd->open();
        cmd->writeTexture(state.placeholderTex, 0, 0, &white, 4);
        cmd->close();
        nv->executeCommandList(cmd);
    }

    state.initialized = state.pipeline && state.ib && state.placeholderTex;
}

void EnsureVB(nvrhi::IDevice* nv, GlowPassState& state, u32 vertCount)
{
    if (state.vb && state.vbCapacityVerts >= vertCount)
        return;
    nvrhi::BufferDesc vd;
    vd.byteSize = sizeof(SunVertex) * _max(vertCount, 64u);
    vd.isVertexBuffer = true;
    vd.initialState = nvrhi::ResourceStates::VertexBuffer;
    vd.keepInitialState = true;
    vd.debugName = "GlowBillboardVB";
    state.vb = nv->createBuffer(vd);
    state.vbCapacityVerts = (u32)(vd.byteSize / sizeof(SunVertex));
}
}

void GlowRegistry_Register(void* glow)
{
    if (!glow)
        return;
    if (std::find(g_glowRegistry.begin(), g_glowRegistry.end(), glow) == g_glowRegistry.end())
        g_glowRegistry.push_back(glow);
}

void GlowRegistry_Unregister(void* glow)
{
    g_glowRegistry.erase(std::remove(g_glowRegistry.begin(), g_glowRegistry.end(), glow), g_glowRegistry.end());
}

void GlowRegistry_SetCollect(GlowCollectFn fn)
{
    g_glowCollect = fn;
}

void GlowRegistry_Collect(xr_vector<GlowBillboard>& out)
{
    out.clear();
    if (!g_glowCollect)
        return;
    for (void* g : g_glowRegistry)
    {
        GlowBillboard b{};
        if (g_glowCollect(g, b) && b.radius > 0.f)
            out.push_back(b);
    }
}

framegraph::VirtualResourceHandle setupGlowBillboardPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    u32 width,
    u32 height,
    GlowPassState& state)
{
    xr_vector<GlowBillboard> glows;
    GlowRegistry_Collect(glows);
    if (glows.empty() || !device)
        return sceneColor;

    nvrhi::IDevice* nv = device->GetNVRHIDevice();
    EnsureGlowResources(nv, state);
    if (!state.initialized)
        return sceneColor;

    EnsureVB(nv, state, (u32)glows.size() * 4u);

    struct PassData
    {
        VirtualResourceHandle color;
        VirtualResourceHandle depth;
        GlowPassState* state = nullptr;
        u32 width = 0, height = 0;
        xr_vector<GlowBillboard> glows;
    };

    auto& pass = fg.addCallbackPass<PassData>(
        "Glow Billboards",
        [&](FrameGraph& builder, PassHandle passHandle, PassData& data) {
            RenderPassBuilder pb(builder, passHandle);
            data.color = pb.readWrite(sceneColor, ResourceState::RenderTarget);
            if (depth.is_valid())
                data.depth = pb.read(depth, ResourceState::DepthStencilRead);
            pb.sideEffects();
            data.state = &state;
            data.width = width;
            data.height = height;
            data.glows = glows;
        },
        [](const PassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            if (!data.state || !ctx || data.glows.empty())
                return;
            auto* colorTex = fgGraph.GetPhysicalTexture(data.color);
            auto* depthTex = data.depth.is_valid() ? fgGraph.GetPhysicalTexture(data.depth) : nullptr;
            if (!colorTex)
                return;

            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd->getDevice();
            auto* renderDevice = ctx->GetDevice();
            if (!nv || !renderDevice)
                return;

            if (depthTex &&
                (depthTex->getDesc().width != colorTex->getDesc().width ||
                 depthTex->getDesc().height != colorTex->getDesc().height))
                return;
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTex);
            if (depthTex)
                fbDesc.setDepthAttachment(depthTex);
            auto fb = nv->createFramebuffer(fbDesc);
            if (!fb)
                return;

            auto& cache = GetPassResourceCache();
            auto* vsRefl = RImplementation.GetShaderLoader()->GetCachedReflection("glow_forward", ".vs");
            auto* psRefl = RImplementation.GetShaderLoader()->GetCachedReflection("glow_forward", ".ps");
            if (!vsRefl || !psRefl)
                return;

            auto dynamicCBBuffer = cache.GetOrCreateVolatileCB(
                "GlowBillboard", "DynamicTransforms", sizeof(DynamicTransforms), renderDevice);
            DynamicTransforms dynCB{};
            FillDynamicTransforms(dynCB, Fidentity);
            cmd->writeBuffer(dynamicCBBuffer, &dynCB, sizeof(dynCB));

            Fvector right = Device.vCameraRight;
            Fvector up = Device.vCameraTop;

            for (const auto& g : data.glows)
            {
                Fvector sx, sy;
                sx.mul(right, g.radius);
                sy.mul(up, g.radius);
                const u32 c = g.color.get();
                SunVertex verts[4];
                verts[0].position = { g.pos.x + sx.x - sy.x, g.pos.y + sx.y - sy.y, g.pos.z + sx.z - sy.z };
                verts[0].color = c; verts[0].u = 0.f; verts[0].v = 0.f;
                verts[1].position = { g.pos.x + sx.x + sy.x, g.pos.y + sx.y + sy.y, g.pos.z + sx.z + sy.z };
                verts[1].color = c; verts[1].u = 0.f; verts[1].v = 1.f;
                verts[2].position = { g.pos.x - sx.x - sy.x, g.pos.y - sx.y - sy.y, g.pos.z - sx.z - sy.z };
                verts[2].color = c; verts[2].u = 1.f; verts[2].v = 0.f;
                verts[3].position = { g.pos.x - sx.x + sy.x, g.pos.y - sx.y + sy.y, g.pos.z - sx.z + sy.z };
                verts[3].color = c; verts[3].u = 1.f; verts[3].v = 1.f;
                cmd->writeBuffer(data.state->vb, verts, sizeof(verts));

                nvrhi::ITexture* tex = data.state->placeholderTex.Get();
                if (g.texture.size())
                {
                    auto* texManager = renderDevice->GetFGResourceManager()
                        ? renderDevice->GetFGResourceManager()->GetTextureManager() : nullptr;
                    if (texManager)
                    {
                        if (auto* t = texManager->GetNVRHITexture(texManager->LoadTexture(g.texture.c_str())))
                            tex = t;
                    }
                }

                BindingSetBuilder bsb(*vsRefl, *psRefl, nv, "GlowBillboard");
                bsb.ConstantBuffer("dynamic_transforms", dynamicCBBuffer)
                   .Texture("s_sun", tex);
                auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.state->bindingLayout, nv);

                nvrhi::GraphicsState gs;
                gs.pipeline = data.state->pipeline;
                gs.framebuffer = fb;
                gs.bindings.push_back(bindingSet);
                gs.vertexBuffers = { { data.state->vb, 0, 0 } };
                gs.indexBuffer = { data.state->ib, nvrhi::Format::R16_UINT, 0 };
                gs.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
                    nvrhi::Viewport((float)data.width, (float)data.height));
                cmd->setGraphicsState(gs);
                cmd->drawIndexed(nvrhi::DrawArguments{ 6, 1, 0, 0, 0 });
            }
        });

    return pass.color;
}

}
