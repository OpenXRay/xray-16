// xrRender/FrameGraphPasses/ParticlePassSetup.cpp
// Batched particle rendering with GPU culling
#include "stdafx.h"
#include "ParticlePassSetup.h"
#include "ParticleGPUCullingManager.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ParticleEffect.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleGroup.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "xrParticles/psystem.h"
#include "xrCDB/Frustum.h"  // For CFrustum (frustum plane extraction)

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;
using RENDER_NAMESPACE::PS::CPEDef;

// Static pipeline state
static nvrhi::GraphicsPipelineHandle s_particlePipelineBlend;
static nvrhi::GraphicsPipelineHandle s_particlePipelineAdd;
static nvrhi::BindingLayoutHandle s_particleLayout;
static nvrhi::InputLayoutHandle s_particleInputLayout;
static nvrhi::ShaderHandle s_particleVS;
static nvrhi::ShaderHandle s_particlePS;
static nvrhi::SamplerHandle s_particleSampler;
static nvrhi::BindingSetHandle s_particleBindingSet;
static bool s_particleInitialized = false;

// Dynamic buffers (CPU fallback path)
static nvrhi::BufferHandle s_particleVB;
static u32 s_particleVBSize = 0;
static nvrhi::BufferHandle s_quadIB;
static u32 s_maxQuads = 0;

// GPU culling manager
static std::unique_ptr<ParticleGPUCullingManager> s_gpuCullingManager;
static constexpr u32 MAX_GPU_PARTICLES = 65536;

static void EnsureQuadIndexBuffer(nvrhi::IDevice* nvDevice, u32 maxQuads)
{
    if (s_quadIB && s_maxQuads >= maxQuads)
        return;

    u32 numIndices = maxQuads * 6;
    xr_vector<u16> indices;
    indices.reserve(numIndices);

    for (u32 i = 0; i < maxQuads; i++) {
        u16 base = (u16)(i * 4);
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
    }

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = numIndices * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "ParticleQuadIB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

    s_quadIB = nvDevice->createBuffer(ibDesc);
    if (!s_quadIB)
        return;

    nvrhi::CommandListHandle cmdList = nvDevice->createCommandList();
    cmdList->open();
    cmdList->beginTrackingBufferState(s_quadIB, nvrhi::ResourceStates::IndexBuffer);
    cmdList->writeBuffer(s_quadIB, indices.data(), indices.size() * sizeof(u16));
    cmdList->close();
    nvDevice->executeCommandList(cmdList);

    s_maxQuads = maxQuads;
}

static void EnsureParticleVertexBuffer(nvrhi::IDevice* nvDevice, u32 sizeBytes)
{
    if (s_particleVB && s_particleVBSize >= sizeBytes)
        return;

    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

    s_particleVB = nvDevice->createBuffer(vbDesc);
    s_particleVBSize = s_particleVB ? allocSize : 0;
}

static void FillSprite(
    ParticleVertex*& pv,
    const Fvector& T, const Fvector& R,
    const Fvector& pos,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr, u32 matID,
    float sina, float cosa)
{
    Fvector Vr, Vt;
    Vr.x = T.x * r1 * sina + R.x * r1 * cosa;
    Vr.y = T.y * r1 * sina + R.y * r1 * cosa;
    Vr.z = T.z * r1 * sina + R.z * r1 * cosa;
    Vt.x = T.x * r2 * cosa - R.x * r2 * sina;
    Vt.y = T.y * r2 * cosa - R.y * r2 * sina;
    Vt.z = T.z * r2 * cosa - R.z * r2 * sina;

    Fvector a, b, c, d;
    a.sub(Vt, Vr);
    b.add(Vt, Vr);
    c.invert(a);
    d.invert(b);

    pv->p.set(d.x + pos.x, d.y + pos.y, d.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, rb.y);
    pv->materialID = matID;
    pv++;

    pv->p.set(a.x + pos.x, a.y + pos.y, a.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, lt.y);
    pv->materialID = matID;
    pv++;

    pv->p.set(c.x + pos.x, c.y + pos.y, c.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, rb.y);
    pv->materialID = matID;
    pv++;

    pv->p.set(b.x + pos.x, b.y + pos.y, b.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, lt.y);
    pv->materialID = matID;
    pv++;
}

