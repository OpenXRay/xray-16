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

        uint numVisible = min(g_VisibleLightCount.Load(0), (uint)cb_gridDims.w);

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

            // Test against the actual cluster side planes. A projected radius
            // that omits focal length underestimates light coverage at narrow FOV.
            float left = tileMin.x / cb_screenSize.x * 2.0 - 1.0;
            float right = tileMax.x / cb_screenSize.x * 2.0 - 1.0;
            float top = 1.0 - tileMin.y / cb_screenSize.y * 2.0;
            float bottom = 1.0 - tileMax.y / cb_screenSize.y * 2.0;
            float4 planes[4] = {
                m_VP[0] - left * m_VP[3], right * m_VP[3] - m_VP[0],
                top * m_VP[3] - m_VP[1], m_VP[1] - bottom * m_VP[3]
            };
            bool outside = false;
            [unroll] for (uint p = 0; p < 4; ++p)
                outside = outside || dot(planes[p], float4(lightPos, 1.0)) < -range * length(planes[p].xyz);
            if (outside)
                continue;

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
        if (s_count > MAX_LIGHTS_PER_CLUSTER)
        {
            // Receivers evaluate the full light buffer for an overflowing cluster.
            g_ClusterGrid[clusterIdx] = uint2(0xFFFFFFFFu, (uint)cb_gridDims.w);
        }
        else if (lightCount > 0)
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
                g_ClusterGrid[clusterIdx] = uint2(0xFFFFFFFFu, (uint)cb_gridDims.w);
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
