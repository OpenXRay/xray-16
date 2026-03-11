#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "restir_gi_common.h"

cbuffer ReSTIRTemporalParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint3 g_Pad;
};

Texture2D<float4> t_PrevReservoirA : register(t0);
Texture2D<float4> t_PrevReservoirB : register(t1);
Texture2D<float2> t_MotionVectors : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_PrevNormal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_PrevWorldPos : register(t8);
Texture2D<float4> t_Normal : register(t9);

RWTexture2D<float4> u_ReservoirA : register(u0);
RWTexture2D<float4> u_ReservoirB : register(u1);

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
        return;
    }

    GIReservoir currRes = UnpackReservoir(
        u_ReservoirA[pixel],
        u_ReservoirB[pixel]
    );

    float3 worldPos = t_WorldPos.Load(int3(pixel, 0)).xyz;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(normalData.xyz);
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float3 albedo = baseColorData.rgb;
    float metallic = baseColorData.a;

    float3 target_curr = 0;
    if (IsReservoirValid(currRes)) {
        float3 wi = normalize(currRes.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        target_curr = currRes.Lo * kD * albedo / PI * cosTheta;
    }
    float targetLum_curr = Luminance(target_curr);

    GIReservoir output = EmptyReservoir();
    uint rng = pcg_hash(pixel.x + pixel.y * 7919u + g_FrameIndex * 48611u);

    if (targetLum_curr > 0) {
        output.samplePos = currRes.samplePos;
        output.sampleNormal = currRes.sampleNormal;
        output.Lo = currRes.Lo;
        output.w_sum = targetLum_curr * currRes.W;
        output.M = 1;
    }

    float2 motion = t_MotionVectors.Load(int3(pixel, 0));
    float2 currUV = (float2(pixel) + 0.5) * g_InvScreenSize;
    float2 prevUV = currUV + motion;

    if (all(prevUV >= 0) && all(prevUV < 1.0)) {
        int2 prevPixel = int2(prevUV * g_ScreenSize);
        float3 prevWorldPos = t_PrevWorldPos.Load(int3(prevPixel, 0)).xyz;
        float3 prevN = normalize(t_PrevNormal.Load(int3(prevPixel, 0)).xyz);

        float viewDist = length(worldPos - g_CameraPos.xyz);
        float posDist = length(worldPos - prevWorldPos);
        bool valid = posDist < 0.1 * viewDist && dot(N, prevN) > 0.906;

        if (valid) {
            GIReservoir prevRes = UnpackReservoir(
                t_PrevReservoirA.Load(int3(prevPixel, 0)),
                t_PrevReservoirB.Load(int3(prevPixel, 0))
            );

            if (IsReservoirValid(prevRes)) {
                float3 wi_prev = normalize(prevRes.samplePos - worldPos);
                float cosTheta_prev = max(dot(N, wi_prev), 0);
                float3 F0 = CalculateF0(albedo, metallic);
                float3 kD = (1.0 - F_Schlick(cosTheta_prev, F0)) * (1.0 - metallic);
                float3 target_prev = prevRes.Lo * kD * albedo / PI * cosTheta_prev;
                float targetLum_prev = Luminance(target_prev);

                if (targetLum_prev > 0) {
                    uint clampedM = min(prevRes.M, RESTIR_M_MAX);
                    float w_prev = targetLum_prev * prevRes.W * clampedM;

                    ReservoirUpdate(output, w_prev, prevRes.samplePos, prevRes.sampleNormal, prevRes.Lo, rng);
                    output.M += clampedM - 1;
                }
            }
        }
    }

    float outTargetLum = targetLum_curr;
    if (output.samplePos.x != currRes.samplePos.x || output.samplePos.y != currRes.samplePos.y) {
        float3 wi_out = normalize(output.samplePos - worldPos);
        float cosTheta_out = max(dot(N, wi_out), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta_out, F0)) * (1.0 - metallic);
        float3 target_out = output.Lo * kD * albedo / PI * cosTheta_out;
        outTargetLum = Luminance(target_out);
    }

    output.W = (outTargetLum > 0 && output.M > 0) ? output.w_sum / (outTargetLum * output.M) : 0;
    output.age = min(output.age + 1, 255);

    float4 outA, outB;
    PackReservoir(output, outA, outB);
    u_ReservoirA[pixel] = outA;
    u_ReservoirB[pixel] = outB;
}
