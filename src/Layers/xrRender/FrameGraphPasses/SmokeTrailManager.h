// SmokeTrailManager.h
// GPU smoke trail: emit/simulate/compact compute pipeline.
// Owns GPU buffers and per-frame constant buffer data.
// Compact CS outputs GPUTrailControlPoint (32B) for trail.vs consumption.
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "TrailPassSetup.h"  // GPUTrailControlPoint (32B)
#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
    class RenderDevice;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ────────────────────────────────────────────────────────
//  GPU simulation struct (must match smoke_common.hlsli SmokeSimPoint)
// ────────────────────────────────────────────────────────
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

// ────────────────────────────────────────────────────────
//  Emit/Sim/Compact constant buffers (must match HLSL cbuffer at b5)
// ────────────────────────────────────────────────────────
struct SmokeEmitParams
{
    float prevPosX, prevPosY, prevPosZ, currPosX;   // row 0
    float currPosY, currPosZ, emitDirX, emitDirY;   // row 1
    float emitDirZ, baseLifetime, lifetimeVariance, maxWidth; // row 2
    u32   emitCount, maxPoints;                      // row 3
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

// ────────────────────────────────────────────────────────
//  SmokeTrailManager
// ────────────────────────────────────────────────────────
class SmokeTrailManager
{
public:
    SmokeTrailManager()  = default;
    ~SmokeTrailManager() { Shutdown(); }

    bool Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Call once per frame with current weapon muzzle transform.
    // Constant emission — no heat system for first pass.
    void Update(float dt, const Fvector& muzzlePos, const Fvector& muzzleDir);

    // Buffer accessors for pass setup
    nvrhi::IBuffer* GetSimBuffer()      const { return m_simBuffer.Get(); }
    nvrhi::IBuffer* GetCompactBuffer()  const { return m_compactBuffer.Get(); }
    nvrhi::IBuffer* GetStateBuffer()    const { return m_stateBuffer.Get(); }
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer.Get(); }

    // Per-frame CB data
    const SmokeEmitParams&    GetEmitParams()    const { return m_emitParams; }
    const SmokeSimParams&     GetSimParams()     const { return m_simParams; }
    const SmokeCompactParams& GetCompactParams() const { return m_compactParams; }
    u32 GetEmitCount() const { return m_emitParams.emitCount; }

    bool IsReady() const { return m_initialized; }

    static constexpr u32 MAX_POINTS = 256;
    static constexpr u32 GROUP_SIZE = 64;

private:
    // GPU buffers
    nvrhi::BufferHandle m_simBuffer;       // 256 × 48B (SmokeSimPoint ring buffer)
    nvrhi::BufferHandle m_compactBuffer;   // 256 × 32B (GPUTrailControlPoint output)
    nvrhi::BufferHandle m_stateBuffer;     // 16B raw: {head, totalSpawned, liveCount, totalDist_bits}
    nvrhi::BufferHandle m_drawArgsBuffer;  // 16B (DrawIndirectArguments, non-indexed)

    // Muzzle tracking
    Fvector m_prevMuzzlePos = {0, 0, 0};
    bool    m_hasPrevMuzzle = false;

    // Emission accumulator
    float m_emitAccum = 0.f;

    // Per-frame CB data (computed in Update, consumed by pass setup)
    SmokeEmitParams    m_emitParams    = {};
    SmokeSimParams     m_simParams     = {};
    SmokeCompactParams m_compactParams = {};

    ng::RenderDevice* m_device      = nullptr;
    bool              m_initialized = false;
};

} // namespace xray::render::RENDER_NAMESPACE::passes
