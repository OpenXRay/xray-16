#include "stdafx.h"

#include "fgLensFlareRender.h"
#include "fgFlareRender.h"

#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg
{
namespace
{
constexpr u32 kMaxFlares = 24;

struct FlareVisParams
{
    float sunPosX;
    float sunPosY;
    float radiusPx;
    float emaAlpha;
    u32 valid;
    u32 pad[3];
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

    auto vsOverlayResult = shaderLoader->LoadVertexShader("effects_flare", "main");
    R_ASSERT2(vsOverlayResult.handle, "FGLensFlareRender: failed to load effects_flare.vs");
    m_vsOverlay = vsOverlayResult.handle;

    auto csResult = shaderLoader->LoadComputeShader("flare_visibility", "main");
    R_ASSERT2(csResult.handle, "FGLensFlareRender: failed to load flare_visibility.cs");
    m_visCS = csResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::RGBA8_UNORM).setOffset(12).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(16).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGLensFlareRender: createInputLayout failed");
    m_inputLayoutOverlay = m_device->createInputLayout(vertexAttrs, 3, m_vsOverlay);
    R_ASSERT2(m_inputLayoutOverlay, "FGLensFlareRender: createInputLayout (overlay) failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(passes::DynamicTransforms);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGLensFlareRender_CB";
    m_constantBuffer = m_device->createBuffer(cbDesc);
    R_ASSERT2(m_constantBuffer, "FGLensFlareRender: createBuffer(CB) failed");

    nvrhi::BufferDesc visCbDesc;
    visCbDesc.byteSize = sizeof(FlareVisParams);
    visCbDesc.isConstantBuffer = true;
    visCbDesc.isVolatile = true;
    visCbDesc.maxVersions = 16;
    visCbDesc.debugName = "FGLensFlareRender_VisCB";
    m_visConstantBuffer = m_device->createBuffer(visCbDesc);
    R_ASSERT2(m_visConstantBuffer, "FGLensFlareRender: createBuffer(VisCB) failed");

    nvrhi::BufferDesc visDesc;
    visDesc.byteSize = sizeof(float);
    visDesc.structStride = sizeof(float);
    visDesc.canHaveUAVs = true;
    visDesc.debugName = "FGLensFlareRender_Vis";
    visDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    visDesc.keepInitialState = true;
    m_visBuffer = m_device->createBuffer(visDesc);
    R_ASSERT2(m_visBuffer, "FGLensFlareRender: createBuffer(Vis) failed");

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

    nvrhi::BindingLayoutDesc overlayLayoutDesc;
    overlayLayoutDesc.visibility = nvrhi::ShaderType::All;
    overlayLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),
    };
    m_overlayBindingLayout = m_device->createBindingLayout(overlayLayoutDesc);
    R_ASSERT2(m_overlayBindingLayout, "FGLensFlareRender: createBindingLayout (overlay) failed");

    nvrhi::BindingLayoutDesc visLayoutDesc;
    visLayoutDesc.visibility = nvrhi::ShaderType::Compute;
    visLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
    };
    m_visBindingLayout = m_device->createBindingLayout(visLayoutDesc);
    R_ASSERT2(m_visBindingLayout, "FGLensFlareRender: createBindingLayout (vis) failed");

    nvrhi::FramebufferInfo fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.setDepthFormat(nvrhi::Format::D32);
    fbInfo.setSampleCount(1);

    nvrhi::GraphicsPipelineDesc sourceDesc;
    sourceDesc.VS = m_vs;
    sourceDesc.PS = m_ps;
    sourceDesc.inputLayout = m_inputLayout;
    sourceDesc.bindingLayouts = { m_bindingLayout };
    sourceDesc.primType = nvrhi::PrimitiveType::TriangleList;
    sourceDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    sourceDesc.renderState.depthStencilState.depthTestEnable = true;
    sourceDesc.renderState.depthStencilState.depthWriteEnable = false;
    sourceDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    sourceDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::One)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::One);

    m_pipelineSource = m_device->createGraphicsPipeline(sourceDesc, fbInfo);
    R_ASSERT2(m_pipelineSource, "FGLensFlareRender: createGraphicsPipeline (source) failed");

    nvrhi::GraphicsPipelineDesc overlayDesc = sourceDesc;
    overlayDesc.VS = m_vsOverlay;
    overlayDesc.inputLayout = m_inputLayoutOverlay;
    overlayDesc.bindingLayouts = { m_overlayBindingLayout };
    overlayDesc.renderState.depthStencilState.depthTestEnable = false;
    overlayDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;

    m_pipelineOverlay = m_device->createGraphicsPipeline(overlayDesc, fbInfo);
    R_ASSERT2(m_pipelineOverlay, "FGLensFlareRender: createGraphicsPipeline (overlay) failed");

    nvrhi::ComputePipelineDesc visPipelineDesc;
    visPipelineDesc.CS = m_visCS;
    visPipelineDesc.bindingLayouts = { m_visBindingLayout };
    m_visPipeline = m_device->createComputePipeline(visPipelineDesc);
    R_ASSERT2(m_visPipeline, "FGLensFlareRender: createComputePipeline (vis) failed");
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

