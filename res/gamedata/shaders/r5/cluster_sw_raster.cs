#define SM_6_0
#include "common.h"
#include "visbuffer_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_SwEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);
StructuredBuffer<InstanceData> g_DynamicInstanceData : register(t20);
StructuredBuffer<InstanceData> g_TerrainInstanceData : register(t21);
RWStructuredBuffer<uint64_t> g_VisBuffer : register(u0);

cbuffer SwRasterParams : register(b5)
{
    uint g_Width;
    uint g_Height;
    uint2 g_SwPad;
};

float EdgeFunction(float2 a, float2 b, float2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

float Min3(float a, float b, float c) { return min(a, min(b, c)); }
float Max3(float a, float b, float c) { return max(a, max(b, c)); }

void WritePixel(float x, float y, float z, uint id)
{
    uint64_t v = (uint64_t(asuint(z)) << 32) | uint64_t(id);
    uint64_t prev;
    InterlockedMax(g_VisBuffer[uint(y) * g_Width + uint(x)], v, prev);
}

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
        float4 clip = mul(wvp, float4(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z), 1.0));
        if (clip.w <= 0.0)
            return;
        float3 ndc = clip.xyz / clip.w;
        v[c] = float3((ndc.x * 0.5 + 0.5) * float(g_Width), (0.5 - ndc.y * 0.5) * float(g_Height), ndc.z);
    }

    float area = EdgeFunction(v[0].xy, v[1].xy, v[2].xy);
    if (area <= 0.0)
        return;

    float3 wX = float3(v[1].y - v[2].y, v[2].y - v[0].y, v[0].y - v[1].y);
    float3 wY = float3(v[2].x - v[1].x, v[0].x - v[2].x, v[1].x - v[0].x);
    float3 zOverArea = float3(v[0].z, v[1].z, v[2].z) / area;
    float zX = dot(zOverArea, wX);
    float zY = dot(zOverArea, wY);

    float minX = max(floor(Min3(v[0].x, v[1].x, v[2].x)), 0.0);
    float minY = max(floor(Min3(v[0].y, v[1].y, v[2].y)), 0.0);
    float maxX = min(ceil(Max3(v[0].x, v[1].x, v[2].x)), float(g_Width) - 1.0);
    float maxY = min(ceil(Max3(v[0].y, v[1].y, v[2].y)), float(g_Height) - 1.0);
    if (minX > maxX || minY > maxY)
        return;

    uint id = PackVisID(entryIdx, tri);
    float2 start = float2(minX, minY) + 0.5;
    float3 wRow = float3(
        EdgeFunction(v[1].xy, v[2].xy, start),
        EdgeFunction(v[2].xy, v[0].xy, start),
        EdgeFunction(v[0].xy, v[1].xy, start));
    float zRow = dot(zOverArea, wRow);

    if (maxX - minX > 4.0)
    {
        float3 edge = -wX;
        bool3 open = edge < 0.0;
        float3 invEdge = select(edge == 0.0, float3(1e8, 1e8, 1e8), 1.0 / edge);
        float3 spanX = float3(maxX - minX, maxX - minX, maxX - minX);
        for (float y = minY; y <= maxY; y += 1.0)
        {
            float3 cross = wRow * invEdge;
            float3 lo = select(open, cross, float3(0.0, 0.0, 0.0));
            float3 hi = select(open, spanX, cross);
            float x0 = ceil(Max3(lo.x, lo.y, lo.z));
            float x1 = Min3(hi.x, hi.y, hi.z);
            float3 w = wRow + wX * x0;
            float z = zRow + zX * x0;
            x0 += minX;
            x1 += minX;
            for (float x = x0; x <= x1; x += 1.0)
            {
                if (Min3(w.x, w.y, w.z) >= 0.0)
                    WritePixel(x, y, z, id);
                w += wX;
                z += zX;
            }
            wRow += wY;
            zRow += zY;
        }
    }
    else
    {
        for (float y = minY; y <= maxY; y += 1.0)
        {
            float3 w = wRow;
            float z = zRow;
            for (float x = minX; x <= maxX; x += 1.0)
            {
                if (Min3(w.x, w.y, w.z) >= 0.0)
                    WritePixel(x, y, z, id);
                w += wX;
                z += zX;
            }
            wRow += wY;
            zRow += zY;
        }
    }
}
