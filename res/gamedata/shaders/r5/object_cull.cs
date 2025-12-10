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

    if (!HiZTestSphere(obj.position, obj.radius, g_CameraPos, g_ViewProj,
                       g_HiZPyramid, g_PointSampler, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        return;

    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);

    g_VisibleIndices[visibleIdx] = obj.batchIndex;

    uint argsBaseOffset = obj.batchIndex * 20;
    g_DrawArgs.Store(argsBaseOffset + 4, 1);
}
