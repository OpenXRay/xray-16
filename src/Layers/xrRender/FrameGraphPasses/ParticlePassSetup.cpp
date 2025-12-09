// xrRender/FrameGraphPasses/ParticlePassSetup.cpp
// Modernized particle rendering with bindless material system
#include "stdafx.h"
#include "ParticlePassSetup.h"
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

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;
using RENDER_NAMESPACE::PS::CPEDef;

// ═══════════════════════════════════════════════════════════════════════════
//  PARTICLE PIPELINE INFRASTRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

// Static pipeline state (created once, used for all particles)
static nvrhi::GraphicsPipelineHandle s_particlePipelineBlend;     // Alpha blend
static nvrhi::GraphicsPipelineHandle s_particlePipelineAdd;       // Additive
static nvrhi::BindingLayoutHandle s_particleLayout;
static nvrhi::InputLayoutHandle s_particleInputLayout;
static nvrhi::ShaderHandle s_particleVS;
static nvrhi::ShaderHandle s_particlePS;
static nvrhi::SamplerHandle s_particleSampler;
static bool s_particleInitialized = false;

// Dynamic buffers (grow as needed)
static nvrhi::BufferHandle s_particleVB;
static u32 s_particleVBSize = 0;
static nvrhi::BufferHandle s_quadIB;
static u32 s_maxQuads = 0;

// ═══════════════════════════════════════════════════════════════════════════
//  HUD FOV ADJUSTMENT
// ═══════════════════════════════════════════════════════════════════════════

static Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    float fovScale = 1.0f / psHUD_FOV;
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;
    fovScaleMatrix._22 = fovScale;
    fovScaleMatrix._33 = 1.0f;

    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);
    temp2.mul(fovScaleMatrix, temp1);
    result.mul(invView, temp2);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BUFFER MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

static void EnsureQuadIndexBuffer(nvrhi::IDevice* nvDevice, u32 maxQuads)
{
    if (s_quadIB && s_maxQuads >= maxQuads)
        return;

    u32 numIndices = maxQuads * 6;
    xr_vector<u16> indices;
    indices.reserve(numIndices);

    for (u32 i = 0; i < maxQuads; i++) {
        u16 baseVertex = (u16)(i * 4);
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 3);
    }

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = numIndices * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "ParticleQuadIB";
    ibDesc.keepInitialState = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

    s_quadIB = nvDevice->createBuffer(ibDesc);
    if (!s_quadIB) {
        Msg("! [ParticlePass] ERROR: Failed to create quad index buffer");
        return;
    }

    // Upload via backend (handles in-frame batching or immediate execution)
    if (GEnv.Backend) {
        GEnv.Backend->UploadBufferData(s_quadIB, indices.data(), indices.size() * sizeof(u16));
    }

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
    vbDesc.keepInitialState = true;

    s_particleVB = nvDevice->createBuffer(vbDesc);
    if (!s_particleVB) {
        Msg("! [ParticlePass] ERROR: Failed to create particle vertex buffer");
        return;
    }

    s_particleVBSize = allocSize;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BILLBOARD GENERATION
// ═══════════════════════════════════════════════════════════════════════════

