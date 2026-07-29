// xrRender/FrameGraphPasses/ParticlePassSetup.cpp
// Batched particle rendering with GPU culling
#include "stdafx.h"
#include "ParticlePassSetup.h"
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

namespace xray::render::fg::passes {

using namespace framegraph;
using namespace bindless;
using fg::PS::CParticleEffect;
using fg::PS::CParticleGroup;
using fg::PS::CPEDef;

static constexpr u32 PARTICLE_CULL_MAX_SLOTS = 1024;

struct ParticleCullSlot {
    Fvector position;
    float radius;
    u32 batchIndex;
    u32 flags;
    float pad0;
    float pad1;
};
static_assert(sizeof(ParticleCullSlot) == 32, "ParticleCullSlot must be 32 bytes");

struct ParticleDrawArgs {
    u32 indexCount;
    u32 instanceCount;
    u32 startIndex;
    s32 baseVertex;
    u32 startInstance;
};
static_assert(sizeof(ParticleDrawArgs) == 20, "ParticleDrawArgs must be 20 bytes");

struct ParticleCullParamsCB {
    Fmatrix prevViewProj;
    Fvector4 frustumPlanes[6];
    Fvector4 cameraPos;
    u32 slotCount;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
};
static_assert(sizeof(ParticleCullParamsCB) == 192, "ParticleCullParamsCB must be 192 bytes");

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
    xr_vector<ParticleVertex>& vertices,
    xr_vector<u32>* outCounts = nullptr)
{
    u32 totalParticles = 0;

    for (const auto& batch : batches) {
        if (outCounts)
            outCounts->push_back(0);

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

        if (outCounts)
            outCounts->back() = particleCount;
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

void InitializeParticleResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ParticlePassState& state)
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
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
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

    if (!state.cullPipeline) {
        auto csResult = shaderLoader->LoadComputeShader("particle_cull");
        if (csResult.handle) {
            state.cullLayout = cache.GetOrCreateBindingLayoutFromReflection("ParticleCull", *csResult.reflection, nvDevice);
            if (state.cullLayout) {
                nvrhi::ComputePipelineDesc cullDesc;
                cullDesc.CS = csResult.handle;
                cullDesc.bindingLayouts = { state.cullLayout };
                state.cullPipeline = cache.GetOrCreateComputePipeline("ParticleCull", cullDesc, nvDevice);
            }
        }

        if (state.cullPipeline) {
            nvrhi::BufferDesc od;
            od.debugName = "ParticleCullSlots";
            od.byteSize = PARTICLE_CULL_MAX_SLOTS * sizeof(ParticleCullSlot);
            od.structStride = sizeof(ParticleCullSlot);
            od.initialState = nvrhi::ResourceStates::ShaderResource;
            od.keepInitialState = true;
            state.cullObjectBuffer = nvDevice->createBuffer(od);

            nvrhi::BufferDesc ad;
            ad.debugName = "ParticleCullArgs";
            ad.byteSize = PARTICLE_CULL_MAX_SLOTS * sizeof(ParticleDrawArgs);
            ad.structStride = sizeof(ParticleDrawArgs);
            ad.canHaveUAVs = true;
            ad.isDrawIndirectArgs = true;
            ad.initialState = nvrhi::ResourceStates::IndirectArgument;
            ad.keepInitialState = true;
            state.cullArgsBuffer = nvDevice->createBuffer(ad);
        }

        if (state.cullPipeline && state.cullObjectBuffer && state.cullArgsBuffer)
            Msg("* [ParticlePass] Hi-Z particle culling ready (%u slots)", PARTICLE_CULL_MAX_SLOTS);
        else
            Msg("! [ParticlePass] Hi-Z particle culling unavailable, drawing unculled");
    }
}

