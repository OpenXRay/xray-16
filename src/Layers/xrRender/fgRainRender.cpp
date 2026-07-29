#include "stdafx.h"
#include "fgRainRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg
{
namespace
{
    constexpr float kSourceRadius = 12.5f;
    constexpr float kSourceOffset = 40.f;
    constexpr float kMaxDistance  = kSourceOffset * 1.25f;
    constexpr float kSinkOffset   = -(kMaxDistance - kSourceOffset);
    constexpr float kDropLength   = 5.f;
    constexpr float kDropWidth    = 0.30f;
    constexpr int   kMaxDesired   = 2500;
    constexpr int   kParticleCacheLimit = 400;
}

FGRainRender::FGRainRender()
{
    IReader* F = FS.r_open("$game_meshes$", "dm" DELIMITER "rain.dm");
    if (F)
    {
        m_dropModel = RImplementation.model_CreateDM(F);
        FS.r_close(F);
    }
    InitResources();
}

FGRainRender::~FGRainRender()
{
    if (m_dropModel)
        RImplementation.model_Delete(m_dropModel);
}

void FGRainRender::InitResources()
{
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    auto* textureManager = renderDevice->GetFGResourceManager()->GetTextureManager();
    R_ASSERT(textureManager);
    m_streakTexture = textureManager->GetNVRHITexture(textureManager->LoadTexture("fx" DELIMITER "fx_rain"));
    R_ASSERT2(m_streakTexture, "FGRainRender: failed to load fx/fx_rain texture");

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("effects_world_textured", "main");
    R_ASSERT2(vsResult.handle, "FGRainRender: failed to load effects_world_textured.vs");
    m_vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("effects_world_textured", "main");
    R_ASSERT2(psResult.handle, "FGRainRender: failed to load effects_world_textured.ps");
    m_ps = psResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR")   .setFormat(nvrhi::Format::RGBA8_UNORM) .setOffset(12).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)  .setOffset(16).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGRainRender: createInputLayout failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(passes::DynamicTransforms);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGRainRender_CB";
    m_constantBuffer = m_device->createBuffer(cbDesc);
    R_ASSERT2(m_constantBuffer, "FGRainRender: createBuffer(CB) failed");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
    samplerDesc.setAllFilters(true);
    m_sampler = m_device->createSampler(samplerDesc);
    R_ASSERT2(m_sampler, "FGRainRender: createSampler failed");

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);
    R_ASSERT2(m_bindingLayout, "FGRainRender: createBindingLayout failed");

    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, m_streakTexture),
        nvrhi::BindingSetItem::Sampler(0, m_sampler),
    };
    m_bindingSet = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
    R_ASSERT2(m_bindingSet, "FGRainRender: createBindingSet failed at init");

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
    pipelineDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipelineDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, fbInfo);
    R_ASSERT2(m_pipeline, "FGRainRender: createGraphicsPipeline failed");
}

void FGRainRender::Copy(IRainRender& _in) { *this = *static_cast<FGRainRender*>(&_in); }

const Fsphere& FGRainRender::GetDropBounds() const
{
    static Fsphere fallback;
    return m_dropModel ? m_dropModel->bv_sphere : fallback;
}

void FGRainRender::Clear()
{
    m_vertices.clear();
    m_indices.clear();
    m_quadBatches.clear();
}

