// smoke_trail.vs
// Specialized smoke trail vertex shader — no Catmull-Rom subdivision.
// Control points from compact CS are already dense enough; we just
// generate a quad strip directly from them (1 quad per segment).
// Turbulence displacement via Perlin4D 3D volume texture.
#define SM_6_0
#include "common.h"

// ═══════════════════════════════════════════════════════
//  Control point data (from smoke_trail_compact.cs)
// ═══════════════════════════════════════════════════════

struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;   // pre-scaled width direction (world-space, magnitude = halfWidth)
    float ageNorm;             // age / maxAge (0=head, 1=tail)
    float cumDist;             // cumulative distance from head
};

StructuredBuffer<TrailControlPoint> g_ControlPoints : register(t10);
ByteAddressBuffer g_TrailState : register(t11);

// ═══════════════════════════════════════════════════════
//  Per-group constant buffer (matches TrailParamsCB)
// ═══════════════════════════════════════════════════════

cbuffer TrailParams : register(b5)
{
    uint  g_ControlPointCount;
    uint  g_Subdivisions;      // unused — kept for CB layout compatibility
    float g_TexCoordsFactor;
    uint  g_UVPolicy;

    float g_TotalDist;
    uint  g_EnableTailFade;
    uint  g_SmoothingMode;     // unused
    uint  g_EdgePolicy;

    uint  g_FlipX;
    uint  g_FlipY;
    uint  g_Rotate90;
    uint  g_MaterialID;

    uint  g_UseGPUState;
    float g_TurbAmount;
    float g_TurbFrequency;
    float g_TurbEvolution;

    float g_SphereCenterX;
    float g_SphereCenterY;
    float g_SphereCenterZ;
    float g_SphereRadius;

    uint  g_pad5_0, g_pad5_1, g_pad5_2, g_pad5_3;
};

// Perlin4D 3D volume — bound directly at t12
Texture3D g_Perlin4D : register(t12);

// ═══════════════════════════════════════════════════════
//  Output
// ═══════════════════════════════════════════════════════

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color    : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 normal   : TEXCOORD3;
    nointerpolation uint materialID : TEXCOORD4;
};

// ═══════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════

float3 LoadPos(uint idx)
{
    TrailControlPoint cp = g_ControlPoints[idx];
    return float3(cp.posX, cp.posY, cp.posZ);
}

float3 LoadDir(uint idx)
{
    TrailControlPoint cp = g_ControlPoints[idx];
    return float3(cp.dirX, cp.dirY, cp.dirZ);
}

float2 ApplyUVTransform(float u, float v)
{
    float xP = g_FlipX ? (1.0 - u) : u;
    float yP = g_FlipY ? (1.0 - v) : v;
    return g_Rotate90 ? float2(yP, xP) : float2(xP, yP);
}

// ═══════════════════════════════════════════════════════
//  MAIN — no subdivision, 1 quad per control point pair
// ═══════════════════════════════════════════════════════

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    // GPU-driven mode: read liveCount and totalDist from state buffer
    uint controlPointCount = g_ControlPointCount;
    float totalDist = g_TotalDist;
    if (g_UseGPUState)
    {
        controlPointCount = g_TrailState.Load(8);
        totalDist = asfloat(g_TrailState.Load(12));
    }

    if (controlPointCount < 2)
    {
        output.hpos = float4(0, 0, 0, 0);
        return output;
    }

    uint segments = controlPointCount - 1;

    // Decode vertexID → (quadIdx, corner)
    uint quadIdx = vertexID / 6;
    uint cornerInQuad = vertexID % 6;

    // Map corner to (cpIndex, side)
    // Tri 0: corners 0,1,2 → (quad,L), (quad,R), (quad+1,L)
    // Tri 1: corners 3,4,5 → (quad+1,L), (quad,R), (quad+1,R)
    static const uint cpOffsets[6] = { 0, 0, 1, 1, 0, 1 };
    static const uint sides[6]    = { 0, 1, 0, 0, 1, 1 };

    uint cpIdx = quadIdx + cpOffsets[cornerInQuad];
    uint side = sides[cornerInQuad];

    // Bounds check
    if (cpIdx >= controlPointCount)
    {
        output.hpos = float4(0, 0, 0, 0);
        return output;
    }

    // --- Load control point directly (no interpolation) ---
    TrailControlPoint cp = g_ControlPoints[cpIdx];
    float3 pos = float3(cp.posX, cp.posY, cp.posZ);
    float3 widthDir = float3(cp.dirX, cp.dirY, cp.dirZ);
    float smoothAge = cp.ageNorm;

    // --- Tail fade ---
    float tailFade = 1.0;
    if (g_EnableTailFade)
    {
        float globalT = float(cpIdx) / float(max(controlPointCount - 1, 1u));
        tailFade = 1.0 - globalT * globalT;
    }

    // --- Final position (EdgePolicy) ---
    float3 finalPos;
    if (g_EdgePolicy == 1) // Center
    {
        float sideSign = (side == 0) ? -1.0 : 1.0;
        finalPos = pos + widthDir * sideSign;
    }
    else // Edge
    {
        finalPos = pos + widthDir * float(side);
    }

    // --- UV computation ---
    float u = (side == 0) ? 0.0 : 1.0;
    float v = 0.0;

    if (g_UVPolicy == 0) // DistanceBased
    {
        v = (totalDist > 0.001) ? (cp.cumDist / totalDist) * g_TexCoordsFactor : 0.0;
    }
    else if (g_UVPolicy == 1) // Stretched
    {
        v = (float(cpIdx) / float(max(controlPointCount - 1, 1u))) * g_TexCoordsFactor;
    }
    else // AsIs
    {
        v = cp.ageNorm * g_TexCoordsFactor;
    }

    float2 uv = ApplyUVTransform(u, v);

    // --- Instance offset (Y) ---
    finalPos.y += float(instanceID) * 0.0001;

    // --- Per-instance turbulence displacement (3D volume texture) ---
    {
        float ageStrength = smoothstep(0.0, 0.5, smoothAge);
        // Sphere center = trail midpoint (not muzzle — muzzle moves with camera)
        float3 trailHead = LoadPos(0);
        float3 trailTail = LoadPos(controlPointCount - 1);
        float3 sphereCenter = (trailHead + trailTail) * 0.5 + float3(0, 1.75, 0);
        float instanceStrength = float(instanceID) * 0.0005;
        float instEvolution = g_TurbEvolution * instanceStrength * ageStrength;

        float3 noiseUVW = finalPos * g_TurbFrequency * 0.6 / 4.0;
        noiseUVW += float3(0.7071, 0.0, 0.7071) * instEvolution;

        float3 disp = g_Perlin4D.SampleLevel(smp_linear, noiseUVW, 0).xyz * 2.0 - 1.0;

        float dist = length(finalPos - sphereCenter);
        float falloff = 1.0 - smoothstep(0.0, g_SphereRadius * 0.7, dist);

        float ageStrength2 = smoothstep(0, 1, smoothAge * 1.5);

        finalPos += disp * g_TurbAmount * 1.5 * ageStrength2;
    }

    // --- Output ---
    output.hpos = mul(m_VP, float4(finalPos, 1.0));
    output.texcoord = uv;
    output.color = float4(1.0, 1.0, 1.0, smoothAge);
    output.worldPos = finalPos;
    output.normal = normalize(eye_position - finalPos);
    output.materialID = g_MaterialID;

    return output;
}
