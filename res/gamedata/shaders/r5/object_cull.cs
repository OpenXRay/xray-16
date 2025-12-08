#define SM_5_0
#include "common.h"
#include "cull_utils.h"

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

bool OcclusionTestSphere(float3 center, float radius)
{
    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    if (clipPos.w <= 0.001)
        return true;

    float3 ndc = clipPos.xyz / clipPos.w;

    float projScale = max(abs(g_ViewProj[0][0]), abs(g_ViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / clipPos.w;

    float2 minNDC = ndc.xy - ndcSize;
    float2 maxNDC = ndc.xy + ndcSize;

    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return true;

    float2 minUV = saturate(minNDC * 0.5 + 0.5);
    float2 maxUV = saturate(maxNDC * 0.5 + 0.5);

    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;

    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    float boxWidth = (boxUV.z - boxUV.x) * float(g_HiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(g_HiZHeight);

    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    float d1 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.z, boxUV.w), mipLevel);

    float hiZDepth = max(max(d1, d2), max(d3, d4));

    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;

    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));

    if (frontClip.w <= 0.001)
        return true;

    float objectDepth = saturate(frontClip.z / frontClip.w);

    float depthBias = 0.0001;
    return objectDepth <= (hiZDepth + depthBias);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;

    if (objectIdx >= g_ObjectCount)
        return;

    GPUObjectData obj = g_Objects[objectIdx];

    if (obj.flags & 0x4)
        return;

    if (!DistanceTestSphere(obj.position, obj.radius, g_CameraPos, g_MaxDistance))
        return;

    if (!FrustumTestSphere(obj.position, obj.radius, g_FrustumPlanes))
        return;

    if (!OcclusionTestSphere(obj.position, obj.radius))
        return;

    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);

    g_VisibleIndices[visibleIdx] = obj.batchIndex;

    uint argsBaseOffset = obj.batchIndex * 20;
    g_DrawArgs.Store(argsBaseOffset + 4, 1);
}
