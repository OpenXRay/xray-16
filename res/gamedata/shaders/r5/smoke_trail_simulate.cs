// smoke_trail_simulate.cs
// Minimal simulation: age particles and integrate position.

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

    // Age
    p.age += g_DT;
    if (p.age >= p.lifetime)
    {
        p.age = p.lifetime + 1.0f;
        g_SimBuffer[dtid.x] = p;
        return;
    }

    // Integrate position
    p.px += p.vx * g_DT;
    p.py += p.vy * g_DT;
    p.pz += p.vz * g_DT;

    g_SimBuffer[dtid.x] = p;
}
