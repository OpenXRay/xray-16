// trail.vs
// GPU trail vertex shader — SV_VertexID driven (Stride ShapeBuilderTrail parity)
// Reads control points from StructuredBuffer, generates Catmull-Rom
// subdivided trail with stored direction width. No camera dependency.
// Parallel to ribbon.vs (which implements ShapeBuilderRibbon).
#define SM_6_0
#include "common.h"
// noise4d.h no longer needed — turbulence sampled from perlin4d 3D volume texture

// ═══════════════════════════════════════════════════════
//  Control point data (uploaded from CPU per group, or GPU compact CS)
// ═══════════════════════════════════════════════════════

struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;   // pre-scaled width direction (world-space, magnitude = halfWidth)
    float ageNorm;             // age / maxAge (0=head, 1=tail)
    float cumDist;             // cumulative distance from head
};

StructuredBuffer<TrailControlPoint> g_ControlPoints : register(t10);
ByteAddressBuffer g_TrailState : register(t11);  // GPU-driven: {head, totalSpawned, liveCount, totalDist_bits}

// ═══════════════════════════════════════════════════════
//  Per-group constant buffer
// ═══════════════════════════════════════════════════════

cbuffer TrailParams : register(b5)
{
    uint  g_ControlPointCount;
    uint  g_Subdivisions;
    float g_TexCoordsFactor;
    uint  g_UVPolicy;           // 0=DistanceBased, 1=Stretched, 2=AsIs

    float g_TotalDist;
    uint  g_EnableTailFade;
    uint  g_SmoothingMode;      // 0=CatmullRom, 1=Circumcircle
    uint  g_EdgePolicy;         // 0=Edge, 1=Center

    uint  g_FlipX;
    uint  g_FlipY;
    uint  g_Rotate90;
    uint  g_MaterialID;

    uint  g_UseGPUState;        // 1 = read liveCount/totalDist from g_TrailState
    float g_TurbAmount;         // per-instance turbulence displacement
    float g_TurbFrequency;      // spatial frequency (1/size)
    float g_TurbEvolution;      // time (4th noise dimension)

    float g_SphereCenterX;
    float g_SphereCenterY;
    float g_SphereCenterZ;
    float g_SphereRadius;

    uint  g_pad5_0, g_pad5_1, g_pad5_2, g_pad5_3;
};

// Perlin4D 3D volume — bound directly at t12 (not bindless, since bindless is Texture2D only)
Texture3D g_Perlin4D : register(t12);

// ═══════════════════════════════════════════════════════
//  Output (matches ribbon.ps / trail.ps / bindless_particle.vs)
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

float LoadAgeNorm(uint idx)
{
    return g_ControlPoints[idx].ageNorm;
}

float LoadCumDist(uint idx)
{
    return g_ControlPoints[idx].cumDist;
}

