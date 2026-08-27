#include "shared/common.h"
#include "shared/clustered_lighting.h"

cbuffer ClusterParams : register(b5)
{
    float4 cb_gridDims;
    float4 cb_screenSize;
    float4 cb_depthParams;
    float4 cb_pad;
};

StructuredBuffer<GPULightData> g_Lights : register(t0);
StructuredBuffer<uint> g_VisibleLightIndices : register(t1);
ByteAddressBuffer g_VisibleLightCount : register(t2);

RWStructuredBuffer<uint2> g_ClusterGrid : register(u0);
RWStructuredBuffer<uint> g_LightIndexList : register(u1);
RWByteAddressBuffer g_LightIndexCounter : register(u2);

#define ASSIGN_THREADS 64
#define MAX_LIGHTS_PER_CLUSTER 256

groupshared uint s_count;
groupshared uint s_base;
groupshared uint s_indices[MAX_LIGHTS_PER_CLUSTER];

[numthreads(ASSIGN_THREADS, 1, 1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    uint tileX = gid.x;
    uint tileY = gid.y;
    uint slice = gid.z;

    uint tilesX = (uint)cb_gridDims.x;
    uint tilesY = (uint)cb_gridDims.y;
    uint numSlices = (uint)cb_gridDims.z;
    float tileSize = cb_depthParams.w;

    bool clusterValid = (tileX < tilesX && tileY < tilesY && slice < numSlices);
    uint clusterIdx = tileX + tileY * tilesX + slice * tilesX * tilesY;

    if (gtid.x == 0)
    {
        s_count = 0;
        s_base = 0xFFFFFFFFu;
    }
    GroupMemoryBarrierWithGroupSync();

    if (clusterValid)
    {
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

        for (uint iter = gtid.x; iter < numVisible; iter += ASSIGN_THREADS)
        {
            uint i = g_VisibleLightIndices[iter];
            GPULightData ld = g_Lights[i];
            float3 lightPos = ld.positionAndInvRangeSq.xyz;
            float range = ld.colorAndRange.w;

            float4 clipPos = mul(m_VP, float4(lightPos, 1.0));
            float lightDepth = clipPos.w;

            float depthNear = lightDepth - range;
            float depthFar = lightDepth + range;

            if (depthFar < sliceNear || depthNear > sliceFar)
                continue;

            if (lightDepth + range <= 0)
                continue;

            if (clipPos.w > 0 && lightDepth > range)
            {
                float3 ndc = clipPos.xyz / clipPos.w;
                float2 screenPos;
                screenPos.x = (ndc.x * 0.5 + 0.5) * cb_screenSize.x;
                screenPos.y = (0.5 - ndc.y * 0.5) * cb_screenSize.y;

                float zNearOverlap = max(sliceNear, max(depthNear, zNear));
                float screenRadius = (range / zNearOverlap) * cb_screenSize.y * 0.5;

                float2 closest = clamp(screenPos, tileMin, tileMax);
                float2 diff = screenPos - closest;
                if (dot(diff, diff) > screenRadius * screenRadius)
                    continue;
            }

            uint slot;
            InterlockedAdd(s_count, 1, slot);
            if (slot < MAX_LIGHTS_PER_CLUSTER)
                s_indices[slot] = i;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint lightCount = min(s_count, (uint)MAX_LIGHTS_PER_CLUSTER);

    if (gtid.x == 0 && clusterValid)
    {
        if (lightCount > 0)
        {
            uint globalOffset;
            g_LightIndexCounter.InterlockedAdd(0, lightCount, globalOffset);

            if (globalOffset + lightCount <= 1048576)
            {
                s_base = globalOffset;
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
    GroupMemoryBarrierWithGroupSync();

    if (clusterValid && s_base != 0xFFFFFFFFu)
    {
        for (uint j = gtid.x; j < lightCount; j += ASSIGN_THREADS)
            g_LightIndexList[s_base + j] = s_indices[j];
    }
}