// Returns total particle count generated, fills vertices
static u32 GenerateParticleVertices(
    const xr_vector<ParticleBatch>& batches,
    xr_vector<ParticleVertex>& vertices)
{
    u32 totalParticles = 0;

    for (const auto& batch : batches) {
        if (!batch.visual || batch.visual->getType() != MT_PARTICLE_EFFECT)
            continue;

        CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);
        auto* pDef = pEffect->GetDefinition();
        if (!pDef || !pDef->m_Flags.is(CPEDef::dfSprite))
            continue;

        PAPI::Particle* particles = nullptr;
        u32 particleCount = 0;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

        if (particleCount == 0 || !particles)
            continue;

        u32 baseVertex = (u32)vertices.size();
        vertices.resize(baseVertex + particleCount * 4);
        ParticleVertex* pv = &vertices[baseVertex];

        float sina = 0.0f, cosa = 0.0f;
        float angle = float(0xFFFFFFFF);

        for (u32 i = 0; i < particleCount; i++) {
            auto& m = particles[i];

            if (angle != m.rot.x) {
                angle = m.rot.x;
                sina = _sin(angle);
                cosa = _cos(angle);
            }

            Fvector2 lt, rb;
            lt.set(0.f, 0.f);
            rb.set(1.f, 1.f);

            if (pDef->m_Flags.is(CPEDef::dfFramed))
                pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), lt, rb);

            float r_x = m.size.x * 0.5f;
            float r_y = m.size.y * 0.5f;

            if (pDef->m_Flags.is(CPEDef::dfVelocityScale)) {
                float speed = m.vel.magnitude();
                r_x += speed * pDef->m_VelocityScale.x;
                r_y += speed * pDef->m_VelocityScale.y;
            }

            FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb,
                       r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
        }

        totalParticles += particleCount;
    }

    return totalParticles;
}

// Collect GPUParticleData for GPU culling path
static u32 CollectGPUParticleData(
    const xr_vector<ParticleBatch>& batches,
    xr_vector<GPUParticleData>& gpuParticles)
{
    u32 totalParticles = 0;

    for (const auto& batch : batches) {
        if (!batch.visual || batch.visual->getType() != MT_PARTICLE_EFFECT)
            continue;

        CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);
        auto* pDef = pEffect->GetDefinition();
        if (!pDef || !pDef->m_Flags.is(CPEDef::dfSprite))
            continue;

        PAPI::Particle* particles = nullptr;
        u32 particleCount = 0;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

        if (particleCount == 0 || !particles)
            continue;

        u32 baseIdx = (u32)gpuParticles.size();
        gpuParticles.resize(baseIdx + particleCount);

        for (u32 i = 0; i < particleCount; i++) {
            auto& m = particles[i];
            auto& gp = gpuParticles[baseIdx + i];

            gp.position = m.pos;
            gp.rotation = m.rot.x;

            float r_x = m.size.x * 0.5f;
            float r_y = m.size.y * 0.5f;

            if (pDef->m_Flags.is(CPEDef::dfVelocityScale)) {
                float speed = m.vel.magnitude();
                r_x += speed * pDef->m_VelocityScale.x;
                r_y += speed * pDef->m_VelocityScale.y;
            }

            gp.size.set(r_x, r_y);
            gp.color = m.color;
            gp.materialID = batch.bindlessMaterialID;

            if (pDef->m_Flags.is(CPEDef::dfFramed)) {
                pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), gp.uvMin, gp.uvMax);
            } else {
                gp.uvMin.set(0.f, 0.f);
                gp.uvMax.set(1.f, 1.f);
            }
        }

        totalParticles += particleCount;
    }

    return totalParticles;
}

