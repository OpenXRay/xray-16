#include "stdafx.h"

#include "fgThunderboltRender.h"

#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/dxThunderboltDescRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render::fg
{
namespace
{
constexpr size_t kCBSize = 256;

struct EffectsWorldCB
{
    Fmatrix m_VP;
    Fmatrix m_W;
};
} // namespace

FGThunderboltRender::FGThunderboltRender()
{
    InitResources();
}

FGThunderboltRender::~FGThunderboltRender() = default;

void FGThunderboltRender::InitResources()
{
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("effects_world_textured", "main");
    R_ASSERT2(vsResult.handle, "FGThunderboltRender: failed to load effects_world_textured.vs");
    m_vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("effects_world_textured", "main");
    R_ASSERT2(psResult.handle, "FGThunderboltRender: failed to load effects_world_textured.ps");
    m_ps = psResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::RGBA8_UNORM).setOffset(12).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(16).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGThunderboltRender: createInputLayout failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = kCBSize;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGThunderboltRender_CB";
    m_constantBuffer = m_device->createBuffer(cbDesc);
    R_ASSERT2(m_constantBuffer, "FGThunderboltRender: createBuffer(CB) failed");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
    samplerDesc.setAllFilters(true);
    m_sampler = m_device->createSampler(samplerDesc);
    R_ASSERT2(m_sampler, "FGThunderboltRender: createSampler failed");

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);
    R_ASSERT2(m_bindingLayout, "FGThunderboltRender: createBindingLayout failed");

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
    pipelineDesc.renderState.depthStencilState.depthTestEnable = true;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipelineDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, fbInfo);
    R_ASSERT2(m_pipeline, "FGThunderboltRender: createGraphicsPipeline failed");
}

void FGThunderboltRender::Copy(IThunderboltRender& _in)
{
    *this = *static_cast<FGThunderboltRender*>(&_in);
}

void FGThunderboltRender::Clear()
{
    m_vertices.clear();
    m_indices.clear();
    m_batches.clear();
}

nvrhi::ITexture* FGThunderboltRender::ResolveTexture(const shared_str& name)
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

