#ifndef CLUSTER_GEO_PAYLOAD_H
#define CLUSTER_GEO_PAYLOAD_H

#include "cluster_geo_common.h"

#ifndef CLUSTER_GEO_T_PAYLOAD
#define CLUSTER_GEO_T_PAYLOAD t18
#endif
#ifndef CLUSTER_GEO_T_VERTICES
#define CLUSTER_GEO_T_VERTICES t19
#endif
#ifndef CLUSTER_GEO_T_PAGES
#define CLUSTER_GEO_T_PAGES t24
#endif

ByteAddressBuffer g_ClusterPayload : register(CLUSTER_GEO_T_PAYLOAD);
ByteAddressBuffer g_ClusterVertices : register(CLUSTER_GEO_T_VERTICES);
StructuredBuffer<ClusterPage> g_ClusterPages : register(CLUSTER_GEO_T_PAGES);

#define CLUSTER_PAGE_VERTEX_STRIDE 32u
#define CLUSTER_MAX_VERTICES 128
#define CLUSTER_MAX_TRIANGLES 128
#define CLUSTER_SW_LANES 128
#define CLUSTER_MESH_LANES 128
#define CLUSTER_PAGE_UV1_STRIDE 8u
#define CLUSTER_PAGE_COLOR_STRIDE 4u
#define CLUSTER_PAGE_FLAGS_STRIDE 4u

#define CLUSTER_VERTEX_PACKED_BASIS 0u
#define CLUSTER_VERTEX_FLOAT_NORMAL 1u

#define CLUSTER_PAGE_ATTR_UV1 1u
#define CLUSTER_PAGE_ATTR_COLOR 2u
#define CLUSTER_PAGE_ATTR_FLAGS 4u

struct ClusterGeoView
{
    uint payloadBase;
    uint vertexBase;
    uint vertexCount;
    uint triangleCount;
    uint vertexFormat;
    uint attributeMask;
    uint pageVertexCount;
    uint triangleBase;
};

struct ClusterVertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 binormal;
    float2 uv;
};

uint ClusterAlign4(uint value)
{
    return (value + 3u) & ~3u;
}

ClusterGeoView ClusterGeoResolve(ClusterEntry e)
{
    ClusterPage page = g_ClusterPages[e.page];
    ClusterGeoView view;
    view.payloadBase = page.payloadBase + e.payloadOffset;
    view.vertexBase = page.vertexBase;
    view.vertexCount = e.vertexCount;
    view.triangleCount = e.indexCount / 3u;
    view.vertexFormat = page.vertexFormat;
    view.attributeMask = page.attributeMask;
    view.pageVertexCount = page.vertexCount;
    view.triangleBase = ClusterAlign4(e.vertexCount * 2u);
    return view;
}

uint ClusterPayloadSlot(ClusterGeoView view, uint localVertex)
{
    uint byteOffset = view.payloadBase + localVertex * 2u;
    uint word = g_ClusterPayload.Load(byteOffset & ~3u);
    return ((byteOffset & 2u) != 0u) ? (word >> 16) : (word & 0xFFFFu);
}

uint3 ClusterPayloadTriangle(ClusterGeoView view, uint triangle)
{
    uint byteOffset = view.payloadBase + view.triangleBase + triangle * 3u;
    uint aligned = byteOffset & ~3u;
    uint shift = (byteOffset & 3u) * 8u;
    uint2 words = uint2(g_ClusterPayload.Load(aligned), g_ClusterPayload.Load(aligned + 4u));
    uint packed = words.x >> shift;
    if (shift != 0u)
        packed |= words.y << ((32u - shift) & 31u);
    return uint3(packed & 0xFFu, (packed >> 8) & 0xFFu, (packed >> 16) & 0xFFu);
}

void ClusterDeriveBasis(float3 n, out float3 tangent, out float3 binormal)
{
    float lenSq = dot(n, n);
    n = (lenSq > 1e-12 && isfinite(lenSq)) ? n / sqrt(lenSq) : float3(0.0, 0.0, 1.0);
    float s = (n.z >= 0.0) ? 1.0 : -1.0;
    float a = -1.0 / (s + n.z);
    float b = n.x * n.y * a;
    tangent = float3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
    binormal = float3(b, s + n.y * n.y * a, -n.y);
}

