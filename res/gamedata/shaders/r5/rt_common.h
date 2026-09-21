#ifndef RT_COMMON_H
#define RT_COMMON_H

static const uint RT_STATIC_VERTEX_STRIDE = 40;
static const uint RT_SKINNED_VERTEX_STRIDE = 32;
static const uint RT_GRASS_VERTEX_STRIDE = 24;
static const float RT_PACKED_VECTOR_MIN_LENGTH_SQ = 0.25;

struct RTBatchInfo {
    uint materialID;
    uint startIndex;
    int baseVertex;
    uint indexCount;
};

struct RTTriangleVertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
    bool authoredBasis;
};

float3 DecodePackedNormal(uint packed)
{
    return float3(
        ((packed >> 16) & 0xFF) / 127.5 - 1.0,
        ((packed >>  8) & 0xFF) / 127.5 - 1.0,
        ((packed >>  0) & 0xFF) / 127.5 - 1.0);
}

void RTLoadTriangleIndices(ByteAddressBuffer indices, RTBatchInfo info, uint primitiveIndex,
    out uint i0, out uint i1, out uint i2)
{
    uint triBase = info.startIndex + primitiveIndex * 3;
    i0 = indices.Load(triBase * 4 + 0) + info.baseVertex;
    i1 = indices.Load(triBase * 4 + 4) + info.baseVertex;
    i2 = indices.Load(triBase * 4 + 8) + info.baseVertex;
}

float2 RTInterpolateUV(float2 uv0, float2 uv1, float2 uv2, float2 barycentrics)
{
    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return uv0 * w0 + uv1 * barycentrics.x + uv2 * barycentrics.y;
}

float2 RTLoadStaticVertexUV(ByteAddressBuffer vb, uint index)
{
    return asfloat(vb.Load2(index * RT_STATIC_VERTEX_STRIDE + 24));
}

float2 RTLoadSkinnedVertexUV(ByteAddressBuffer vb, uint index)
{
    return asfloat(vb.Load2(index * RT_SKINNED_VERTEX_STRIDE + 16));
}

float2 RTLoadGrassVertexUV(ByteAddressBuffer vb, uint index)
{
    return asfloat(vb.Load2(index * RT_GRASS_VERTEX_STRIDE + 16));
}

RTTriangleVertex RTLoadStaticVertex(ByteAddressBuffer vb, uint index)
{
    uint addr = index * RT_STATIC_VERTEX_STRIDE;
    RTTriangleVertex v;
    v.position = asfloat(vb.Load3(addr));
    v.normal = asfloat(vb.Load3(addr + 12));
    v.uv = asfloat(vb.Load2(addr + 24));
    v.tangent = DecodePackedNormal(vb.Load(addr + 32));
    v.bitangent = DecodePackedNormal(vb.Load(addr + 36));
    v.authoredBasis = true;
    return v;
}

RTTriangleVertex RTLoadSkinnedVertex(ByteAddressBuffer vb, uint index)
{
    uint addr = index * RT_SKINNED_VERTEX_STRIDE;
    RTTriangleVertex v;
    v.position = asfloat(vb.Load3(addr));
    v.normal = DecodePackedNormal(vb.Load(addr + 12));
    v.uv = asfloat(vb.Load2(addr + 16));
    v.tangent = DecodePackedNormal(vb.Load(addr + 24));
    v.bitangent = DecodePackedNormal(vb.Load(addr + 28));
    v.authoredBasis = true;
    return v;
}

RTTriangleVertex RTLoadGrassVertex(ByteAddressBuffer vb, uint index)
{
    uint addr = index * RT_GRASS_VERTEX_STRIDE;
    RTTriangleVertex v;
    v.position = asfloat(vb.Load3(addr));
    v.normal = DecodePackedNormal(vb.Load(addr + 12));
    v.uv = asfloat(vb.Load2(addr + 16));
    v.tangent = 0.0;
    v.bitangent = 0.0;
    v.authoredBasis = false;
    return v;
}

struct RTShadingVertex
{
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
    bool authoredBasis;
};

RTShadingVertex RTInterpolateTriangleVertex(RTTriangleVertex v0, RTTriangleVertex v1, RTTriangleVertex v2, float2 barycentrics)
{
    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    RTShadingVertex r;
    r.normal = v0.normal * w0 + v1.normal * barycentrics.x + v2.normal * barycentrics.y;
    r.uv = v0.uv * w0 + v1.uv * barycentrics.x + v2.uv * barycentrics.y;
    r.tangent = v0.tangent * w0 + v1.tangent * barycentrics.x + v2.tangent * barycentrics.y;
    r.bitangent = v0.bitangent * w0 + v1.bitangent * barycentrics.x + v2.bitangent * barycentrics.y;
    r.authoredBasis = v0.authoredBasis;
    return r;
}

float3 RTSafeNormalize(float3 v, float3 fallback)
{
    float lenSq = dot(v, v);
    return lenSq > 1e-12 ? v * rsqrt(lenSq) : fallback;
}

bool RTPackedVectorValid(float3 v)
{
    float lenSq = dot(v, v);
    return lenSq > RT_PACKED_VECTOR_MIN_LENGTH_SQ && all(isfinite(v));
}

float3x3 RTModelTransform(float3x4 objectToWorld)
{
    return float3x3(objectToWorld[0].xyz, objectToWorld[1].xyz, objectToWorld[2].xyz);
}

float3 TransformNormalToWorld(float3 localNormal, float3x4 objectToWorld)
{
    float3x3 model = RTModelTransform(objectToWorld);
    float3x3 cofactors = float3x3(cross(model[1], model[2]), cross(model[2], model[0]), cross(model[0], model[1]));
    float determinant = dot(model[0], cofactors[0]);
    float3x3 normalTransform = abs(determinant) > 1e-12 ? cofactors / determinant : cofactors;
    return RTSafeNormalize(mul(normalTransform, localNormal), float3(0.0, 1.0, 0.0));
}

float3 TransformPointToWorld(float3 localPosition, float3x4 objectToWorld)
{
    return mul(objectToWorld, float4(localPosition, 1.0));
}

void RTUVDerivedBasis(float3 p0, float3 p1, float3 p2, float2 uv0, float2 uv1, float2 uv2,
    out float3 tangent, out float3 bitangent)
{
    float3 e1 = p1 - p0;
    float3 e2 = p2 - p0;
    float2 d1 = uv1 - uv0;
    float2 d2 = uv2 - uv0;
    float det = d1.x * d2.y - d2.x * d1.y;
    if (abs(det) < 1e-12)
    {
        tangent = 0.0;
        bitangent = 0.0;
        return;
    }
    float inv = 1.0 / det;
    tangent = (e1 * d2.y - e2 * d1.y) * inv;
    bitangent = (e2 * d1.x - e1 * d2.x) * inv;
}

void RTNormalizedBasis(float3 N, float3 tangent, float3 bitangent, out float3 T, out float3 B)
{
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float tLenSq = dot(tangent, tangent);
    T = tLenSq > 1e-10 ? tangent * rsqrt(tLenSq) : normalize(cross(up, N));
    float bLenSq = dot(bitangent, bitangent);
    B = bLenSq > 1e-10 ? bitangent * rsqrt(bLenSq) : cross(N, T);
}

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand_float(inout uint seed)
{
    seed = pcg_hash(seed);
    return float(seed >> 8u) * (1.0 / 16777216.0);
}

#endif