void FGLensFlareRender::PushQuad(const Fvector& center, const Fvector& vecX, const Fvector& vecY, u32 color,
    nvrhi::ITexture* tex, bool depthTested)
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
    b.depthTested = depthTested;
    m_batches.push_back(b);
}

void FGLensFlareRender::Render(CLensFlare& owner, BOOL bSun, BOOL bFlares, BOOL bGradient)
{
    Clear();
    if (!owner.m_Current)
        return;

    m_sunValid = false;
    Fvector4 clip;
    Device.mFullTransform.transform(clip, owner.vecLight);
    if (clip.w > 0.f)
    {
        m_sunValid = true;
        m_sunPosPx.set((clip.x * 0.5f + 0.5f) * float(Device.dwWidth),
            (1.f - (clip.y * 0.5f + 0.5f)) * float(Device.dwHeight));
        const float radius =
            owner.m_Current->m_Flags.is(CLensFlareDescriptor::flSource) ? owner.m_Current->m_Source.fRadius : 0.15f;
        m_sunRadiusPx = radius * 0.25f * float(Device.dwHeight) / tanf(deg2rad(Device.fFOV) * 0.5f);
        clamp(m_sunRadiusPx, 4.f, 96.f);
    }

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

        auto* flare = static_cast<FGFlareRender*>(&*owner.m_Current->m_Source.m_pRender);
        nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
        PushQuad(owner.vecLight, vecSx, vecSy, color.get(), tex, true);
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

                auto* flare = static_cast<FGFlareRender*>(&*F.m_pRender);
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

            auto* flare = static_cast<FGFlareRender*>(&*owner.m_Current->m_Gradient.m_pRender);
            nvrhi::ITexture* tex = ResolveTexture(flare ? flare->m_textureName : shared_str{});
            PushQuad(owner.vecLight, vecSx, vecSy, color.get(), tex);
        }
    }
}

void FGLensFlareRender::DispatchVisibility(nvrhi::ICommandList* cmdList, nvrhi::ITexture* depth)
{
    if (!depth || !m_visPipeline)
        return;

    if (!m_visInitialized)
    {
        cmdList->clearBufferUInt(m_visBuffer, 0);
        m_visInitialized = true;
    }

    FlareVisParams cb{};
    cb.sunPosX = m_sunPosPx.x;
    cb.sunPosY = m_sunPosPx.y;
    cb.radiusPx = m_sunRadiusPx;
    cb.emaAlpha = 1.f - expf(-8.f * Device.fTimeDelta);
    cb.valid = m_sunValid ? 1u : 0u;
    cmdList->writeBuffer(m_visConstantBuffer, &cb, sizeof(cb));

    auto it = m_visBindingSetCache.find(depth);
    if (it == m_visBindingSetCache.end())
    {
        nvrhi::BindingSetDesc d;
        d.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(0, m_visConstantBuffer),
            nvrhi::BindingSetItem::Texture_SRV(0, depth),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, m_visBuffer),
        };
        nvrhi::BindingSetHandle bs = m_device->createBindingSet(d, m_visBindingLayout);
        R_ASSERT2(bs, "FGLensFlareRender: createBindingSet (vis) failed");
        it = m_visBindingSetCache.emplace(depth, std::move(bs)).first;
    }

    nvrhi::ComputeState state;
    state.pipeline = m_visPipeline;
    state.bindings = { it->second };
    cmdList->setComputeState(state);
    cmdList->dispatch(1, 1, 1);
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

    passes::DynamicTransforms cb{};
    passes::FillDynamicTransforms(cb);
    cmdList->writeBuffer(m_constantBuffer, &cb, sizeof(cb));

    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_indexBuffer, nvrhi::ResourceStates::IndexBuffer);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);
    cmdList->setBufferState(m_visBuffer, nvrhi::ResourceStates::ShaderResource);

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::VertexBufferBinding vertexBinding;
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.slot = 0;
    vertexBinding.offset = 0;

    for (const Batch& b : m_batches)
    {
        if (b.indexCount == 0 || !b.texture)
            continue;

        auto& cache = b.depthTested ? m_sourceBindingSetCache : m_overlayBindingSetCache;
        auto it = cache.find(b.texture);
        if (it == cache.end())
        {
            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
                nvrhi::BindingSetItem::Texture_SRV(0, b.texture),
                nvrhi::BindingSetItem::Sampler(0, m_sampler),
            };
            if (!b.depthTested)
                bindingSetDesc.bindings.push_back(nvrhi::BindingSetItem::StructuredBuffer_SRV(1, m_visBuffer));
            nvrhi::BindingSetHandle bs =
                m_device->createBindingSet(bindingSetDesc, b.depthTested ? m_bindingLayout : m_overlayBindingLayout);
            R_ASSERT2(bs, "FGLensFlareRender: createBindingSet failed");
            it = cache.emplace(b.texture, std::move(bs)).first;
        }

        nvrhi::GraphicsState state;
        state.pipeline = b.depthTested ? m_pipelineSource : m_pipelineOverlay;
        state.framebuffer = framebuffer;
        state.bindings = { it->second };
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
