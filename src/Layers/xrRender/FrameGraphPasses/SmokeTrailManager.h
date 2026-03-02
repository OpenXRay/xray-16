#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "TrailPassSetup.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderDevice;
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct SmokeSimPoint
{
    float px, py, pz;
    float lifetime;
    float vx, vy, vz;
    float age;
    float width;
    float seedF;
    float pad0;
    float pad1;
};
static_assert(sizeof(SmokeSimPoint) == 48,
    "SmokeSimPoint must be 48 bytes to match HLSL StructuredBuffer stride");

struct SmokeEmitParams
{
    float prevPosX, prevPosY, prevPosZ, currPosX;
    float currPosY, currPosZ, emitDirX, emitDirY;
    float emitDirZ, baseLifetime, lifetimeVariance, maxWidth;
    u32   emitCount, maxPoints;
    float frameSeed, pad0;
};
static_assert(sizeof(SmokeEmitParams) == 64);

struct SmokeSimParams
{
    float dt;
    float gravity;
    float buoyancy;
    float turbulence;
    float drag;
    float time;
    u32   maxPoints;
    float heat01;
};
static_assert(sizeof(SmokeSimParams) == 32);

struct SmokeCompactParams
{
    u32   maxPoints;
    u32   subdivisions;
    float maxWidth;
    float pad0;
};
static_assert(sizeof(SmokeCompactParams) == 16);

class SmokeTrailManager
{
public:
    SmokeTrailManager()  = default;
    ~SmokeTrailManager() { Shutdown(); }

    bool Initialize(ng::RenderDevice* device);
    void Shutdown();

    void Update(float dt, const Fvector& muzzlePos, const Fvector& muzzleDir);

    nvrhi::IBuffer* GetSimBuffer()      const { return m_simBuffer.Get(); }
    nvrhi::IBuffer* GetCompactBuffer()  const { return m_compactBuffer.Get(); }
    nvrhi::IBuffer* GetStateBuffer()    const { return m_stateBuffer.Get(); }
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer.Get(); }

    const SmokeEmitParams&    GetEmitParams()    const { return m_emitParams; }
    const SmokeSimParams&     GetSimParams()     const { return m_simParams; }
    const SmokeCompactParams& GetCompactParams() const { return m_compactParams; }
    u32 GetEmitCount() const { return m_emitParams.emitCount; }

    bool IsReady() const { return m_initialized; }

    static constexpr u32 MAX_POINTS = 256;
    static constexpr u32 GROUP_SIZE = 64;

private:
    nvrhi::BufferHandle m_simBuffer;
    nvrhi::BufferHandle m_compactBuffer;
    nvrhi::BufferHandle m_stateBuffer;
    nvrhi::BufferHandle m_drawArgsBuffer;

    Fvector m_prevMuzzlePos = {0, 0, 0};
    bool    m_hasPrevMuzzle = false;

    float m_emitAccum = 0.f;

    SmokeEmitParams    m_emitParams    = {};
    SmokeSimParams     m_simParams     = {};
    SmokeCompactParams m_compactParams = {};

    ng::RenderDevice* m_device      = nullptr;
    bool              m_initialized = false;
};

}
