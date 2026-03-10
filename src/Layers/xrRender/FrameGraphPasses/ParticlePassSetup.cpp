// xrRender/FrameGraphPasses/ParticlePassSetup.cpp
// Batched particle rendering with GPU culling
#include "stdafx.h"
#include "ParticlePassSetup.h"
#include "ParticleGPUCullingManager.h"
#include "PassCommon.h"
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
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "xrParticles/psystem.h"
#include "xrCDB/Frustum.h"  // For CFrustum (frustum plane extraction)

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;
using RENDER_NAMESPACE::PS::CPEDef;

static constexpr u32 MAX_GPU_PARTICLES = 65536;

static void EnsureQuadIndexBuffer(nvrhi::IDevice* nvDevice, u32 maxQuads, ParticlePassState& state)
{
    if (state.quadIB && state.maxQuads >= maxQuads)
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
    ibDesc.keepInitialState = true;

    state.quadIB = nvDevice->createBuffer(ibDesc);
    if (!state.quadIB)
        return;

    nvrhi::CommandListHandle cmdList = nvDevice->createCommandList();
    cmdList->open();
    cmdList->writeBuffer(state.quadIB, indices.data(), indices.size() * sizeof(u16));
    cmdList->close();
    nvDevice->executeCommandList(cmdList);

    state.maxQuads = maxQuads;
}

