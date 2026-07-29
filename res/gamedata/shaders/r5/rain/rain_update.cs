// rain_update.cs — GPU rain streak simulation + rain-SM cover cull
#define THREAD_GROUP_SIZE 64

// Match fgRainRender.cpp / Rain.cpp constants (never read from CB — avoids layout bugs).
static const float kSourceRadius  = 12.5;
static const float kSourceOffset  = 40.0;
static const float kMaxDistance   = 50.0;   // source_offset * 1.25
static const float kSinkOffset    = -10.0;  // -(max_distance - source_offset)
static const float kDropAngleRad  = 0.0523598776; // deg2rad(3.f)
static const float kDropSpeedMin  = 40.0;
static const float kDropSpeedMax  = 80.0;

struct RainParticle {
    float4 headSpeed; // xyz=head, w=speed
    float4 dirPad;    // xyz=dir (normalized), w=unused
    uint2  uvFlags;   // x=uvSet, y=alive (1=valid)
    uint2  pad;
};

cbuffer RainSimParams : register(b0) {
    float4   g_CameraPos;
    float4   g_WindAxis;
    float4   g_Params;         // x=dt, y=factorVisual, z=unused, w=rainDensity
    float4x4 g_RainSampleVP;
    float4   g_RainSM;         // x=enabled, y=res, zw=unused
    uint     g_ParticleCount;
    uint     g_FrameSeed;
    uint2    g_Pad;
};

#include "common_samplers.h"

RWStructuredBuffer<RainParticle> g_Particles : register(u0);
RWStructuredBuffer<uint> g_VisibleIndices : register(u1);
RWByteAddressBuffer g_VisibleCount : register(u2);

Texture2D<float> g_RainShadow : register(t0);

static const float PI = 3.14159265;

float Hash11(uint n)
{
    n = (n << 13u) ^ n;
    return frac(float(n) * (1.0 / 4294967296.0));
}

float2 Hash22(uint n)
{
    return float2(Hash11(n), Hash11(n * 1664525u + 1013904223u));
}

float3 RandomUnitSphere(uint seed)
{
    float z = cos(Hash11(seed) * PI);
    float a = Hash11(seed + 1u) * PI * 2.0;
    float r = sqrt(max(0.0, 1.0 - z * z));
    return float3(r * cos(a), r * sin(a), z);
}

float3 RandomConeDir(float3 axis, float coneAngle, uint seed)
{
    float3 rnd = RandomUnitSphere(seed);
    float scale = Hash11(seed + 2u) * tan(coneAngle);
    return normalize(axis + rnd * scale);
}

bool NeedsRespawn(RainParticle p)
{
    if (p.uvFlags.y != 1u)
        return true;
    float3 dir = p.dirPad.xyz;
    if (any(isnan(dir)) || dot(dir, dir) < 0.25)
        return true;
    return dir.y > -0.5;
}

void RespawnParticle(uint idx, inout RainParticle p)
{
    uint seed = idx * 747796405u + g_FrameSeed * 2891336453u;

    float2 h = Hash22(seed);
    float angle = h.x * PI * 2.0;
    float dist = sqrt(h.y) * kSourceRadius;
    float3 cam = g_CameraPos.xyz;

    float3 axis = g_WindAxis.xyz;
    if (dot(axis, axis) < 1e-4)
        axis = float3(0.0, -1.0, 0.0);
    else
        axis = normalize(axis);

    float3 randDir = RandomConeDir(axis, kDropAngleRad, seed + 17u);
    float3 head = float3(
        dist * cos(angle) + cam.x - randDir.x * kSourceOffset,
        kSourceOffset + cam.y,
        dist * sin(angle) + cam.z - randDir.z * kSourceOffset);

    p.headSpeed = float4(head, lerp(kDropSpeedMin, kDropSpeedMax, Hash11(seed + 31u)));
    p.dirPad = float4(randDir, 0.0);
    p.uvFlags = uint2(uint(Hash11(seed + 47u) * 2.0) & 1u, 1u);
    p.pad = uint2(0, 0);
}

bool UnderRoof(float3 headPos)
{
    if (g_RainSM.x < 0.5)
        return false;

    float4 tc = mul(g_RainSampleVP, float4(headPos, 1.0));
    float3 uvz = tc.xyz / max(tc.w, 1e-5);
    if (any(uvz.xy < 0.0) || any(uvz.xy > 1.0) || uvz.z < 0.0 || uvz.z > 1.0)
        return false;

    float depth = saturate(uvz.z);
    const float bias = 0.0015;
    float2 smSize;
    g_RainShadow.GetDimensions(smSize.x, smSize.y);
    float2 texel = 1.0 / max(smSize, float2(1.0, 1.0));
    float cover = 0.0;
    [unroll] for (int y = -2; y <= 2; ++y)
    {
        [unroll] for (int x = -2; x <= 2; ++x)
        {
            float2 o = float2((float)x, (float)y) * texel * 1.25;
            float shadowDepth = g_RainShadow.SampleLevel(smp_nofilter, uvz.xy + o, 0).x;
            cover += saturate((shadowDepth - (depth - bias)) * 120.0);
        }
    }
    cover *= (1.0 / 25.0);
    float edge = saturate(min(uvz.x, 1.0 - uvz.x) * 8.0) * saturate(min(uvz.y, 1.0 - uvz.y) * 8.0);
    cover = lerp(1.0, cover, edge);
    return cover < 0.5;
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_ParticleCount)
        return;

    RainParticle p = g_Particles[idx];
    if (NeedsRespawn(p))
        RespawnParticle(idx, p);

    const float dt = g_Params.x;
    const float3 cam = g_CameraPos.xyz;
    const float3 dir = normalize(p.dirPad.xyz);
    const float speed = p.headSpeed.w;

    p.headSpeed.xyz += dir * speed * dt;

    const float radiusWrap = kSourceRadius + 0.5;
    const float radiusWrapSqr = radiusWrap * radiusWrap;

    float3 wdir = float3(p.headSpeed.x - cam.x, 0.0, p.headSpeed.z - cam.z);
    float wlenSqr = dot(wdir, wdir);
    if (wlenSqr > radiusWrapSqr)
    {
        float wlen = sqrt(wlenSqr);
        if ((p.headSpeed.y - cam.y) < kSinkOffset)
            RespawnParticle(idx, p);
        else
        {
            wdir /= wlen;
            p.headSpeed.xyz -= wdir * (wlen + kSourceRadius);
        }
    }

    p.dirPad.xyz = dir;
    g_Particles[idx] = p;

    if (UnderRoof(p.headSpeed.xyz))
        return;

    if ((p.headSpeed.y - cam.y) < kSinkOffset)
        return;

    uint outIdx;
    g_VisibleCount.InterlockedAdd(0, 1, outIdx);
    g_VisibleIndices[outIdx] = idx;
}
