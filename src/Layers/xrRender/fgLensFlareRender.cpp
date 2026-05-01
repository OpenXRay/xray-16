#include "stdafx.h"

#include "fgLensFlareRender.h"

#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/dxLensFlareRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg
{
namespace
{
constexpr size_t kCBSize = 256;
constexpr u32 kMaxFlares = 24;

struct EffectsWorldCB
{
    Fmatrix m_VP;
    Fmatrix m_W;
};
} // namespace

FGLensFlareRender::FGLensFlareRender()
{
    InitResources();
}

FGLensFlareRender::~FGLensFlareRender() = default;

void FGLensFlareRender::InitResources()
{
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("effects_world_textured", "main");
    R_ASSERT2(vsResult.handle, "FGLensFlareRender: failed to load effects_world_textured.vs");
    m_vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("effects_world_textured", "main");
    R_ASSERT2(psResult.handle, "FGLensFlareRender: failed to load effects_world_textured.ps");
    m_ps = psResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::RGBA8_UNORM).setOffset(12).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(16).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGLensFlareRender: createInputLayout failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = kCBSize;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGLensFlareRender_CB";
    m_constantBuffer = m_device->createBuffer(cbDesc);
    R_ASSERT2(m_constantBuffer, "FGLensFlareRender: createBuffer(CB) failed");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    samplerDesc.setAllFilters(true);
    m_sampler = m_device->createSampler(samplerDesc);
    R_ASSERT2(m_sampler, "FGLensFlareRender: createSampler failed");

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);
    R_ASSERT2(m_bindingLayout, "FGLensFlareRender: createBindingLayout failed");

    nvrhi::FramebufferInfo fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.setDepthFormat(nvrhi::Format::D32);
    fbInfo.setSampleCount(1);

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.VS = m_vs;
    pipelineDesc.PS = m_ps;
    pipelineDesc.inputLayout = m_inputLayout;
    pipelineDesc.bindingLayouts = { m_bindingLayout };
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::One)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::One);

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, fbInfo);
    R_ASSERT2(m_pipeline, "FGLensFlareRender: createGraphicsPipeline failed");
}

void FGLensFlareRender::Copy(ILensFlareRender& _in)
{
    *this = *static_cast<FGLensFlareRender*>(&_in);
}

void FGLensFlareRender::Clear()
{
    m_vertices.clear();
    m_indices.clear();
    m_batches.clear();
}

nvrhi::ITexture* FGLensFlareRender::ResolveTexture(const shared_str& name)
{
    if (!name || !name.size())
        return nullptr;

    auto it = m_textureCache.find(name);
    if (it != m_textureCache.end())
        return it->second;

    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* textureManager = fgRenderer->GetRenderDevice()->GetFGResourceManager()->GetTextureManager();
    nvrhi::TextureHandle handle = textureManager->GetNVRHITexture(textureManager->LoadTexture(name.c_str()));
    m_textureCache.emplace(name, handle);
    return handle;
}

void FGLensFlareRender::PushQuad(const Fvector& center, const Fvector& vecX, const Fvector& vecY, u32 color, nvrhi::ITexture* tex)
{
    if (!tex)
        return;

    const u32 base = static_cast<u32>(m_vertices.size());
    const u32 iStart = static_cast<u32>(m_indices.size());

    Vertex v0;
    v0.x = center.x + vecX.x - vecY.x;
    v0.y = center.y + vecX.y - vecY.y;
    v0.z = center.z + vecX.z - vecY.z;
    v0.color = color;
    v0.u = 0;
    v0.v = 0;
    Vertex v1;
    v1.x = center.x + vecX.x + vecY.x;
    v1.y = center.y + vecX.y + vecY.y;
    v1.z = center.z + vecX.z + vecY.z;
    v1.color = color;
    v1.u = 0;
    v1.v = 1;
    Vertex v2;
    v2.x = center.x - vecX.x - vecY.x;
    v2.y = center.y - vecX.y - vecY.y;
    v2.z = center.z - vecX.z - vecY.z;
    v2.color = color;
    v2.u = 1;
    v2.v = 0;
    Vertex v3;
    v3.x = center.x - vecX.x + vecY.x;
    v3.y = center.y - vecX.y + vecY.y;
    v3.z = center.z - vecX.z + vecY.z;
    v3.color = color;
    v3.u = 1;
    v3.v = 1;

    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
    m_vertices.push_back(v3);

    m_indices.push_back(static_cast<u16>(base + 0));
    m_indices.push_back(static_cast<u16>(base + 1));
    m_indices.push_back(static_cast<u16>(base + 2));
    m_indices.push_back(static_cast<u16>(base + 1));
    m_indices.push_back(static_cast<u16>(base + 3));
    m_indices.push_back(static_cast<u16>(base + 2));

    Batch b{};
    b.indexOffset = iStart;
    b.indexCount = 6;
    b.texture = tex;
    m_batches.push_back(b);
}

