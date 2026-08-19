#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDITemporalParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_MMax;
    float g_CurrJitterX;
    float g_CurrJitterY;
    float g_PrevJitterX;
    float g_PrevJitterY;
    float g_FullWidth;
    float g_FullHeight;
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
Texture2D<float> t_PrevDepth : register(t11);
Texture2D<float4> t_Normal : register(t12);
Texture2D<float> t_SkyOpen : register(t13);

RWTexture2D<float4> u_DIReservoir : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float2 giSize = g_ScreenSize;
    float2 fullSize = float2(g_FullWidth, g_FullHeight);
    if (fullSize.x < 1.0 || fullSize.y < 1.0) {
        uint fw = 0, fh = 0;
        t_Depth.GetDimensions(fw, fh);
        fullSize = float2(max(fw, 1u), max(fh, 1u));
    }

    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0) {
        u_DIReservoir[pixel] = PackDIReservoir(EmptyDIReservoir());
        return;
    }

    DIReservoir curr = UnpackDIReservoir(u_DIReservoir[pixel]);
    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    const float centerMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    const bool hudSurf = IsHudSurfMark(centerMark);
    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float3 albedo = max(baseColorData.rgb, 0.0);
    float sssMaskUnused = 0.0;
    float metallic = UnpackGBufferMetallic(baseColorData.a, hudSurf || IsCharSurfMark(centerMark), sssMaskUnused);
    float3 V = normalize(g_CameraPos.xyz - worldPos);

    uint rng = pcg_hash(pixel.x + pixel.y * 7919u + g_FrameIndex * 48611u);
    DIReservoir output = EmptyDIReservoir();
    const uint lightCount = (uint)g_ClusterParams.w;

    float targetCurr = 0;
    if (IsDIReservoirValid(curr) && curr.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[curr.lightIndex])) {
        if (hudSurf || !IsHudLightDI(g_LightData[curr.lightIndex])) {
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

    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float2 motion = t_MotionVectors.Load(int3(fullPx, 0));
    float2 prevUV = uv + motion;
    float motionPx = length(motion * fullSize);

    if (!IsCharSurfMark(centerMark) && motionPx < 16.0 && all(prevUV >= 0) && all(prevUV < 1.0)) {
        int2 prevPixel = int2(prevUV * giSize);
        prevPixel = clamp(prevPixel, int2(0, 0), int2(giSize) - 1);
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
            float posTol = (motionPx < 1.0) ? 0.16 : 0.12;
            valid = posDist < posTol * max(viewDist, 1.0) && dot(N, prevN) > 0.8;
        }

        if (valid) {
            DIReservoir prev = UnpackDIReservoir(t_PrevDI.Load(int3(prevPixel, 0)));
            if (IsDIReservoirValid(prev) && prev.lightIndex < lightCount &&
                IsLiveLightDI(g_LightData[prev.lightIndex]) &&
                (hudSurf || !IsHudLightDI(g_LightData[prev.lightIndex]))) {
                float targetPrev = EvalLocalLightTargetPdfDI(
                    g_LightData[prev.lightIndex], worldPos, N, V, albedo, metallic, roughness);
                if (targetPrev > 0) {
                    uint mCap = g_MMax;
                    if (motionPx > 6.0)
                        mCap = max(1u, g_MMax / 4u);
                    else if (motionPx > 2.0)
                        mCap = max(1u, g_MMax / 2u);
                    float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
                    float skyOpenP = saturate(RestirLoadTex1(t_SkyOpen, uint2(prevFull), fullSize, fullSize));
                    uint currZone = IsInteriorSurfMark(centerMark) ? 1u : 0u;
                    if (currZone != prev.zone) {
                        if (abs(skyOpenC - skyOpenP) > 0.35)
                            mCap = min(mCap, 4u);
                        else
                            mCap = min(mCap, 8u);
                    }
                    if (targetCurr > 0 && max(targetPrev, targetCurr) / max(min(targetPrev, targetCurr), 1e-6) > 8.0)
                        mCap = min(mCap, 8u);
                    if (!IsDIReservoirValid(output)) {
                        output.lightIndex = prev.lightIndex;
                        output.targetPdf = targetPrev;
                        output.w_sum = targetPrev * prev.W * min(prev.M, mCap);
                        output.M = min(prev.M, mCap);
                        output.age = prev.age + 1;
                    } else {
                        uint clampedM = min(prev.M, mCap);
                        float wPrev = targetPrev * prev.W * clampedM;
                        DIReservoirUpdate(output, wPrev, prev.lightIndex, targetPrev, rng);
                        output.M += clampedM - 1;
                        output.age = prev.age + 1;
                    }
                }
            }
        }
    }

    if (!IsDIReservoirValid(output) && IsDIReservoirValid(curr) && curr.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[curr.lightIndex]) &&
        (hudSurf || !IsHudLightDI(g_LightData[curr.lightIndex])))
        output = curr;

    float outPdf = output.targetPdf;
    if (outPdf <= 0 && IsDIReservoirValid(output) && output.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[output.lightIndex])) {
        outPdf = EvalLocalLightTargetPdfDI(
            g_LightData[output.lightIndex], worldPos, N, V, albedo, metallic, roughness);
        output.targetPdf = outPdf;
    }

    if (outPdf > 0 && output.M > 0)
        output.W = ClampDIReservoirW(output.w_sum / max(outPdf * (float)output.M, 1e-6));
    else if (IsDIReservoirValid(curr) && curr.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[curr.lightIndex]) &&
        (hudSurf || !IsHudLightDI(g_LightData[curr.lightIndex])))
        output = curr;
    else
        output = EmptyDIReservoir();

    if (output.W <= 0 || (IsDIReservoirValid(output) && !IsLiveLightDI(g_LightData[output.lightIndex])))
        output = EmptyDIReservoir();

    output.M = min(max(output.M, 1u), g_MMax);
    output.zone = IsInteriorSurfMark(centerMark) ? 1u : 0u;
    u_DIReservoir[pixel] = PackDIReservoir(output);
}