float3 CatmullRom(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5 * ((2.0 * p1)
        + (-p0 + p2) * t
        + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
        + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

// Circumcenter of triangle ABC in 3D
float3 Circumcenter(float3 A, float3 B, float3 C)
{
    float3 a = A - C;
    float3 b = B - C;
    float3 crossAB = cross(a, b);
    float denom = 2.0 * dot(crossAB, crossAB);

    if (denom < 1e-12)
        return C;

    float a2 = dot(a, a);
    float b2 = dot(b, b);
    float3 diff = b * a2 - a * b2;
    float3 num = cross(diff, crossAB);

    return C + num / denom;
}

// Circumcircle-based smooth interpolation (Stride BestFit)
float3 CircumcircleSmooth(float3 P0, float3 P1, float3 P2, float3 P3, float t)
{
    float3 O1 = Circumcenter(P0, P1, P2);
    float R1 = length(O1 - P1);

    float3 O2 = Circumcenter(P1, P2, P3);
    float R2 = length(O2 - P2);

    float3 guess = lerp(P1, P2, t);

    float3 d1 = guess - O1;
    float len1 = length(d1);
    d1 = (len1 > 0.0001) ? d1 / len1 : float3(0, 1, 0);

    float3 d2 = guess - O2;
    float len2 = length(d2);
    d2 = (len2 > 0.0001) ? d2 / len2 : float3(0, 1, 0);

    float3 p1OnCircle = O1 + d1 * R1;
    float3 p2OnCircle = O2 + d2 * R2;

    return lerp(p1OnCircle, p2OnCircle, t);
}

float2 ApplyUVTransform(float u, float v)
{
    float xP = g_FlipX ? (1.0 - u) : u;
    float yP = g_FlipY ? (1.0 - v) : v;
    return g_Rotate90 ? float2(yP, xP) : float2(xP, yP);
}

// Evaluate smooth position for a given smoothIdx
float3 EvalSmoothPos(uint smoothIdx, uint segments, uint cpCount)
{
    uint segment = smoothIdx / g_Subdivisions;
    float localT = float(smoothIdx % g_Subdivisions) / float(g_Subdivisions);

    if (segment >= segments)
    {
        segment = segments - 1;
        localT = 1.0;
    }

    float3 p1 = LoadPos(segment);
    float3 p2 = LoadPos(segment + 1);
    float3 p0 = (segment > 0) ? LoadPos(segment - 1) : (2.0 * p1 - p2);
    float3 p3 = (segment + 2 < cpCount) ? LoadPos(segment + 2) : p2;

    if (g_SmoothingMode == 1)
        return CircumcircleSmooth(p0, p1, p2, p3, localT);
    else
        return CatmullRom(p0, p1, p2, p3, localT);
}

// ═══════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    // GPU-driven mode: read liveCount and totalDist from state buffer instead of CB
    uint controlPointCount = g_ControlPointCount;
    float totalDist = g_TotalDist;
    if (g_UseGPUState)
    {
        controlPointCount = g_TrailState.Load(8);   // liveCount at offset 8
        totalDist = asfloat(g_TrailState.Load(12));  // totalDist at offset 12
    }

    uint segments = controlPointCount - 1;
    uint smoothCount = segments * g_Subdivisions + 1;

    // Decode vertexID → (quadIdx, corner)
    uint quadIdx = vertexID / 6;
    uint cornerInQuad = vertexID % 6;

    // Map corner to (smoothPointOffset, side)
    // Tri 0: corners 0,1,2 → (quad,L), (quad,R), (quad+1,L)
    // Tri 1: corners 3,4,5 → (quad+1,L), (quad,R), (quad+1,R)
    static const uint smoothOffsets[6] = { 0, 0, 1, 1, 0, 1 };
    static const uint sides[6]         = { 0, 1, 0, 0, 1, 1 };

    uint smoothIdx = quadIdx + smoothOffsets[cornerInQuad];
    uint side = sides[cornerInQuad];

    // Bounds check
    if (smoothIdx >= smoothCount)
    {
        output.hpos = float4(0, 0, 0, 0);
        return output;
    }

    // --- Decode smoothIdx → segment + localT ---
    uint segment = smoothIdx / g_Subdivisions;
    float localT = float(smoothIdx % g_Subdivisions) / float(g_Subdivisions);

    if (segment >= segments)
    {
        segment = segments - 1;
        localT = 1.0;
    }

    // --- Load 4 control points ---
    float3 p1 = LoadPos(segment);
    float3 p2 = LoadPos(segment + 1);
    float3 p0 = (segment > 0) ? LoadPos(segment - 1) : (2.0 * p1 - p2);  // mirror
    float3 p3 = (segment + 2 < controlPointCount) ? LoadPos(segment + 2) : p2;  // clamp

    // --- Evaluate smooth position ---
    float3 smoothPos;
    if (g_SmoothingMode == 1)
        smoothPos = CircumcircleSmooth(p0, p1, p2, p3, localT);
    else
        smoothPos = CatmullRom(p0, p1, p2, p3, localT);

    // --- Interpolate direction (Stride: directions linearly interpolated between CPs) ---
    float3 dir1 = LoadDir(segment);
    float3 dir2 = LoadDir(segment + 1);
    float3 widthDir = lerp(dir1, dir2, localT);
    // widthDir is pre-scaled to half-width magnitude — no separate size multiply

    // --- Interpolate ageNorm ---
    float age1 = LoadAgeNorm(segment);
    float age2 = LoadAgeNorm(segment + 1);
    float smoothAge = lerp(age1, age2, localT);

    // --- Tail fade ---
    float tailFade = 1.0;
    if (g_EnableTailFade)
    {
        float globalT = float(smoothIdx) / float(max(smoothCount - 1, 1u));
        tailFade = 1.0 - globalT * globalT;
    }

    // --- Final position (EdgePolicy) ---
    float3 finalPos;
    if (g_EdgePolicy == 1) // Center: position ± widthDir
    {
        float sideSign = (side == 0) ? -1.0 : 1.0;
        finalPos = smoothPos + widthDir * sideSign;
    }
    else // Edge: position is one side, position + widthDir is other
    {
        finalPos = smoothPos + widthDir * float(side);
    }

    // --- UV computation ---
    float u = (side == 0) ? 0.0 : 1.0;
    float v = 0.0;

    if (g_UVPolicy == 0)  // DistanceBased
    {
        float cd1 = LoadCumDist(segment);
        float cd2 = LoadCumDist(segment + 1);
        float cumD = lerp(cd1, cd2, localT);
        v = (totalDist > 0.001) ? (cumD / totalDist) * g_TexCoordsFactor : 0.0;
    }
    else if (g_UVPolicy == 1)  // Stretched
    {
        v = (float(smoothIdx) / float(max(smoothCount - 1, 1u))) * g_TexCoordsFactor;
    }
    else  // AsIs
    {
        v = localT * g_TexCoordsFactor;
    }

    float2 uv = ApplyUVTransform(u, v);

    // --- Output ---
    output.hpos = mul(m_VP, float4(finalPos, 1.0));
    output.texcoord = uv;
    output.color = float4(1.0, 1.0, 1.0, smoothAge);
    output.worldPos = finalPos;
    output.normal = normalize(eye_position - finalPos);
    output.materialID = g_MaterialID;

    return output;
}