void FGLensFlareRender::Render(CLensFlare& owner, BOOL bSun, BOOL bFlares, BOOL bGradient)
{
    Clear();
    if (!owner.m_Current)
        return;

    Fcolor dwLight;
    dwLight.set(owner.LightColor);

    const float fDistance = g_pGamePersistent->Environment().CurrentEnv.far_plane * 0.75f;

    Fvector vecSx, vecSy;

    if (bSun && owner.m_Current->m_Flags.is(CLensFlareDescriptor::flSource))
    {
        vecSx.mul(owner.vecX, owner.m_Current->m_Source.fRadius * fDistance);
        vecSy.mul(owner.vecY, owner.m_Current->m_Source.fRadius * fDistance);

        Fcolor color;
        if (owner.m_Current->m_Source.ignore_color)
            color.set(1.f, 1.f, 1.f, 1.f);
        else
            color.set(dwLight);
        color.a *= owner.m_StateBlend;

        auto* flare = static_cast<dxFlareRender*>(&*owner.m_Current->m_Source.m_pRender);
        nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
        PushQuad(owner.vecLight, vecSx, vecSy, color.get(), tex);
    }

    if (owner.fBlend >= EPS_L)
    {
        if (bFlares && owner.m_Current->m_Flags.is(CLensFlareDescriptor::flFlare))
        {
            Fvector vecDx, vecDy;
            vecDx.normalize(owner.vecAxis);
            vecDy.crossproduct(vecDx, owner.vecDir);

            for (auto& F : owner.m_Current->m_Flares)
            {
                Fvector vec;
                vec.mul(owner.vecAxis, F.fPosition);
                vec.add(owner.vecCenter);
                vecSx.mul(vecDx, F.fRadius * fDistance);
                vecSy.mul(vecDy, F.fRadius * fDistance);

                const float cl = F.fOpacity * owner.fBlend * owner.m_StateBlend;
                Fcolor color;
                color.set(dwLight);
                color.mul_rgba(cl);

                auto* flare = static_cast<dxFlareRender*>(&*F.m_pRender);
                nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
                PushQuad(vec, vecSx, vecSy, color.get(), tex);
            }
        }

        if (bGradient && owner.fGradientValue >= EPS_L && owner.m_Current->m_Flags.is(CLensFlareDescriptor::flGradient))
        {
            vecSx.mul(owner.vecX, owner.m_Current->m_Gradient.fRadius * owner.fGradientValue * fDistance);
            vecSy.mul(owner.vecY, owner.m_Current->m_Gradient.fRadius * owner.fGradientValue * fDistance);

            Fcolor color;
            color.set(dwLight);
            color.mul_rgba(owner.fGradientValue * owner.m_StateBlend);

            auto* flare = static_cast<dxFlareRender*>(&*owner.m_Current->m_Gradient.m_pRender);
            nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
            PushQuad(owner.vecLight, vecSx, vecSy, color.get(), tex);
        }
    }
}

void FGLensFlareRender::EnsureGeometryCapacity(size_t vertexCount, size_t indexCount)
{
    if (vertexCount > m_vertexCapacity || !m_vertexBuffer)
    {
        const size_t newCap = std::max<size_t>(vertexCount * 2, kMaxFlares * 4);
        nvrhi::BufferDesc d;
        d.byteSize = newCap * sizeof(Vertex);
        d.isVertexBuffer = true;
        d.debugName = "FGLensFlareRender_VB";
        d.initialState = nvrhi::ResourceStates::VertexBuffer;
        d.keepInitialState = true;
        m_vertexBuffer = m_device->createBuffer(d);
        m_vertexCapacity = newCap;
    }
    if (indexCount > m_indexCapacity || !m_indexBuffer)
    {
        const size_t newCap = std::max<size_t>(indexCount * 2, kMaxFlares * 6);
        nvrhi::BufferDesc d;
        d.byteSize = newCap * sizeof(u16);
        d.isIndexBuffer = true;
        d.debugName = "FGLensFlareRender_IB";
        d.initialState = nvrhi::ResourceStates::IndexBuffer;
        d.keepInitialState = true;
        m_indexBuffer = m_device->createBuffer(d);
        m_indexCapacity = newCap;
    }
}

void FGLensFlareRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer)
{
    if (m_batches.empty())
        return;

    EnsureGeometryCapacity(m_vertices.size(), m_indices.size());

    if (!m_vertices.empty())
        cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
    if (!m_indices.empty())
        cmdList->writeBuffer(m_indexBuffer, m_indices.data(), m_indices.size() * sizeof(u16));

    EffectsWorldCB cb{};
    cb.m_VP = Device.mFullTransform;
    cb.m_W.identity();
    u8 cbData[kCBSize] = {};
    std::memcpy(cbData, &cb, sizeof(cb));
    cmdList->writeBuffer(m_constantBuffer, cbData, kCBSize);

    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_indexBuffer, nvrhi::ResourceStates::IndexBuffer);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::ITexture* currentTexture = nullptr;
    nvrhi::BindingSetHandle currentBindingSet;

    nvrhi::VertexBufferBinding vertexBinding;
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.slot = 0;
    vertexBinding.offset = 0;

    for (const Batch& b : m_batches)
    {
        if (b.indexCount == 0 || !b.texture)
            continue;

        if (b.texture != currentTexture)
        {
            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
                nvrhi::BindingSetItem::Texture_SRV(0, b.texture),
                nvrhi::BindingSetItem::Sampler(0, m_sampler),
            };
            currentBindingSet = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
            R_ASSERT2(currentBindingSet, "FGLensFlareRender: createBindingSet failed");
            currentTexture = b.texture;
        }

        nvrhi::GraphicsState state;
        state.pipeline = m_pipeline;
        state.framebuffer = framebuffer;
        state.bindings = { currentBindingSet };
        state.vertexBuffers = { vertexBinding };
        state.indexBuffer.buffer = m_indexBuffer;
        state.indexBuffer.format = nvrhi::Format::R16_UINT;
        state.indexBuffer.offset = 0;
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height)));
        cmdList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.vertexCount = b.indexCount;
        args.instanceCount = 1;
        args.startIndexLocation = b.indexOffset;
        cmdList->drawIndexed(args);
    }

    Clear();
}
} // namespace xray::render::fg