float3 ClusterLoadPositionSlot(ClusterGeoView view, uint slot)
{
    uint3 w = g_ClusterVertices.Load3(view.vertexBase + slot * CLUSTER_PAGE_VERTEX_STRIDE);
    return float3(asfloat(w.x), asfloat(w.y), asfloat(w.z));
}

float2 ClusterLoadUVSlot(ClusterGeoView view, uint slot)
{
    uint2 w = g_ClusterVertices.Load2(view.vertexBase + slot * CLUSTER_PAGE_VERTEX_STRIDE + 24u);
    return float2(asfloat(w.x), asfloat(w.y));
}

ClusterVertex ClusterLoadVertexSlot(ClusterGeoView view, uint slot)
{
    uint base = view.vertexBase + slot * CLUSTER_PAGE_VERTEX_STRIDE;
    uint4 w0 = g_ClusterVertices.Load4(base);
    uint4 w1 = g_ClusterVertices.Load4(base + 16u);

    ClusterVertex v;
    v.position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    if (view.vertexFormat == CLUSTER_VERTEX_FLOAT_NORMAL)
    {
        v.normal = float3(asfloat(w0.w), asfloat(w1.x), asfloat(w1.y));
        ClusterDeriveBasis(v.normal, v.tangent, v.binormal);
    }
    else
    {
        v.normal = UnpackD3DColorDir(w0.w);
        v.tangent = UnpackD3DColorDir(w1.x);
        v.binormal = UnpackD3DColorDir(w1.y);
    }
    v.uv = float2(asfloat(w1.z), asfloat(w1.w));
    return v;
}

float3 ClusterLoadPosition(ClusterGeoView view, uint localVertex)
{
    return ClusterLoadPositionSlot(view, ClusterPayloadSlot(view, localVertex));
}

ClusterVertex ClusterLoadVertex(ClusterGeoView view, uint localVertex)
{
    return ClusterLoadVertexSlot(view, ClusterPayloadSlot(view, localVertex));
}

uint ClusterPageUV1Base(ClusterGeoView view)
{
    return view.vertexBase + view.pageVertexCount * CLUSTER_PAGE_VERTEX_STRIDE;
}

uint ClusterPageColorBase(ClusterGeoView view)
{
    uint base = ClusterPageUV1Base(view);
    if ((view.attributeMask & CLUSTER_PAGE_ATTR_UV1) != 0u)
        base += view.pageVertexCount * CLUSTER_PAGE_UV1_STRIDE;
    return base;
}

uint ClusterPageFlagsBase(ClusterGeoView view)
{
    uint base = ClusterPageColorBase(view);
    if ((view.attributeMask & CLUSTER_PAGE_ATTR_COLOR) != 0u)
        base += view.pageVertexCount * CLUSTER_PAGE_COLOR_STRIDE;
    return base;
}

float2 ClusterLoadUV1Slot(ClusterGeoView view, uint slot)
{
    if ((view.attributeMask & CLUSTER_PAGE_ATTR_UV1) == 0u)
        return float2(0.0, 0.0);
    uint2 w = g_ClusterVertices.Load2(ClusterPageUV1Base(view) + slot * CLUSTER_PAGE_UV1_STRIDE);
    return float2(asfloat(w.x), asfloat(w.y));
}

uint ClusterLoadColorSlot(ClusterGeoView view, uint slot)
{
    if ((view.attributeMask & CLUSTER_PAGE_ATTR_COLOR) == 0u)
        return 0xFFFFFFFFu;
    return g_ClusterVertices.Load(ClusterPageColorBase(view) + slot * CLUSTER_PAGE_COLOR_STRIDE);
}

uint ClusterLoadFlagsSlot(ClusterGeoView view, uint slot)
{
    if ((view.attributeMask & CLUSTER_PAGE_ATTR_FLAGS) == 0u)
        return 0u;
    return g_ClusterVertices.Load(ClusterPageFlagsBase(view) + slot * CLUSTER_PAGE_FLAGS_STRIDE);
}

#endif