static void InitializeDistortionPipeline(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ParticlePassState& state)
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
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
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
    fg::RenderDevice* device,
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
    const Fmatrix* prevViewProj,
    VirtualResourceHandle prevDepth,
    ParticlePassState* state)
{
    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeParticleResources(device, fbInfo, *state);

        nvrhi::FramebufferInfoEx distortFbInfo;
        distortFbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        distortFbInfo.depthFormat = nvrhi::Format::D32;
        InitializeDistortionPipeline(device, distortFbInfo, *state);
    }

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
            data.hasPrevViewProj = prevViewProj != nullptr;
            if (prevViewProj)
                data.prevViewProj = *prevViewProj;
            else
                data.prevViewProj.identity();
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
            if (prevDepth.is_valid())
                data.prevDepth = passBuilder.read(prevDepth, ResourceState::ShaderResource);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.depth = data.depth;
        },
        [](const ParticlePassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
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
            auto* prevDepthTex = data.prevDepth.is_valid() ? fg.GetPhysicalTexture(data.prevDepth) : depthRT;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("ParticlePass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            const auto& rtDesc = colorRT->getDesc();

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_particle", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_particle", ".ps");
            BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Particle");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
               .BufferSRV("g_Materials", matBuffer.GetBuffer())
               .Texture("g_SceneDepth", prevDepthTex);
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

            struct WorldGroupDraw {
                u8 mode;
                xr_vector<ParticleVertex> vertices;
                u32 totalParticles;
                u32 firstSlot;
                u32 slotCount;
            };

            xr_vector<WorldGroupDraw> worldGroups;
            xr_vector<ParticleCullSlot> cullSlots;
            xr_vector<ParticleDrawArgs> cullArgs;

            bool cullReady = data.passState->cullPipeline && data.passState->cullObjectBuffer &&
                             data.passState->cullArgsBuffer && data.hasPrevViewProj &&
                             data.hiZPyramid.is_valid() && data.hiZMipLevels > 0;
            nvrhi::ITexture* hizTexture = cullReady ? fg.GetPhysicalTexture(data.hiZPyramid) : nullptr;
            if (!hizTexture)
                cullReady = false;

            if (totalWorld > 0 && cullReady) {
                for (u8 mode : s_renderOrder) {
                    xr_vector<ParticleBatch> filtered;
                    for (const auto& b : *data.worldParticleBatches) {
                        if (b.blendMode == mode && b.shaderVariant == ParticleShaderVariant::Standard)
                            filtered.push_back(b);
                    }
                    if (filtered.empty())
                        continue;

                    WorldGroupDraw group;
                    group.mode = mode;
                    xr_vector<u32> counts;
                    group.totalParticles = GenerateParticleVertices(filtered, group.vertices, &counts);
                    if (group.totalParticles == 0)
                        continue;

                    group.firstSlot = (u32)cullSlots.size();
                    u32 firstParticle = 0;
                    for (size_t i = 0; i < filtered.size(); ++i) {
                        u32 count = counts[i];
                        if (count == 0)
                            continue;

                        ParticleCullSlot slot;
                        slot.position = filtered[i].visual->vis.sphere.P;
                        slot.radius = filtered[i].visual->vis.sphere.R;
                        slot.batchIndex = (u32)cullSlots.size();
                        slot.flags = 0;
                        slot.pad0 = 0.0f;
                        slot.pad1 = 0.0f;
                        cullSlots.push_back(slot);

                        ParticleDrawArgs args;
                        args.indexCount = count * 6;
                        args.instanceCount = 1;
                        args.startIndex = firstParticle * 6;
                        args.baseVertex = 0;
                        args.startInstance = 0;
                        cullArgs.push_back(args);

                        firstParticle += count;
                    }
                    group.slotCount = (u32)cullSlots.size() - group.firstSlot;
                    if (group.slotCount == 0)
                        continue;
                    worldGroups.push_back(std::move(group));
                }

                if (cullSlots.empty() || cullSlots.size() > PARTICLE_CULL_MAX_SLOTS)
                    cullReady = false;
            }

            if (totalWorld > 0 && cullReady) {
                cmdList->writeBuffer(data.passState->cullObjectBuffer, cullSlots.data(),
                                     cullSlots.size() * sizeof(ParticleCullSlot));
                cmdList->writeBuffer(data.passState->cullArgsBuffer, cullArgs.data(),
                                     cullArgs.size() * sizeof(ParticleDrawArgs));

                ParticleCullParamsCB cullCBData;
                cullCBData.prevViewProj = data.prevViewProj;
                CFrustum frustum;
                frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
                u32 planeCount = std::min<u32>((u32)frustum.p_count, 6);
                for (u32 i = 0; i < 6; i++) {
                    if (i < planeCount)
                        cullCBData.frustumPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y,
                                                        frustum.planes[i].n.z, frustum.planes[i].d);
                    else
                        cullCBData.frustumPlanes[i].set(0.0f, 0.0f, 0.0f, -1000000.0f);
                }
                cullCBData.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y,
                                         Device.vCameraPosition.z, 0.0f);
                cullCBData.slotCount = (u32)cullSlots.size();
                cullCBData.hiZWidth = data.hiZWidth;
                cullCBData.hiZHeight = data.hiZHeight;
                cullCBData.hiZMipLevels = data.hiZMipLevels;

                auto cullCB = cache.GetOrCreateVolatileCB("ParticleCull", "ParticleCullParams",
                                                          sizeof(ParticleCullParamsCB), data.device);
                cmdList->writeBuffer(cullCB, &cullCBData, sizeof(cullCBData));

                auto* cullRefl = shaderLoader->GetCachedReflection("particle_cull", ".cs");
                BindingSetBuilder cullBsb(*cullRefl, nvDevice, "ParticleCull");
                cullBsb.ConstantBuffer("ParticleCullParams", cullCB)
                       .BufferSRV("g_ParticleData", data.passState->cullObjectBuffer)
                       .Texture("g_HiZPyramid", hizTexture)
                       .BufferUAV("g_DrawArgs", data.passState->cullArgsBuffer);
                auto cullBindingSet = cache.GetOrCreateBindingSet(cullBsb.Build(), data.passState->cullLayout, nvDevice);

                if (cullBindingSet) {
                    nvrhi::ComputeState cullState;
                    cullState.pipeline = data.passState->cullPipeline;
                    cullState.bindings = { cullBindingSet };
                    cmdList->setComputeState(cullState);
                    cmdList->dispatch(((u32)cullSlots.size() + 63) / 64, 1, 1);
                } else {
                    cullReady = false;
                }
            }

            if (totalWorld > 0 && cullReady) {
                for (auto& group : worldGroups) {
                    EnsureParticleVertexBuffer(nvDevice, (u32)(group.vertices.size() * sizeof(ParticleVertex)), *data.passState);
                    EnsureQuadIndexBuffer(nvDevice, group.totalParticles, *data.passState);
                    if (!data.passState->particleVB || !data.passState->quadIB)
                        continue;

                    cmdList->writeBuffer(data.passState->particleVB, group.vertices.data(),
                                         group.vertices.size() * sizeof(ParticleVertex));

                    auto pipeline = data.passState->pipelines[group.mode];
                    if (!pipeline)
                        pipeline = data.passState->pipelines[PARTICLE_BLEND_BLEND];
                    if (!pipeline)
                        continue;

                    nvrhi::Viewport viewport(
                        0.0f, static_cast<float>(rtDesc.width),
                        0.0f, static_cast<float>(rtDesc.height),
                        0.0f, 1.0f
                    );

                    nvrhi::GraphicsState gfxState;
                    gfxState.pipeline = pipeline;
                    gfxState.framebuffer = framebuffer;
                    gfxState.bindings = { bindingSet };
                    if (bindlessTable)
                        gfxState.addBindingSet(bindlessTable);
                    gfxState.vertexBuffers = { {data.passState->particleVB, 0, 0} };
                    gfxState.indexBuffer = { data.passState->quadIB, nvrhi::Format::R16_UINT, 0 };
                    gfxState.indirectParams = data.passState->cullArgsBuffer;
                    gfxState.viewport.addViewport(viewport);
                    gfxState.viewport.addScissorRect(scissor);

                    cmdList->setGraphicsState(gfxState);
                    cmdList->drawIndexedIndirect(group.firstSlot * sizeof(ParticleDrawArgs), group.slotCount);
                }
            } else if (totalWorld > 0) {
                for (u8 mode : s_renderOrder)
                    renderBatchGroup(*data.worldParticleBatches, mode, 0.0f, 1.0f);
            }

            if (totalHUD > 0) {
                for (u8 mode : s_renderOrder)
                    renderBatchGroup(*data.hudParticleBatches, mode, 0.9f, 1.0f);
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
                renderDistortGroup(*data.hudParticleBatches, 0.9f, 1.0f);
        }
    );

    ParticlePassOutput output;
    output.layout.albedo = passData.outputColor;
    output.layout.normal = passData.outputNormal;
    output.layout.baseColor = passData.baseColor;
    output.layout.depth = passData.depth;
    output.distortionRT = passData.distortionRT;
    return output;
}

} // namespace xray::render::fg::passes
