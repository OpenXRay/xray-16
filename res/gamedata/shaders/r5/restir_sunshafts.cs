#include "bindless_common.h"
#include "rt_common.h"
#include "restir_gi_common.h"
#include "rt_grass_alpha.h"
#include "rt_material_alpha.h"
#include "rt_visibility.h"

cbuffer SunshaftParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor;
    float2 g_ScreenSize;
    float g_ShaftIntensity;
    float g_ShaftLength;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_HudSkinnedStart;
    uint g_ShaftSteps;
    uint g_AlphaEveryN;
    float2 g_FullScreenSize;
    uint g_ParticleBatchStart;
    uint g_FrameIndex;
    uint g_HasPrev;
    float3 g_PrevSunDir;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t4);
ByteAddressBuffer g_SkinnedVB : register(t7);
ByteAddressBuffer g_SkinnedIB : register(t11);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
Texture2D<float> t_Depth : register(t14);
Texture3D<float> t_BlueNoise : register(t16);
Texture2D<float4> t_PrevSunshafts : register(t17);
Texture2D<float> t_PrevDepth : register(t18);

RWTexture2D<float4> u_Sunshafts : register(u0);

float2 ProjectToUv(float4x4 viewProj, float3 worldPos)
{
    float4 clip = mul(viewProj, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(abs(clip.w), 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

float BlueNoiseJitter(uint2 pixel)
{
    return SampleSTBN(t_BlueNoise, pixel, g_FrameIndex, 0);
}

float TraceShaftVis(float3 origin, float3 dir, float tMax, uint2 pixel)
{
    return EvaluateSunVisibilityWithGrass(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        origin, dir, tMax,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, g_HudSkinnedStart,
        t_BlueNoise, pixel, g_FrameIndex, RT_MASK_SHADOW);
}

groupshared float3 gs_Curr[8][8];

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID, uint3 groupThread : SV_GroupThreadID)
{
    uint2 pixel = dispatchID.xy;
    bool validPixel = pixel.x < (uint)g_ScreenSize.x && pixel.y < (uint)g_ScreenSize.y;

    float3 curr = 0;
    float rayLen = 0;
    float3 worldPos = 0;
    bool isSky = false;
    float2 uv = 0;

    if (validPixel) {
        uv = (float2(pixel) + 0.5) / g_ScreenSize;
        int2 fullPx = clamp(int2(uv * g_FullScreenSize), int2(0, 0), int2(g_FullScreenSize) - 1);
        float depth = t_Depth.Load(int3(fullPx, 0));
        float3 camPos = g_CameraPos.xyz;
        float3 lightDir = normalize(g_SunDir_Intensity.xyz);
        float3 toSun = -lightDir;
        float3 sunCol = g_SunColor.xyz;
        isSky = depth <= 1e-5;
        if (isSky)
            worldPos = camPos + normalize(ReconstructWorldPosReverseZ(uv, 1e-3, g_InvViewProj) - camPos) * g_ShaftLength;
        else
            worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);
        float3 toSurf = worldPos - camPos;
        rayLen = length(toSurf);
        if (isSky)
            rayLen = g_ShaftLength;
        else
            rayLen = min(rayLen, g_ShaftLength);
        if (rayLen >= 0.3) {
            float3 marchDir = toSurf / max(length(toSurf), 1e-4);
            uint steps = clamp(g_ShaftSteps, 8u, 40u);
            float density = g_ShaftIntensity / float(steps);
            float res = 0.0;
            float jitter = BlueNoiseJitter(pixel);
            float shadowTMax = min(g_ShaftLength * 1.25, 800.0);
            [loop]
            for (uint i = 0; i < steps; i++) {
                float tNorm = (float(i) + jitter) / float(steps);
                float t = tNorm * rayLen;
                if (t < 0.3)
                    continue;
                float3 samplePos = camPos + marchDir * t + toSun * 0.15;
                float vis = TraceShaftVis(samplePos, toSun, shadowTMax, pixel);
                res += density * vis;
            }
            float fSaturation = -lightDir.y;
            fSaturation = 0.5 * fSaturation + 0.5;
            fSaturation = 0.80 * fSaturation + 0.20;
            res *= saturate(fSaturation);
            curr = max(res, 0.0) * sunCol;
        } else {
            rayLen = 0;
        }
    }

    gs_Curr[groupThread.y][groupThread.x] = curr;
    GroupMemoryBarrierWithGroupSync();

    float3 aabbMin = curr;
    float3 aabbMax = curr;
    [unroll]
    for (int oy = -1; oy <= 1; oy++) {
        [unroll]
        for (int ox = -1; ox <= 1; ox++) {
            int nx = (int)groupThread.x + ox;
            int ny = (int)groupThread.y + oy;
            if (nx < 0 || ny < 0 || nx > 7 || ny > 7)
                continue;
            float3 n = gs_Curr[ny][nx];
            aabbMin = min(aabbMin, n);
            aabbMax = max(aabbMax, n);
        }
    }

    float histW = 0.0;
    float3 hist = curr;
    float sunStable = saturate(dot(normalize(g_SunDir_Intensity.xyz), normalize(g_PrevSunDir + 1e-5)));
    if (validPixel && g_HasPrev != 0 && sunStable >= 0.9995) {
        float2 prevUV = ProjectToUv(g_PrevViewProj, worldPos);
        if (all(prevUV >= 0.0) && all(prevUV < 1.0)) {
            float2 prevPx = prevUV * g_ScreenSize - 0.5;
            int2 p0 = int2(floor(prevPx));
            float2 f = saturate(prevPx - float2(p0));
            int2 maxP = int2(g_ScreenSize) - 1;
            int2 c00 = clamp(p0 + int2(0, 0), int2(0, 0), maxP);
            int2 c10 = clamp(p0 + int2(1, 0), int2(0, 0), maxP);
            int2 c01 = clamp(p0 + int2(0, 1), int2(0, 0), maxP);
            int2 c11 = clamp(p0 + int2(1, 1), int2(0, 0), maxP);
            float4 h00 = t_PrevSunshafts.Load(int3(c00, 0));
            float4 h10 = t_PrevSunshafts.Load(int3(c10, 0));
            float4 h01 = t_PrevSunshafts.Load(int3(c01, 0));
            float4 h11 = t_PrevSunshafts.Load(int3(c11, 0));
            hist = h00.rgb * (1.0 - f.x) * (1.0 - f.y) + h10.rgb * f.x * (1.0 - f.y) +
                h01.rgb * (1.0 - f.x) * f.y + h11.rgb * f.x * f.y;
            float histDepth = h00.a * (1.0 - f.x) * (1.0 - f.y) + h10.a * f.x * (1.0 - f.y) +
                h01.a * (1.0 - f.x) * f.y + h11.a * f.x * f.y;
            int2 prevFull = clamp(int2(prevUV * g_FullScreenSize), int2(0, 0), int2(g_FullScreenSize) - 1);
            float prevD = t_PrevDepth.Load(int3(prevFull, 0));
            bool depthOk = isSky ? (prevD <= 1e-5) : (prevD > 1e-5 && abs(histDepth - rayLen) < max(0.15 * rayLen, 1.0));
            float2 motion = (prevUV - uv) * g_ScreenSize;
            float motionPx = length(motion);
            if (depthOk && motionPx < 48.0)
                histW = lerp(0.60, 0.20, saturate(motionPx / 24.0));
        }
    }

    hist = clamp(hist, aabbMin, aabbMax);
    if (validPixel)
        u_Sunshafts[pixel] = float4(max(lerp(curr, hist, histW), 0.0), rayLen);
}