void InitializeParticlePipelines(ng::RenderDevice* device)
{
    if (s_particleInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    Msg("* [ParticlePass] Initializing particle pipelines...");

    // Dummy framebuffer for pipeline creation
    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "ParticleInit_DummyColor";
    auto dummyColorRT = nvDevice->createTexture(colorDesc);

    nvrhi::TextureDesc normalDesc = colorDesc;
    normalDesc.debugName = "ParticleInit_DummyNormal";
    auto dummyNormalRT = nvDevice->createTexture(normalDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "ParticleInit_DummyDepth";
    auto dummyDepthRT = nvDevice->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.addColorAttachment(dummyNormalRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);
    if (!framebuffer)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    // Binding layout - no per-material CB needed anymore
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials (t8)
        nvrhi::BindingLayoutItem::Sampler(0),                 // g_LinearSampler (s0)
    };
    s_particleLayout = nvDevice->createBindingLayout(layoutDesc);

    // Load shaders
    auto vsResult = shaderLoader->LoadVertexShader("bindless_particle", "main");
    if (!vsResult.handle)
        return;
    s_particleVS = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("bindless_particle", "main");
    if (!psResult.handle)
        return;
    s_particlePS = psResult.handle;

    // Sampler
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    s_particleSampler = nvDevice->createSampler(samplerDesc);

    // Input layout (28 bytes per vertex now)
    constexpr u32 stride = sizeof(ParticleVertex);
    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setBufferIndex(0)
            .setOffset(0)
            .setElementStride(stride),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0)
            .setOffset(12)
            .setElementStride(stride),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setBufferIndex(0)
            .setOffset(16)
            .setElementStride(stride),
        nvrhi::VertexAttributeDesc()
            .setName("MATERIALID")
            .setFormat(nvrhi::Format::R32_UINT)
            .setBufferIndex(0)
            .setOffset(24)
            .setElementStride(stride),
    };
    s_particleInputLayout = nvDevice->createInputLayout(attribs, 4, s_particleVS);

    // Alpha blend pipeline
    {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_particleVS;
        pipeDesc.PS = s_particlePS;
        pipeDesc.inputLayout = s_particleInputLayout;
        pipeDesc.bindingLayouts.push_back(s_particleLayout);
        if (bindlessLayout)
            pipeDesc.bindingLayouts.push_back(bindlessLayout);
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
        pipeDesc.renderState.blendState.targets[0].enableBlend();
        pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
        pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;

        s_particlePipelineBlend = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        Msg("* [ParticlePass] Alpha blend pipeline: %s", s_particlePipelineBlend ? "OK" : "FAILED");

        // CRITICAL FIX: Query binding layout from pipeline
        if (s_particlePipelineBlend) {
            const nvrhi::GraphicsPipelineDesc& actualDesc = s_particlePipelineBlend->getDesc();
            if (!actualDesc.bindingLayouts.empty()) {
                s_particleLayout = actualDesc.bindingLayouts[0];
            }
        }
    }

    // Additive blend pipeline
    {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_particleVS;
        pipeDesc.PS = s_particlePS;
        pipeDesc.inputLayout = s_particleInputLayout;
        pipeDesc.bindingLayouts.push_back(s_particleLayout);
        if (bindlessLayout)
            pipeDesc.bindingLayouts.push_back(bindlessLayout);
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
        pipeDesc.renderState.blendState.targets[0].enableBlend();
        pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::One;

        s_particlePipelineAdd = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        Msg("* [ParticlePass] Additive blend pipeline: %s", s_particlePipelineAdd ? "OK" : "FAILED");

        // CRITICAL FIX: Query binding layout from pipeline
        if (s_particlePipelineAdd) {
            const nvrhi::GraphicsPipelineDesc& actualDesc = s_particlePipelineAdd->getDesc();
            if (!actualDesc.bindingLayouts.empty()) {
                s_particleLayout = actualDesc.bindingLayouts[0];
            }
        }
    }

    s_particleInitialized = true;
    Msg("* [ParticlePass] Pipeline initialization complete");

    // Initialize GPU culling manager
    s_gpuCullingManager = std::make_unique<ParticleGPUCullingManager>();
    if (!s_gpuCullingManager->Initialize(device, MAX_GPU_PARTICLES)) {
        Msg("! [ParticlePass] GPU culling initialization failed, using CPU fallback");
        s_gpuCullingManager.reset();
    }
}

void ShutdownParticlePipelines()
{
    if (s_gpuCullingManager) {
        s_gpuCullingManager->Shutdown();
        s_gpuCullingManager.reset();
    }
    s_particlePipelineBlend = nullptr;
    s_particlePipelineAdd = nullptr;
    s_particleLayout = nullptr;
    s_particleInputLayout = nullptr;
    s_particleVS = nullptr;
    s_particlePS = nullptr;
    s_particleSampler = nullptr;
    s_particleBindingSet = nullptr;
    s_particleVB = nullptr;
    s_quadIB = nullptr;
    s_particleVBSize = 0;
    s_maxQuads = 0;
    s_particleInitialized = false;
}

