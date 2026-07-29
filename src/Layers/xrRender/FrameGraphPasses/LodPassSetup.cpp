#include "stdafx.h"
#include "LodPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FLOD.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/TextureDescrManager.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
#pragma pack(push, 1)
struct LodVertex
{
    Fvector p0, p1, n0, n1;
    u32 sun_af;
    u32 rgbh0, rgbh1;
    Fvector2 t0, t1;
};
#pragma pack(pop)

void EnsureLodPipeline(nvrhi::IDevice* nv, LodPassState& st)
{
    if (st.initialized)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        st.initialized = true;
        return;
    }
    auto vs = loader->LoadVertexShader("bindless_lod");
    auto ps = loader->LoadPixelShader("bindless_lod");
    if (!vs.handle || !ps.handle)
    {
        Msg("! [LodPass] bindless_lod shaders missing");
        st.initialized = true;
        return;
    }
    st.vs = vs.handle;
    st.ps = ps.handle;
    auto& cache = GetPassResourceCache();
    st.layout = cache.GetOrCreateBindingLayoutFromReflection("LodPass_v2", *vs.reflection, *ps.reflection, nv);

    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
            .setArraySize(2).setOffset(offsetof(LodVertex, p0)).setElementStride(sizeof(LodVertex)),
        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
            .setArraySize(2).setOffset(offsetof(LodVertex, n0)).setElementStride(sizeof(LodVertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::RGBA8_UNORM)
            .setArraySize(3).setOffset(offsetof(LodVertex, sun_af)).setElementStride(sizeof(LodVertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)
            .setArraySize(2).setOffset(offsetof(LodVertex, t0)).setElementStride(sizeof(LodVertex)),
    };
    st.inputLayout = cache.GetOrCreateInputLayout("LodPass", attribs, std::size(attribs), st.vs, nv);

    nvrhi::GraphicsPipelineDesc desc;
    desc.setVertexShader(st.vs);
    desc.setPixelShader(st.ps);
    desc.setInputLayout(st.inputLayout);
    desc.addBindingLayout(st.layout);
    desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    desc.renderState.depthStencilState.setDepthTestEnable(true);
    desc.renderState.depthStencilState.setDepthWriteEnable(true);
    desc.renderState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::GreaterOrEqual);
    desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
    desc.renderState.blendState.targets[0].setBlendEnable(false);

    nvrhi::FramebufferInfoEx fb;
    fb.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fb.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fb.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
    fb.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
    fb.depthFormat = nvrhi::Format::D32;
    st.pipeline = cache.GetOrCreatePipeline("LodPass_v2", desc, fb, nv);
    st.initialized = true;
}

void FillFacet(LodVertex* V, const FLOD::_face& F, float factor)
{
    for (int i = 0; i < 4; ++i)
    {
        V[i].p0 = F.v[i].v;
        V[i].p1 = F.v[i].v;
        V[i].n0 = F.N;
        V[i].n1 = F.N;
        V[i].t0 = F.v[i].t;
        V[i].t1 = F.v[i].t;
        V[i].rgbh0 = F.v[i].c_rgb_hemi;
        V[i].rgbh1 = F.v[i].c_rgb_hemi;
        u32 sun = F.v[i].c_sun;
        V[i].sun_af = color_rgba(sun, sun, 255, u8(clampr(iFloor(factor * 255.f), 0, 255)));
    }
}
} // namespace

