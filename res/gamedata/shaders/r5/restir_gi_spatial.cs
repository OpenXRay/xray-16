#define SM_6_0
#include "common.h"
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
    uint g_HudSkinnedStart;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
ByteAddressBuffer g_ParticleVB : register(t20);
ByteAddressBuffer g_ParticleIB : register(t21);

StructuredBuffer<uint4> t_InReservoir : register(t0);
Texture2D<float> t_Depth : register(t4);
Texture2D<float4> t_Normal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture3D<float> t_BlueNoise : register(t8);
Texture2D<float> t_SkyOpen : register(t9);

RWStructuredBuffer<uint4> u_OutReservoir : register(u0);
RWTexture2D<float4> u_NoisyDiffuse : register(u1);

bool VisibilityOK(float3 worldPos, float3 N, float3 samplePos, uint2 pixel)
{
    float3 biasedPos = worldPos + N * 0.01;
    return TraceVisibilityClear(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        g_ParticleVB, g_ParticleIB,
        biasedPos, samplePos,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, g_HudSkinnedStart,
        t_BlueNoise, pixel, g_FrameIndex);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    uint width = (uint)g_ScreenSize.x;
    uint height = (uint)g_ScreenSize.y;
    if (pixel.x >= width || pixel.y >= height)
        return;

    float2 giSize = g_ScreenSize;
    uint fullW = 0, fullH = 0;
    t_Depth.GetDimensions(fullW, fullH);
    float2 fullSize = float2(max(fullW, 1u), max(fullH, 1u));

    uint pixelIdx = pixel.y * width + pixel.x;
    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0 || depth >= 1.0) {
        u_OutReservoir[pixelIdx] = 0;
        return;
    }

    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float surfMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    if (IsCharSurfMark(surfMark)) {
        u_OutReservoir[pixelIdx] = t_InReservoir[pixelIdx];
        return;
    }
    if (IsWaterSurfMark(surfMark)) {
        u_OutReservoir[pixelIdx] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    float3 N = normalize(RestirLoadTex4(t_Normal, pixel, giSize, fullSize).xyz);
    float3 albedo = baseColorData.rgb;
    float sssMaskUnused = 0.0;
    float metallic = UnpackGBufferMetallic(baseColorData.a, IsHudSurfMark(surfMark) || IsCharSurfMark(surfMark), sssMaskUnused);
    float linearDepth = length(worldPos - g_CameraPos.xyz);
    const bool hudSurf = IsHudSurfMark(surfMark);
    const bool vegSurf = IsVegSurfMark(surfMark);
    float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));

    GIReservoir center = UnpackReservoirU4(t_InReservoir[pixelIdx], worldPos);
    GIReservoir output = EmptyReservoir();
    uint wx = asuint(worldPos.x * 8.0);
    uint wy = asuint(worldPos.y * 8.0);
    uint wz = asuint(worldPos.z * 8.0);
    uint rng = pcg_hash(wx + wy * 3343u + wz * 9157u + pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);

    if (IsReservoirValid(center)) {
        float3 wi = normalize(center.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        float targetLum = Luminance(center.Lo * kD * albedo / PI * cosTheta);
        if (targetLum > 0) {
            output = center;
            output.w_sum = targetLum * center.W;
            output.M = max(center.M, 1u);
        }
    }

    float lod = saturate(linearDepth / max(g_LodDist, 1.0));
    uint samples = min(g_SpatialSamples, 16u);
    if (vegSurf)
        samples = 0;
    if (lod > 0.9)
        samples = min(samples, max(samples / 2u, 2u));
    for (uint i = 0; i < samples; i++) {
        float ang = rand_float(rng) * 6.2831853;
        float rad = sqrt(rand_float(rng)) * g_SpatialRadius;
        int2 np = int2(pixel) + int2(int(cos(ang) * rad), int(sin(ang) * rad));
        if (np.x < 0 || np.y < 0 || np.x >= (int)width || np.y >= (int)height)
            continue;

        float nDepth = RestirLoadDepth(t_Depth, uint2(np), giSize, fullSize);
        if (nDepth <= 0.0 || nDepth >= 1.0)
            continue;
        float4 nWorldPosData = RestirLoadTex4(t_WorldPos, uint2(np), giSize, fullSize);
        float4 nBase = RestirLoadTex4(t_BaseColor, uint2(np), giSize, fullSize);
        float nMark = SurfMarkFromGBuffer(nWorldPosData.w, nBase.a);
        if (IsHudSurfMark(nMark) != hudSurf)
            continue;
        float skyOpenN = saturate(RestirLoadTex1(t_SkyOpen, uint2(np), giSize, fullSize));
        if (!SameLightZone(surfMark, nMark))
            continue;
        if (abs(skyOpenC - skyOpenN) > 0.25)
            continue;

        float3 nN = normalize(RestirLoadTex4(t_Normal, uint2(np), giSize, fullSize).xyz);
        float2 nUV = (float2(np) + 0.5) * g_InvScreenSize;
        float3 nWorld = ReconstructWorldPosReverseZ(nUV, nDepth, g_InvViewProj);
        float nLinear = length(nWorld - g_CameraPos.xyz);
        if (!ValidateTemporalNeighbor(linearDepth, N, nLinear, nN))
            continue;

        uint nIdx = (uint)np.y * width + (uint)np.x;
        GIReservoir neighbor = UnpackReservoirU4(t_InReservoir[nIdx], nWorld);
        if (!IsReservoirValid(neighbor))
            continue;

        float3 wi = normalize(neighbor.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        if (cosTheta <= 0)
            continue;

        float jac = JacobianReconnectionShift(neighbor.sampleNormal, worldPos, nWorld, neighbor.samplePos);
        jac = clamp(jac, 0.25, 4.0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        float3 target = min(neighbor.Lo, RESTIR_MAX_RADIANCE) * kD * albedo / PI * cosTheta;
        float targetLum = Luminance(target);
        if (targetLum <= 0)
            continue;

        float conf = AgeConfidence(neighbor.age, neighbor.M);
        uint clampedM = TemporalMClamp(neighbor.M, neighbor.age, g_MMax);
        float w = targetLum * neighbor.W * jac * (float)clampedM * conf;
        ReservoirUpdate(output, w, neighbor.samplePos, neighbor.sampleNormal, neighbor.Lo, neighbor.lightId, rng);
        output.M += clampedM > 0 ? (clampedM - 1) : 0;
    }

    float outTargetLum = 0;
    if (IsReservoirValid(output)) {
        float3 wi = normalize(output.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        outTargetLum = Luminance(min(output.Lo, RESTIR_MAX_RADIANCE) * kD * albedo / PI * cosTheta);
    }

    output.Lo = min(output.Lo, RESTIR_MAX_RADIANCE);
    if (IsReservoirValid(center) && output.w_sum > 8.0 * max(center.w_sum, 1e-6))
    {
        output = center;
        outTargetLum = 0;
        if (IsReservoirValid(output)) {
            float3 wiB = normalize(output.samplePos - worldPos);
            float cosB = max(dot(N, wiB), 0);
            float3 F0b = CalculateF0(albedo, metallic);
            float3 kDb = (1.0 - F_Schlick(cosB, F0b)) * (1.0 - metallic);
            outTargetLum = Luminance(min(output.Lo, RESTIR_MAX_RADIANCE) * kDb * albedo / PI * cosB);
        }
    }
    output.W = (outTargetLum > 0 && output.M > 0) ? min(output.w_sum / (outTargetLum * output.M), 4.0) : 0;
    output.age = min(output.age + 1, 127);
    if (IsReservoirValid(output) && output.W > 0) {
        float sampleDist = length(output.samplePos - worldPos);
        if (sampleDist < 80.0 && !VisibilityOK(worldPos, N, output.samplePos, pixel)) {
            if (IsReservoirValid(center)) {
                output = center;
                outTargetLum = 0;
                if (IsReservoirValid(output)) {
                    float3 wiC = normalize(output.samplePos - worldPos);
                    float cosC = max(dot(N, wiC), 0);
                    float3 F0c = CalculateF0(albedo, metallic);
                    float3 kDc = (1.0 - F_Schlick(cosC, F0c)) * (1.0 - metallic);
                    outTargetLum = Luminance(min(output.Lo, RESTIR_MAX_RADIANCE) * kDc * albedo / PI * cosC);
                }
                output.W = (outTargetLum > 0 && output.M > 0) ? min(output.w_sum / (outTargetLum * output.M), 4.0) : min(center.W, 4.0);
            }
        }
    }
    u_OutReservoir[pixelIdx] = PackReservoirU4(output, worldPos);

    float3 gi = 0;
    if (IsReservoirValid(output) && output.W > 0 && outTargetLum > 0) {
        float3 wiS = normalize(output.samplePos - worldPos);
        float cosS = max(dot(N, wiS), 0);
        float3 F0s = CalculateF0(albedo, metallic);
        float3 kDs = (1.0 - F_Schlick(cosS, F0s)) * (1.0 - metallic);
        gi = min(output.Lo, RESTIR_MAX_RADIANCE) * min(output.W, 4.0) * kDs * albedo / PI * cosS;
        gi = min(gi, RESTIR_MAX_RADIANCE);
    }
    u_NoisyDiffuse[pixel] = float4(gi, 1.0);
}
