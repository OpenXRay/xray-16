#include "restir_gi_common.h"

cbuffer ReSTIRTemporalParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_DiffuseMode;
    uint g_Pad0;
    uint g_Pad1;
};

Texture2D<float4> t_PrevReservoirA : register(t0);
Texture2D<float4> t_PrevReservoirB : register(t1);
Texture2D<float2> t_MotionVectors : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_PrevNormal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float> t_PrevDepth : register(t8);
Texture2D<float4> t_Normal : register(t9);
Texture2D<float2> t_Material : register(t17);

RWTexture2D<float4> u_ReservoirA : register(u0);
RWTexture2D<float4> u_ReservoirB : register(u1);

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
        return;
    }
    if (depth >= 0.9)
        return;

    GIReservoir currRes = UnpackReservoir(
        u_ReservoirA[pixel],
        u_ReservoirB[pixel]
    );

    float2 giUV = (float2(pixel) + 0.5) * g_InvScreenSize;
    float4 giClip = float4(giUV.x * 2.0 - 1.0, 1.0 - giUV.y * 2.0, depth, 1.0);
    float4 giWorld = mul(g_InvViewProj, giClip);
    float3 worldPos = giWorld.xyz / giWorld.w;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    if (!all(isfinite(worldPos)) || !all(isfinite(normalData.xyz)) || dot(normalData.xyz, normalData.xyz) < 0.25)
        return;

    MaterialSurface primary = GBufferMaterialSurface(normalData, t_BaseColor.Load(int3(pixel, 0)), t_Material.Load(int3(pixel, 0)));
    float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, primary.N);

    float targetLum_curr = 0.0;
    if (IsReservoirValid(currRes))
        targetLum_curr = Luminance(GITargetRadiance(primary, V, worldPos, currRes.samplePos, currRes.Lo, g_DiffuseMode));

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
        float prevDepth = t_PrevDepth.Load(int3(prevPixel, 0));
        float3 prevNormal = t_PrevNormal.Load(int3(prevPixel, 0)).xyz;

        float viewDist = length(worldPos - g_CameraPos.xyz);
        bool valid = false;
        if (prevDepth > 0.0 && prevDepth < 0.9 && all(isfinite(prevNormal)) && dot(prevNormal, prevNormal) >= 0.25) {
            float3 prevN = normalize(prevNormal);
            float2 prevNdcUV = (float2(prevPixel) + 0.5) * g_InvScreenSize;
            float4 prevClip = float4(prevNdcUV.x * 2.0 - 1.0, 1.0 - prevNdcUV.y * 2.0, prevDepth, 1.0);
            float4 prevWorld = mul(g_PrevInvViewProj, prevClip);
            float3 prevWorldPos = prevWorld.xyz / prevWorld.w;
            float posDist = length(worldPos - prevWorldPos);
            valid = posDist < 0.1 * viewDist && dot(primary.N, prevN) > 0.906;
        }

        if (valid) {
            GIReservoir prevRes = UnpackReservoir(
                t_PrevReservoirA.Load(int3(prevPixel, 0)),
                t_PrevReservoirB.Load(int3(prevPixel, 0))
            );

            if (IsReservoirValid(prevRes)) {
                float targetLum_prev = Luminance(GITargetRadiance(primary, V, worldPos, prevRes.samplePos, prevRes.Lo, g_DiffuseMode));

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
    if (output.samplePos.x != currRes.samplePos.x || output.samplePos.y != currRes.samplePos.y)
        outTargetLum = Luminance(GITargetRadiance(primary, V, worldPos, output.samplePos, output.Lo, g_DiffuseMode));

    ReservoirFinalize(output, outTargetLum);
    output.age = min(output.age + 1, 255);

    float4 outA, outB;
    PackReservoir(output, outA, outB);
    u_ReservoirA[pixel] = outA;
    u_ReservoirB[pixel] = outB;
}