DefaultOutputLayout setupLodPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& inputs,
    const xr_vector<LodImpostorInstance>* impostors,
    u32 width,
    u32 height,
    LodPassState& state)
{
    if (!device || !device->GetNVRHIDevice() || !impostors || impostors->empty())
        return inputs;

    EnsureLodPipeline(device->GetNVRHIDevice(), state);
    if (!state.pipeline || !state.layout)
        return inputs;

    struct PassData
    {
        VirtualResourceHandle color, normal, baseColor, worldPos, depth;
        DefaultOutputLayout outputs;
        const xr_vector<LodImpostorInstance>* impostors = nullptr;
        LodPassState* st = nullptr;
        fg::RenderDevice* device = nullptr;
        u32 width = 0, height = 0;
    };

    auto& pd = fg.addCallbackPass<PassData>(
        "LodImpostors",
        [&](FrameGraph& b, PassHandle ph, PassData& data) {
            RenderPassBuilder pb(b, ph);
            data.impostors = impostors;
            data.st = &state;
            data.device = device;
            data.width = width;
            data.height = height;
            data.color = pb.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.normal = pb.readWrite(inputs.normal, ResourceState::RenderTarget);
            data.baseColor = pb.readWrite(inputs.baseColor, ResourceState::RenderTarget);
            data.worldPos = pb.readWrite(inputs.worldPos, ResourceState::RenderTarget);
            data.depth = pb.readWrite(inputs.depth, ResourceState::DepthStencilWrite);
            data.outputs.albedo = data.color;
            data.outputs.normal = data.normal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.worldPos = data.worldPos;
            data.outputs.depth = data.depth;
        },
        [](const PassData& data, const FrameGraph& graph, fg::RenderContext* ctx) {
            auto* cmd = ctx->GetCommandList();
            auto* nv = cmd ? cmd->getDevice() : nullptr;
            if (!cmd || !nv || !data.st || !data.impostors)
                return;

            auto* colorRT = graph.GetPhysicalTexture(data.color);
            auto* normalRT = graph.GetPhysicalTexture(data.normal);
            auto* baseRT = data.baseColor.is_valid() ? graph.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldRT = data.worldPos.is_valid() ? graph.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* depthRT = graph.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT || !normalRT || !baseRT || !worldRT)
                return;

            const u32 maxVerts = u32(data.impostors->size()) * 4;
            if (!data.st->dynamicVB || data.st->dynamicVBCapacity < maxVerts)
            {
                nvrhi::BufferDesc bd;
                bd.byteSize = maxVerts * sizeof(LodVertex);
                bd.debugName = "LodPass_DynamicVB";
                bd.isVertexBuffer = true;
                bd.initialState = nvrhi::ResourceStates::VertexBuffer;
                bd.keepInitialState = true;
                data.st->dynamicVB = nv->createBuffer(bd);
                data.st->dynamicVBCapacity = maxVerts;
            }

            xr_vector<LodVertex> verts;
            verts.reserve(maxVerts);
            xr_vector<u32> drawCounts;
            drawCounts.reserve(data.impostors->size());

            for (const auto& inst : *data.impostors)
            {
                if (!inst.lod)
                    continue;
                Fvector Ldir;
                Fvector center;
                inst.world.transform_tiny(center, inst.lod->vis.sphere.P);
                Ldir.sub(center, Device.vCameraPosition);
                Ldir.normalize_safe();

                int best_id = 0;
                float best_dot = Ldir.dotproduct(inst.lod->facets[0].N);
                for (int i = 1; i < 8; ++i)
                {
                    float d = Ldir.dotproduct(inst.lod->facets[i].N);
                    if (d > best_dot) { best_id = i; best_dot = d; }
                }

                LodVertex V[4];
                FillFacet(V, inst.lod->facets[best_id], 0.f);
                for (int i = 0; i < 4; ++i)
                {
                    inst.world.transform_tiny(V[i].p0, V[i].p0);
                    V[i].p1 = V[i].p0;
                    inst.world.transform_dir(V[i].n0, V[i].n0);
                    V[i].n1 = V[i].n0;
                    verts.push_back(V[i]);
                }
                drawCounts.push_back(1);
            }

            if (verts.empty())
                return;

            cmd->writeBuffer(data.st->dynamicVB, verts.data(), verts.size() * sizeof(LodVertex));

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.addColorAttachment(normalRT);
            fbDesc.addColorAttachment(baseRT);
            fbDesc.addColorAttachment(worldRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto fb = cache.GetOrCreateFramebuffer("LodPass", fbDesc, nv);
            if (!fb)
                return;

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsR = loader->GetCachedReflection("bindless_lod", ".vs");
            auto* psR = loader->GetCachedReflection("bindless_lod", ".ps");
            if (!vsR || !psR)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmd->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            nvrhi::ITexture* baseTex = cache.GetDummyContactHistory(nv);
            nvrhi::ITexture* hemiTex = baseTex;
            nvrhi::ITexture* pbrTex = baseTex;
            auto* resMgr = data.device->GetFGResourceManager();
            auto* texMgr = resMgr ? resMgr->GetTextureManager() : nullptr;

            u32 vertOffset = 0;
            u32 instIdx = 0;
            for (const auto& inst : *data.impostors)
            {
                if (!inst.lod)
                    continue;

                if (texMgr && inst.lod->textureName.size())
                {
                    xr_string lodTexName = inst.lod->textureName.c_str();
                    if (size_t comma = lodTexName.find(','); comma != xr_string::npos)
                        lodTexName.resize(comma);
                    auto h = texMgr->LoadTexture(lodTexName.c_str());
                    if (auto* t = texMgr->GetNVRHITexture(h))
                        baseTex = t;
                    xr_string hemiName = lodTexName;
                    hemiName += "_nm";
                    auto hh = texMgr->LoadTexture(hemiName.c_str());
                    if (auto* t = texMgr->GetNVRHITexture(hh))
                        hemiTex = t;
                    else
                        hemiTex = baseTex;

                    pbrTex = baseTex;
                    shared_str pbrName = TextureDescr.GetPBRName(shared_str(lodTexName.c_str()));
                    if (pbrName.size())
                    {
                        auto hp = texMgr->LoadTexture(pbrName.c_str());
                        if (auto* t = texMgr->GetNVRHITexture(hp))
                            pbrTex = t;
                    }
                    else
                    {
                        xr_string fallback = lodTexName;
                        fallback += "_pbr";
                        auto hp = texMgr->LoadTexture(fallback.c_str());
                        if (auto* t = texMgr->GetNVRHITexture(hp))
                            pbrTex = t;
                    }
                }

                BindingSetBuilder bsb(*vsR, *psR, nv, "LodPass");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.Texture("s_base", baseTex);
                bsb.Texture("s_hemi", hemiTex);
                bsb.Texture("s_pbr", pbrTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.st->layout, nv);
                if (!set)
                {
                    vertOffset += 4;
                    ++instIdx;
                    continue;
                }

                static const u16 kQuadIdx[6] = {0, 1, 2, 0, 2, 3};
                if (!data.st->quadIB)
                {
                    nvrhi::BufferDesc ibDesc;
                    ibDesc.byteSize = sizeof(kQuadIdx);
                    ibDesc.debugName = "LodPass_QuadIB";
                    ibDesc.isIndexBuffer = true;
                    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
                    ibDesc.keepInitialState = true;
                    data.st->quadIB = nv->createBuffer(ibDesc);
                    cmd->writeBuffer(data.st->quadIB, kQuadIdx, sizeof(kQuadIdx));
                }

                nvrhi::GraphicsState gs;
                gs.pipeline = data.st->pipeline;
                gs.framebuffer = fb;
                gs.bindings = {set};
                gs.vertexBuffers = {{data.st->dynamicVB, 0, vertOffset * sizeof(LodVertex)}};
                gs.indexBuffer = {data.st->quadIB, nvrhi::Format::R16_UINT, 0};
                gs.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(data.width), float(data.height)));
                cmd->setGraphicsState(gs);
                cmd->drawIndexed(nvrhi::DrawArguments{6, 1, 0, 0, 0});

                vertOffset += 4;
                ++instIdx;
            }
        });

    return pd.outputs;
}

} // namespace
