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

#endif
