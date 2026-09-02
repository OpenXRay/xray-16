#include "shared/common.h"
#include "visbuffer_common.h"

Texture2D<uint> g_VisID : register(t30);
RWTexture2D<float4> g_VisDebug : register(u0);

cbuffer VisDebugParams : register(b5)
{
    uint visDebugMode;
    uint3 visDebugPad;
};

float3 HashColor(uint seed)
{
    uint h = seed * 747796405u + 2891336453u;
    h = ((h >> ((h >> 28u) + 4u)) ^ h) * 277803737u;
    h = (h >> 22u) ^ h;
    return float3(float(h & 255u), float((h >> 8u) & 255u), float((h >> 16u) & 255u)) / 255.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint width, height;
    g_VisID.GetDimensions(width, height);
    if (dtid.x >= width || dtid.y >= height)
        return;

    uint id = g_VisID[dtid.xy];
    if (id == 0u)
    {
        g_VisDebug[dtid.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    uint entry = id >> VIS_ID_TRI_BITS;
    uint tri = id & VIS_ID_TRI_MASK;
    float3 color = (visDebugMode == 2u) ? HashColor(entry * 131u + tri) : HashColor(entry) * (0.6 + 0.4 * float(tri & 1u));
    g_VisDebug[dtid.xy] = float4(color, 1.0);
}
