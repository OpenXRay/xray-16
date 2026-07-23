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
    constexpr float kParticlesTime = 0.3f;
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

    // Splash: prefer detail-model texture, else SSFX rain_splash, else streak
    m_splashTexture = m_streakTexture;
    if (m_dropModel && m_dropModel->textureName.size())
    {
        auto h = textureManager->LoadTexture(*m_dropModel->textureName);
        if (auto* t = textureManager->GetNVRHITexture(h))
            m_splashTexture = t;
    }
    if (m_splashTexture == m_streakTexture)
    {
        auto h = textureManager->LoadTexture("fx" DELIMITER "rain_splash");
        if (auto* t = textureManager->GetNVRHITexture(h))
            m_splashTexture = t;
        else
        {
            h = textureManager->LoadTexture("fx" DELIMITER "rain_drop");
            if (auto* t = textureManager->GetNVRHITexture(h))
                m_splashTexture = t;
        }
    }

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("effects_world_soft", "main");
    R_ASSERT2(vsResult.handle, "FGRainRender: failed to load effects_world_soft.vs");
    m_vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("effects_world_soft", "main");
    R_ASSERT2(psResult.handle, "FGRainRender: failed to load effects_world_soft.ps");
    m_ps = psResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR")   .setFormat(nvrhi::Format::RGBA8_UNORM) .setOffset(12).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)  .setOffset(16).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGRainRender: createInputLayout failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(passes::SoftFXConstants);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGRainRender_SoftCB";
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
        nvrhi::BindingLayoutItem::Texture_SRV(1),
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);
    R_ASSERT2(m_bindingLayout, "FGRainRender: createBindingLayout failed");

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
    // Soft occlusion is primary; HW depth is a backup.
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
    R_ASSERT2(m_pipeline, "FGRainRender: createGraphicsPipeline failed");

    if (renderDevice)
        m_gpuSimReady = m_gpuSim.Initialize(renderDevice);

    Msg("* [FGRainRender] Soft-depth rain pipeline ready (GPU sim=%s)",
        m_gpuSimReady ? "on" : "off");
}

void FGRainRender::Copy(IRainRender& _in) { *this = *static_cast<FGRainRender*>(&_in); }

void FGRainRender::SetRainShadowInputs(nvrhi::ITexture* rainSM, const Fmatrix& sampleVP, bool valid)
{
    m_rainShadow = rainSM;
    m_rainSampleVP = sampleVP;
    m_rainSMValid = valid;
}

bool FGRainRender::HasWork() const
{
    return m_rainDensity > EPS_L || !m_splashBatches.empty() || !m_quadBatches.empty();
}

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
    m_splashBatches.clear();
    m_desiredParticles = 0;
    m_rainDensity = 0.f;
}

void FGRainRender::AppendSplashParticles(CEffect_Rain& owner, u32 rainColor)
{
    if (!m_dropModel || m_dropModel->number_vertices == 0 || m_dropModel->number_indices == 0)
        return;

    const float dt = Device.fTimeDelta;
    CEffect_Rain::Particle* P = owner.particle_active;
    if (!P)
        return;

    Fmatrix mScale, mXform;
    int pcount = 0;
    u32 splashIndexStart = static_cast<u32>(m_indices.size());

    auto flushSplashBatch = [&]() {
        const u32 splashIndexCount = static_cast<u32>(m_indices.size()) - splashIndexStart;
        if (splashIndexCount > 0)
        {
            Batch b{};
            b.indexOffset = splashIndexStart;
            b.indexCount = splashIndexCount;
            m_splashBatches.push_back(b);
        }
        splashIndexStart = static_cast<u32>(m_indices.size());
        pcount = 0;
    };

    while (P)
    {
        CEffect_Rain::Particle* next = P->next;

        P->time -= dt;
        if (P->time < 0.f)
        {
            owner.p_free(P);
            P = next;
            continue;
        }

        if (RImplementation.ViewBase.testSphere_dirty(P->bounds.P, P->bounds.R))
        {
            const float scale = P->time / kParticlesTime;
            mScale.scale(scale, scale, scale);
            mXform.mul_43(P->mXForm, mScale);

            const u32 vBase = static_cast<u32>(m_vertices.size());
            // Stay within 16-bit index range for absolute indices
            if (vBase + m_dropModel->number_vertices > 65000)
            {
                flushSplashBatch();
                // Drop remaining if still over (extreme); rare with pool=1000
                if (m_vertices.size() + m_dropModel->number_vertices > 65000)
                {
                    P = next;
                    continue;
                }
            }

            const u32 iBase = static_cast<u32>(m_indices.size());
            m_vertices.resize(vBase + m_dropModel->number_vertices);
            m_indices.resize(iBase + m_dropModel->number_indices);

            m_dropModel->transfer(
                mXform,
                reinterpret_cast<IRender_DetailModel::fvfVertexOut*>(m_vertices.data() + vBase),
                rainColor,
                m_indices.data() + iBase,
                vBase);

            ++pcount;
            if (pcount >= kParticleCacheLimit)
                flushSplashBatch();
        }

        P = next;
    }

    flushSplashBatch();
}

