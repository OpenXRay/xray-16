// SmokeTrailManager.cpp
// GPU smoke trail manager: owns buffers and per-frame compute constants.
// First pass: constant emission at muzzle position, no heat system.
#include "stdafx.h"
#include "SmokeTrailManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg {
    extern float ps_r_smoke_max_emit_rate;
    extern float ps_r_smoke_point_lifetime;
    extern float ps_r_smoke_max_width;
    extern float ps_r_smoke_gravity;
    extern float ps_r_smoke_buoyancy;
    extern float ps_r_smoke_turbulence;
    extern int   ps_r_smoke_trail_enabled;
}

namespace xray::render::fg::passes {

bool SmokeTrailManager::Initialize(fg::RenderDevice* device)
{
    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Simulation ring buffer (UAV): SmokeSimPoint[MAX_POINTS]
    {
        nvrhi::BufferDesc desc;
        desc.structStride     = sizeof(SmokeSimPoint);
        desc.byteSize         = MAX_POINTS * sizeof(SmokeSimPoint);
        desc.initialState     = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs      = true;
        desc.debugName        = "SmokeSimBuffer";
        m_simBuffer = nvDevice->createBuffer(desc);
        if (!m_simBuffer)
            return false;
    }

    // Compact output buffer (UAV during compact, SRV during draw): GPUTrailControlPoint[MAX_POINTS]
    {
        nvrhi::BufferDesc desc;
        desc.structStride     = sizeof(GPUTrailControlPoint);
        desc.byteSize         = MAX_POINTS * sizeof(GPUTrailControlPoint);
        desc.initialState     = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs      = true;
        desc.debugName        = "SmokeCompactBuffer";
        m_compactBuffer = nvDevice->createBuffer(desc);
        if (!m_compactBuffer)
            return false;
    }

    // Raw state buffer (UAV/SRV): {head, totalSpawned, liveCount, totalDist_bits}
    {
        nvrhi::BufferDesc desc;
        desc.byteSize         = 4 * sizeof(u32);
        desc.initialState     = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs      = true;
        desc.canHaveRawViews  = true;
        desc.debugName        = "SmokeStateBuffer";
        m_stateBuffer = nvDevice->createBuffer(desc);
        if (!m_stateBuffer)
            return false;
    }

    // DrawIndirectArguments buffer (non-indexed: vertexCount, instanceCount, startVertex, startInstance)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize          = 4 * sizeof(u32);
        desc.initialState      = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState  = true;
        desc.canHaveUAVs       = true;
        desc.canHaveRawViews   = true;
        desc.isDrawIndirectArgs = true;
        desc.debugName         = "SmokeDrawArgs";
        m_drawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_drawArgsBuffer)
            return false;
    }

    // Zero-initialize all GPU buffers
    {
        auto cmdList = nvDevice->createCommandList();
        cmdList->open();
        cmdList->clearBufferUInt(m_simBuffer, 0);
        cmdList->clearBufferUInt(m_compactBuffer, 0);
        cmdList->clearBufferUInt(m_stateBuffer, 0);
        cmdList->clearBufferUInt(m_drawArgsBuffer, 0);
        cmdList->close();
        nvDevice->executeCommandList(cmdList);
    }

    m_initialized = true;
    return true;
}

void SmokeTrailManager::Shutdown()
{
    m_simBuffer      = nullptr;
    m_compactBuffer  = nullptr;
    m_stateBuffer    = nullptr;
    m_drawArgsBuffer = nullptr;

    m_hasPrevMuzzle = false;
    m_initialized   = false;
}

void SmokeTrailManager::Update(float dt, const Fvector& muzzlePos, const Fvector& muzzleDir)
{
    using namespace xray::render::fg;

    if (!m_initialized || !ps_r_smoke_trail_enabled)
        return;

    if (!m_hasPrevMuzzle)
    {
        m_prevMuzzlePos = muzzlePos;
        m_hasPrevMuzzle = true;
    }

    // Constant emission — no heat gating for first pass
    m_emitAccum += ps_r_smoke_max_emit_rate * dt;
    u32 emitCount = static_cast<u32>(m_emitAccum);
    m_emitAccum -= static_cast<float>(emitCount);

    // Emit CB
    m_emitParams.prevPosX        = m_prevMuzzlePos.x;
    m_emitParams.prevPosY        = m_prevMuzzlePos.y;
    m_emitParams.prevPosZ        = m_prevMuzzlePos.z;
    m_emitParams.currPosX        = muzzlePos.x;
    m_emitParams.currPosY        = muzzlePos.y;
    m_emitParams.currPosZ        = muzzlePos.z;
    m_emitParams.emitDirX        = muzzleDir.x;
    m_emitParams.emitDirY        = muzzleDir.y;
    m_emitParams.emitDirZ        = muzzleDir.z;
    m_emitParams.baseLifetime    = ps_r_smoke_point_lifetime;
    m_emitParams.lifetimeVariance = 0.f;
    m_emitParams.maxWidth        = ps_r_smoke_max_width;
    m_emitParams.emitCount       = emitCount;
    m_emitParams.maxPoints       = MAX_POINTS;

    static u32 s_frameSeed = 0;
    s_frameSeed = s_frameSeed * 1664525u + 1013904223u;
    m_emitParams.frameSeed = static_cast<float>(s_frameSeed) * (1.f / 4294967296.f);
    m_emitParams.pad0      = 0.f;

    m_prevMuzzlePos = muzzlePos;

    // Sim CB
    m_simParams.dt         = dt;
    m_simParams.gravity    = ps_r_smoke_gravity;
    m_simParams.buoyancy   = ps_r_smoke_buoyancy;
    m_simParams.turbulence = ps_r_smoke_turbulence;
    m_simParams.drag       = 0.7f;
    m_simParams.time       = Device.fTimeGlobal;
    m_simParams.maxPoints  = MAX_POINTS;
    m_simParams.heat01     = 1.f;  // always full heat for constant emission

    // Compact CB
    m_compactParams.maxPoints      = MAX_POINTS;
    m_compactParams.subdivisions   = TRAIL_SUBDIVISIONS;
    m_compactParams.maxWidth       = ps_r_smoke_max_width;
    m_compactParams.turbAmount     = ps_r_smoke_turbulence;
    m_compactParams.turbFrequency  = 2.0f;
    m_compactParams.turbEvolution  = Device.fTimeGlobal * 0.5f;
    m_compactParams.sphereRadius   = 3.0f;
    m_compactParams.pad0           = 0.f;
    m_compactParams.sphereCenterX  = muzzlePos.x;
    m_compactParams.sphereCenterY  = muzzlePos.y;
    m_compactParams.sphereCenterZ  = muzzlePos.z;
    m_compactParams.pad1           = 0.f;
}

} // namespace xray::render::fg::passes
