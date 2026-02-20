#ifndef RT_COMMON_H
#define RT_COMMON_H

struct RTBatchInfo {
    uint materialID;
    uint startIndex;
    int baseVertex;
    uint indexCount;
};

float3 WorldPosFromDepth(int2 pixel, float depth, float2 screenSize, float4x4 invViewProj)
{
    float2 uv = (float2(pixel) + 0.5) / screenSize;
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;
    float4 world = mul(invViewProj, clip);
    return world.xyz / world.w;
}

float2 GetHitUV(ByteAddressBuffer megaVB, ByteAddressBuffer megaIB,
                RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = megaIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = megaIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = megaIB.Load(triBase * 4 + 8) + info.baseVertex;

    float2 uv0 = asfloat(megaVB.Load2(i0 * 48 + 24));
    float2 uv1 = asfloat(megaVB.Load2(i1 * 48 + 24));
    float2 uv2 = asfloat(megaVB.Load2(i2 * 48 + 24));

    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return uv0 * w0 + uv1 * barycentrics.x + uv2 * barycentrics.y;
}

float3 DecodePackedNormal(uint packed)
{
    return float3(
        ((packed >> 16) & 0xFF) / 127.5 - 1.0,
        ((packed >>  8) & 0xFF) / 127.5 - 1.0,
        ((packed >>  0) & 0xFF) / 127.5 - 1.0);
}

float3 GetHitNormal(ByteAddressBuffer megaVB, ByteAddressBuffer megaIB,
                    RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = megaIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = megaIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = megaIB.Load(triBase * 4 + 8) + info.baseVertex;

    float3 n0 = DecodePackedNormal(megaVB.Load(i0 * 48 + 12));
    float3 n1 = DecodePackedNormal(megaVB.Load(i1 * 48 + 12));
    float3 n2 = DecodePackedNormal(megaVB.Load(i2 * 48 + 12));

    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return normalize(n0 * w0 + n1 * barycentrics.x + n2 * barycentrics.y);
}

float3 GetHitGeometricNormal(ByteAddressBuffer megaVB, ByteAddressBuffer megaIB,
                             RTBatchInfo info, uint primitiveIndex)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = megaIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = megaIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = megaIB.Load(triBase * 4 + 8) + info.baseVertex;

    float3 p0 = asfloat(megaVB.Load3(i0 * 48));
    float3 p1 = asfloat(megaVB.Load3(i1 * 48));
    float3 p2 = asfloat(megaVB.Load3(i2 * 48));

    return normalize(cross(p1 - p0, p2 - p0));
}

float2 GetSkinnedHitUV(ByteAddressBuffer skinnedVB, ByteAddressBuffer skinnedIB,
                       RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = skinnedIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = skinnedIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = skinnedIB.Load(triBase * 4 + 8) + info.baseVertex;

    float2 uv0 = asfloat(skinnedVB.Load2(i0 * 24 + 16));
    float2 uv1 = asfloat(skinnedVB.Load2(i1 * 24 + 16));
    float2 uv2 = asfloat(skinnedVB.Load2(i2 * 24 + 16));

    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return uv0 * w0 + uv1 * barycentrics.x + uv2 * barycentrics.y;
}

float3 GetSkinnedHitNormal(ByteAddressBuffer skinnedVB, ByteAddressBuffer skinnedIB,
                           RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = skinnedIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = skinnedIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = skinnedIB.Load(triBase * 4 + 8) + info.baseVertex;

    float3 n0 = DecodePackedNormal(skinnedVB.Load(i0 * 24 + 12));
    float3 n1 = DecodePackedNormal(skinnedVB.Load(i1 * 24 + 12));
    float3 n2 = DecodePackedNormal(skinnedVB.Load(i2 * 24 + 12));

    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return normalize(n0 * w0 + n1 * barycentrics.x + n2 * barycentrics.y);
}

float3 GetSkinnedHitGeoNormal(ByteAddressBuffer skinnedVB, ByteAddressBuffer skinnedIB,
                              RTBatchInfo info, uint primitiveIndex)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = skinnedIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = skinnedIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = skinnedIB.Load(triBase * 4 + 8) + info.baseVertex;

    float3 p0 = asfloat(skinnedVB.Load3(i0 * 24));
    float3 p1 = asfloat(skinnedVB.Load3(i1 * 24));
    float3 p2 = asfloat(skinnedVB.Load3(i2 * 24));

    return normalize(cross(p1 - p0, p2 - p0));
}

float3 TransformNormalToWorld(float3 localNormal, float3x4 objectToWorld)
{
    float3x3 rot = float3x3(objectToWorld[0].xyz, objectToWorld[1].xyz, objectToWorld[2].xyz);
    return normalize(mul(rot, localNormal));
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
    return float(seed) / 4294967295.0;
}

float3 cosine_weighted_hemisphere(float2 u, float3 N)
{
    float r = sqrt(u.x);
    float phi = 6.28318530718 * u.y;
    float3 dir = float3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));

    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    return normalize(T * dir.x + B * dir.y + N * dir.z);
}

#endif
