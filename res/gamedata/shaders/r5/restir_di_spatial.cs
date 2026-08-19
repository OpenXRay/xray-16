#define SM_6_0
#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDISpatialParams : register(b5) {
    float4x4 g_InvViewProj;
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
    float g_FullWidth;
    float g_FullHeight;
};

StructuredBuffer<GPULightDataDI> g_LightData : register(t20);
Texture2D<float4> t_SrcDI : register(t0);
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_Normal : register(t11);

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
    if (depth <= 0.0 || g_SpatialSamples == 0) {
        u_DIReservoir[pixel] = t_SrcDI.Load(int3(pixel, 0));
        return;
    }

    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float surfMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    if (IsWaterSurfMark(surfMark) || IsCharSurfMark(surfMark)) {
        u_DIReservoir[pixel] = t_SrcDI.Load(int3(pixel, 0));
        return;
    }
    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    const float centerMark = surfMark;
    const bool hudSurf = IsHudSurfMark(centerMark);
    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float3 albedo = max(baseColorData.rgb, 0.0);
    float sssMaskUnused = 0.0;
    float metallic = UnpackGBufferMetallic(baseColorData.a, hudSurf || IsCharSurfMark(centerMark), sssMaskUnused);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float linearDepth = abs(mul(g_WorldToView, float4(worldPos, 1.0)).z);
    const uint lightCount = (uint)g_ClusterParams.w;

    DIReservoir center = UnpackDIReservoir(t_SrcDI.Load(int3(pixel, 0)));
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);
    DIReservoir output = EmptyDIReservoir();

    float targetCenter = 0;
    if (IsDIReservoirValid(center) && center.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[center.lightIndex]) &&
        (hudSurf || !IsHudLightDI(g_LightData[center.lightIndex]))) {
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

    float viewDist = length(worldPos - g_CameraPos.xyz);
    const bool interiorCenter = IsInteriorSurfMark(centerMark);
    float spatialRadius = g_SpatialRadius;
    uint spatialN = g_SpatialSamples;
    if (interiorCenter) {
        spatialRadius = g_SpatialRadius * 2.5;
        spatialN = min(max(spatialN, 8u), 16u);
    } else {
        spatialRadius = min(g_SpatialRadius, 8.0);
        if (viewDist > 60.0)
            spatialN = min(spatialN, 1u);
        else if (viewDist > 30.0)
            spatialN = min(spatialN, max(spatialN / 2u, 1u));
    }
    for (uint i = 0; i < spatialN; i++) {
        float2 disc = float2(rand_float(rng), rand_float(rng));
        float ang = disc.x * 6.2831853;
        float rad = sqrt(disc.y) * spatialRadius;
        int2 nPixel = int2(float2(pixel) + float2(cos(ang), sin(ang)) * rad);
        if (nPixel.x < 0 || nPixel.y < 0 ||
            nPixel.x >= (int)g_ScreenSize.x || nPixel.y >= (int)g_ScreenSize.y)
            continue;

        float4 nWorldData = RestirLoadTex4(t_WorldPos, uint2(nPixel), giSize, fullSize);
        float4 nBase = RestirLoadTex4(t_BaseColor, uint2(nPixel), giSize, fullSize);
        float nMark = SurfMarkFromGBuffer(nWorldData.w, nBase.a);
        if (!SameHudSurfClass(centerMark, nMark))
            continue;
        if (!SameLightZone(centerMark, nMark))
            continue;
        if (interiorCenter && (IsVegSurfMark(nMark) || IsTerrainSurfMark(nMark)))
            continue;
        float2 nUV = (float2(nPixel) + 0.5) * g_InvScreenSize;
        float nDepthRaw = RestirLoadDepth(t_Depth, uint2(nPixel), giSize, fullSize);
        float3 nWorld = ReconstructWorldPosReverseZ(nUV, nDepthRaw, g_InvViewProj);
        float3 nN = normalize(RestirLoadTex4(t_Normal, uint2(nPixel), giSize, fullSize).xyz);
        float nDepth = abs(mul(g_WorldToView, float4(nWorld, 1.0)).z);
        if (abs(nDepth - linearDepth) / max(linearDepth, 1e-3) > 0.1)
            continue;
        if (dot(N, nN) < 0.906)
            continue;

        DIReservoir neighbor = UnpackDIReservoir(t_SrcDI.Load(int3(nPixel, 0)));
        if (!IsDIReservoirValid(neighbor) || neighbor.lightIndex >= lightCount)
            continue;
        if (!IsLiveLightDI(g_LightData[neighbor.lightIndex]))
            continue;
        if ((!hudSurf && IsHudLightDI(g_LightData[neighbor.lightIndex])))
            continue;

        float targetN = EvalLocalLightTargetPdfDI(
            g_LightData[neighbor.lightIndex], worldPos, N, V, albedo, metallic, roughness);
        if (targetN <= 0)
            continue;

        if (!IsDIReservoirValid(output)) {
            output.lightIndex = neighbor.lightIndex;
            output.targetPdf = targetN;
            output.w_sum = targetN * neighbor.W * min(neighbor.M, g_MMax);
            output.M = min(neighbor.M, g_MMax);
            output.age = neighbor.age;
        } else {
            uint clampedM = min(neighbor.M, g_MMax);
            float w = targetN * neighbor.W * clampedM;
            DIReservoirUpdate(output, w, neighbor.lightIndex, targetN, rng);
            output.M += clampedM - 1;
        }
    }

    if (!IsDIReservoirValid(output) && IsDIReservoirValid(center) &&
        center.lightIndex < lightCount && IsLiveLightDI(g_LightData[center.lightIndex]) &&
        (hudSurf || !IsHudLightDI(g_LightData[center.lightIndex])))
        output = center;

    float outPdf = output.targetPdf;
    if (outPdf <= 0 && IsDIReservoirValid(output) && output.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[output.lightIndex])) {
        outPdf = EvalLocalLightTargetPdfDI(
            g_LightData[output.lightIndex], worldPos, N, V, albedo, metallic, roughness);
        output.targetPdf = outPdf;
    }

    if (outPdf > 0 && output.M > 0)
        output.W = ClampDIReservoirW(output.w_sum / max(outPdf * (float)output.M, 1e-6));
    else if (IsDIReservoirValid(center) && center.lightIndex < lightCount &&
        IsLiveLightDI(g_LightData[center.lightIndex]) &&
        (hudSurf || !IsHudLightDI(g_LightData[center.lightIndex])))
        output = center;
    else
        output = EmptyDIReservoir();

    if (IsDIReservoirValid(output) && !IsLiveLightDI(g_LightData[output.lightIndex]))
        output = EmptyDIReservoir();

    if (IsDIReservoirValid(center) && output.w_sum > 8.0 * max(center.w_sum, 1e-6))
        output = center;

    if (output.W <= 0 && IsDIReservoirValid(center))
        output = center;

    output.M = min(max(output.M, 1u), g_MMax);
    output.zone = interiorCenter ? 1u : 0u;
    u_DIReservoir[pixel] = PackDIReservoir(output);
}
