#include "stdafx.h"
#include "RainGPUSimManager.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/device.h"
#include <cstring>

namespace xray::render::fg::passes
{
namespace
{
constexpr float kSourceRadius = 12.5f;
constexpr float kSourceOffset = 40.f;
constexpr float kMaxDistance = kSourceOffset * 1.25f;
constexpr float kSinkOffset = -(kMaxDistance - kSourceOffset);
constexpr float kDropLength = 5.f;
constexpr float kDropWidth = 0.30f;
constexpr float kDropAngle = 3.0f;
constexpr float kDropMaxAngle = deg2rad(10.f);
constexpr float kDropMaxWindVel = 20.0f;
}

RainGPUSimManager::~RainGPUSimManager() { Shutdown(); }

bool RainGPUSimManager::Initialize(fg::RenderDevice* device)
{
    if (m_initialized)
        return true;
    if (!device)
        return false;

    m_device = device;
    if (!CreateShaders() || !CreateBuffers() || !CreatePipelines())
    {
        Shutdown();
        return false;
    }

    ClearParticleBuffer();
    m_simCBSize = GetSimCBSize();
    m_initialized = true;
    Msg("* [RainGPUSim] Initialized (%u max particles, CB=%u bytes)", kMaxParticles, m_simCBSize);
    return true;
}

void RainGPUSimManager::Shutdown()
{
    m_updateCS = nullptr;
    m_billboardCS = nullptr;
    m_updatePipeline = nullptr;
    m_billboardPipeline = nullptr;
    m_updateLayout = nullptr;
    m_billboardLayout = nullptr;
    m_updateBindingSet = nullptr;
    m_billboardBindingSet = nullptr;
    m_pointSampler = nullptr;
    m_particleBuffer = nullptr;
    m_visibleIndicesBuffer = nullptr;
    m_visibleCountBuffer = nullptr;
    m_vertexBuffer = nullptr;
    m_drawArgsBuffer = nullptr;
    m_updateParamsCB = nullptr;
    m_billboardParamsCB = nullptr;
    m_device = nullptr;
    m_initialized = false;
    m_lastVisibleCount = 0;
}

bool RainGPUSimManager::CreateShaders()
{
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return false;

    auto upd = shaderLoader->LoadComputeShader("rain\\rain_update", "main");
    if (!upd.handle)
    {
        Msg("! [RainGPUSim] Failed to load rain\\rain_update.cs");
        return false;
    }
    m_updateCS = upd.handle;

    auto bb = shaderLoader->LoadComputeShader("rain\\rain_billboard", "main");
    if (!bb.handle)
    {
        Msg("! [RainGPUSim] Failed to load rain\\rain_billboard.cs");
        return false;
    }
    m_billboardCS = bb.handle;
    return true;
}

bool RainGPUSimManager::CreateBuffers()
{
    nvrhi::IDevice* nv = m_device->GetNVRHIDevice();
    if (!nv)
        return false;

    constexpr u32 particleStride = 48; // matches RainParticle in shader

    {
        nvrhi::BufferDesc d;
        d.byteSize = kMaxParticles * particleStride;
        d.structStride = particleStride;
        d.debugName = "RainGPUParticles";
        d.canHaveUAVs = true;
        d.keepInitialState = true;
        d.initialState = nvrhi::ResourceStates::UnorderedAccess;
        m_particleBuffer = nv->createBuffer(d);
        if (!m_particleBuffer) return false;
    }
    {
        nvrhi::BufferDesc d;
        d.byteSize = kMaxParticles * sizeof(u32);
        d.structStride = sizeof(u32);
        d.debugName = "RainGPUVisibleIndices";
        d.canHaveUAVs = true;
        d.keepInitialState = true;
        d.initialState = nvrhi::ResourceStates::UnorderedAccess;
        m_visibleIndicesBuffer = nv->createBuffer(d);
        if (!m_visibleIndicesBuffer) return false;
    }
    {
        nvrhi::BufferDesc d;
        d.byteSize = sizeof(u32);
        d.debugName = "RainGPUVisibleCount";
        d.canHaveUAVs = true;
        d.canHaveRawViews = true;
        d.keepInitialState = true;
        d.initialState = nvrhi::ResourceStates::UnorderedAccess;
        m_visibleCountBuffer = nv->createBuffer(d);
        if (!m_visibleCountBuffer) return false;
    }
    {
        nvrhi::BufferDesc d;
        d.byteSize = kMaxParticles * 6 * 24;
        d.structStride = 24;
        d.isVertexBuffer = true;
        d.canHaveUAVs = true;
        d.canHaveRawViews = true;
        d.keepInitialState = true;
        d.initialState = nvrhi::ResourceStates::UnorderedAccess;
        d.debugName = "RainGPUVertexBuffer";
        m_vertexBuffer = nv->createBuffer(d);
        if (!m_vertexBuffer) return false;
    }
    {
        nvrhi::BufferDesc d;
        d.byteSize = 5 * sizeof(u32);
        d.isDrawIndirectArgs = true;
        d.canHaveUAVs = true;
        d.canHaveRawViews = true;
        d.debugName = "RainGPUDrawArgs";
        d.keepInitialState = true;
        d.initialState = nvrhi::ResourceStates::UnorderedAccess;
        m_drawArgsBuffer = nv->createBuffer(d);
        if (!m_drawArgsBuffer) return false;
    }

    nvrhi::SamplerDesc sd;
    sd.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    sd.setAllFilters(false);
    m_pointSampler = nv->createSampler(sd);

    auto& cache = framegraph::GetPassResourceCache();
    m_updateParamsCB = cache.GetOrCreateVolatileCB(
        "RainGPUSim", "UpdateParamsV3", GetSimCBSize(), m_device);
    m_billboardParamsCB = cache.GetOrCreateVolatileCB(
        "RainGPUSim", "BillboardParamsV2", sizeof(RainBillboardParams), m_device);
    return m_pointSampler && m_updateParamsCB && m_billboardParamsCB;
}

u32 RainGPUSimManager::GetSimCBSize() const
{
    u32 size = sizeof(RainSimParams);
    auto* shaderLoader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    auto* refl = shaderLoader ? shaderLoader->GetCachedReflection("rain\\rain_update", ".cs") : nullptr;
    if (!refl)
        return size;

    const auto& cbs = framegraph::ShaderReflector::GetConstantBuffers(refl);
    if (const auto* cb = cbs.GetByName("RainSimParams"))
    {
        if (cb->size > 0)
        {
            if (cb->size != size)
            {
                Msg("! [RainGPUSim] RainSimParams size mismatch: C++=%u shader=%u (using shader)",
                    size, cb->size);
            }
            size = cb->size;
        }
    }
    return size;
}

bool RainGPUSimManager::CreatePipelines()
{
    nvrhi::IDevice* nv = m_device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* updRefl = shaderLoader->GetCachedReflection("rain\\rain_update", ".cs");
    auto* bbRefl = shaderLoader->GetCachedReflection("rain\\rain_billboard", ".cs");
    if (!nv || !updRefl || !bbRefl)
        return false;

    m_updateLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("RainGPUUpdate", *updRefl, nv);
    m_billboardLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("RainGPUBillboard", *bbRefl, nv);
    if (!m_updateLayout || !m_billboardLayout)
        return false;

    nvrhi::ComputePipelineDesc pd;
    pd.CS = m_updateCS;
    pd.bindingLayouts = { m_updateLayout };
    m_updatePipeline = nv->createComputePipeline(pd);

    pd.CS = m_billboardCS;
    pd.bindingLayouts = { m_billboardLayout };
    m_billboardPipeline = nv->createComputePipeline(pd);

    return m_updatePipeline && m_billboardPipeline;
}

void RainGPUSimManager::ClearParticleBuffer()
{
    nvrhi::IDevice* nv = m_device->GetNVRHIDevice();
    if (!nv || !m_particleBuffer)
        return;

    constexpr u32 particleStride = 48;
    xr_vector<u8> zeros(kMaxParticles * particleStride, 0);

    nvrhi::CommandListHandle cmd = nv->createCommandList();
    cmd->open();
    cmd->writeBuffer(m_particleBuffer, zeros.data(), zeros.size());
    cmd->close();
    nv->executeCommandList(cmd);
}

void RainGPUSimManager::Dispatch(
    nvrhi::ICommandList* cmdList,
    nvrhi::ITexture* rainShadow,
    const Fmatrix& rainSampleVP,
    bool rainSMValid,
    u32 particleCount,
    float rainDensity,
    float factorVisual,
    u32 rainColor)
{
    if (!m_initialized || !cmdList || particleCount == 0)
    {
        m_lastVisibleCount = 0;
        return;
    }

    particleCount = std::min(particleCount, kMaxParticles);
    nvrhi::IDevice* nv = m_device->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* updRefl = shaderLoader->GetCachedReflection("rain\\rain_update", ".cs");
    auto* bbRefl = shaderLoader->GetCachedReflection("rain\\rain_billboard", ".cs");
    if (!nv || !updRefl || !bbRefl)
        return;

    const auto& env = g_pGamePersistent->Environment().CurrentEnv;
    float gust = g_pGamePersistent->Environment().wind_strength_factor / 10.f;
    float k = env.wind_velocity * gust / kDropMaxWindVel;
    clamp(k, 0.f, 1.f);
    float pitch = kDropMaxAngle * k - PI_DIV_2;
    Fvector axis(0.f, -1.f, 0.f);
    axis.setHP(env.wind_direction, pitch);
    if (axis.square_magnitude() < 1e-4f)
        axis.set(0.f, -1.f, 0.f);
    else
        axis.normalize();

    RainSimParams sim{};
    sim.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.f);
    sim.windAxis.set(axis.x, axis.y, axis.z, 0.f);
    sim.params.set(Device.fTimeDelta, factorVisual, 0.f, rainDensity);
    sim.rainSampleVP = rainSampleVP;
    sim.rainSM.set(rainSMValid ? 1.f : 0.f, float(ps_r3_dyn_wet_surf_sm_res), 0.f, 0.f);
    sim.particleCount = particleCount;
    sim.frameSeed = Device.dwFrame;

