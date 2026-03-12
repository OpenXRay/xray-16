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

RWStructuredBuffer<uint2> g_ClusterGrid : register(u0);
RWStructuredBuffer<uint> g_LightIndexList : register(u1);
RWByteAddressBuffer g_LightIndexCounter : register(u2);

[numthreads(1, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint tileX = dtid.x;
    uint tileY = dtid.y;
    uint slice = dtid.z;

    uint tilesX = (uint)cb_gridDims.x;
    uint tilesY = (uint)cb_gridDims.y;
    uint numSlices = (uint)cb_gridDims.z;
    uint numLights = (uint)cb_gridDims.w;
    float tileSize = cb_depthParams.w;

    if (tileX >= tilesX || tileY >= tilesY || slice >= numSlices)
        return;

    uint clusterIdx = tileX + tileY * tilesX + slice * tilesX * tilesY;

    float zNear = cb_depthParams.x;
    float zFar = cb_depthParams.y;
    float logRatio = cb_depthParams.z;

    float sliceNear = zNear * exp2((float)slice / logRatio);
    float sliceFar = zNear * exp2((float)(slice + 1) / logRatio);
    sliceFar = min(sliceFar, zFar);

    float2 tileMin = float2(tileX * tileSize, tileY * tileSize);
    float2 tileMax = float2(min((tileX + 1) * tileSize, cb_screenSize.x),
                             min((tileY + 1) * tileSize, cb_screenSize.y));

    uint lightCount = 0;
    uint lightIndices[256];

    for (uint i = 0; i < numLights; i++)
    {
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

        if (lightCount < 256)
            lightIndices[lightCount] = i;
        lightCount++;
    }

    lightCount = min(lightCount, 256);

    if (lightCount > 0)
    {
        uint globalOffset;
        g_LightIndexCounter.InterlockedAdd(0, lightCount, globalOffset);

        if (globalOffset + lightCount <= 1048576)
        {
            for (uint j = 0; j < lightCount; j++)
                g_LightIndexList[globalOffset + j] = lightIndices[j];

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
