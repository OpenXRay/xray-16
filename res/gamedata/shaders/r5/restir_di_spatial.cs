#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDISpatialParams : register(b5) {
    float4x4 g_WorldToView;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_SpatialSamples;
    float g_SpatialRadius;
    uint g_MMax;
    float4 g_ClusterParams;
    float4 g_ClusterScales;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_ParticleBatchStart;
    uint g_Pad1;
    uint g_Pad2;
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
Texture2D<float4> t_SrcDI : register(t0);
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_Normal : register(t11);

RWTexture2D<float4> u_DIReservoir : register(u0);

float TraceShadowDI(float3 origin, float3 dir, float tMax)
{
    return TraceVisibilityAtten(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB, g_ParticleVB, g_ParticleIB,
        origin, dir, tMax,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, true, 0.0);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0 || g_SpatialSamples == 0) {
        u_DIReservoir[pixel] = t_SrcDI.Load(int3(pixel, 0));
        return;
    }

    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    float3 worldPos = worldPosData.xyz;
    const float centerMark = worldPosData.w;
    const bool hudSurf = IsHudSurfMark(centerMark);
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = max(baseColorData.rgb, 0.0);
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float linearDepth = abs(mul(g_WorldToView, float4(worldPos, 1.0)).z);

    DIReservoir center = UnpackDIReservoir(t_SrcDI.Load(int3(pixel, 0)));
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);
    DIReservoir output = EmptyDIReservoir();

    float targetCenter = 0;
    if (IsDIReservoirValid(center) && center.lightIndex < (uint)g_ClusterParams.w) {
        if (!(hudSurf && !IsHudLightDI(g_LightData[center.lightIndex]))) {
            targetCenter = EvalLocalLightTargetPdfDI(
                g_LightData[center.lightIndex], worldPos, N, V, albedo, metallic, roughness);
            if (targetCenter > 0) {
                output.lightIndex = center.lightIndex;
                output.targetPdf = targetCenter;
                output.w_sum = targetCenter * center.W;
                output.M = 1;
                output.age = center.age;
            }
        }
    }

    for (uint i = 0; i < g_SpatialSamples; i++) {
        float2 disc = float2(rand_float(rng), rand_float(rng));
        float ang = disc.x * 6.2831853;
        float rad = sqrt(disc.y) * g_SpatialRadius;
        int2 nPixel = int2(float2(pixel) + float2(cos(ang), sin(ang)) * rad);
        if (nPixel.x < 0 || nPixel.y < 0 ||
            nPixel.x >= (int)g_ScreenSize.x || nPixel.y >= (int)g_ScreenSize.y)
            continue;

        float4 nWorldData = t_WorldPos.Load(int3(nPixel, 0));
        if (!SameHudSurfClass(centerMark, nWorldData.w))
            continue;
        float3 nWorld = nWorldData.xyz;
        float3 nN = normalize(t_Normal.Load(int3(nPixel, 0)).xyz);
        float nDepth = abs(mul(g_WorldToView, float4(nWorld, 1.0)).z);
        if (abs(nDepth - linearDepth) / max(linearDepth, 1e-3) > 0.1)
            continue;
        if (dot(N, nN) < 0.906)
            continue;

        DIReservoir neighbor = UnpackDIReservoir(t_SrcDI.Load(int3(nPixel, 0)));
        if (!IsDIReservoirValid(neighbor) || neighbor.lightIndex >= (uint)g_ClusterParams.w)
            continue;
        if (hudSurf && !IsHudLightDI(g_LightData[neighbor.lightIndex]))
            continue;

        float targetN = EvalLocalLightTargetPdfDI(
            g_LightData[neighbor.lightIndex], worldPos, N, V, albedo, metallic, roughness);
        if (targetN <= 0)
            continue;

        uint clampedM = min(neighbor.M, g_MMax);
        float w = targetN * neighbor.W * clampedM;
        DIReservoirUpdate(output, w, neighbor.lightIndex, targetN, rng);
        output.M += clampedM - 1;
    }

    float outPdf = output.targetPdf;
    if (outPdf <= 0 && IsDIReservoirValid(output) && output.lightIndex < (uint)g_ClusterParams.w) {
        outPdf = EvalLocalLightTargetPdfDI(
            g_LightData[output.lightIndex], worldPos, N, V, albedo, metallic, roughness);
        output.targetPdf = outPdf;
    }

    if (outPdf > 0 && output.M > 0)
        output.W = ClampDIReservoirW(output.w_sum / max(outPdf * (float)output.M, 1e-6));
    else
        output = EmptyDIReservoir();

    if (IsDIReservoirValid(output) && output.lightIndex < (uint)g_ClusterParams.w) {
        GPULightDataDI light = g_LightData[output.lightIndex];
        float3 L, lightColor;
        float dist;
        float atten = EvalLocalLightAttenuationDI(light, worldPos, L, dist, lightColor);
        if (atten <= 0.001) {
            output.W = 0;
        } else if (!IsHudLightDI(light) && asuint(light.spotParamsAndType.w) != 0xFFFFFFFFu) {
            float3 biased = worldPos + N * 0.02;
            float shadowDist = max(dist - 0.35, dist * 0.96);
            if (TraceShadowDI(biased, L, shadowDist) <= 0.001)
                output.W = 0;
        }
    }

    if (output.W <= 0)
        output = EmptyDIReservoir();

    output.M = min(output.M, g_MMax);
    u_DIReservoir[pixel] = PackDIReservoir(output);
}
