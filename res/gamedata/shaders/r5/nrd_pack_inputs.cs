#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/nrd_helpers.h"

cbuffer NrdPackParams : register(b5) {
    float4x4 g_WorldToView;
    float4x4 g_WorldToViewPrev;
    float4x4 g_WorldToClip;
    float4x4 g_WorldToClipPrev;
    float4x4 g_InvViewProj;
    float4x4 g_InvViewProjPrev;
    float4 g_ScreenNearFar;
    float4 g_HitDistMethod;
    float4 g_CameraPos_Range;
};

Texture2D<float4> t_NoisyDiffuse : register(t0);
Texture2D<float4> t_NoisySpecular : register(t1);
Texture2D<float> t_HitDist : register(t2);
Texture2D<float4> t_Normal : register(t3);
Texture2D<float> t_Depth : register(t4);
Texture2D<float4> t_WorldPos : register(t5);
Texture2D<float4> t_BaseColor : register(t6);

RWTexture2D<float4> u_DiffRadianceHitDist : register(u0);
RWTexture2D<float4> u_SpecRadianceHitDist : register(u1);
RWTexture2D<float4> u_NormalRoughness : register(u2);
RWTexture2D<float> u_ViewZ : register(u3);
RWTexture2D<float4> u_MotionVectors : register(u4);

float4 PackNormalRoughness(float3 N, float roughness)
{
    N /= max(abs(N.x), max(abs(N.y), abs(N.z)));
    return float4(N * 0.5 + 0.5, saturate(roughness));
}

float3 ReconstructWorldPosReverseZ(float2 uv, float depth, float4x4 invViewProj)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = mul(invViewProj, clip);
    return world.xyz / max(world.w, 1e-8);
}

float2 ProjectToUv(float4x4 worldToClip, float3 worldPos)
{
    float4 clip = mul(worldToClip, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(clip.w, 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    float2 screenSize = g_ScreenNearFar.xy;
    if (pixel.x >= (uint)screenSize.x || pixel.y >= (uint)screenSize.y)
        return;

    float nearZ = g_ScreenNearFar.z;
    float denoisingRange = g_CameraPos_Range.w;
    float3 hitDistParams = g_HitDistMethod.xyz;
    uint method = (uint)g_HitDistMethod.w;
    float3 cameraPos = g_CameraPos_Range.xyz;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0) {
        u_DiffRadianceHitDist[pixel] = 0;
        u_SpecRadianceHitDist[pixel] = 0;
        u_NormalRoughness[pixel] = float4(0.5, 0.5, 1.0, 1.0);
        u_ViewZ[pixel] = denoisingRange * 2.0;
        u_MotionVectors[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / screenSize;
    float3 worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);
    float viewZ = mul(g_WorldToView, float4(worldPos, 1.0)).z;
    viewZ = max(abs(viewZ), nearZ);

    float viewZPrev = mul(g_WorldToViewPrev, float4(worldPos, 1.0)).z;
    viewZPrev = max(abs(viewZPrev), nearZ);

    float2 uvPrev = ProjectToUv(g_WorldToClipPrev, worldPos);
    float3 motion;
    motion.xy = (uvPrev - uv) * screenSize;
    motion.z = viewZPrev - viewZ;
    u_MotionVectors[pixel] = float4(motion, 0.0);

    float4 nData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(nData.xyz);
    float roughness = max(saturate(abs(nData.w)), MIN_ROUGHNESS);
    float4 baseColor = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = max(baseColor.rgb, 0.0);
    float metallic = UnpackMetallicFromBaseA(baseColor.a);
    float3 V = normalize(cameraPos - worldPos);
    float3 F0 = CalculateF0(albedo, metallic);

    float3 diffFactor, specFactor;
    NRD_MaterialFactors(N, V, albedo, F0, roughness, diffFactor, specFactor);

    float diffHit = NRD_TrimHitDistance(max(t_HitDist.Load(int3(pixel, 0)), 0.0), 1e-3);
    float4 specData = t_NoisySpecular.Load(int3(pixel, 0));
    float specHit = NRD_TrimHitDistance(max(specData.a, 0.0), 1e-3);

    float3 diffIrradiance = max(t_NoisyDiffuse.Load(int3(pixel, 0)).rgb, 0.0);
    float3 specIrradiance = max(specData.rgb, 0.0);

    float3 diff = NRD_SanitizeRadiance(diffIrradiance / max(diffFactor, 1e-3));
    float3 spec = NRD_SanitizeRadiance(specIrradiance / max(specFactor, 1e-3));

    if (metallic >= 0.999) {
        diff = 0;
        diffHit = 0;
    }

    float4 packedDiff;
    float4 packedSpec;
    if (method == 0) {
        packedDiff = float4(NRD_LinearToYCoCg(diff), REBLUR_GetNormHitDist(diffHit, viewZ, hitDistParams, 1.0));
        packedSpec = float4(NRD_LinearToYCoCg(spec), REBLUR_GetNormHitDist(specHit, viewZ, hitDistParams, roughness));
    } else {
        packedDiff = float4(diff, diffHit);
        packedSpec = float4(spec, specHit);
    }

    u_DiffRadianceHitDist[pixel] = packedDiff;
    u_SpecRadianceHitDist[pixel] = packedSpec;
    u_NormalRoughness[pixel] = PackNormalRoughness(N, roughness);
    u_ViewZ[pixel] = viewZ;
}
