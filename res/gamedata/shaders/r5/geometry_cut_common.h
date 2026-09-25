#ifndef GEOMETRY_CUT_COMMON_H
#define GEOMETRY_CUT_COMMON_H

ByteAddressBuffer g_GeometryCuts;

uint geometryFirstCutAfter(uint revision, uint count)
{
    uint first = 0u;
    uint last = count;
    while (first < last)
    {
        uint middle = first + (last - first) / 2u;
        if (g_GeometryCuts.Load(48u + middle * 48u) <= revision)
            first = middle + 1u;
        else
            last = middle;
    }
    return first;
}

void geometryCutBounds(uint index, out float3 center, out float3 extent)
{
    uint address = 16u + index * 48u;
    float3 lo = asfloat(g_GeometryCuts.Load3(address));
    float3 hi = asfloat(g_GeometryCuts.Load3(address + 16u));
    center = (lo + hi) * 0.5;
    extent = (hi - lo) * 0.5;
}

bool geometryCutTouchesPlanes(uint index, float4 planes[6])
{
    float3 center, extent;
    geometryCutBounds(index, center, extent);
    for (uint p = 0u; p < 5u; ++p)
    {
        if (dot(planes[p].xyz, center) + planes[p].w > dot(abs(planes[p].xyz), extent))
            return false;
    }
    return true;
}

#endif
