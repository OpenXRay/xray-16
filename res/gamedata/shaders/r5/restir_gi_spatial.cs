#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer ReSTIRSpatialParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_SpatialSamples;
    float g_SpatialRadius;
    uint g_MMax;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_ParticleBatchStart;
    float g_LodDist;
    float g_CameraMotion;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
ByteAddressBuffer g_ParticleVB : register(t4);
ByteAddressBuffer g_ParticleIB : register(t8);

Texture2D<float> t_Depth : register(t0);
Texture2D<float4> t_Normal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_ReservoirA : register(t14);
Texture2D<float4> t_ReservoirB : register(t15);
Texture2D<float2> t_ReservoirC : register(t16);
Texture2D<float4> t_SpecA : register(t17);
Texture2D<float4> t_SpecB : register(t19);

RWTexture2D<float4> u_ReservoirA : register(u0);
RWTexture2D<float4> u_ReservoirB : register(u1);
RWTexture2D<float2> u_ReservoirC : register(u2);
RWTexture2D<float4> u_SpecReservoirA : register(u3);
RWTexture2D<float4> u_SpecReservoirB : register(u4);

bool VisibilityOK(float3 worldPos, float3 N, float3 samplePos)
{
    float3 biasedPos = worldPos + N * 0.01;
    return TraceVisibilityClear(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB, g_ParticleVB, g_ParticleIB,
        biasedPos, samplePos,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex);
}

float TargetLuminance(GIReservoir r, float3 worldPos, float3 N, float3 albedo, float metallic)
{
    if (!IsReservoirValid(r))
        return 0;
    float3 wi = normalize(r.samplePos - worldPos);
    float cosTheta = max(dot(N, wi), 0);
    float3 F0 = CalculateF0(albedo, metallic);
    float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
    float3 target = r.Lo * kD * albedo / PI * cosTheta;
    return Luminance(target);
}

