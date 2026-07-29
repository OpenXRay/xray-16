#include "shared/surface_marks.h"

cbuffer TemporalFilterParams : register(b5) {
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    float g_Alpha;
    float g_Pad0;
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
Texture2D<float4> t_PrevWorldPos : register(t8);
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

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float4 currD4 = t_CurrDiffuse.Load(int3(pixel, 0));
    float4 currS4 = t_CurrSpecular.Load(int3(pixel, 0));
    float3 currD = currD4.rgb;
    float3 currS = currS4.rgb;
    float specHitDist = currS4.a;

    if (depth >= 1.0 || g_Enabled == 0) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    float2 mv = t_MotionVectors.Load(int3(pixel, 0));
    float2 histUV = (float2(pixel) + 0.5) * g_InvScreenSize - mv;
    if (any(histUV < 0.0) || any(histUV > 1.0)) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    int2 histPixel = int2(histUV * g_ScreenSize);
    histPixel = clamp(histPixel, int2(0, 0), int2(g_ScreenSize) - 1);

    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    float4 prevWorldData = t_PrevWorldPos.Load(int3(histPixel, 0));
    float3 worldPos = worldPosData.xyz;
    float3 prevWorld = prevWorldData.xyz;
    float3 prevN = normalize(t_PrevNormal.Load(int3(histPixel, 0)).xyz);
    float3 N = normalize(t_Normal.Load(int3(pixel, 0)).xyz);

    if (!SameHudSurfClass(worldPosData.w, prevWorldData.w)) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }

    float posErr = length(worldPos - prevWorld) / max(length(worldPos), 1.0);
    float nDot = saturate(dot(N, prevN));
    float trust = 1.0;
    if (posErr > 0.08 || nDot < 0.7) {
        u_OutDiffuse[pixel] = float4(currD, 1.0);
        u_OutSpecular[pixel] = float4(currS, specHitDist);
        return;
    }
    if (posErr > 0.03)
        trust *= saturate(1.0 - (posErr - 0.03) / 0.05);
    if (nDot < 0.9)
        trust *= saturate((nDot - 0.7) / 0.2);

    float3 histD = t_HistDiffuse.Load(int3(histPixel, 0)).rgb;
    float3 histS = t_HistSpecular.Load(int3(histPixel, 0)).rgb;

    float3 minD = currD, maxD = currD, minS = currS, maxS = currS;
    [unroll] for (int iy = -1; iy <= 1; ++iy) {
        [unroll] for (int ix = -1; ix <= 1; ++ix) {
            int2 np = int2(pixel) + int2(ix, iy);
            if (np.x < 0 || np.y < 0 || np.x >= (int)g_ScreenSize.x || np.y >= (int)g_ScreenSize.y)
                continue;
            if (!SameHudSurfClass(worldPosData.w, t_WorldPos.Load(int3(np, 0)).w))
                continue;
            float3 d = t_CurrDiffuse.Load(int3(np, 0)).rgb;
            float3 s = t_CurrSpecular.Load(int3(np, 0)).rgb;
            minD = min(minD, d);
            maxD = max(maxD, d);
            minS = min(minS, s);
            maxS = max(maxS, s);
        }
    }

    histD = ClipAABB(histD, minD * 0.15, maxD * 4.5);
    histS = ClipAABB(histS, minS * 0.1, maxS * 5.0);

    float alpha = saturate(g_Alpha) * trust;
    float3 outD = lerp(currD, histD, alpha);
    float3 outS = lerp(currS, histS, alpha * 0.96);
    float currLd = max(Luma(currD), 1e-4);
    float histLd = max(Luma(outD), 1e-4);
    if (currLd > histLd * 3.5)
        outD *= (histLd * 3.5) / currLd;
    float currLs = max(Luma(currS), 1e-4);
    float histLs = max(Luma(outS), 1e-4);
    if (currLs > histLs * 4.5)
        outS *= (histLs * 4.5) / currLs;

    u_OutDiffuse[pixel] = float4(outD, 1.0);
    u_OutSpecular[pixel] = float4(outS, specHitDist);
}
