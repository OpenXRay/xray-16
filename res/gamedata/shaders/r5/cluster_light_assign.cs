#include "shared/common.h"
#include "shared/clustered_lighting.h"

cbuffer ClusterParams : register(b5)
{
    float4 cb_gridDims;
    float4 cb_screenSize;
    float4 cb_depthParams;
    float4 cb_pad; // x = m_P._22 = cot(fovY/2)
};

StructuredBuffer<GPULightData> g_Lights : register(t0);
StructuredBuffer<uint> g_VisibleLightIndices : register(t1);
ByteAddressBuffer g_VisibleLightCount : register(t2);

RWStructuredBuffer<uint2> g_ClusterGrid : register(u0);
RWStructuredBuffer<uint> g_LightIndexList : register(u1);
RWByteAddressBuffer g_LightIndexCounter : register(u2);

groupshared uint gs_hits[256];
groupshared uint gs_hitCount;

bool LightHitsCluster(
    GPULightData ld,
    float sliceNear,
    float sliceFar,
    float2 tileMin,
    float2 tileMax,
    float tileSize,
    float cotFovY,
    float zNear)
{
    float3 lightPos = ld.positionAndInvRangeSq.xyz;
    float range = ld.colorAndRange.w;

    float4 clipPos = mul(m_VP, float4(lightPos, 1.0));
    float lightDepth = clipPos.w;

    float depthNear = lightDepth - range;
    float depthFar = lightDepth + range;

    if (depthFar < sliceNear || depthNear > sliceFar)
        return false;

    if (lightDepth + range <= 0)
        return false;

    if (clipPos.w > 1e-4 && lightDepth > range)
    {
        float3 ndc = clipPos.xyz / clipPos.w;
        float2 screenPos;
        screenPos.x = (ndc.x * 0.5 + 0.5) * cb_screenSize.x;
        screenPos.y = (0.5 - ndc.y * 0.5) * cb_screenSize.y;

        float zNearOverlap = max(sliceNear, max(depthNear, zNear));
        float screenRadius = (range / zNearOverlap) * cotFovY * cb_screenSize.y * 0.5;
        screenRadius += tileSize;

        float2 closest = clamp(screenPos, tileMin, tileMax);
        float2 diff = screenPos - closest;
        if (dot(diff, diff) > screenRadius * screenRadius)
            return false;
    }

    return true;
}

[numthreads(64, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint gtid : SV_GroupIndex)
{
    uint tilesX = (uint)cb_gridDims.x;
    uint tilesY = (uint)cb_gridDims.y;
    uint numSlices = (uint)cb_gridDims.z;
    float tileSize = cb_depthParams.w;

    uint clusterIdx = groupId.x;
    uint clustersTotal = tilesX * tilesY * numSlices;
    if (clusterIdx >= clustersTotal)
        return;

    uint slice = clusterIdx / (tilesX * tilesY);
    uint rem = clusterIdx - slice * tilesX * tilesY;
    uint tileY = rem / tilesX;
    uint tileX = rem - tileY * tilesX;

    float zNear = cb_depthParams.x;
    float zFar = cb_depthParams.y;
    float logRatio = cb_depthParams.z;

    float sliceNear = zNear * exp2((float)slice / logRatio);
    float sliceFar = zNear * exp2((float)(slice + 1) / logRatio);
    sliceFar = min(sliceFar, zFar);

    float2 tileMin = float2(tileX * tileSize, tileY * tileSize);
    float2 tileMax = float2(min((tileX + 1) * tileSize, cb_screenSize.x),
                             min((tileY + 1) * tileSize, cb_screenSize.y));

    uint numVisible = min(g_VisibleLightCount.Load(0), 1024u);
    float cotFovY = max(cb_pad.x, 0.5);

    if (gtid == 0)
        gs_hitCount = 0;
    GroupMemoryBarrierWithGroupSync();

    for (uint iter = gtid; iter < numVisible; iter += 64)
    {
        uint i = g_VisibleLightIndices[iter];
        GPULightData ld = g_Lights[i];
        if (!LightHitsCluster(ld, sliceNear, sliceFar, tileMin, tileMax, tileSize, cotFovY, zNear))
            continue;

        uint slot;
        InterlockedAdd(gs_hitCount, 1, slot);
        if (slot < 256)
            gs_hits[slot] = i;
    }

    GroupMemoryBarrierWithGroupSync();

    uint lightCount = min(gs_hitCount, 256u);

    if (gtid == 0)
    {
        if (lightCount > 1)
        {
            for (uint a = 1; a < lightCount; a++)
            {
                uint key = gs_hits[a];
                uint b = a;
                while (b > 0 && gs_hits[b - 1] > key)
                {
                    gs_hits[b] = gs_hits[b - 1];
                    b--;
                }
                gs_hits[b] = key;
            }
        }

        if (lightCount > 0)
        {
            uint globalOffset;
            g_LightIndexCounter.InterlockedAdd(0, lightCount, globalOffset);

            if (globalOffset + lightCount <= 1048576)
            {
                for (uint j = 0; j < lightCount; j++)
                    g_LightIndexList[globalOffset + j] = gs_hits[j];

                g_ClusterGrid[clusterIdx] = uint2(globalOffset, lightCount);
            }
            else
            {
                g_ClusterGrid[clusterIdx] = uint2(0, 0);
            }
        }
        else
        {
            g_ClusterGrid[clusterIdx] = uint2(0, 0);
        }
    }
}

#define LOCAL_SHADOW_RECT_F4_V1 1