GIReservoir SpatialReuse(
    GIReservoir center,
    Texture2D<float4> srcA,
    Texture2D<float4> srcB,
    Texture2D<float2> srcC,
    bool hasC,
    uint2 pixel,
    float3 worldPos,
    float centerMark,
    float3 N,
    float3 albedo,
    float metallic,
    float linearDepth,
    float radiusScale,
    inout uint rng)
{
    float centerLum = TargetLuminance(center, worldPos, N, albedo, metallic);
    GIReservoir output = EmptyReservoir();

    if (centerLum > 0) {
        output.samplePos = center.samplePos;
        output.sampleNormal = center.sampleNormal;
        output.Lo = center.Lo;
        output.w_sum = centerLum * center.W;
        output.M = max(center.M, 1u);
        output.age = center.age;
    }

    float farLod = saturate(linearDepth / max(g_LodDist, 1.0));
    uint samples = min(g_SpatialSamples, 16u);
    if (farLod > 0.7)
        samples = min(samples, 2u);
    for (uint i = 0; i < samples; ++i) {
        float ang = rand_float(rng) * 6.2831853;
        float rad = sqrt(rand_float(rng)) * g_SpatialRadius * radiusScale;
        int2 offset = int2(cos(ang) * rad, sin(ang) * rad);
        int2 nPixel = int2(pixel) + offset;
        if (nPixel.x < 0 || nPixel.y < 0 ||
            nPixel.x >= (int)g_ScreenSize.x || nPixel.y >= (int)g_ScreenSize.y)
            continue;

        float nDepth = t_Depth.Load(int3(nPixel, 0));
        if (nDepth <= 0.0)
            continue;

        float4 nWorldPosData = t_WorldPos.Load(int3(nPixel, 0));
        if (!SameHudSurfClass(centerMark, nWorldPosData.w))
            continue;
        float3 nWorldPos = nWorldPosData.xyz;
        float3 nN = normalize(t_Normal.Load(int3(nPixel, 0)).xyz);
        float nLinearDepth = length(nWorldPos - g_CameraPos.xyz);
        if (!ValidateTemporalNeighbor(linearDepth, N, nLinearDepth, nN))
            continue;

        GIReservoir neighbor;
        if (hasC)
            neighbor = UnpackReservoir(srcA.Load(int3(nPixel, 0)), srcB.Load(int3(nPixel, 0)), srcC.Load(int3(nPixel, 0)));
        else
            neighbor = UnpackReservoir(srcA.Load(int3(nPixel, 0)), srcB.Load(int3(nPixel, 0)));
        if (!IsReservoirValid(neighbor))
            continue;

        float jac = JacobianReconnectionShift(neighbor.sampleNormal, worldPos, nWorldPos, neighbor.samplePos);
        if (jac < 1e-4 || jac > 20.0)
            continue;

        float neighLum = TargetLuminance(neighbor, worldPos, N, albedo, metallic);
        if (neighLum <= 0)
            continue;
        if (centerLum > 1e-5 && neighLum > centerLum * 8.0)
            continue;

        float conf = AgeConfidence(neighbor.age, 32u);
        uint clampedM = TemporalMClamp(neighbor.M, neighbor.age, g_MMax);
        float w = neighLum * neighbor.W * jac * clampedM * conf;
        ReservoirUpdate(output, w, neighbor.samplePos, neighbor.sampleNormal, neighbor.Lo, rng);
        output.M += clampedM - 1;
    }

    float outLum = TargetLuminance(output, worldPos, N, albedo, metallic);
    output.W = (outLum > 0 && output.M > 0) ? output.w_sum / (outLum * output.M) : 0;
    output.age = min(output.age + 1, 255);

    if (IsReservoirValid(output) && output.W > 0) {
        if (!VisibilityOK(worldPos, N, output.samplePos)) {
            output.W = 0;
            output.w_sum = 0;
        }
    }
    return output;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0) {
        u_ReservoirA[pixel] = 0;
        u_ReservoirB[pixel] = 0;
        u_ReservoirC[pixel] = 0;
        u_SpecReservoirA[pixel] = 0;
        u_SpecReservoirB[pixel] = 0;
        return;
    }

    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    float3 worldPos = worldPosData.xyz;
    const float centerMark = worldPosData.w;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = baseColorData.rgb;
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);
    float linearDepth = length(worldPos - g_CameraPos.xyz);
    uint rng = pcg_hash(pixel.x + pixel.y * 6287u + g_FrameIndex * 33461u);
    float radiusScale = lerp(1.0, 0.7, saturate(g_CameraMotion));

    GIReservoir center = UnpackReservoir(
        t_ReservoirA.Load(int3(pixel, 0)),
        t_ReservoirB.Load(int3(pixel, 0)),
        t_ReservoirC.Load(int3(pixel, 0)));
    GIReservoir centerSpec = UnpackReservoir(
        t_SpecA.Load(int3(pixel, 0)),
        t_SpecB.Load(int3(pixel, 0)));

    GIReservoir output = SpatialReuse(center, t_ReservoirA, t_ReservoirB, t_ReservoirC, true,
        pixel, worldPos, centerMark, N, albedo, metallic, linearDepth, radiusScale, rng);
    float3 specAlbedo = lerp(float3(1, 1, 1), albedo, metallic);
    float specRadius = roughness < 0.2 ? 0.35 : (roughness < 0.45 ? 0.6 : 0.15);
    GIReservoir outputSpec = centerSpec;
    if (roughness < 0.55) {
        outputSpec = SpatialReuse(centerSpec, t_SpecA, t_SpecB, t_ReservoirC, false,
            pixel, worldPos, centerMark, N, specAlbedo, metallic, linearDepth, specRadius * radiusScale, rng);
    }

    float4 outA, outB;
    PackReservoir(output, outA, outB);
    u_ReservoirA[pixel] = outA;
    u_ReservoirB[pixel] = outB;
    u_ReservoirC[pixel] = PackReservoirC(output);

    float4 sA, sB;
    PackReservoir(outputSpec, sA, sB);
    u_SpecReservoirA[pixel] = sA;
    u_SpecReservoirB[pixel] = sB;
}
