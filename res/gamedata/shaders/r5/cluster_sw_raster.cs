#define SM_6_0
#include "common.h"
#include "visbuffer_common.h"
#include "sw_raster_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_SwEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);
StructuredBuffer<InstanceData> g_DynamicInstanceData : register(t20);
StructuredBuffer<InstanceData> g_TerrainInstanceData : register(t21);

cbuffer SwRasterParams : register(b5)
{
    uint g_Width;
    uint g_Height;
    uint2 g_SwPad;
};

[numthreads(128, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint tri : SV_GroupIndex)
{
    uint entryIdx = g_SwEntries[groupID.x];
    ClusterEntry e = g_Entries[entryIdx];
    if (tri * 3u + 2u >= e.indexCount)
        return;

    float4x4 world;
    if ((e.flags & CLUSTER_ENTRY_FLAG_DYNAMIC) != 0u)
        world = g_DynamicInstanceData[e.batchIndex].world;
    else if ((e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u)
        world = g_TerrainInstanceData[e.batchIndex].world;
    else
        world = g_InstanceData[e.batchIndex].world;
    float4x4 wvp = mul(m_VP, world);

    float3 v[3];
    uint3 idx = g_MegaIB.Load3((e.ibFirst + tri * 3u) * 4u);
    [unroll]
    for (uint c = 0; c < 3; ++c)
    {
        uint3 w0 = g_MegaVB.Load3((e.firstVertex + idx[c]) * 48u);
        if (!SwProjectVertex(wvp, float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z)), g_Width, g_Height, v[c]))
            return;
    }

    SwRasterizeTriangle(v[0], v[1], v[2], g_Width, g_Height, PackVisID(entryIdx, tri));
}
