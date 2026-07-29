#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDITemporalParams : register(b5) {
    float4x4 g_WorldToView;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_MMax;
    float2 g_Pad;
    float4 g_ClusterParams;
    float4 g_ClusterScales;
};

StructuredBuffer<GPULightDataDI> g_LightData : register(t20);
Texture2D<float4> t_PrevDI : register(t0);
Texture2D<float2> t_MotionVectors : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_PrevNormal : register(t5);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_PrevWorldPos : register(t11);
Texture2D<float4> t_Normal : register(t12);

RWTexture2D<float4> u_DIReservoir : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0) {
        u_DIReservoir[pixel] = PackDIReservoir(EmptyDIReservoir());
        return;
    }

    DIReservoir curr = UnpackDIReservoir(u_DIReservoir[pixel]);
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

    uint rng = pcg_hash(pixel.x + pixel.y * 7919u + g_FrameIndex * 48611u);
    DIReservoir output = EmptyDIReservoir();

    float targetCurr = 0;
    if (IsDIReservoirValid(curr) && curr.lightIndex < (uint)g_ClusterParams.w) {
        if (!(hudSurf && !IsHudLightDI(g_LightData[curr.lightIndex]))) {
            targetCurr = EvalLocalLightTargetPdfDI(
                g_LightData[curr.lightIndex], worldPos, N, V, albedo, metallic, roughness);
            if (targetCurr > 0) {
                output.lightIndex = curr.lightIndex;
                output.targetPdf = targetCurr;
                output.w_sum = targetCurr * curr.W;
                output.M = 1;
                output.age = curr.age;
            }
        }
    }

    float2 motion = t_MotionVectors.Load(int3(pixel, 0));
    float2 currUV = (float2(pixel) + 0.5) * g_InvScreenSize;
    float2 prevUV = currUV + motion;

    if (all(prevUV >= 0) && all(prevUV < 1.0)) {
        int2 prevPixel = int2(prevUV * g_ScreenSize);
        float4 prevWorldPosData = t_PrevWorldPos.Load(int3(prevPixel, 0));
        float3 prevWorldPos = prevWorldPosData.xyz;
        float3 prevN = normalize(t_PrevNormal.Load(int3(prevPixel, 0)).xyz);
        float viewDist = length(worldPos - g_CameraPos.xyz);
        float posDist = length(worldPos - prevWorldPos);
        bool valid = SameHudSurfClass(centerMark, prevWorldPosData.w) &&
            posDist < 0.1 * viewDist && dot(N, prevN) > 0.906;

        if (valid) {
            DIReservoir prev = UnpackDIReservoir(t_PrevDI.Load(int3(prevPixel, 0)));
            if (IsDIReservoirValid(prev) && prev.lightIndex < (uint)g_ClusterParams.w &&
                !(hudSurf && !IsHudLightDI(g_LightData[prev.lightIndex]))) {
                float targetPrev = EvalLocalLightTargetPdfDI(
                    g_LightData[prev.lightIndex], worldPos, N, V, albedo, metallic, roughness);
                if (targetPrev > 0) {
                    uint clampedM = min(prev.M, g_MMax);
                    float wPrev = targetPrev * prev.W * clampedM;
                    DIReservoirUpdate(output, wPrev, prev.lightIndex, targetPrev, rng);
                    output.M += clampedM - 1;
                    output.age = prev.age + 1;
                }
            }
        }
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

    if (output.W <= 0)
        output = EmptyDIReservoir();

    output.M = min(output.M, g_MMax);
    u_DIReservoir[pixel] = PackDIReservoir(output);
}
