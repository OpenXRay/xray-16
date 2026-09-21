#ifndef RESTIR_GI_COMMON_H
#define RESTIR_GI_COMMON_H

#include "rt_bsdf.h"

static const float RESTIR_MAX_RADIANCE = 100.0;
static const uint RESTIR_M_MAX = 20;

struct GIReservoir
{
    float3 samplePos;
    float3 sampleNormal;
    float3 Lo;
    float W;
    float w_sum;
    uint M;
    uint age;
};

GIReservoir EmptyReservoir()
{
    GIReservoir r;
    r.samplePos = 0;
    r.sampleNormal = 0;
    r.Lo = 0;
    r.W = 0;
    r.w_sum = 0;
    r.M = 0;
    r.age = 0;
    return r;
}

bool IsReservoirValid(GIReservoir r)
{
    return r.M > 0 && any(r.Lo > 0);
}

bool ReservoirUpdate(inout GIReservoir r, float weight, float3 pos, float3 normal, float3 lo, inout uint rng)
{
    if (isnan(weight) || isinf(weight))
        return false;

    r.w_sum += weight;
    r.M += 1;

    float xi = rand_float(rng);
    if (xi < weight / max(r.w_sum, 1e-6)) {
        r.samplePos = pos;
        r.sampleNormal = normal;
        r.Lo = lo;
        return true;
    }
    return false;
}

void ReservoirFinalize(inout GIReservoir r, float targetLuminance)
{
    r.W = (targetLuminance > 0.0 && r.M > 0) ? r.w_sum / (targetLuminance * r.M) : 0.0;
}

float2 OctEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (n.xy >= 0 ? 1.0 : -1.0);
        n.xy = wrap;
    }
    return n.xy * 0.5 + 0.5;
}

float3 OctDecode(float2 e)
{
    e = e * 2.0 - 1.0;
    float3 n = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (n.xy >= 0 ? 1.0 : -1.0);
        n.xy = wrap;
    }
    return normalize(n);
}

uint PackNormalMAge(float3 normal, uint M, uint age)
{
    float2 oct = OctEncode(normal);
    uint ox = (uint)(saturate(oct.x) * 255.0) & 0xFF;
    uint oy = (uint)(saturate(oct.y) * 255.0) & 0xFF;
    uint mp = min(M, 255) & 0xFF;
    uint ap = min(age, 255) & 0xFF;
    return (ox << 24) | (oy << 16) | (mp << 8) | ap;
}

void UnpackNormalMAge(uint packed, out float3 normal, out uint M, out uint age)
{
    float2 oct;
    oct.x = float((packed >> 24) & 0xFF) / 255.0;
    oct.y = float((packed >> 16) & 0xFF) / 255.0;
    normal = OctDecode(oct);
    M = (packed >> 8) & 0xFF;
    age = packed & 0xFF;
}

void PackReservoir(GIReservoir r, out float4 A, out float4 B)
{
    A = float4(r.samplePos, r.W);
    B = float4(r.Lo, asfloat(PackNormalMAge(r.sampleNormal, r.M, r.age)));
}

GIReservoir UnpackReservoir(float4 A, float4 B)
{
    GIReservoir r;
    r.samplePos = A.xyz;
    r.W = A.w;
    r.Lo = B.xyz;
    UnpackNormalMAge(asuint(B.w), r.sampleNormal, r.M, r.age);
    r.w_sum = 0;
    return r;
}

MaterialSurface GBufferMaterialSurface(float4 normalData, float4 baseColorData, float2 materialData)
{
    MaterialSurface surface;
    surface.albedo = baseColorData.rgb;
    surface.N = normalize(normalData.xyz);
    surface.roughness = abs(normalData.w);
    surface.metallic = baseColorData.a;
    surface.ao = 1.0;
    surface.emissive = 0.0;
    surface.shadingClass = GBufferShadingClass(materialData);
    surface.transmission = materialData.y;
    return surface;
}

float3 GITargetRadiance(MaterialSurface primary, float3 V, float3 worldPos, float3 samplePos, float3 Lo, uint diffuseMode)
{
    float3 toSample = samplePos - worldPos;
    float distSq = dot(toSample, toSample);
    if (distSq <= 1e-12 || !all(isfinite(toSample)) || !all(isfinite(Lo)))
        return 0.0;
    return RTEvaluateBSDF(primary, V, toSample * rsqrt(distSq), Lo, diffuseMode);
}

#endif
