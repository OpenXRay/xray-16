#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer ReSTIRTemporalParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint2 g_Pad;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);

Texture2D<float4> t_PrevReservoirA : register(t0);
Texture2D<float4> t_PrevReservoirB : register(t4);
Texture2D<float2> t_MotionVectors : register(t5);
Texture2D<float> t_Depth : register(t6);
Texture2D<float4> t_PrevNormal : register(t7);
Texture2D<float4> t_BaseColor : register(t14);
Texture2D<float4> t_WorldPos : register(t15);
Texture2D<float4> t_PrevWorldPos : register(t16);
Texture2D<float4> t_Normal : register(t17);
Texture2D<float2> t_PrevReservoirC : register(t19);
Texture2D<float4> t_PrevSpecA : register(t20);
Texture2D<float4> t_PrevSpecB : register(t21);

RWTexture2D<float4> u_ReservoirA : register(u0);
RWTexture2D<float4> u_ReservoirB : register(u1);
RWTexture2D<float2> u_ReservoirC : register(u2);
RWTexture2D<float4> u_SpecReservoirA : register(u3);
RWTexture2D<float4> u_SpecReservoirB : register(u4);

bool VisibilityOK(float3 worldPos, float3 N, float3 samplePos)
{
    float3 biasedPos = worldPos + N * 0.01;
    return TraceVisibilityClear(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        biasedPos, samplePos,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_DetailAtlasIndex);
}

void TemporalMerge(
    inout GIReservoir output,
    GIReservoir currRes,
    GIReservoir prevRes,
    float3 worldPos,
    float3 prevWorldPos,
    float3 N,
    float3 albedo,
    float metallic,
    bool requireVis,
    inout uint rng)
{
    float3 target_curr = 0;
    if (IsReservoirValid(currRes)) {
        float3 wi = normalize(currRes.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        target_curr = currRes.Lo * kD * albedo / PI * cosTheta;
    }
    float targetLum_curr = Luminance(target_curr);

    if (targetLum_curr > 0) {
        output.samplePos = currRes.samplePos;
        output.sampleNormal = currRes.sampleNormal;
        output.Lo = currRes.Lo;
        output.w_sum = targetLum_curr * currRes.W;
        output.M = 1;
        output.age = currRes.age;
    }

    if (IsReservoirValid(prevRes)) {
        if (requireVis && !VisibilityOK(worldPos, N, prevRes.samplePos))
            prevRes = EmptyReservoir();
        if (IsReservoirValid(prevRes)) {
            float jac = JacobianReconnectionShift(prevRes.sampleNormal, worldPos, prevWorldPos, prevRes.samplePos);
            if (jac >= 1e-4 && jac <= 20.0) {
                float3 wi_prev = normalize(prevRes.samplePos - worldPos);
                float cosTheta_prev = max(dot(N, wi_prev), 0);
                float3 F0 = CalculateF0(albedo, metallic);
                float3 kD = (1.0 - F_Schlick(cosTheta_prev, F0)) * (1.0 - metallic);
                float3 target_prev = prevRes.Lo * kD * albedo / PI * cosTheta_prev;
                float targetLum_prev = Luminance(target_prev);
                if (targetLum_prev > 0) {
                    float conf = AgeConfidence(prevRes.age, 32u);
                    uint clampedM = TemporalMClamp(prevRes.M, prevRes.age, RESTIR_M_MAX);
                    float w_prev = targetLum_prev * prevRes.W * jac * clampedM * conf;
                    ReservoirUpdate(output, w_prev, prevRes.samplePos, prevRes.sampleNormal, prevRes.Lo, rng);
                    output.M += clampedM - 1;
                    output.age = min(prevRes.age + 1, 255);
                }
            }
        }
    }

    float outTargetLum = targetLum_curr;
    if (IsReservoirValid(output)) {
        float3 wi_out = normalize(output.samplePos - worldPos);
        float cosTheta_out = max(dot(N, wi_out), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta_out, F0)) * (1.0 - metallic);
        outTargetLum = Luminance(output.Lo * kD * albedo / PI * cosTheta_out);
    }
    output.W = (outTargetLum > 0 && output.M > 0) ? output.w_sum / (outTargetLum * output.M) : 0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0) {
        u_ReservoirA[pixel] = 0;
        u_ReservoirB[pixel] = 0;
        u_ReservoirC[pixel] = 0;
        u_SpecReservoirA[pixel] = 0;
        u_SpecReservoirB[pixel] = 0;
        return;
    }

    GIReservoir currRes = UnpackReservoir(u_ReservoirA[pixel], u_ReservoirB[pixel], u_ReservoirC[pixel]);
    GIReservoir currSpec = UnpackReservoir(u_SpecReservoirA[pixel], u_SpecReservoirB[pixel]);

    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    float3 worldPos = worldPosData.xyz;
    const float centerMark = worldPosData.w;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = baseColorData.rgb;
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);

    GIReservoir output = EmptyReservoir();
    GIReservoir outputSpec = EmptyReservoir();
    uint rng = pcg_hash(pixel.x + pixel.y * 7919u + g_FrameIndex * 48611u);

    float2 motion = t_MotionVectors.Load(int3(pixel, 0));
    float2 currUV = (float2(pixel) + 0.5) * g_InvScreenSize;
    float2 prevUV = currUV + motion;

    GIReservoir prevRes = EmptyReservoir();
    GIReservoir prevSpec = EmptyReservoir();
    float3 prevWorldPos = worldPos;

    if (all(prevUV >= 0) && all(prevUV < 1.0)) {
        int2 prevPixel = int2(prevUV * g_ScreenSize);
        float4 prevWorldPosData = t_PrevWorldPos.Load(int3(prevPixel, 0));
        prevWorldPos = prevWorldPosData.xyz;
        float3 prevN = normalize(t_PrevNormal.Load(int3(prevPixel, 0)).xyz);
        float viewDist = length(worldPos - g_CameraPos.xyz);
        float posDist = length(worldPos - prevWorldPos);
        bool valid = SameHudSurfClass(centerMark, prevWorldPosData.w) &&
            posDist < 0.1 * viewDist && dot(N, prevN) > 0.906;
        if (valid) {
            prevRes = UnpackReservoir(
                t_PrevReservoirA.Load(int3(prevPixel, 0)),
                t_PrevReservoirB.Load(int3(prevPixel, 0)),
                t_PrevReservoirC.Load(int3(prevPixel, 0)));
            prevSpec = UnpackReservoir(
                t_PrevSpecA.Load(int3(prevPixel, 0)),
                t_PrevSpecB.Load(int3(prevPixel, 0)));
        }
    }

    TemporalMerge(output, currRes, prevRes, worldPos, prevWorldPos, N, albedo, metallic, true, rng);
    float3 specAlbedo = lerp(float3(1, 1, 1), albedo, metallic);
    TemporalMerge(outputSpec, currSpec, prevSpec, worldPos, prevWorldPos, N, specAlbedo, metallic, true, rng);
    if (roughness > 0.55)
        outputSpec = currSpec;

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