static void FillSprite(
    ParticleVertex*& pv,
    const Fvector& T, const Fvector& R,
    const Fvector& pos,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr,
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
    pv++;

    pv->p.set(a.x + pos.x, a.y + pos.y, a.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, lt.y);
    pv++;

    pv->p.set(c.x + pos.x, c.y + pos.y, c.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, rb.y);
    pv++;

    pv->p.set(b.x + pos.x, b.y + pos.y, b.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, lt.y);
    pv++;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void InitializeParticlePipelines(ng::RenderDevice* device)
{
    if (s_particleInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    Msg("* [ParticlePass] Initializing particle pipelines...");

    // Create dummy framebuffer for pipeline creation
    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "ParticleInit_DummyColor";
    auto dummyColorRT = nvDevice->createTexture(colorDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D24S8;
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "ParticleInit_DummyDepth";
    auto dummyDepthRT = nvDevice->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    if (!framebuffer) {
        Msg("! [ParticlePass] Failed to create dummy framebuffer");
        return;
    }

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    // Create binding layout (matches bindless pattern)
    nvrhi::BindingLayoutDesc particleLayoutDesc;
    particleLayoutDesc.visibility = nvrhi::ShaderType::All;
    particleLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // ParticleMaterialCB (b4)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials (t8)
        nvrhi::BindingLayoutItem::Sampler(0),                 // g_LinearSampler (s0)
    };
    s_particleLayout = nvDevice->createBindingLayout(particleLayoutDesc);

    // Load shaders
    auto vsResult = shaderLoader->LoadVertexShader("bindless_particle", "main");
    if (!vsResult.handle) {
        Msg("! [ParticlePass] Failed to load vertex shader");
        return;
    }
    s_particleVS = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("bindless_particle", "main");
    if (!psResult.handle) {
        Msg("! [ParticlePass] Failed to load pixel shader");
        return;
    }
    s_particlePS = psResult.handle;

    // Create sampler
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    s_particleSampler = nvDevice->createSampler(samplerDesc);

    // Create input layout (ParticleVertex: 24 bytes)
    constexpr u32 stride = sizeof(ParticleVertex);  // 24 bytes
    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)
            .setElementStride(stride),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setOffset(12)
            .setElementStride(stride),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(16)
            .setElementStride(stride),
    };
    s_particleInputLayout = nvDevice->createInputLayout(attribs, 3, s_particleVS);

    // ─────────────────────────────────────────────────────
    //  ALPHA BLEND PIPELINE
    // ─────────────────────────────────────────────────────
    {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_particleVS;
        pipeDesc.PS = s_particlePS;
        pipeDesc.inputLayout = s_particleInputLayout;
        if (bindlessLayout) {
            pipeDesc.bindingLayouts = { s_particleLayout, bindlessLayout };
        } else {
            pipeDesc.bindingLayouts = { s_particleLayout };
        }
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;

        // Depth: test enabled, write disabled (particles don't write depth)
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;

        // Culling: none (billboards face camera)
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

        // Blending: alpha blend (src*srcAlpha + dst*invSrcAlpha)
        pipeDesc.renderState.blendState.targets[0].enableBlend();
        pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
        pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;

        s_particlePipelineBlend = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        Msg("* [ParticlePass] Alpha blend pipeline: %s", s_particlePipelineBlend ? "OK" : "FAILED");
    }

    // ─────────────────────────────────────────────────────
    //  ADDITIVE BLEND PIPELINE
    // ─────────────────────────────────────────────────────
    {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_particleVS;
        pipeDesc.PS = s_particlePS;
        pipeDesc.inputLayout = s_particleInputLayout;
        if (bindlessLayout) {
            pipeDesc.bindingLayouts = { s_particleLayout, bindlessLayout };
        } else {
            pipeDesc.bindingLayouts = { s_particleLayout };
        }
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;

        // Depth: test enabled, write disabled
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;

        // Culling: none
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

        // Blending: additive (src + dst)
        pipeDesc.renderState.blendState.targets[0].enableBlend();
        pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::One;

        s_particlePipelineAdd = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        Msg("* [ParticlePass] Additive blend pipeline: %s", s_particlePipelineAdd ? "OK" : "FAILED");
    }

    s_particleInitialized = true;
    Msg("* [ParticlePass] Pipeline initialization complete");
}

void ShutdownParticlePipelines()
{
    s_particlePipelineBlend = nullptr;
    s_particlePipelineAdd = nullptr;
    s_particleLayout = nullptr;
    s_particleInputLayout = nullptr;
    s_particleVS = nullptr;
    s_particlePS = nullptr;
    s_particleSampler = nullptr;
    s_particleVB = nullptr;
    s_quadIB = nullptr;
    s_particleVBSize = 0;
    s_maxQuads = 0;
    s_particleInitialized = false;

    Msg("* [ParticlePass] Pipeline resources released");
}

// ═══════════════════════════════════════════════════════════════════════════
//  RENDER PARTICLE EFFECT
// ═══════════════════════════════════════════════════════════════════════════

