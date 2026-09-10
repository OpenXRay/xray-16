#ifndef SHARED_SHADING_CLASS_H
#define SHARED_SHADING_CLASS_H

#define SHADING_CLASS_STANDARD 0u
#define SHADING_CLASS_FOLIAGE  1u

float2 PackGBufferMaterial(uint shadingClass, float transmission)
{
    return float2(float(shadingClass) * (1.0 / 255.0), saturate(transmission));
}

uint GBufferShadingClass(float2 material)
{
    return uint(material.x * 255.0 + 0.5);
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
