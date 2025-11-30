#define SM_5_0
#include "common.h"

struct GPUObjectData
{
    float3 position;
    float radius;
    uint batchIndex;
    uint flags;
    float2 padding;
};

struct IndirectDrawArgs
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

cbuffer CullParams : register(b5)
{
    float4x4 g_ViewProj;
    float3 g_CameraPos;
    float g_MaxDistance;
    float4 g_FrustumPlanes[6];
    uint g_ObjectCount;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
};

StructuredBuffer<GPUObjectData> g_Objects : register(t0);

Texture2D<float> g_HiZPyramid : register(t1);

SamplerState g_PointSampler : register(s0);

RWStructuredBuffer<uint> g_VisibleIndices : register(u0);

RWByteAddressBuffer g_VisibleCount : register(u1);

RWByteAddressBuffer g_DrawArgs : register(u2);

bool DistanceTest(float3 position, float radius)
{
    float3 delta = position - g_CameraPos;
    float distSq = dot(delta, delta);

    float effectiveDistSq = distSq - (radius * radius);
    effectiveDistSq = max(0.0, effectiveDistSq);

    return effectiveDistSq <= g_MaxDistance;
}

bool FrustumTestSphere(float3 center, float radius)
{

    for (uint i = 0; i < 6; i++)
    {
        float dist = dot(g_FrustumPlanes[i].xyz, center) + g_FrustumPlanes[i].w;

        if (dist > radius)
            return false;
    }

    return true;
}

bool OcclusionTestSphere(float3 center, float radius)
{
    return true;

    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    if (clipPos.w <= 0.0)
        return true;

    float3 ndc = clipPos.xyz / clipPos.w;

    if (any(abs(ndc.xy) > 1.0 + radius / clipPos.w))
        return true;

    float2 uv = ndc.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;

    float screenRadius = (radius / clipPos.w) * float(g_HiZWidth) * 0.5;

    float mipLevel = log2(max(1.0, screenRadius * 2.0));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    float hiZDepth = g_HiZPyramid.SampleLevel(g_PointSampler, uv, mipLevel);

    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;

    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));
    float objectDepth = frontClip.z / frontClip.w;

    objectDepth = saturate(objectDepth);

    float depthBias = 0.001;

    return objectDepth <= (hiZDepth + depthBias);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;

    if (objectIdx >= g_ObjectCount)
        return;

    GPUObjectData obj = g_Objects[objectIdx];

    #if 0
    if (!DistanceTest(obj.position, obj.radius))
        return;

    if (!FrustumTestSphere(obj.position, obj.radius))
        return;

    if (!OcclusionTestSphere(obj.position, obj.radius))
        return;
    #endif

    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);

    g_VisibleIndices[visibleIdx] = obj.batchIndex;

    uint argsBaseOffset = obj.batchIndex * 20;
    g_DrawArgs.Store(argsBaseOffset + 4, 1);
}