    cmdList->writeBuffer(m_updateParamsCB, &sim, sizeof(sim));

    u32 zero = 0;
    cmdList->writeBuffer(m_visibleCountBuffer, &zero, sizeof(zero));

    framegraph::BindingSetBuilder updBsb(*updRefl, nv, "RainGPUUpdate");
    updBsb.ConstantBuffer("RainSimParams", m_updateParamsCB);
    updBsb.BufferUAV("g_Particles", m_particleBuffer);
    updBsb.BufferUAV("g_VisibleIndices", m_visibleIndicesBuffer);
    updBsb.BufferUAV("g_VisibleCount", m_visibleCountBuffer);
    if (rainShadow && rainSMValid)
        updBsb.Texture("g_RainShadow", rainShadow);
    m_updateBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(updBsb.Build(), m_updateLayout, nv);

    nvrhi::ComputeState cs;
    cs.pipeline = m_updatePipeline;
    cs.bindings = { m_updateBindingSet };
    cmdList->setComputeState(cs);
    const u32 groups = (particleCount + kThreadGroupSize - 1) / kThreadGroupSize;
    cmdList->dispatch(groups, 1, 1);

    cmdList->setBufferState(m_particleBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_visibleIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(m_visibleCountBuffer, nvrhi::ResourceStates::ShaderResource);

    RainBillboardParams bb{};
    bb.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.f);
    float rainColorBits = 0.f;
    std::memcpy(&rainColorBits, &rainColor, sizeof(u32));
    bb.params.set(factorVisual, kDropLength, kDropWidth, rainColorBits);
    cmdList->writeBuffer(m_billboardParamsCB, &bb, sizeof(bb));

    framegraph::BindingSetBuilder bbBsb(*bbRefl, nv, "RainGPUBillboard");
    bbBsb.ConstantBuffer("RainBillboardParams", m_billboardParamsCB);
    bbBsb.BufferSRV("g_Particles", m_particleBuffer);
    bbBsb.BufferSRV("g_VisibleIndices", m_visibleIndicesBuffer);
    bbBsb.BufferSRV("g_VisibleCountBuf", m_visibleCountBuffer);
    bbBsb.BufferUAV("g_Vertices", m_vertexBuffer);
    bbBsb.BufferUAV("g_DrawArgs", m_drawArgsBuffer);
    m_billboardBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bbBsb.Build(), m_billboardLayout, nv);

    cs.pipeline = m_billboardPipeline;
    cs.bindings = { m_billboardBindingSet };
    cmdList->setComputeState(cs);
    cmdList->dispatch(groups, 1, 1);

    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_drawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

    m_lastVisibleCount = particleCount;
}

} // namespace xray::render::fg::passes