void FGRainRender::Render(CEffect_Rain& owner)
{
    Clear();
    if (!m_dropModel) return;

    const float factor = g_pGamePersistent->Environment().CurrentEnv.rain_density;
    if (factor < EPS_L) return;

    const u32 desired = iFloor(0.5f * (1.f + factor) * float(kMaxDesired));

    if (owner.items.size() < desired)
    {
        owner.items.reserve(desired);
        while (owner.items.size() < desired)
        {
            CEffect_Rain::Item one;
            owner.Born(one, kSourceRadius);
            owner.items.push_back(one);
        }
    }

    const float factor_visual = factor / 2.f + .5f;
    const Fvector3 fc = g_pGamePersistent->Environment().CurrentEnv.rain_color;
    const u32 col = color_rgba_f(fc.x, fc.y, fc.z, factor_visual);
    const float radius_wrap_sqr = _sqr(kSourceRadius + .5f);

    Fplane src_plane;
    Fvector norm = {0.f, -1.f, 0.f};
    Fvector upper;
    upper.set(Device.vCameraPosition.x, Device.vCameraPosition.y + kSourceOffset, Device.vCameraPosition.z);
    src_plane.build(upper, norm);

    static Fvector2 UV[2][4] = {{{0,1},{0,0},{1,1},{1,0}},{{1,0},{1,1},{0,0},{0,1}}};

    const Fvector& vEye = Device.vCameraPosition;
    const u32 quadIndexStart = static_cast<u32>(m_indices.size());

    for (u32 I = 0; I < desired; I++)
    {
        CEffect_Rain::Item& one = owner.items[I];
        if (one.dwTime_Hit < Device.dwTimeGlobal) owner.Hit(one.Phit);
        if (one.dwTime_Life < Device.dwTimeGlobal) owner.Born(one, kSourceRadius);

        float dt = Device.fTimeDelta;
        one.P.mad(one.D, one.fSpeed * dt);
        Fvector wdir;
        wdir.set(one.P.x - vEye.x, 0, one.P.z - vEye.z);
        float wlen = wdir.square_magnitude();
        if (wlen > radius_wrap_sqr)
        {
            wlen = _sqrt(wlen);
            if ((one.P.y - vEye.y) < kSinkOffset) { one.invalidate(); }
            else
            {
                Fvector inv_dir, src_p;
                inv_dir.invert(one.D);
                wdir.div(wlen);
                one.P.mad(one.P, wdir, -(wlen + kSourceRadius));
                if (src_plane.intersectRayPoint(one.P, inv_dir, src_p))
                {
                    float dist_sqr = one.P.distance_to_sqr(src_p);
                    float height = kMaxDistance;
                    if (owner.RayPick(src_p, one.D, height, collide::rqtBoth))
                    {
                        if (_sqr(height) <= dist_sqr) one.invalidate();
                        else owner.RenewItem(one, height - _sqrt(dist_sqr), TRUE);
                    }
                    else owner.RenewItem(one, kMaxDistance - _sqrt(dist_sqr), FALSE);
                }
                else one.invalidate();
            }
        }

        Fvector& pos_head = one.P;
        Fvector pos_trail;
        pos_trail.mad(pos_head, one.D, -kDropLength * factor_visual);

        Fvector sC, lineD;
        float sR;
        sC.sub(pos_head, pos_trail);
        lineD.normalize(sC);
        sC.mul(.5f);
        sR = sC.magnitude();
        sC.add(pos_trail);
        if (!RImplementation.ViewBase.testSphere_dirty(sC, sR))
            continue;

        Fvector lineTop, camDir;
        camDir.sub(sC, vEye);
        camDir.normalize();
        lineTop.crossproduct(camDir, lineD);
        const float w = kDropWidth;
        const u32 s = one.uv_set;
        Fvector P;

        const u32 base = static_cast<u32>(m_vertices.size());
        P.mad(pos_trail, lineTop, -w);
        m_vertices.push_back({P.x, P.y, P.z, col, UV[s][0].x, UV[s][0].y});
        P.mad(pos_trail, lineTop,  w);
        m_vertices.push_back({P.x, P.y, P.z, col, UV[s][1].x, UV[s][1].y});
        P.mad(pos_head,  lineTop, -w);
        m_vertices.push_back({P.x, P.y, P.z, col, UV[s][2].x, UV[s][2].y});
        P.mad(pos_head,  lineTop,  w);
        m_vertices.push_back({P.x, P.y, P.z, col, UV[s][3].x, UV[s][3].y});

        m_indices.push_back(static_cast<u16>(base + 0));
        m_indices.push_back(static_cast<u16>(base + 1));
        m_indices.push_back(static_cast<u16>(base + 2));
        m_indices.push_back(static_cast<u16>(base + 1));
        m_indices.push_back(static_cast<u16>(base + 3));
        m_indices.push_back(static_cast<u16>(base + 2));
    }

    const u32 quadIndexCount = static_cast<u32>(m_indices.size()) - quadIndexStart;
    if (quadIndexCount > 0)
    {
        Batch b{};
        b.indexOffset = quadIndexStart;
        b.indexCount  = quadIndexCount;
        m_quadBatches.push_back(b);
    }
}

void FGRainRender::EnsureGeometryCapacity(size_t vertexCount, size_t indexCount)
{
    if (vertexCount > m_vertexCapacity || !m_vertexBuffer)
    {
        const size_t newCap = std::max<size_t>(vertexCount * 2, 4096);
        nvrhi::BufferDesc d; d.byteSize = newCap * sizeof(Vertex);
        d.isVertexBuffer = true; d.debugName = "FGRainRender_VB";
        d.initialState = nvrhi::ResourceStates::VertexBuffer; d.keepInitialState = true;
        m_vertexBuffer = m_device->createBuffer(d);
        m_vertexCapacity = newCap;
    }
    if (indexCount > m_indexCapacity || !m_indexBuffer)
    {
        const size_t newCap = std::max<size_t>(indexCount * 2, 8192);
        nvrhi::BufferDesc d; d.byteSize = newCap * sizeof(u16);
        d.isIndexBuffer = true; d.debugName = "FGRainRender_IB";
        d.initialState = nvrhi::ResourceStates::IndexBuffer; d.keepInitialState = true;
        m_indexBuffer = m_device->createBuffer(d);
        m_indexCapacity = newCap;
    }
}

void FGRainRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer)
{
    EnsureGeometryCapacity(m_vertices.size(), m_indices.size());

    if (!m_vertices.empty())
        cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
    if (!m_indices.empty())
        cmdList->writeBuffer(m_indexBuffer, m_indices.data(), m_indices.size() * sizeof(u16));

    passes::DynamicTransforms cb{};
    passes::FillDynamicTransforms(cb);
    cmdList->writeBuffer(m_constantBuffer, &cb, sizeof(cb));

    cmdList->setBufferState(m_vertexBuffer,   nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_indexBuffer,    nvrhi::ResourceStates::IndexBuffer);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    DrawBatches(cmdList, framebuffer, m_quadBatches, m_streakTexture);

    Clear();
}

void FGRainRender::DrawBatches(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                               const xr_vector<Batch>& batches, nvrhi::ITexture* texture)
{
    if (!texture || batches.empty() || !m_bindingSet)
        return;

    nvrhi::VertexBufferBinding vertexBinding;
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.slot = 0;
    vertexBinding.offset = 0;

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::GraphicsState state;
    state.pipeline = m_pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { m_bindingSet };
    state.vertexBuffers = { vertexBinding };
    state.indexBuffer.buffer = m_indexBuffer;
    state.indexBuffer.format = nvrhi::Format::R16_UINT;
    state.indexBuffer.offset = 0;
    state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
        nvrhi::Viewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height)));
    cmdList->setGraphicsState(state);

    for (const Batch& b : batches)
    {
        if (b.indexCount == 0)
            continue;

        nvrhi::DrawArguments args;
        args.vertexCount = b.indexCount;
        args.instanceCount = 1;
        args.startIndexLocation = b.indexOffset;
        cmdList->drawIndexed(args);
    }
}
}
