#ifndef SHARED_SHADING_CLASS_H
#define SHARED_SHADING_CLASS_H

#define SHADING_CLASS_STANDARD 0u
#define SHADING_CLASS_FOLIAGE  1u

#define GBUFFER_GEOMETRIC_NORMAL_FLAG 128u
#define GBUFFER_BAKED_SKY_FLAG 64u
#define GBUFFER_SHADING_CLASS_MASK 63u

float2 GBufferOctSignNotZero(float2 v)
{
    return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 GBufferOctEncode(float3 n)
{
    float2 p = n.xy / (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0.0)
        p = (1.0 - abs(p.yx)) * GBufferOctSignNotZero(p);
    return p * 0.5 + 0.5;
}

float3 GBufferOctDecode(float2 e)
{
    e = e * 2.0 - 1.0;
    float3 v = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * GBufferOctSignNotZero(v.xy);
    return normalize(v);
}

float4 PackGBufferMaterial(uint shadingClass, float transmission, float3 geometricNormal)
{
    return float4(float(shadingClass | GBUFFER_GEOMETRIC_NORMAL_FLAG) * (1.0 / 255.0), saturate(transmission),
        GBufferOctEncode(geometricNormal));
}

float4 PackGBufferMaterialBakedSky(uint shadingClass, float transmission, float skyVisibility)
{
    return float4(float(shadingClass | GBUFFER_BAKED_SKY_FLAG) * (1.0 / 255.0), saturate(transmission),
        1.0 - saturate(skyVisibility), 0.0);
}

uint GBufferShadingClass(float2 material)
{
    return uint(material.x * 255.0 + 0.5) & GBUFFER_SHADING_CLASS_MASK;
}

uint GBufferShadingClass(float4 material)
{
    return GBufferShadingClass(material.xy);
}

float3 GBufferGeometricNormal(float4 material, float3 fallback)
{
    if ((uint(material.x * 255.0 + 0.5) & GBUFFER_GEOMETRIC_NORMAL_FLAG) == 0u)
        return fallback;
    float3 n = GBufferOctDecode(material.zw);
    return dot(n, fallback) < 0.0 ? fallback : n;
}

float GBufferSkyVisibility(float4 material)
{
    if ((uint(material.x * 255.0 + 0.5) & GBUFFER_BAKED_SKY_FLAG) == 0u)
        return -1.0;
    return 1.0 - material.z;
}

float3 FaceToward(float3 n, float3 dir)
{
    return (dot(n, dir) < 0.0) ? -n : n;
}

float3 FoliageViewerNormal(float3 N, float3 e1, float3 e2, float3 toEye)
{
    return FaceToward(N, FaceToward(cross(e1, e2), toEye));
}

#endif
