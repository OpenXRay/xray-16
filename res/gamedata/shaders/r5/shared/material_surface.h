#ifndef MATERIAL_SURFACE_H
#define MATERIAL_SURFACE_H

struct MaterialSurface
{
    float3 albedo;
    float3 N;
    float roughness;
    float metallic;
    float ao;
    float3 emissive;
    uint shadingClass;
    float transmission;
};

#endif
