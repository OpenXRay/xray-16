// ribbon.vs
// GPU ribbon vertex shader — SV_VertexID driven
// Reads control points from StructuredBuffer, generates Catmull-Rom
// subdivided ribbon with camera-facing width. No input layout needed.
#define SM_6_0
#include "common.h"

// ═══════════════════════════════════════════════════════
//  Control point data (uploaded from CPU per group)
// ═══════════════════════════════════════════════════════

struct RibbonControlPoint
{
    float posX, posY, posZ;
    float halfWidth;
    float ageNorm;   // age / maxAge (0=head, 1=tail)
    float cumDist;   // cumulative distance from head
};

StructuredBuffer<RibbonControlPoint> g_ControlPoints : register(t10);

// ═══════════════════════════════════════════════════════
//  Per-group constant buffer
// ═══════════════════════════════════════════════════════

cbuffer RibbonParams : register(b5)
{
    uint  g_ControlPointCount;
    uint  g_Subdivisions;
    float g_TexCoordsFactor;
    uint  g_UVPolicy;           // 0=DistanceBased, 1=Stretched, 2=AsIs

    float g_TotalDist;
    uint  g_EnableTailFade;
    uint  g_SmoothingMode;      // 0=CatmullRom, 1=Circumcircle
    uint  g_UseScreenSpaceWidth;

    uint  g_FlipX;
    uint  g_FlipY;
    uint  g_Rotate90;
    uint  g_MaterialID;

    float3 g_InvViewX;         // camera right (from inverse view matrix row 0)
    float  _pad0;

    float3 g_InvViewY;         // camera up (from inverse view matrix row 1)
    float  _pad1;
};

// ═══════════════════════════════════════════════════════
//  Output (matches ribbon.ps / bindless_particle.vs)
// ═══════════════════════════════════════════════════════

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color    : TEXCOORD1;
    nointerpolation uint materialID : TEXCOORD2;
};

// ═══════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════

float3 LoadPos(uint idx)
{
    RibbonControlPoint cp = g_ControlPoints[idx];
    return float3(cp.posX, cp.posY, cp.posZ);
}

