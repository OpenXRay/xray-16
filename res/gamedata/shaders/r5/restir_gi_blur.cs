#include "shared/surface_marks.h"

cbuffer BlurParams : register(b5) {
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    float g_PhiNormal;
    float g_PhiDepth;
    uint g_Step;
    uint g_Mode;
};

Texture2D<float4> t_DirectLighting : register(t0);
Texture2D<float4> t_NoisyDiffuse : register(t1);
Texture2D<float> t_Depth : register(t2);
Texture2D<float4> t_Normal : register(t3);
Texture2D<float4> t_SceneColorIn : register(t4);
Texture2D<float4> t_NoisySpecular : register(t5);
Texture2D<float4> t_ClassifyWorldPos : register(t6);
Texture2D<float4> t_WorldPos : register(t7);

RWTexture2D<float4> u_SceneColor : register(u0);
RWTexture2D<float4> u_FilteredDiffuse : register(u1);
RWTexture2D<float4> u_FilteredSpecular : register(u2);

static const float kKernel[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

float Luma(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float3 SoftClampFirefly(float3 c, float centerLuma, float maxScale)
{
    float l = Luma(c);
    float limit = max(centerLuma * maxScale, 0.05);
    if (l > limit)
        c *= limit / max(l, 1e-4);
    return c;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;
    float3 noisyD = t_NoisyDiffuse.Load(int3(pixel, 0)).rgb;
    float4 noisyS4 = t_NoisySpecular.Load(int3(pixel, 0));
    float3 noisyS = noisyS4.rgb;
    float specHitDist = noisyS4.a;

    float guideMark = t_WorldPos.Load(int3(pixel, 0)).w;
    float classifyMark = t_ClassifyWorldPos.Load(int3(pixel, 0)).w;
    if (depth >= 1.0 || SkipRtSurfLighting(classifyMark, guideMark)) {
        u_SceneColor[pixel] = t_SceneColorIn.Load(int3(pixel, 0));
        u_FilteredDiffuse[pixel] = 0;
        u_FilteredSpecular[pixel] = 0;
        return;
    }

    float3 N = normalize(t_Normal.Load(int3(pixel, 0)).xyz);
    float centerLumaD = Luma(noisyD);
    float centerLumaS = Luma(noisyS);
    float directLuma = max(Luma(direct), 0.02);
    int step = (int)max(g_Step, 1u);

    float3 sumD = 0;
    float3 sumS = 0;
    float wSumD = 0;
    float wSumS = 0;
    float3 nbMinD = noisyD;
    float3 nbMaxD = noisyD;
    float3 nbMinS = noisyS;
    float3 nbMaxS = noisyS;

    [unroll] for (int iy = -2; iy <= 2; ++iy) {
        [unroll] for (int ix = -2; ix <= 2; ++ix) {
            int2 np = int2(pixel) + int2(ix, iy) * step;
            if (np.x < 0 || np.y < 0 || np.x >= (int)g_ScreenSize.x || np.y >= (int)g_ScreenSize.y)
                continue;

            float nd = t_Depth.Load(int3(np, 0));
            if (nd >= 1.0)
                continue;
            if (!SameHudSurfClass(guideMark, t_WorldPos.Load(int3(np, 0)).w))
                continue;

            float3 nN = normalize(t_Normal.Load(int3(np, 0)).xyz);
            float3 nD = t_NoisyDiffuse.Load(int3(np, 0)).rgb;
            float3 nS = t_NoisySpecular.Load(int3(np, 0)).rgb;
            nD = SoftClampFirefly(nD, directLuma, 5.0);
            nS = SoftClampFirefly(nS, directLuma, 6.5);

            nbMinD = min(nbMinD, nD);
            nbMaxD = max(nbMaxD, nD);
            nbMinS = min(nbMinS, nS);
            nbMaxS = max(nbMaxS, nS);

            float w = kKernel[ix + 2] * kKernel[iy + 2];
            float wGeo = exp(-abs(depth - nd) * g_PhiDepth);
            wGeo *= pow(saturate(dot(N, nN)), g_PhiNormal);
            w *= max(wGeo, 0.05);

            float wD = w * exp(-abs(Luma(nD) - centerLumaD) * 1.25);
            float wS = w * exp(-abs(Luma(nS) - centerLumaS) * 0.75);
            sumD += nD * wD;
            sumS += nS * wS;
            wSumD += wD;
            wSumS += wS;
        }
    }

    float3 filteredD = (wSumD > 1e-4) ? (sumD / wSumD) : SoftClampFirefly(noisyD, directLuma, 5.0);
    float3 filteredS = (wSumS > 1e-4) ? (sumS / wSumS) : SoftClampFirefly(noisyS, directLuma, 6.5);
    filteredD = clamp(filteredD, nbMinD * 0.5, nbMaxD * 1.5 + 0.01);
    filteredS = clamp(filteredS, nbMinS * 0.35, nbMaxS * 1.75 + 0.01);

    u_FilteredDiffuse[pixel] = float4(filteredD, 1.0);
    u_FilteredSpecular[pixel] = float4(filteredS, specHitDist);

    float3 ambientBase = t_SceneColorIn.Load(int3(pixel, 0)).rgb;
    if (g_Mode == 1)
        u_SceneColor[pixel] = float4(ambientBase + direct + filteredD + filteredS, 1.0);
    else
        u_SceneColor[pixel] = float4(ambientBase + direct + filteredD, 1.0);
}