DefaultOutputLayout setupParticlePass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    VirtualResourceHandle hiZPyramid,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels)
{
    struct ParticlePassData {
        VirtualResourceHandle inputColor;
        VirtualResourceHandle depth;
        VirtualResourceHandle outputColor;
        VirtualResourceHandle outputNormal;
        VirtualResourceHandle hiZPyramid;
        ng::RenderDevice* device;
        const xr_vector<ParticleBatch>* worldParticleBatches;
        const xr_vector<ParticleBatch>* hudParticleBatches;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        u32 hiZWidth;
        u32 hiZHeight;
        u32 hiZMipLevels;
    };

    auto& passData = fg.addCallbackPass<ParticlePassData>(
        "Particles",
        [&, width, height, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels](FrameGraph& builder, PassHandle passHandle, ParticlePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.worldParticleBatches = worldParticleBatches;
            data.hudParticleBatches = hudParticleBatches;
            data.materialCache = materialCache;
            data.hiZPyramid = hiZPyramid;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;

            // Add Hi-Z as read dependency if valid
            if (hiZPyramid.is_valid())
                passBuilder.read(hiZPyramid);

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);
            data.outputNormal = passBuilder.readWrite(forwardInputs.normal, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.depth = data.depth;
        },
        [](const ParticlePassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            u32 totalWorld = data.worldParticleBatches ? (u32)data.worldParticleBatches->size() : 0;
            u32 totalHUD = data.hudParticleBatches ? (u32)data.hudParticleBatches->size() : 0;

            if ((totalWorld == 0 && totalHUD == 0) || !s_particleInitialized)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* normalRT = fg.GetPhysicalTexture(data.outputNormal);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            if (data.materialCache)
                data.materialCache->FinalizePendingMaterials(ctx);
            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = nvDevice->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            const auto& rtDesc = colorRT->getDesc();

            // Create constant buffers
            nvrhi::BufferDesc cbDesc;
            cbDesc.byteSize = sizeof(DynamicTransforms);
            cbDesc.isConstantBuffer = true;
            cbDesc.isVolatile = true;
            cbDesc.maxVersions = 16;
            auto dynTransformsCB = nvDevice->createBuffer(cbDesc);

            cbDesc.byteSize = sizeof(StaticGlobals);
            auto staticGlobalsCB = nvDevice->createBuffer(cbDesc);

            // Fill constants
            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dynTransformsCB, &dynTrans, sizeof(dynTrans));

            StaticGlobals staticGlobals;
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            // Create binding set (shared for all particles)
            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                nvrhi::BindingSetItem::Sampler(0, s_particleSampler),
            };
            auto bindingSet = nvDevice->createBindingSet(bindDesc, s_particleLayout);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            // Get Hi-Z texture for GPU culling
            nvrhi::ITexture* hiZTexture = data.hiZPyramid.is_valid() ? fg.GetPhysicalTexture(data.hiZPyramid) : nullptr;

            // Check if GPU culling is available
            // TODO: Re-enable once GPU particle culling is debugged
            bool useGPUCulling = s_gpuCullingManager && s_gpuCullingManager->IsReady() && hiZTexture;

            // Extract frustum planes at execute time (from current view matrix)
            Fvector4 frustumPlanes[6] = {};
            if (useGPUCulling) {
                CFrustum frustum;
                frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
                for (u32 i = 0; i < frustum.p_count && i < 6; i++) {
                    frustumPlanes[i].set(
                        frustum.planes[i].n.x,
                        frustum.planes[i].n.y,
                        frustum.planes[i].n.z,
                        frustum.planes[i].d
                    );
                }
            }

            // GPU culling path
            auto renderParticlesGPU = [&](const xr_vector<ParticleBatch>& batches, float depthMin, float depthMax) {
                xr_vector<GPUParticleData> gpuParticles;
                u32 totalParticles = CollectGPUParticleData(batches, gpuParticles);

                if (totalParticles == 0)
                    return;

                // Clamp to max particles
                if (totalParticles > s_gpuCullingManager->GetMaxParticles()) {
                    gpuParticles.resize(s_gpuCullingManager->GetMaxParticles());
                    totalParticles = s_gpuCullingManager->GetMaxParticles();
                }

                // Upload particle data
                s_gpuCullingManager->UploadParticleData(cmdList, gpuParticles);

                // Clear visible count
                s_gpuCullingManager->ClearVisibleCount(cmdList);

                // Dispatch culling
                s_gpuCullingManager->DispatchCulling(
                    cmdList,
                    hiZTexture,
                    Device.mFullTransform,
                    frustumPlanes,
                    Device.vCameraPosition,
                    Device.vCameraTop,
                    Device.vCameraRight,
                    totalParticles,
                    data.hiZWidth, data.hiZHeight, data.hiZMipLevels
                );

                // Dispatch billboard generation
                s_gpuCullingManager->DispatchBillboardGeneration(
                    cmdList,
                    Device.vCameraTop,
                    Device.vCameraRight,
                    totalParticles
                );

                // Ensure index buffer
                EnsureQuadIndexBuffer(nvDevice, totalParticles);
                if (!s_quadIB)
                    return;

                // Initialize buffer state tracking for this command list
                cmdList->beginTrackingBufferState(s_quadIB, nvrhi::ResourceStates::IndexBuffer);

                // Setup graphics state
                nvrhi::Viewport viewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    depthMin, depthMax
                );

                nvrhi::GraphicsState state;
                state.pipeline = s_particlePipelineBlend;
                state.framebuffer = framebuffer;
                state.bindings = { bindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.vertexBuffers = { {s_gpuCullingManager->GetVertexBuffer(), 0, 0} };
                state.indexBuffer = { s_quadIB, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = s_gpuCullingManager->GetDrawArgsBuffer();
                state.viewport.addViewport(viewport);
                state.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(state);

                // Indirect draw
                cmdList->drawIndexedIndirect(0, 1);
            };

            // CPU fallback path
            auto renderParticlesCPU = [&](const xr_vector<ParticleBatch>& batches, float depthMin, float depthMax) {
                xr_vector<ParticleVertex> vertices;
                u32 totalParticles = GenerateParticleVertices(batches, vertices);

                if (totalParticles == 0)
                    return;

                EnsureParticleVertexBuffer(nvDevice, (u32)(vertices.size() * sizeof(ParticleVertex)));
                EnsureQuadIndexBuffer(nvDevice, totalParticles);

                if (!s_particleVB || !s_quadIB)
                    return;

                // Initialize buffer state tracking for this command list
                cmdList->beginTrackingBufferState(s_particleVB, nvrhi::ResourceStates::VertexBuffer);
                cmdList->beginTrackingBufferState(s_quadIB, nvrhi::ResourceStates::IndexBuffer);

                cmdList->writeBuffer(s_particleVB, vertices.data(), vertices.size() * sizeof(ParticleVertex));

                nvrhi::Viewport viewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    depthMin, depthMax
                );

                nvrhi::GraphicsState state;
                state.pipeline = s_particlePipelineBlend;
                state.framebuffer = framebuffer;
                state.bindings = { bindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.vertexBuffers = { {s_particleVB, 0, 0} };
                state.indexBuffer = { s_quadIB, nvrhi::Format::R16_UINT, 0 };
                state.viewport.addViewport(viewport);
                state.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(state);

                cmdList->drawIndexed(
                    nvrhi::DrawArguments()
                        .setVertexCount(totalParticles * 6)
                        .setStartIndexLocation(0)
                        .setStartVertexLocation(0)
                );
            };

            // Render world particles (depth 0.0 - 1.0)
            if (totalWorld > 0) {
                if (useGPUCulling)
                    renderParticlesGPU(*data.worldParticleBatches, 0.0f, 1.0f);
                else
                    renderParticlesCPU(*data.worldParticleBatches, 0.0f, 1.0f);
            }

            // Render HUD particles (depth 0.0 - 0.1) - always use CPU path (no culling needed for HUD)
            if (totalHUD > 0)
                renderParticlesCPU(*data.hudParticleBatches, 0.0f, 0.1f);
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.normal = passData.outputNormal;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
