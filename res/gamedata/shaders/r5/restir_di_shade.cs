#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/foliage_sss.h"
#include "shared/skin_sss.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDIShadeParams : register(b5) {
    float4 g_CameraPos;
    float2 g_ScreenSize;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    float4 g_ClusterParams;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_ParticleBatchStart;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<GPULightDataDI> g_LightData : register(t20);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
ByteAddressBuffer g_ParticleVB : register(t19);
ByteAddressBuffer g_ParticleIB : register(t4);
Texture2D<float4> t_DIReservoir : register(t0);
Texture2D<float> t_Depth : register(t4);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_Normal : register(t11);

RWTexture2D<float4> u_DirectLighting : register(u0);

float TraceShadowRayDI(float3 origin, float3 dir, float tMax, float skinnedSelfMax)
{
    return TraceVisibilityAtten(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB, g_ParticleVB, g_ParticleIB,
        origin, dir, tMax,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, true, skinnedSelfMax);
}

float TraceSoftShadowDI(float3 origin, float3 lightPos, float emitterRadius, bool hudLight, float skinnedSelfMax, inout uint rng)
{
    float3 toLight = lightPos - origin;
    float dist = length(toLight);
    if (dist < 1e-4)
        return 1.0;

    float3 L = toLight / dist;
    float soft = hudLight ? 0.0 : SoftShadowAmountDI(dist);
    float radius = emitterRadius * lerp(0.12, 1.0, soft);
    uint samples = 1;
    if (soft > 0.05)
        samples = 2;
    if (soft > 0.25)
        samples = 4;

    float3 up = (abs(L.y) < 0.99) ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, L));
    float3 B = cross(L, T);

    float endSkip = hudLight ? 0.05 : max(0.55, emitterRadius * 3.0 + 0.25);
    float vis = 0.0;
    for (uint i = 0; i < samples; i++) {
        float2 u = float2(rand_float(rng), rand_float(rng));
        float r = sqrt(u.x) * radius;
        float a = u.y * 6.2831853;
        float3 samplePos = lightPos + T * (cos(a) * r) + B * (sin(a) * r);
        float3 dir = samplePos - origin;
        float d = length(dir);
        float tMax = max(d - endSkip, d * 0.88);
        if (tMax < 0.02)
        {
            vis += 1.0;
            continue;
        }
        vis += TraceShadowRayDI(origin, dir / max(d, 1e-4), tMax, skinnedSelfMax);
    }
    return vis / (float)samples;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0)
        return;

    DIReservoir r = UnpackDIReservoir(t_DIReservoir.Load(int3(pixel, 0)));
    if (!IsDIReservoirValid(r) || r.lightIndex >= (uint)g_ClusterParams.w)
        return;

    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    float3 worldPos = worldPosData.xyz;
    const bool hudSurf = IsHudSurfMark(worldPosData.w);
    const float skinnedSelfMax = IsCharSurfMark(worldPosData.w) ? 0.65 : 0.0;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = max(baseColorData.rgb, 0.0);
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);
    float sssMask = UnpackSSSMaskFromBaseA(baseColorData.a);
    float3 V = normalize(g_CameraPos.xyz - worldPos);

    GPULightDataDI light = g_LightData[r.lightIndex];
    const bool hudLight = IsHudLightDI(light);
    if (hudSurf && !hudLight)
        return;

    float3 L, lightColor;
    float dist;
    float atten = EvalLocalLightAttenuationDI(light, worldPos, L, dist, lightColor);
    if (atten <= 0.001)
        return;

    float shadow = 1.0;
    if (!hudLight && asuint(light.spotParamsAndType.w) != 0xFFFFFFFFu) {
        float3 biased = worldPos + N * (skinnedSelfMax > 0.0 ? 0.06 : 0.05);
        uint rng = pcg_hash(pixel.x + pixel.y * 1973u + r.lightIndex * 26699u + asuint(dist * 100.0));
        shadow = TraceSoftShadowDI(biased, light.positionAndInvRangeSq.xyz, LightEmitterRadiusDISoft(light), false, skinnedSelfMax, rng);
        if (!hudSurf && shadow <= 0.001 && light.spotParamsAndType.y < 0.5)
            shadow = 0.35;
    }
    if (shadow <= 0.001)
        return;

    float3 shaded = PBRDirectLighting(albedo, N, V, L, lightColor * atten * shadow, metallic, roughness, 1);
    if (!hudSurf && sssMask > 0.01) {
        float3 sssTint = SkinSSSTint();
        float sssThickness = saturate(0.55 + sssMask * 0.35);
        shaded += EvaluateFoliageSSS(
            albedo, N, V, L, lightColor * atten, max(shadow, 0.35),
            sssTint, sssThickness, sssMask);
    }

    float3 direct = u_DirectLighting[pixel].rgb;
    direct += shaded * r.W;
    u_DirectLighting[pixel] = float4(direct, 1.0);
}
