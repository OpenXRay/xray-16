#define SM_6_0
#include "common.h"

struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float ageNorm;
    float cumDist;
};

StructuredBuffer<TrailControlPoint> g_ControlPoints : register(t10);
ByteAddressBuffer g_TrailState : register(t11);

cbuffer TrailParams : register(b5)
{
    uint  g_ControlPointCount;
    uint  g_Subdivisions;
    float g_TexCoordsFactor;
    uint  g_UVPolicy;

    float g_TotalDist;
    uint  g_EnableTailFade;
    uint  g_SmoothingMode;
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

Texture3D g_Perlin4D : register(t12);

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color    : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 normal   : TEXCOORD3;
    nointerpolation uint materialID : TEXCOORD4;
};

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

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

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

    uint quadIdx = vertexID / 6;
    uint cornerInQuad = vertexID % 6;

    static const uint cpOffsets[6] = { 0, 0, 1, 1, 0, 1 };
    static const uint sides[6]    = { 0, 1, 0, 0, 1, 1 };

    uint cpIdx = quadIdx + cpOffsets[cornerInQuad];
    uint side = sides[cornerInQuad];

    if (cpIdx >= controlPointCount)
    {
        output.hpos = float4(0, 0, 0, 0);
        return output;
    }

    TrailControlPoint cp = g_ControlPoints[cpIdx];
    float3 pos = float3(cp.posX, cp.posY, cp.posZ);
    float3 widthDir = float3(cp.dirX, cp.dirY, cp.dirZ);
    float smoothAge = cp.ageNorm;

    float tailFade = 1.0;
    if (g_EnableTailFade)
    {
        float globalT = float(cpIdx) / float(max(controlPointCount - 1, 1u));
        tailFade = 1.0 - globalT * globalT;
    }

    float3 finalPos;
    if (g_EdgePolicy == 1)
    {
        float sideSign = (side == 0) ? -1.0 : 1.0;
        finalPos = pos + widthDir * sideSign;
    }
    else
    {
        finalPos = pos + widthDir * float(side);
    }

    float u = (side == 0) ? 0.0 : 1.0;
    float v = 0.0;

    if (g_UVPolicy == 0)
    {
        v = (totalDist > 0.001) ? (cp.cumDist / totalDist) * g_TexCoordsFactor : 0.0;
    }
    else if (g_UVPolicy == 1)
    {
        v = (float(cpIdx) / float(max(controlPointCount - 1, 1u))) * g_TexCoordsFactor;
    }
    else
    {
        v = cp.ageNorm * g_TexCoordsFactor;
    }

    float2 uv = ApplyUVTransform(u, v);

    finalPos.y += float(instanceID) * 0.0001;

    {
        float ageStrength = smoothstep(0.0, 0.5, smoothAge);
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

    output.hpos = mul(m_VP, float4(finalPos, 1.0));
    output.texcoord = uv;
    output.color = float4(1.0, 1.0, 1.0, smoothAge);
    output.worldPos = finalPos;
    output.normal = normalize(eye_position - finalPos);
    output.materialID = g_MaterialID;

    return output;
}
