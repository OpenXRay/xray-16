#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"


cbuffer ReSTIRTemporalParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    float g_EnvAdapt;
    float g_CurrJitterX;
    float g_CurrJitterY;
    float g_PrevJitterX;
    float g_PrevJitterY;
};

StructuredBuffer<uint4> t_PrevReservoir : register(t0);
Texture2D<float2> t_MotionVectors : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_PrevNormal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float> t_PrevDepth : register(t8);
Texture2D<float4> t_Normal : register(t9);
Texture2D<float> t_SkyOpen : register(t10);

RWStructuredBuffer<uint4> u_Reservoir : register(u0);

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
        u_Reservoir[pixelIdx] = 0;
        return;
    }

    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float surfMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    if (IsWaterSurfMark(surfMark)) {
        u_Reservoir[pixelIdx] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    GIReservoir currRes = UnpackReservoirU4(u_Reservoir[pixelIdx], worldPos);

    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 N = normalize(normalData.xyz);
    float3 albedo = baseColorData.rgb;
    float sssMaskUnused = 0.0;
    float metallic = UnpackGBufferMetallic(baseColorData.a, IsHudSurfMark(surfMark) || IsCharSurfMark(surfMark), sssMaskUnused);

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
        output.Lo = min(currRes.Lo, RESTIR_MAX_RADIANCE);
        output.lightId = currRes.lightId;
        output.w_sum = targetLum_curr * currRes.W;
        output.M = 1;
        output.age = currRes.age;
    }

    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float2 motion = t_MotionVectors.Load(int3(fullPx, 0));
    float2 prevUV = uv + motion;
    float motionPx = length(motion * fullSize);

    if (!IsCharSurfMark(surfMark) && motionPx < 16.0 && all(prevUV >= 0) && all(prevUV < 1.0)) {
        int2 prevPixel = int2(prevUV * giSize);
        prevPixel = clamp(prevPixel, int2(0, 0), int2(width, height) - 1);
        int2 prevFull = clamp(int2(prevUV * fullSize), int2(0, 0), int2(fullSize) - 1);
        float prevDepth = t_PrevDepth.Load(int3(prevFull, 0));
        float3 prevN = normalize(t_PrevNormal.Load(int3(prevFull, 0)).xyz);

        float viewDist = length(worldPos - g_CameraPos.xyz);
        bool valid = false;
        float3 prevWorldPos = worldPos;
        if (prevDepth > 0.0 && prevDepth < 1.0) {
            float2 prevNdcUV = (float2(prevFull) + 0.5) / fullSize;
            prevWorldPos = ReconstructWorldPosReverseZ(prevNdcUV, prevDepth, g_PrevInvViewProj);
            float posDist = length(worldPos - prevWorldPos);
            float posTol = (motionPx < 1.0) ? 0.05 : 0.035;
            valid = posDist < posTol * max(min(viewDist, 4.0), 1.0) && dot(N, prevN) > 0.9;
            float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
            float skyOpenP = saturate(t_SkyOpen.Load(int3(prevFull, 0)));
            valid = valid && abs(skyOpenC - skyOpenP) <= 0.25;
        }

        if (valid) {
            uint prevIdx = (uint)prevPixel.y * width + (uint)prevPixel.x;
            GIReservoir prevRes = UnpackReservoirU4(t_PrevReservoir[prevIdx], prevWorldPos);

            if (IsReservoirValid(prevRes)) {
                prevRes.Lo *= g_EnvAdapt;
                float3 wi_prev = normalize(prevRes.samplePos - worldPos);
                float cosTheta_prev = max(dot(N, wi_prev), 0);
                float3 F0 = CalculateF0(albedo, metallic);
                float3 kD = (1.0 - F_Schlick(cosTheta_prev, F0)) * (1.0 - metallic);
                float3 target_prev = prevRes.Lo * kD * albedo / PI * cosTheta_prev;
                float targetLum_prev = Luminance(target_prev);

                float jacobian = JacobianReconnectionShift(
                    prevRes.sampleNormal, worldPos, prevWorldPos, prevRes.samplePos);
                jacobian = clamp(jacobian, 0.25, 4.0);

                if (targetLum_prev > 0) {
                    uint clampedM = TemporalMClamp(prevRes.M, prevRes.age, RESTIR_M_MAX);
                    if (motionPx > 6.0)
                        clampedM = max(1u, clampedM / 4u);
                    else if (motionPx > 2.0)
                        clampedM = max(1u, clampedM / 2u);
                    float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
                    float skyOpenP = saturate(t_SkyOpen.Load(int3(prevFull, 0)));
                    if (abs(skyOpenC - skyOpenP) > 0.35)
                        clampedM = min(clampedM, 4u);
                    else if (targetLum_curr > 0 && max(targetLum_prev, targetLum_curr) / max(min(targetLum_prev, targetLum_curr), 1e-6) > 8.0)
                        clampedM = min(clampedM, 8u);
                    if (!IsReservoirValid(output)) {
                        output = prevRes;
                        output.w_sum = targetLum_prev * prevRes.W * jacobian;
                        output.M = max(clampedM, 1u);
                    } else {
                        float w_prev = targetLum_prev * prevRes.W * clampedM * jacobian;
                        ReservoirUpdate(output, w_prev, prevRes.samplePos, prevRes.sampleNormal, prevRes.Lo, prevRes.lightId, rng);
                        output.M += clampedM - 1;
                    }
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
        float3 target_out = output.Lo * kD * albedo / PI * cosTheta_out;
        outTargetLum = Luminance(target_out);
    }

    output.Lo = min(output.Lo, RESTIR_MAX_RADIANCE);
    output.W = (outTargetLum > 0 && output.M > 0) ? min(output.w_sum / (outTargetLum * output.M), 4.0) : 0;
    output.age = min(output.age + 1, 127);

    u_Reservoir[pixelIdx] = PackReservoirU4(output, worldPos);
}