float LoadWidth(uint idx)
{
    return g_ControlPoints[idx].halfWidth;
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

// Catmull-Rom first derivative (tangent)
float3 CatmullRomDeriv(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float t2 = t * t;
    return 0.5 * ((-p0 + p2)
        + (4.0 * p0 - 10.0 * p1 + 8.0 * p2 - 2.0 * p3) * t
        + (-3.0 * p0 + 9.0 * p1 - 9.0 * p2 + 3.0 * p3) * t2);
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
float3 EvalSmoothPos(uint smoothIdx, uint segments)
{
    uint segment = smoothIdx / g_Subdivisions;
    float localT = float(smoothIdx % g_Subdivisions) / float(g_Subdivisions);

    // Handle last smooth point
    if (segment >= segments)
    {
        segment = segments - 1;
        localT = 1.0;
    }

    // Load 4 control points with mirror boundary at start, clamp at end
    float3 p1 = LoadPos(segment);
    float3 p2 = LoadPos(segment + 1);
    float3 p0 = (segment > 0) ? LoadPos(segment - 1) : (2.0 * p1 - p2);
    float3 p3 = (segment + 2 < g_ControlPointCount) ? LoadPos(segment + 2) : p2;

    if (g_SmoothingMode == 1)
        return CircumcircleSmooth(p0, p1, p2, p3, localT);
    else
        return CatmullRom(p0, p1, p2, p3, localT);
}

// ═══════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    uint segments = g_ControlPointCount - 1;
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
    float3 p3 = (segment + 2 < g_ControlPointCount) ? LoadPos(segment + 2) : p2;  // clamp

    // --- Evaluate smooth position ---
    float3 smoothPos;
    if (g_SmoothingMode == 1)
        smoothPos = CircumcircleSmooth(p0, p1, p2, p3, localT);
    else
        smoothPos = CatmullRom(p0, p1, p2, p3, localT);

    // --- Interpolate width and ageNorm ---
    float w1 = LoadWidth(segment);
    float w2 = LoadWidth(segment + 1);
    float smoothWidth = lerp(w1, w2, localT);

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

    float hw = smoothWidth * tailFade;

    // --- Camera-facing width direction ---
    float3 widthDir;

    if (g_UseScreenSpaceWidth)
    {
        // Screen-space tangent averaging (Stride parity)
        // Compute tangent from adjacent smooth points in NDC
        float3 prevPos = (smoothIdx > 0) ? EvalSmoothPos(smoothIdx - 1, segments) : smoothPos;
        float3 nextPos = (smoothIdx + 1 < smoothCount) ? EvalSmoothPos(smoothIdx + 1, segments) : smoothPos;

        float4 clipCurr = mul(m_VP, float4(smoothPos, 1.0));
        float invWCurr = (abs(clipCurr.w) > 0.0001) ? (1.0 / clipCurr.w) : 0.0;
        float2 ndcCurr = clipCurr.xy * invWCurr;

        // Forward tangent (toward next point)
        float2 axisFwd = float2(0, 0);
        bool hasFwd = false;
        if (smoothIdx + 1 < smoothCount)
        {
            float4 clipNext = mul(m_VP, float4(nextPos, 1.0));
            float invWNext = (abs(clipNext.w) > 0.0001) ? (1.0 / clipNext.w) : 0.0;
            float2 ndcNext = clipNext.xy * invWNext;
            axisFwd = ndcCurr - ndcNext;
            float fwdLen = length(axisFwd);
            axisFwd = (fwdLen > 0.0001) ? axisFwd / fwdLen : float2(1, 0);
            hasFwd = true;
        }

        // Backward tangent (from previous point)
        float2 axisBwd = float2(0, 0);
        bool hasBwd = false;
        if (smoothIdx > 0)
        {
            float4 clipPrev = mul(m_VP, float4(prevPos, 1.0));
            float invWPrev = (abs(clipPrev.w) > 0.0001) ? (1.0 / clipPrev.w) : 0.0;
            float2 ndcPrev = clipPrev.xy * invWPrev;
            axisBwd = ndcPrev - ndcCurr;
            float bwdLen = length(axisBwd);
            axisBwd = (bwdLen > 0.0001) ? axisBwd / bwdLen : float2(1, 0);
            hasBwd = true;
        }

        // Average tangent (Stride: axisAvg = normalize(axis0 + axis1))
        float2 avgAxis;
        if (hasFwd && hasBwd)
        {
            avgAxis = axisFwd + axisBwd;
            float avgLen = length(avgAxis);
            avgAxis = (avgLen > 0.0001) ? avgAxis / avgLen : axisFwd;
        }
        else if (hasFwd)
            avgAxis = axisFwd;
        else
            avgAxis = axisBwd;

        // Perpendicular in screen-space → world-space
        // Stride: unitX.Y * invViewX - unitX.X * invViewY
        widthDir = avgAxis.y * g_InvViewX - avgAxis.x * g_InvViewY;
        float wdLen = length(widthDir);
        widthDir = (wdLen > 0.0001) ? widthDir / wdLen : float3(0, 1, 0);
    }
    else
    {
        // World-space cross product (default, stable)
        float3 tangent;
        if (g_SmoothingMode == 1)
        {
            // For circumcircle, use finite difference
            float3 prevP = (smoothIdx > 0) ? EvalSmoothPos(smoothIdx - 1, segments) : smoothPos;
            float3 nextP = (smoothIdx + 1 < smoothCount) ? EvalSmoothPos(smoothIdx + 1, segments) : smoothPos;
            tangent = nextP - prevP;
        }
        else
        {
            tangent = CatmullRomDeriv(p0, p1, p2, p3, localT);
        }

        float tangentLen = length(tangent);
        tangent = (tangentLen > 0.0001) ? tangent / tangentLen : float3(0, 0, 1);

        float3 toCamera = normalize(eye_position - smoothPos);
        widthDir = cross(tangent, toCamera);
        float wLen = length(widthDir);
        widthDir = (wLen > 0.0001) ? widthDir / wLen : float3(0, 1, 0);
    }

    // --- Final position ---
    float sideSign = (side == 0) ? -1.0 : 1.0;
    float3 finalPos = smoothPos + widthDir * hw * sideSign;

    // --- UV computation ---
    float u = (side == 0) ? 0.0 : 1.0;
    float v = 0.0;

    if (g_UVPolicy == 0)  // DistanceBased
    {
        float cd1 = LoadCumDist(segment);
        float cd2 = LoadCumDist(segment + 1);
        float cumD = lerp(cd1, cd2, localT);
        v = (g_TotalDist > 0.001) ? (cumD / g_TotalDist) * g_TexCoordsFactor : 0.0;
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

    // --- Color ---
    uint alpha = uint(tailFade * 255.0);
    float4 color = float4(1.0, 1.0, 1.0, tailFade);

    // --- Output ---
    output.hpos = mul(m_VP, float4(finalPos, 1.0));
    output.texcoord = uv;
    output.color = color;
    output.materialID = g_MaterialID;

    return output;
}
