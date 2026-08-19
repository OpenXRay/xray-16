#include "rt_common.h"
#include "restir_gi_common.h"
#include "shared/surface_marks.h"

cbuffer TemporalFilterParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    float g_Alpha;
    float g_EnvAdapt;
    float g_CurrJitterX;
    float g_CurrJitterY;
    float g_PrevJitterX;
    float g_PrevJitterY;
    uint g_Enabled;
    uint g_Pad1;
};

Texture2D<float4> t_CurrDiffuse : register(t0);
Texture2D<float4> t_CurrSpecular : register(t1);
Texture2D<float4> t_HistDiffuse : register(t2);
Texture2D<float4> t_HistSpecular : register(t3);
Texture2D<float2> t_MotionVectors : register(t4);
Texture2D<float> t_Depth : register(t5);
Texture2D<float4> t_Normal : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float> t_PrevDepth : register(t8);
Texture2D<float4> t_PrevNormal : register(t9);

RWTexture2D<float4> u_OutDiffuse : register(u0);
RWTexture2D<float4> u_OutSpecular : register(u1);

float Luma(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float3 ClipAABB(float3 hist, float3 minC, float3 maxC)
{
    float3 center = 0.5 * (minC + maxC);
    float3 extents = 0.5 * (maxC - minC) + 1e-4;
    float3 offset = hist - center;
    float3 ts = abs(extents / max(abs(offset), 1e-4));
    float t = saturate(min(min(ts.x, ts.y), ts.z));
    return center + offset * t;
}

float3 SampleHist(Texture2D<float4> tex, float2 uv)
{
    float2 p = uv * g_ScreenSize - 0.5;
    int2 i0 = int2(floor(p));
    float2 f = saturate(p - float2(i0));
    int2 maxP = int2(g_ScreenSize) - 1;
    int2 i1 = clamp(i0 + int2(1, 0), int2(0, 0), maxP);
    int2 i2 = clamp(i0 + int2(0, 1), int2(0, 0), maxP);
    int2 i3 = clamp(i0 + int2(1, 1), int2(0, 0), maxP);
    i0 = clamp(i0, int2(0, 0), maxP);
    float3 a = tex.Load(int3(i0, 0)).rgb;
    float3 b = tex.Load(int3(i1, 0)).rgb;
    float3 c = tex.Load(int3(i2, 0)).rgb;
    float3 d = tex.Load(int3(i3, 0)).rgb;
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float2 giSize = g_ScreenSize;
    uint fullW = 0, fullH = 0;
    t_Depth.GetDimensions(fullW, fullH);
    float2 fullSize = float2(max(fullW, 1u), max(fullH, 1u));

    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    float4 currD4 = t_CurrDiffuse.Load(int3(pixel, 0));
    float4 currS4 = t_CurrSpecular.Load(int3(pixel, 0));
    float3 currD = currD4.rgb;
    float3 currS = currS4.rgb;
    float specHitDist = currS4.a;

    if (depth <= 0.0 || g_Enabled == 0) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float2 mv = t_MotionVectors.Load(int3(fullPx, 0));
    float2 histUV = uv + mv;
    if (any(histUV < 0.0) || any(histUV > 1.0)) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    int2 histPixel = int2(histUV * giSize);
    histPixel = clamp(histPixel, int2(0, 0), int2(giSize) - 1);
    int2 histFull = clamp(int2(histUV * fullSize), int2(0, 0), int2(fullSize) - 1);

    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    if (IsCharSurfMark(worldPosData.w)) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }
    float3 worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);
    float prevDepth = t_PrevDepth.Load(int3(histFull, 0));
    float2 prevNdcUV = (float2(histFull) + 0.5) / fullSize;
    float3 prevWorld = (prevDepth > 0.0 && prevDepth < 1.0)
        ? ReconstructWorldPosReverseZ(prevNdcUV, prevDepth, g_PrevInvViewProj)
        : worldPos;
    float3 prevN = normalize(t_PrevNormal.Load(int3(histFull, 0)).xyz);
    float3 N = normalize(RestirLoadTex4(t_Normal, pixel, giSize, fullSize).xyz);

    if (!SameHudSurfClass(worldPosData.w, t_WorldPos.Load(int3(histFull, 0)).w)) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    float viewDist = max(length(worldPos - g_CameraPos.xyz), 1.0);
    float motionPx = length(mv * fullSize);
    float posErr = length(worldPos - prevWorld) / viewDist;
    float staticTol = (motionPx < 1.0) ? 0.18 : 0.10;
    if (motionPx < 2.0 && posErr > staticTol) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }
    float nDot = saturate(dot(N, prevN));
    float trust = 1.0;
    if (posErr > 0.35 || nDot < 0.4) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }
    if (posErr > 0.10)
        trust *= saturate(1.0 - (posErr - 0.10) / 0.25);
    if (nDot < 0.85)
        trust *= saturate((nDot - 0.4) / 0.45);
    if (motionPx > 1.0)
        trust *= saturate(1.0 - (motionPx - 1.0) / 14.0);

    float3 histD = SampleHist(t_HistDiffuse, histUV) * g_EnvAdapt;
    float3 histS = SampleHist(t_HistSpecular, histUV) * g_EnvAdapt;

    float3 minD = currD, maxD = currD, minS = currS, maxS = currS;
    [unroll] for (int iy = -1; iy <= 1; ++iy) {
        [unroll] for (int ix = -1; ix <= 1; ++ix) {
            int2 np = int2(pixel) + int2(ix, iy);
            if (np.x < 0 || np.y < 0 || np.x >= (int)g_ScreenSize.x || np.y >= (int)g_ScreenSize.y)
                continue;
            if (!SameHudSurfClass(worldPosData.w, RestirLoadTex4(t_WorldPos, uint2(np), giSize, fullSize).w))
                continue;
            float3 d = t_CurrDiffuse.Load(int3(np, 0)).rgb;
            float3 s = t_CurrSpecular.Load(int3(np, 0)).rgb;
            minD = min(minD, d);
            maxD = max(maxD, d);
            minS = min(minS, s);
            maxS = max(maxS, s);
        }
    }

    if (trust < 0.95 || motionPx > 0.5) {
        float lo = lerp(0.15, 0.45, trust);
        float hi = lerp(5.0, 2.5, trust);
        histD = ClipAABB(histD, minD * lo, maxD * hi + 0.02);
        histS = ClipAABB(histS, minS * lo * 0.8, maxS * hi * 1.2 + 0.02);
    }

    float alpha = saturate(g_Alpha) * trust;
    if (motionPx < 0.5)
        alpha = min(0.97, alpha + 0.02);
    float3 outD = lerp(currD, histD, alpha);
    float3 outS = lerp(currS, histS, alpha * 0.96);
    if (trust < 0.85) {
        float currLd = max(Luma(currD), 1e-4);
        float histLd = max(Luma(outD), 1e-4);
        if (currLd > histLd * 6.0)
            outD *= (histLd * 6.0) / currLd;
        float currLs = max(Luma(currS), 1e-4);
        float histLs = max(Luma(outS), 1e-4);
        if (currLs > histLs * 8.0)
            outS *= (histLs * 8.0) / currLs;
    }

    u_OutDiffuse[pixel] = float4(outD, 1.0);
    u_OutSpecular[pixel] = float4(outS, specHitDist);
}