void FGThunderboltRender::Render(CEffect_Thunderbolt& owner)
{
    Clear();
    if (!owner.current)
        return;

    auto* pThRen = static_cast<dxThunderboltDescRender*>(&*owner.current->m_pRender);
    if (!pThRen || !pThRen->l_model)
        return;

    nvrhi::ITexture* lightningTex = ResolveTexture(pThRen->l_model->textureName);
    if (!lightningTex)
        return;

    float dv = owner.lightning_phase * 0.5f;
    dv = (owner.lightning_phase > 0.5f) ? Random.randI(2) * 0.5f : dv;

    const u32 vCount_lightning = pThRen->l_model->number_vertices;
    const u32 iCount_lightning = pThRen->l_model->number_indices;

    const u32 vBase_lightning = static_cast<u32>(m_vertices.size());
    const u32 iStart_lightning = static_cast<u32>(m_indices.size());

    m_vertices.resize(vBase_lightning + vCount_lightning);
    m_indices.resize(iStart_lightning + iCount_lightning);

    xr_vector<IRender_DetailModel::fvfVertexOut> tmpVerts(vCount_lightning);
    pThRen->l_model->transfer(owner.current_xform, tmpVerts.data(), 0xffffffff, m_indices.data() + iStart_lightning, 0, 0.f, dv);

    for (u32 i = 0; i < vCount_lightning; ++i)
    {
        Vertex& dst = m_vertices[vBase_lightning + i];
        const auto& src = tmpVerts[i];
        dst.x = src.P.x;
        dst.y = src.P.y;
        dst.z = src.P.z;
        dst.color = src.C;
        dst.u = src.u;
        dst.v = src.v;
    }
    for (u32 i = 0; i < iCount_lightning; ++i)
        m_indices[iStart_lightning + i] += static_cast<u16>(vBase_lightning);

    Batch lightning{};
    lightning.indexOffset = iStart_lightning;
    lightning.indexCount = iCount_lightning;
    lightning.texture = lightningTex;
    m_batches.push_back(lightning);

    auto pushQuad = [&](const Fvector& center, const Fvector2& radius, float scale, u32 color, nvrhi::ITexture* tex)
    {
        if (!tex || color == 0)
            return;

        Fvector vecSx, vecSy;
        vecSx.mul(Device.vCameraRight, radius.x * scale);
        vecSy.mul(Device.vCameraTop, -radius.y * scale);

        const u32 base = static_cast<u32>(m_vertices.size());
        const u32 iStart = static_cast<u32>(m_indices.size());

        Vertex v0;
        v0.x = center.x + vecSx.x - vecSy.x;
        v0.y = center.y + vecSx.y - vecSy.y;
        v0.z = center.z + vecSx.z - vecSy.z;
        v0.color = color;
        v0.u = 0;
        v0.v = 0;
        Vertex v1;
        v1.x = center.x + vecSx.x + vecSy.x;
        v1.y = center.y + vecSx.y + vecSy.y;
        v1.z = center.z + vecSx.z + vecSy.z;
        v1.color = color;
        v1.u = 0;
        v1.v = 1;
        Vertex v2;
        v2.x = center.x - vecSx.x - vecSy.x;
        v2.y = center.y - vecSx.y - vecSy.y;
        v2.z = center.z - vecSx.z - vecSy.z;
        v2.color = color;
        v2.u = 1;
        v2.v = 0;
        Vertex v3;
        v3.x = center.x - vecSx.x + vecSy.x;
        v3.y = center.y - vecSx.y + vecSy.y;
        v3.z = center.z - vecSx.z + vecSy.z;
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
    };

    if (owner.current->m_GradientTop)
    {
        auto* flare = static_cast<dxFlareRender*>(&*owner.current->m_GradientTop->m_pFlare);
        nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
        const u32 c_val = iFloor(owner.current->m_GradientTop->fOpacity * owner.lightning_phase * 255.f);
        const u32 c = color_rgba(c_val, c_val, c_val, c_val);
        pushQuad(owner.current_xform.c, owner.current->m_GradientTop->fRadius, owner.lightning_size, c, tex);
    }

    if (owner.current->m_GradientCenter)
    {
        auto* flare = static_cast<dxFlareRender*>(&*owner.current->m_GradientCenter->m_pFlare);
        nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
        const u32 c_val = iFloor(owner.current->m_GradientTop->fOpacity * owner.lightning_phase * 255.f);
        const u32 c = color_rgba(c_val, c_val, c_val, c_val);
        pushQuad(owner.lightning_center, owner.current->m_GradientCenter->fRadius, owner.lightning_size, c, tex);
    }
}

void FGThunderboltRender::EnsureGeometryCapacity(size_t vertexCount, size_t indexCount)
{
    if (vertexCount > m_vertexCapacity || !m_vertexBuffer)
    {
        const size_t newCap = std::max<size_t>(vertexCount * 2, 4096);
        nvrhi::BufferDesc d;
        d.byteSize = newCap * sizeof(Vertex);
        d.isVertexBuffer = true;
        d.debugName = "FGThunderboltRender_VB";
        d.initialState = nvrhi::ResourceStates::VertexBuffer;
        d.keepInitialState = true;
        m_vertexBuffer = m_device->createBuffer(d);
        m_vertexCapacity = newCap;
    }
    if (indexCount > m_indexCapacity || !m_indexBuffer)
    {
        const size_t newCap = std::max<size_t>(indexCount * 2, 8192);
        nvrhi::BufferDesc d;
        d.byteSize = newCap * sizeof(u16);
        d.isIndexBuffer = true;
        d.debugName = "FGThunderboltRender_IB";
        d.initialState = nvrhi::ResourceStates::IndexBuffer;
        d.keepInitialState = true;
        m_indexBuffer = m_device->createBuffer(d);
        m_indexCapacity = newCap;
    }
}

void FGThunderboltRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer)
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
            auto it = m_bindingSetCache.find(b.texture);
            if (it == m_bindingSetCache.end())
            {
                nvrhi::BindingSetDesc bindingSetDesc;
                bindingSetDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(0, b.texture),
                    nvrhi::BindingSetItem::Sampler(0, m_sampler),
                };
                nvrhi::BindingSetHandle bs = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
                R_ASSERT2(bs, "FGThunderboltRender: createBindingSet failed");
                it = m_bindingSetCache.emplace(b.texture, std::move(bs)).first;
            }
            currentBindingSet = it->second;
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
