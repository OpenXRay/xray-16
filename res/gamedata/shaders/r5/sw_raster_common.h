#ifndef SW_RASTER_COMMON_H
#define SW_RASTER_COMMON_H

RWStructuredBuffer<uint64_t> g_VisBuffer : register(u0);

float SwEdgeFunction(float2 a, float2 b, float2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

float SwMin3(float a, float b, float c) { return min(a, min(b, c)); }
float SwMax3(float a, float b, float c) { return max(a, max(b, c)); }

void SwWritePixel(uint width, float x, float y, float z, uint id)
{
    uint64_t v = (uint64_t(asuint(z)) << 32) | uint64_t(id);
    uint64_t prev;
    InterlockedMax(g_VisBuffer[uint(y) * width + uint(x)], v, prev);
}

bool SwProjectVertex(float4x4 wvp, float3 p, uint width, uint height, out float3 v)
{
    float4 clip = mul(wvp, float4(p, 1.0));
    v = float3(0.0, 0.0, 0.0);
    if (clip.w <= 0.0)
        return false;
    float3 ndc = clip.xyz / clip.w;
    v = float3((ndc.x * 0.5 + 0.5) * float(width), (0.5 - ndc.y * 0.5) * float(height), ndc.z);
    return true;
}

void SwRasterizeTriangle(float3 v0, float3 v1, float3 v2, uint width, uint height, uint id)
{
    float area = SwEdgeFunction(v0.xy, v1.xy, v2.xy);
    if (area <= 0.0)
        return;

    float3 wX = float3(v1.y - v2.y, v2.y - v0.y, v0.y - v1.y);
    float3 wY = float3(v2.x - v1.x, v0.x - v2.x, v1.x - v0.x);
    float3 zOverArea = float3(v0.z, v1.z, v2.z) / area;
    float zX = dot(zOverArea, wX);
    float zY = dot(zOverArea, wY);

    float minX = max(floor(SwMin3(v0.x, v1.x, v2.x)), 0.0);
    float minY = max(floor(SwMin3(v0.y, v1.y, v2.y)), 0.0);
    float maxX = min(ceil(SwMax3(v0.x, v1.x, v2.x)), float(width) - 1.0);
    float maxY = min(ceil(SwMax3(v0.y, v1.y, v2.y)), float(height) - 1.0);
    if (minX > maxX || minY > maxY)
        return;

    float2 start = float2(minX, minY) + 0.5;
    float3 wRow = float3(
        SwEdgeFunction(v1.xy, v2.xy, start),
        SwEdgeFunction(v2.xy, v0.xy, start),
        SwEdgeFunction(v0.xy, v1.xy, start));
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
            float x0 = ceil(SwMax3(lo.x, lo.y, lo.z));
            float x1 = SwMin3(hi.x, hi.y, hi.z);
            float3 w = wRow + wX * x0;
            float z = zRow + zX * x0;
            x0 += minX;
            x1 += minX;
            for (float x = x0; x <= x1; x += 1.0)
            {
                if (SwMin3(w.x, w.y, w.z) >= 0.0)
                    SwWritePixel(width, x, y, z, id);
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
                if (SwMin3(w.x, w.y, w.z) >= 0.0)
                    SwWritePixel(width, x, y, z, id);
                w += wX;
                z += zX;
            }
            wRow += wY;
            zRow += zY;
        }
    }
}

#endif