static bool RenderParticleEffect(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::IBuffer* dynTransformsCB,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* matIdCB,
    nvrhi::IDescriptorTable* bindlessTable,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    const ParticleBatch& batch,
    bool isAdditive)
{
    CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);
    if (!pEffect)
        return false;

    auto* pDef = pEffect->GetDefinition();
    if (!pDef || !pDef->m_Flags.is(CPEDef::dfSprite))
        return false;

    PAPI::Particle* particles = nullptr;
    u32 particleCount = 0;
    PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

    if (particleCount == 0 || !particles)
        return false;

    // Ensure buffers
    u32 requiredVBSize = particleCount * 4 * sizeof(ParticleVertex);
    EnsureParticleVertexBuffer(nvDevice, requiredVBSize);
    EnsureQuadIndexBuffer(nvDevice, particleCount);

    if (!s_particleVB || !s_quadIB)
        return false;

    // Generate billboard geometry
    xr_vector<ParticleVertex> vertices;
    vertices.resize(particleCount * 4);
    ParticleVertex* pv = vertices.data();

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

        if (pDef->m_Flags.is(CPEDef::dfFramed)) {
            pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), lt, rb);
        }

        float r_x = m.size.x * 0.5f;
        float r_y = m.size.y * 0.5f;

        if (pDef->m_Flags.is(CPEDef::dfVelocityScale)) {
            float speed = m.vel.magnitude();
            r_x += speed * pDef->m_VelocityScale.x;
            r_y += speed * pDef->m_VelocityScale.y;
        }

        FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb, r_x, r_y, m.color, sina, cosa);
    }

    // Upload vertex data
    cmdList->writeBuffer(s_particleVB, vertices.data(), vertices.size() * sizeof(ParticleVertex));

    // Select pipeline based on blend mode
    nvrhi::IGraphicsPipeline* pipeline = isAdditive ? s_particlePipelineAdd.Get() : s_particlePipelineBlend.Get();
    if (!pipeline)
        return false;

    // Upload dynamic transforms (identity for particles - positions are world space)
    DynamicTransforms dynTransData = {};
    FillDynamicTransforms(dynTransData);
    cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

    // Upload material ID
    struct ParticleMaterialCB {
        u32 materialID;
        u32 textureIndex;
        u32 pad0, pad1;
    } matIdData;
    matIdData.materialID = batch.bindlessMaterialID;
    matIdData.textureIndex = 0;  // Not used - using MaterialData lookup
    matIdData.pad0 = matIdData.pad1 = 0;
    cmdList->writeBuffer(matIdCB, &matIdData, sizeof(matIdData));

    // Create binding set
    auto& matBuffer = MaterialBuffer::Instance();
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
        nvrhi::BindingSetItem::ConstantBuffer(4, matIdCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_particleSampler),
    };
    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_particleLayout);

    // Set up graphics state
    nvrhi::GraphicsState state;
    state.pipeline = pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };
    if (bindlessTable) {
        state.addBindingSet(bindlessTable);
    }
    state.vertexBuffers = { {s_particleVB, 0, 0} };
    state.indexBuffer = { s_quadIB, nvrhi::Format::R16_UINT, 0 };
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(scissor);

    cmdList->setGraphicsState(state);

    // Draw
    cmdList->drawIndexed(
        nvrhi::DrawArguments()
            .setVertexCount(particleCount * 6)
            .setStartIndexLocation(0)
            .setStartVertexLocation(0)
    );

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SETUP PARTICLE PASS
// ═══════════════════════════════════════════════════════════════════════════

