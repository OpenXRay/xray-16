#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/nrd_helpers.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer CompositeParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_Pad;
    float4 g_FogParams;
    float4 g_FogColor;
};

Texture2D<float4> t_DirectLighting : register(t0);
Texture2D<float4> t_ReservoirA : register(t1);
Texture2D<float4> t_ReservoirB : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_BaseColor : register(t5);
Texture2D<float4> t_SceneColorIn : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_Normal : register(t8);
Texture2D<float4> t_NoisySpecular : register(t9);
Texture2D<float4> t_ClassifyWorldPos : register(t10);
Texture2D<float4> t_SpecReservoirA : register(t11);
Texture2D<float4> t_SpecReservoirB : register(t12);

RWTexture2D<float4> u_SceneColor : register(u0);
RWTexture2D<float4> u_NoisyDiffuse : register(u1);
RWTexture2D<float> u_HitDist : register(u2);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0) {
        u_SceneColor[pixel] = t_SceneColorIn.Load(int3(pixel, 0));
        u_NoisyDiffuse[pixel] = 0;
        u_HitDist[pixel] = 0;
        return;
    }

    float guideMark = t_WorldPos.Load(int3(pixel, 0)).w;
    float classifyMark = t_ClassifyWorldPos.Load(int3(pixel, 0)).w;
    if (IsWaterSurfMark(classifyMark) && !IsHudSurfMark(guideMark)) {
        u_SceneColor[pixel] = t_SceneColorIn.Load(int3(pixel, 0));
        u_NoisyDiffuse[pixel] = 0;
        u_HitDist[pixel] = 0;
        return;
    }

    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;
    float4 specSample = t_NoisySpecular.Load(int3(pixel, 0));

    GIReservoir r = UnpackReservoir(
        t_ReservoirA.Load(int3(pixel, 0)),
        t_ReservoirB.Load(int3(pixel, 0))
    );
    GIReservoir specR = UnpackReservoir(
        t_SpecReservoirA.Load(int3(pixel, 0)),
        t_SpecReservoirB.Load(int3(pixel, 0))
    );

    float3 worldPos = t_WorldPos.Load(int3(pixel, 0)).xyz;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float3 albedo = max(baseColorData.rgb, 0.0);
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);
    float3 F0 = CalculateF0(albedo, metallic);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 Fenv = NRD_EnvironmentTerm_Rtg(F0, abs(dot(N, V)), roughness);

    float3 diffIrradiance = 0;
    float hitDist = 0;
    if (IsReservoirValid(r) && r.W > 0 && metallic < 0.999) {
        float3 wi = normalize(r.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        float3 brdfCos = kD * albedo / PI * cosTheta;

        diffIrradiance = r.Lo * brdfCos * r.W;
        diffIrradiance = min(diffIrradiance, RESTIR_MAX_RADIANCE);
        hitDist = length(r.samplePos - worldPos);
    }
    if (hitDist < 1e-3)
        hitDist = 0;
    diffIrradiance *= g_GIIntensity;

    float3 specIrradiance = max(specSample.rgb, 0.0) * g_GIIntensity;
    if (IsReservoirValid(specR) && specR.W > 0) {
        float3 fromRes = min(specR.Lo * Fenv * specR.W, RESTIR_MAX_RADIANCE) * g_GIIntensity;
        specIrradiance = lerp(specIrradiance, fromRes, 0.85);
        if (hitDist < 1e-3)
            hitDist = length(specR.samplePos - worldPos);
    }
    if (specSample.a > 1e-3 && hitDist < 1e-3)
        hitDist = specSample.a;

    diffIrradiance = min(diffIrradiance, RESTIR_MAX_RADIANCE);
    specIrradiance = min(specIrradiance, RESTIR_MAX_RADIANCE);

    float giAo = 1.0;
    if (hitDist > 0.35 && hitDist < 3.0)
        giAo = saturate(0.72 + hitDist * 0.09);

    u_NoisyDiffuse[pixel] = float4(diffIrradiance * giAo, 1.0);
    u_HitDist[pixel] = hitDist;

    float3 ambientBase = t_SceneColorIn.Load(int3(pixel, 0)).rgb;
    float3 lit = direct + (diffIrradiance + specIrradiance) * giAo;
    float dist = length(worldPos - g_CameraPos.xyz);
    float fog = saturate(dist * g_FogParams.w + g_FogParams.x);
    fog = saturate(fog * lerp(1.0, 1.12, saturate(g_GIIntensity)));
    lit *= (1.0 - fog);
    u_SceneColor[pixel] = float4(ambientBase + lit, 1.0);
}