void FGRainRender::SimulateRainItems(CEffect_Rain& owner)
{
    const u32 desired = m_desiredParticles;
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

    const float radius_wrap_sqr = _sqr(kSourceRadius + .5f);
    Fplane src_plane;
    Fvector norm = {0.f, -1.f, 0.f};
    Fvector upper;
    upper.set(Device.vCameraPosition.x, Device.vCameraPosition.y + kSourceOffset, Device.vCameraPosition.z);
    src_plane.build(upper, norm);

    const Fvector& vEye = Device.vCameraPosition;

    for (u32 I = 0; I < desired; I++)
    {
        CEffect_Rain::Item& one = owner.items[I];
        if (one.dwTime_Hit < Device.dwTimeGlobal)
            owner.Hit(one.Phit);
        if (one.dwTime_Life < Device.dwTimeGlobal)
            owner.Born(one, kSourceRadius);

        one.P.mad(one.D, one.fSpeed * Device.fTimeDelta);

        if ((one.P.y - vEye.y) < kSinkOffset)
        {
            owner.Hit(one.P);
            one.invalidate();
            owner.Born(one, kSourceRadius);
            continue;
        }

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
    }
}

void FGRainRender::BuildCPUStreaks(CEffect_Rain& owner)
{
    const u32 desired = m_desiredParticles;
    SimulateRainItems(owner);
    static Fvector2 UV[2][4] = {{{0,1},{0,0},{1,1},{1,0}},{{1,0},{1,1},{0,0},{0,1}}};
    const Fvector& vEye = Device.vCameraPosition;
    const u32 quadIndexStart = static_cast<u32>(m_indices.size());

    for (u32 I = 0; I < desired; I++)
    {
        CEffect_Rain::Item& one = owner.items[I];

        Fvector& pos_head = one.P;
        Fvector pos_trail;
        pos_trail.mad(pos_head, one.D, -kDropLength * m_factorVisual);

        Fvector sC, lineD;
        sC.sub(pos_head, pos_trail);
        lineD.normalize(sC);
        sC.mul(.5f);
        sC.add(pos_trail);
        if (!RImplementation.ViewBase.testSphere_dirty(sC, sC.magnitude()))
            continue;

        Fvector sky = pos_head;
        sky.y += 80.f;
        Fvector down(0.f, -1.f, 0.f);
        float coverRange = 80.f;
        if (owner.RayPick(sky, down, coverRange, collide::rqtBoth))
        {
            const float hitY = sky.y - coverRange;
            if (hitY > pos_head.y + 0.35f)
                continue;
        }

        Fvector lineTop, camDir;
        camDir.sub(sC, vEye);
        camDir.normalize();
        lineTop.crossproduct(camDir, lineD);
        const float w = kDropWidth;
        const u32 s = one.uv_set;
        Fvector P;

        const u32 base = static_cast<u32>(m_vertices.size());
        P.mad(pos_trail, lineTop, -w);
        m_vertices.push_back({P.x, P.y, P.z, m_rainColor, UV[s][0].x, UV[s][0].y});
        P.mad(pos_trail, lineTop,  w);
        m_vertices.push_back({P.x, P.y, P.z, m_rainColor, UV[s][1].x, UV[s][1].y});
        P.mad(pos_head,  lineTop, -w);
        m_vertices.push_back({P.x, P.y, P.z, m_rainColor, UV[s][2].x, UV[s][2].y});
        P.mad(pos_head,  lineTop,  w);
        m_vertices.push_back({P.x, P.y, P.z, m_rainColor, UV[s][3].x, UV[s][3].y});

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

void FGRainRender::Render(CEffect_Rain& owner)
{
    Clear();

    const float factor = g_pGamePersistent->Environment().CurrentEnv.rain_density;
    m_rainDensity = factor;
    if (factor < EPS_L)
        return;

    m_desiredParticles = iFloor(0.5f * (1.f + factor) * float(kMaxDesired));
    m_factorVisual = factor / 2.f + .5f;
    const Fvector3 fc = g_pGamePersistent->Environment().CurrentEnv.rain_color;
    m_rainColor = color_rgba_f(fc.x, fc.y, fc.z, m_factorVisual);

    // GPU streaks + rain-SM cover; CPU keeps item sim for splash particles.
    if (m_gpuSimReady)
        SimulateRainItems(owner);
    else if (m_dropModel)
        BuildCPUStreaks(owner);
    else
        SimulateRainItems(owner);

    if (m_dropModel)
        AppendSplashParticles(owner, m_rainColor);
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

void FGRainRender::DrawGPUStreaks(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                                  nvrhi::ITexture* sceneWorldPos)
{
    if (!m_streakTexture || !sceneWorldPos || !m_pipeline || !m_bindingLayout)
        return;

    nvrhi::IBuffer* vb = m_gpuSim.GetVertexBuffer();
    nvrhi::IBuffer* drawArgs = m_gpuSim.GetDrawArgsBuffer();
    if (!vb || !drawArgs)
        return;

    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, m_streakTexture),
        nvrhi::BindingSetItem::Texture_SRV(1, sceneWorldPos),
        nvrhi::BindingSetItem::Sampler(0, m_sampler),
    };
    nvrhi::BindingSetHandle bindingSet = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
    if (!bindingSet)
        return;

    cmdList->setBufferState(vb, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(drawArgs, nvrhi::ResourceStates::IndirectArgument);

    nvrhi::VertexBufferBinding vertexBinding;
    vertexBinding.buffer = vb;
    vertexBinding.slot = 0;
    vertexBinding.offset = 0;

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::GraphicsState state;
    state.pipeline = m_pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };
    state.vertexBuffers = { vertexBinding };
    state.indexBuffer = {};
    state.indirectParams = drawArgs;
    state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
        nvrhi::Viewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height)));
    cmdList->setGraphicsState(state);
    cmdList->drawIndirect(0);
}

void FGRainRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                        nvrhi::ITexture* sceneWorldPos,
                        const passes::RainHeightmapInfo& heightmap)
{
    passes::SoftFXConstants cb{};
    passes::FillSoftFXConstants(cb);
    cmdList->writeBuffer(m_constantBuffer, &cb, sizeof(cb));
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    (void)heightmap;

    if (m_gpuSimReady && m_desiredParticles > 0)
    {
        m_gpuSim.Dispatch(cmdList, m_rainShadow, m_rainSampleVP, m_rainSMValid,
            m_desiredParticles, m_rainDensity, m_factorVisual, m_rainColor);
        DrawGPUStreaks(cmdList, framebuffer, sceneWorldPos);
        if (!m_splashBatches.empty())
        {
            EnsureGeometryCapacity(m_vertices.size(), m_indices.size());
            if (!m_vertices.empty())
                cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
            if (!m_indices.empty())
                cmdList->writeBuffer(m_indexBuffer, m_indices.data(), m_indices.size() * sizeof(u16));
            cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
            cmdList->setBufferState(m_indexBuffer, nvrhi::ResourceStates::IndexBuffer);
            DrawBatches(cmdList, framebuffer, m_splashBatches, m_splashTexture, sceneWorldPos);
        }
        Clear();
        return;
    }

    EnsureGeometryCapacity(m_vertices.size(), m_indices.size());
    if (!m_vertices.empty())
        cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
    if (!m_indices.empty())
        cmdList->writeBuffer(m_indexBuffer, m_indices.data(), m_indices.size() * sizeof(u16));
    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_indexBuffer, nvrhi::ResourceStates::IndexBuffer);

    DrawBatches(cmdList, framebuffer, m_quadBatches, m_streakTexture, sceneWorldPos);
    DrawBatches(cmdList, framebuffer, m_splashBatches, m_splashTexture, sceneWorldPos);
    Clear();
}

void FGRainRender::DrawBatches(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                               const xr_vector<Batch>& batches, nvrhi::ITexture* colorTex,
                               nvrhi::ITexture* sceneWorldPos)
{
    if (!colorTex || !sceneWorldPos || batches.empty() || !m_pipeline || !m_bindingLayout)
        return;

    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, colorTex),
        nvrhi::BindingSetItem::Texture_SRV(1, sceneWorldPos),
        nvrhi::BindingSetItem::Sampler(0, m_sampler),
    };
    nvrhi::BindingSetHandle bindingSet = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
    if (!bindingSet)
        return;

    nvrhi::VertexBufferBinding vertexBinding;
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.slot = 0;
    vertexBinding.offset = 0;

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::GraphicsState state;
    state.pipeline = m_pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };
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