DefaultOutputLayout setupParticlePass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    nvrhi::IBuffer* particleDrawArgsBuffer)
{
    struct ParticlePassData {
        VirtualResourceHandle inputColor;
        VirtualResourceHandle depth;
        VirtualResourceHandle outputColor;

        ng::RenderDevice* device;
        const xr_vector<ParticleBatch>* worldParticleBatches;
        const xr_vector<ParticleBatch>* hudParticleBatches;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<ParticlePassData>(
        "Particles",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA
        // ═══════════════════════════════════════════════════════
        [&, width, height](FrameGraph& builder, PassHandle passHandle, ParticlePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.worldParticleBatches = worldParticleBatches;
            data.hudParticleBatches = hudParticleBatches;
            data.materialCache = materialCache;

            // Read color input (from Forward+/Skinning)
            data.inputColor = passBuilder.read(forwardInputs.albedo);

            // Write to same target (particles render on top)
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);

            // Read-write depth (for depth testing)
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.outputColor;
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA
        // ═══════════════════════════════════════════════════════
        [](const ParticlePassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {

            u32 totalWorld = data.worldParticleBatches ? (u32)data.worldParticleBatches->size() : 0;
            u32 totalHUD = data.hudParticleBatches ? (u32)data.hudParticleBatches->size() : 0;

            if (totalWorld == 0 && totalHUD == 0)
                return;

            if (!s_particleInitialized)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!colorRT || !depthRT) {
                Msg("! [ParticlePass] Failed to get physical textures");
                return;
            }

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            // Create framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = nvDevice->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            const auto& rtDesc = colorRT->getDesc();

            // Create constant buffers
            nvrhi::BufferDesc dynTransCbDesc;
            dynTransCbDesc.byteSize = sizeof(DynamicTransforms);
            dynTransCbDesc.isConstantBuffer = true;
            dynTransCbDesc.isVolatile = true;
            dynTransCbDesc.maxVersions = 128;
            auto dynTransformsCB = nvDevice->createBuffer(dynTransCbDesc);

            nvrhi::BufferDesc staticGlobalsCbDesc;
            staticGlobalsCbDesc.byteSize = sizeof(StaticGlobals);
            staticGlobalsCbDesc.isConstantBuffer = true;
            staticGlobalsCbDesc.isVolatile = true;
            staticGlobalsCbDesc.maxVersions = 16;
            auto staticGlobalsCB = nvDevice->createBuffer(staticGlobalsCbDesc);

            // Fill static globals
            StaticGlobals staticGlobals;
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            // Create material ID buffer
            nvrhi::BufferDesc matIdCbDesc;
            matIdCbDesc.byteSize = 16;
            matIdCbDesc.isConstantBuffer = true;
            matIdCbDesc.isVolatile = true;
            matIdCbDesc.maxVersions = 128;
            auto matIdCB = nvDevice->createBuffer(matIdCbDesc);

            // Finalize any pending materials
            if (data.materialCache) {
                data.materialCache->FinalizePendingMaterials(ctx);
            }

            // Upload material buffer
            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            // Get bindless descriptor table
            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // Scissor rect (same for all particles)
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            // ═══════════════════════════════════════════════════════
            //  PHASE 1: WORLD PARTICLES (depth [0.0, 1.0])
            // ═══════════════════════════════════════════════════════
            if (totalWorld > 0) {
                nvrhi::Viewport worldViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 1.0f
                );

                for (const auto& batch : *data.worldParticleBatches) {
                    if (batch.visual && batch.visual->getType() == MT_PARTICLE_EFFECT) {
                        // Determine if additive based on particle definition
                        bool isAdditive = false;
                        auto* pEffect = static_cast<CParticleEffect*>(batch.visual);
                        auto* pDef = pEffect->GetDefinition();
                        // Could check pDef for blend mode flags if needed

                        RenderParticleEffect(
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, staticGlobalsCB, matIdCB,
                            bindlessTable, worldViewport, scissor,
                            batch, isAdditive
                        );
                    }
                }
            }

            // ═══════════════════════════════════════════════════════
            //  PHASE 2: HUD PARTICLES (depth [0.0, 0.1])
            // ═══════════════════════════════════════════════════════
            if (totalHUD > 0) {
                nvrhi::Viewport hudViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 0.1f
                );

                for (const auto& batch : *data.hudParticleBatches) {
                    if (batch.visual && batch.visual->getType() == MT_PARTICLE_EFFECT) {
                        bool isAdditive = false;

                        RenderParticleEffect(
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, staticGlobalsCB, matIdCB,
                            bindlessTable, hudViewport, scissor,
                            batch, isAdditive
                        );
                    }
                }
            }
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