static void EnsureParticleVertexBuffer(nvrhi::IDevice* nvDevice, u32 sizeBytes, ParticlePassState& state)
{
    if (state.particleVB && state.particleVBSize >= sizeBytes)
        return;

    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;

    state.particleVB = nvDevice->createBuffer(vbDesc);
    state.particleVBSize = state.particleVB ? allocSize : 0;
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

static void FillSpriteAligned(
    ParticleVertex*& pv,
    const Fvector& pos, const Fvector& dir,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr, u32 matID,
    float sina, float cosa)
{
    Fvector R;
    R.crossproduct(dir, Device.vCameraDirection);
    float mag = R.magnitude();
    if (mag > EPS_S)
        R.div(mag);
    else
        R.set(Device.vCameraRight);

    FillSprite(pv, dir, R, pos, lt, rb, r1, r2, clr, matID, sina, cosa);
}

static Fmatrix BuildHUDFOVMatrix()
{
    float fovScale = 1.0f / psHUD_FOV;
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);
    Fmatrix fovScaleMat;
    fovScaleMat.identity();
    fovScaleMat._11 = fovScale;
    fovScaleMat._22 = fovScale;
    Fmatrix t1, result;
    t1.mul(fovScaleMat, viewMatrix);
    result.mul(invView, t1);
    return result;
}

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

        bool alignToPath = pDef->m_Flags.is(CPEDef::dfAlignToPath);
        bool worldAlign = pDef->m_Flags.is(CPEDef::dfWorldAlign);
        bool faceAlign = pDef->m_Flags.is(CPEDef::dfFaceAlign);
        bool hasXForm = pEffect->m_RT_Flags.is(CParticleEffect::flRT_XFORM);
        const Fmatrix& xform = pEffect->m_XFORM;

        Fmatrix hudMat;
        bool isHUD = batch.isHUDMode;
        if (isHUD)
            hudMat = BuildHUDFOVMatrix();

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

            float speed = 0.f;
            bool speedCalc = false;

            if (pDef->m_Flags.is(CPEDef::dfVelocityScale)) {
                speed = m.vel.magnitude();
                speedCalc = true;
                r_x += speed * pDef->m_VelocityScale.x;
                r_y += speed * pDef->m_VelocityScale.y;
            }

            ParticleVertex* pvStart = pv;

            if (alignToPath) {
                if (!speedCalc)
                    speed = m.vel.magnitude();

                if ((speed < EPS_S) && worldAlign) {
                    Fmatrix M;
                    M.setXYZ(pDef->m_APDefaultRotation);
                    if (hasXForm) {
                        Fvector p;
                        xform.transform_tiny(p, m.pos);
                        M.mulA_43(xform);
                        FillSprite(pv, M.k, M.i, p, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    } else {
                        FillSprite(pv, M.k, M.i, m.pos, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    }
                } else if ((speed >= EPS_S) && faceAlign) {
                    Fmatrix M;
                    M.identity();
                    M.k.div(m.vel, speed);
                    M.j.set(0, 1, 0);
                    if (_abs(M.j.dotproduct(M.k)) > .99f)
                        M.j.set(0, 0, 1);
                    M.i.crossproduct(M.j, M.k);
                    M.i.normalize();
                    M.j.crossproduct(M.k, M.i);
                    M.j.normalize();
                    if (hasXForm) {
                        Fvector p;
                        xform.transform_tiny(p, m.pos);
                        M.mulA_43(xform);
                        FillSprite(pv, M.j, M.i, p, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    } else {
                        FillSprite(pv, M.j, M.i, m.pos, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    }
                } else {
                    Fvector dir;
                    if (speed >= EPS_S)
                        dir.div(m.vel, speed);
                    else
                        dir.setHP(-pDef->m_APDefaultRotation.y, -pDef->m_APDefaultRotation.x);
                    if (hasXForm) {
                        Fvector p, d;
                        xform.transform_tiny(p, m.pos);
                        xform.transform_dir(d, dir);
                        FillSpriteAligned(pv, p, d, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    } else {
                        FillSpriteAligned(pv, m.pos, dir, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                    }
                }
            } else {
                if (hasXForm) {
                    Fvector p;
                    xform.transform_tiny(p, m.pos);
                    FillSprite(pv, Device.vCameraTop, Device.vCameraRight, p, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                } else {
                    FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb, r_x, r_y, m.color, batch.bindlessMaterialID, sina, cosa);
                }
            }

            if (isHUD) {
                for (ParticleVertex* v = pvStart; v < pv; v++) {
                    Fvector tmp;
                    hudMat.transform_tiny(tmp, v->p);
                    v->p = tmp;
                }
            }
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

struct ParticleBlendDesc {
    nvrhi::BlendFactor srcBlend;
    nvrhi::BlendFactor destBlend;
    nvrhi::BlendFactor srcBlendAlpha;
    nvrhi::BlendFactor destBlendAlpha;
    bool blendEnable;
    bool depthWrite;
    const char* name;
};

static const ParticleBlendDesc s_blendDescs[PARTICLE_BLEND_COUNT] = {
    { nvrhi::BlendFactor::One,       nvrhi::BlendFactor::Zero,        nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero,        false, true,  "ParticlePass_set" },
    { nvrhi::BlendFactor::SrcAlpha,  nvrhi::BlendFactor::InvSrcAlpha, nvrhi::BlendFactor::One, nvrhi::BlendFactor::InvSrcAlpha, true,  false, "ParticlePass_blend" },
    { nvrhi::BlendFactor::One,       nvrhi::BlendFactor::One,         nvrhi::BlendFactor::One, nvrhi::BlendFactor::One,         true,  false, "ParticlePass_add" },
    { nvrhi::BlendFactor::DstColor,  nvrhi::BlendFactor::Zero,        nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero,        true,  false, "ParticlePass_mul" },
    { nvrhi::BlendFactor::DstColor,  nvrhi::BlendFactor::SrcColor,    nvrhi::BlendFactor::One, nvrhi::BlendFactor::SrcAlpha,    true,  false, "ParticlePass_mul2x" },
    { nvrhi::BlendFactor::SrcAlpha,  nvrhi::BlendFactor::One,         nvrhi::BlendFactor::One, nvrhi::BlendFactor::One,         true,  false, "ParticlePass_alphaAdd" },
};

void InitializeParticleResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, ParticlePassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = framegraph::GetPassResourceCache();
    auto fbInfo = framebuffer->getFramebufferInfo();

    auto vsResult = shaderLoader->LoadVertexShader("bindless_particle", "main");
    if (!vsResult.handle)
        return;
    state.vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("bindless_particle", "main");
    if (!psResult.handle)
        return;
    state.ps = psResult.handle;

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("ParticlePass", *vsResult.reflection, *psResult.reflection, nvDevice);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

    constexpr u32 stride = sizeof(ParticleVertex);
    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(0).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::BGRA8_UNORM).setBufferIndex(0).setOffset(12).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setBufferIndex(0).setOffset(16).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("MATERIALID").setFormat(nvrhi::Format::R32_UINT).setBufferIndex(0).setOffset(24).setElementStride(stride),
    };
    state.inputLayout = nvDevice->createInputLayout(attribs, 4, state.vs);

    for (u32 i = 0; i < PARTICLE_BLEND_COUNT; i++) {
        const auto& bd = s_blendDescs[i];

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = state.vs;
        pipeDesc.PS = state.ps;
        pipeDesc.inputLayout = state.inputLayout;
        pipeDesc.bindingLayouts.push_back(state.layout);
        if (bindlessLayout)
            pipeDesc.bindingLayouts.push_back(bindlessLayout);
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = bd.depthWrite;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

        if (bd.blendEnable) {
            pipeDesc.renderState.blendState.targets[0].enableBlend();
            pipeDesc.renderState.blendState.targets[0].srcBlend = bd.srcBlend;
            pipeDesc.renderState.blendState.targets[0].destBlend = bd.destBlend;
            pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = bd.srcBlendAlpha;
            pipeDesc.renderState.blendState.targets[0].destBlendAlpha = bd.destBlendAlpha;
        }

        state.pipelines[i] = cache.GetOrCreatePipeline(bd.name, pipeDesc, fbInfo, nvDevice);

        if (state.pipelines[i] && i == 0) {
            const auto& actualDesc = state.pipelines[i]->getDesc();
            if (!actualDesc.bindingLayouts.empty())
                state.layout = actualDesc.bindingLayouts[0];
        }
    }

    state.initialized = true;
    Msg("* [ParticlePass] Pipeline initialization complete (6 blend modes + distortion)");

    if (!state.gpuCullingManager) {
        state.gpuCullingManager = xr_make_unique<ParticleGPUCullingManager>();
        if (!state.gpuCullingManager->Initialize(device, MAX_GPU_PARTICLES)) {
            Msg("! [ParticlePass] GPU culling initialization failed, using CPU fallback");
            state.gpuCullingManager.reset();
        }
    }
}

static void InitializeDistortionPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* distortFB, ParticlePassState& state)
{
    if (state.distortInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!nvDevice || !shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = framegraph::GetPassResourceCache();
    auto fbInfo = distortFB->getFramebufferInfo();

    auto distortPsResult = shaderLoader->LoadPixelShader("bindless_particle_distort", "main");
    if (!distortPsResult.handle)
        return;
    state.distortPS = distortPsResult.handle;

    auto distortVsResult = shaderLoader->LoadVertexShader("bindless_particle", "main");
    state.distortLayout = cache.GetOrCreateBindingLayoutFromReflection("ParticlePass_Distort", *distortVsResult.reflection, *distortPsResult.reflection, nvDevice);

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.distortPS;
    pipeDesc.inputLayout = state.inputLayout;
    pipeDesc.bindingLayouts.push_back(state.distortLayout);
    if (bindlessLayout)
        pipeDesc.bindingLayouts.push_back(bindlessLayout);
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipeDesc.renderState.blendState.targets[0].enableBlend();
    pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::One;
    pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::One;
    pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
    pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::One;

    state.distortPipeline = cache.GetOrCreatePipeline("ParticlePass_distort", pipeDesc, fbInfo, nvDevice);

    if (state.distortPipeline) {
        const auto& actualDesc = state.distortPipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty())
            state.distortLayout = actualDesc.bindingLayouts[0];
    }

    state.distortInitialized = true;
}

ParticlePassOutput setupParticlePass(
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
    u32 hiZMipLevels,
    ParticlePassState* state)
{
    auto& passData = fg.addCallbackPass<ParticlePassData>(
        "Particles",
        [&, width, height, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels, state](FrameGraph& builder, PassHandle passHandle, ParticlePassData& data) {
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
            data.passState = state;

            auto hasDistortBatch = [](const xr_vector<ParticleBatch>* batches) {
                if (!batches) return false;
                for (const auto& b : *batches)
                    if (b.shaderVariant == ParticleShaderVariant::Distort) return true;
                return false;
            };
            data.hasDistortion = hasDistortBatch(worldParticleBatches) || hasDistortBatch(hudParticleBatches);

            if (data.hasDistortion) {
                framegraph::ResourceDesc distDesc;
                distDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                distDesc.width = width;
                distDesc.height = height;
                distDesc.format = nvrhi::Format::RGBA16_FLOAT;
                distDesc.isRenderTarget = true;
                distDesc.isTransient = true;
                distDesc.isUAV = true;
                distDesc.debugName = "rt_Distortion";
                data.distortionRT = passBuilder.createTexture("rt_Distortion", distDesc);
            }

            if (hiZPyramid.is_valid())
                passBuilder.read(hiZPyramid);

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);
            data.outputNormal = passBuilder.readWrite(forwardInputs.normal, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);
            if (forwardInputs.baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(forwardInputs.baseColor, ResourceState::RenderTarget);
            if (forwardInputs.worldPos.is_valid()) {
                data.worldPos = passBuilder.readWrite(forwardInputs.worldPos, ResourceState::RenderTarget);

                framegraph::ResourceDesc wpCopyDesc;
                wpCopyDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                wpCopyDesc.width = width;
                wpCopyDesc.height = height;
                wpCopyDesc.format = nvrhi::Format::RGBA32_FLOAT;
                wpCopyDesc.isRenderTarget = true;
                wpCopyDesc.isTransient = true;
                wpCopyDesc.debugName = "rt_WorldPosCopy";
                data.worldPosCopy = passBuilder.createTexture("rt_WorldPosCopy", wpCopyDesc);
            }

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.worldPos = data.worldPos;
            data.outputs.depth = data.depth;
        },
        [](const ParticlePassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            u32 totalWorld = data.worldParticleBatches ? (u32)data.worldParticleBatches->size() : 0;
            u32 totalHUD = data.hudParticleBatches ? (u32)data.hudParticleBatches->size() : 0;

            if ((totalWorld == 0 && totalHUD == 0) || !data.passState)
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

            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;
            auto* worldPosCopyRT = data.worldPosCopy.is_valid() ? fg.GetPhysicalTexture(data.worldPosCopy) : nullptr;
            if (!worldPosCopyRT)
                worldPosCopyRT = worldPosRT;

            if (worldPosRT && worldPosCopyRT && worldPosCopyRT != worldPosRT)
                cmdList->copyTexture(worldPosCopyRT, nvrhi::TextureSlice(), worldPosRT, nvrhi::TextureSlice());

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            if (worldPosRT)
                fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("ParticlePass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            InitializeParticleResources(data.device, framebuffer, *data.passState);
            if (!data.passState->initialized)
                return;

            const auto& rtDesc = colorRT->getDesc();

            auto dynTransformsCB = cache.GetOrCreateVolatileCB("ParticlePass", "DynTransforms", sizeof(DynamicTransforms), ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS, nvDevice).Get();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("ParticlePass", "StaticGlobals", sizeof(StaticGlobals), ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS, nvDevice).Get();

            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dynTransformsCB, &dynTrans, sizeof(dynTrans));

            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_particle", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_particle", ".ps");
            BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Particle");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
               .BufferSRV("g_Materials", matBuffer.GetBuffer())
               .Texture("g_SceneWorldPos", worldPosCopyRT);
            auto bindDesc = bsb.Build();
            auto bindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, data.passState->layout, nvDevice);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            // Render order: SET first (opaque w/ depth write), then transparent modes
            static const u8 s_renderOrder[] = {
                PARTICLE_BLEND_SET,
                PARTICLE_BLEND_MUL,
                PARTICLE_BLEND_MUL_2X,
                PARTICLE_BLEND_BLEND,
                PARTICLE_BLEND_ADD,
                PARTICLE_BLEND_ALPHA_ADD,
            };

            auto renderBatchGroup = [&](const xr_vector<ParticleBatch>& allBatches, u8 blendMode,
                                        float depthMin, float depthMax)
            {
                xr_vector<ParticleBatch> filtered;
                for (const auto& b : allBatches) {
                    if (b.blendMode == blendMode && b.shaderVariant == ParticleShaderVariant::Standard)
                        filtered.push_back(b);
                }
                if (filtered.empty())
                    return;

                xr_vector<ParticleVertex> vertices;
                u32 totalParticles = GenerateParticleVertices(filtered, vertices);
                if (totalParticles == 0)
                    return;

                EnsureParticleVertexBuffer(nvDevice, (u32)(vertices.size() * sizeof(ParticleVertex)), *data.passState);
                EnsureQuadIndexBuffer(nvDevice, totalParticles, *data.passState);
                if (!data.passState->particleVB || !data.passState->quadIB)
                    return;

                cmdList->writeBuffer(data.passState->particleVB, vertices.data(), vertices.size() * sizeof(ParticleVertex));

                auto pipeline = data.passState->pipelines[blendMode];
                if (!pipeline)
                    pipeline = data.passState->pipelines[PARTICLE_BLEND_BLEND];
                if (!pipeline)
                    return;

                nvrhi::Viewport viewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    depthMin, depthMax
                );

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = { bindingSet };
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                gfxState.vertexBuffers = { {data.passState->particleVB, 0, 0} };
                gfxState.indexBuffer = { data.passState->quadIB, nvrhi::Format::R16_UINT, 0 };
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);
                cmdList->drawIndexed(
                    nvrhi::DrawArguments()
                        .setVertexCount(totalParticles * 6)
                        .setStartIndexLocation(0)
                        .setStartVertexLocation(0)
                );
            };

            if (totalWorld > 0) {
                for (u8 mode : s_renderOrder)
                    renderBatchGroup(*data.worldParticleBatches, mode, 0.0f, 1.0f);
            }

            if (totalHUD > 0) {
                for (u8 mode : s_renderOrder)
                    renderBatchGroup(*data.hudParticleBatches, mode, 0.0f, 0.1f);
            }

            if (!data.hasDistortion || !data.distortionRT.is_valid())
                return;

            auto* distortRT = fg.GetPhysicalTexture(data.distortionRT);
            if (!distortRT)
                return;

            nvrhi::FramebufferDesc distortFbDesc;
            distortFbDesc.addColorAttachment(distortRT);
            distortFbDesc.setDepthAttachment(depthRT);
            auto distortFB = cache.GetOrCreateFramebuffer("ParticlePass_distort", distortFbDesc, nvDevice);
            if (!distortFB)
                return;

            InitializeDistortionPipeline(data.device, distortFB, *data.passState);
            if (!data.passState->distortPipeline)
                return;

            cmdList->clearTextureFloat(distortRT, nvrhi::AllSubresources, nvrhi::Color(0.f, 0.f, 0.f, 0.f));

            auto* distortVsReflection = shaderLoader->GetCachedReflection("bindless_particle", ".vs");
            auto* distortPsReflection = shaderLoader->GetCachedReflection("bindless_particle_distort", ".ps");
            BindingSetBuilder distortBsb(*distortVsReflection, *distortPsReflection, nvDevice, "Particle.Distort");
            distortBsb.ConstantBuffer("static_globals", staticGlobalsCB)
                      .BufferSRV("g_Materials", matBuffer.GetBuffer());
            auto distortBindDesc = distortBsb.Build();
            auto distortBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(
                distortBindDesc, data.passState->distortLayout, nvDevice);

            auto renderDistortGroup = [&](const xr_vector<ParticleBatch>& allBatches,
                                          float depthMin, float depthMax)
            {
                xr_vector<ParticleBatch> filtered;
                for (const auto& b : allBatches)
                    if (b.shaderVariant == ParticleShaderVariant::Distort)
                        filtered.push_back(b);
                if (filtered.empty())
                    return;

                xr_vector<ParticleVertex> vertices;
                u32 totalParticles = GenerateParticleVertices(filtered, vertices);
                if (totalParticles == 0)
                    return;

                EnsureParticleVertexBuffer(nvDevice, (u32)(vertices.size() * sizeof(ParticleVertex)), *data.passState);
                EnsureQuadIndexBuffer(nvDevice, totalParticles, *data.passState);
                if (!data.passState->particleVB || !data.passState->quadIB)
                    return;

                cmdList->writeBuffer(data.passState->particleVB, vertices.data(), vertices.size() * sizeof(ParticleVertex));

                nvrhi::Viewport viewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    depthMin, depthMax
                );

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = data.passState->distortPipeline;
                gfxState.framebuffer = distortFB;
                gfxState.bindings = { distortBindingSet };
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                gfxState.vertexBuffers = { {data.passState->particleVB, 0, 0} };
                gfxState.indexBuffer = { data.passState->quadIB, nvrhi::Format::R16_UINT, 0 };
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);
                cmdList->drawIndexed(
                    nvrhi::DrawArguments()
                        .setVertexCount(totalParticles * 6)
                        .setStartIndexLocation(0)
                        .setStartVertexLocation(0)
                );
            };

            if (totalWorld > 0)
                renderDistortGroup(*data.worldParticleBatches, 0.0f, 1.0f);
            if (totalHUD > 0)
                renderDistortGroup(*data.hudParticleBatches, 0.0f, 0.1f);
        }
    );

    ParticlePassOutput output;
    output.layout.albedo = passData.outputColor;
    output.layout.normal = passData.outputNormal;
    output.layout.baseColor = passData.baseColor;
    output.layout.worldPos = passData.worldPos;
    output.layout.depth = passData.depth;
    output.distortionRT = passData.distortionRT;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
