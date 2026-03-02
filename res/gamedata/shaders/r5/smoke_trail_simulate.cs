// smoke_trail_simulate.cs
// Physics simulation for GPU smoke trail points.
// Gravity, temperature-based buoyancy, curl noise turbulence, drag.

#include "smoke_common.h"

cbuffer SmokeSimCB : register(b5)
{
    float g_DT;
    float g_Gravity;
    float g_Buoyancy;
    float g_Turbulence;

    float g_Drag;
    float g_Time;
    uint  g_MaxPoints;
    float g_Heat01;
};

RWStructuredBuffer<SmokeSimPoint> g_SimBuffer : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_MaxPoints)
        return;

    SmokeSimPoint p = g_SimBuffer[dtid.x];

    // Skip empty/dead slots
    if (p.lifetime <= 0.0f || p.age >= p.lifetime)
        return;

    // Age acceleration: cooler smoke ages faster
    float cool = saturate(1.0f - g_Heat01);
    float ageMul = 1.0f + cool * cool * 2.5f;
    p.age += g_DT * ageMul;
    if (p.age >= p.lifetime)
    {
        p.age = p.lifetime + 1.0f;
        g_SimBuffer[dtid.x] = p;
        return;
    }

    float ageNorm = p.age / p.lifetime;

    // Gravity
    p.vy -= 9.8f * g_Gravity * g_DT;

    // Buoyancy (hotter smoke rises more)
    float temperature = 1.0f - ageNorm;
    p.vy += temperature * g_Buoyancy * 3.5f * g_DT;

    // Curl noise turbulence — ramps up with age so newer points stay straighter
    float3 noisePos = float3(p.px, p.py, p.pz) * 1.8f
                    + float3(g_Time * 0.15f, g_Time * 0.08f, g_Time * 0.12f);
    float3 curl = curlNoise(noisePos);

    float turbStrength = g_Turbulence * smoothstep(0.05f, 0.5f, ageNorm);
    p.vx += curl.x * turbStrength * g_DT;
    p.vy += curl.y * turbStrength * 0.3f * g_DT;
    p.vz += curl.z * turbStrength * g_DT;

    // Drag
    float drag = 1.0f - g_Drag * g_DT;
    p.vx *= drag;
    p.vy *= drag;
    p.vz *= drag;

    // Integrate position
    p.px += p.vx * g_DT;
    p.py += p.vy * g_DT;
    p.pz += p.vz * g_DT;

    g_SimBuffer[dtid.x] = p;
}
