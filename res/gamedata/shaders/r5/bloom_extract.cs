cbuffer BloomParams : register(b5)
{
    float2 g_SrcSize;
    float2 g_DstSize;
    float g_Threshold;
    float g_Intensity;
    float2 g_Pad;
};

Texture2D<float4> t_Hdr : register(t0);
Texture2D<float> t_exposure : register(t1);
Texture2D<float> t_depth : register(t2);
Texture2D<float4> t_WorldPos : register(t3);
RWTexture2D<float4> u_Bloom : register(u0);

#include "shared/surface_marks.h"

float3 HighTap(float3 rgb, float depth, float scale)
{
    float3 lin = rgb * scale;
    float defHdr = 9.0;
    if (depth <= 1e-7)
    {
        lin *= 2.0;
        defHdr = 3.0;
    }
    return lin / defHdr;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 dst = id.xy;
    if (dst.x >= (uint)g_DstSize.x || dst.y >= (uint)g_DstSize.y)
        return;

    int2 src = int2((float2(dst) + 0.5) * g_SrcSize / g_DstSize);
    int2 maxS = int2(g_SrcSize) - 1;

    uint dw, dh;
    t_depth.GetDimensions(dw, dh);
    float2 depthScale = float2(dw, dh) / max(g_SrcSize, float2(1.0, 1.0));

    float scale = t_exposure.Load(int3(0, 0, 0));
    scale = clamp(scale, 1.0 / 128.0, 20.0);

    int2 o0 = clamp(src + int2(-1, -1), int2(0, 0), maxS);
    int2 o1 = clamp(src + int2( 1, -1), int2(0, 0), maxS);
    int2 o2 = clamp(src + int2(-1,  1), int2(0, 0), maxS);
    int2 o3 = clamp(src + int2( 1,  1), int2(0, 0), maxS);

    float3 c0 = t_Hdr.Load(int3(o0, 0)).rgb;
    float3 c1 = t_Hdr.Load(int3(o1, 0)).rgb;
    float3 c2 = t_Hdr.Load(int3(o2, 0)).rgb;
    float3 c3 = t_Hdr.Load(int3(o3, 0)).rgb;

    int2 d0 = clamp(int2(float2(o0) * depthScale), int2(0, 0), int2(dw, dh) - 1);
    int2 d1 = clamp(int2(float2(o1) * depthScale), int2(0, 0), int2(dw, dh) - 1);
    int2 d2 = clamp(int2(float2(o2) * depthScale), int2(0, 0), int2(dw, dh) - 1);
    int2 d3 = clamp(int2(float2(o3) * depthScale), int2(0, 0), int2(dw, dh) - 1);

    uint ww, wh;
    t_WorldPos.GetDimensions(ww, wh);
    float2 wpScale = float2(ww, wh) / max(g_SrcSize, float2(1.0, 1.0));
    int2 w0 = clamp(int2(float2(o0) * wpScale), int2(0, 0), int2(ww, wh) - 1);
    int2 w1 = clamp(int2(float2(o1) * wpScale), int2(0, 0), int2(ww, wh) - 1);
    int2 w2 = clamp(int2(float2(o2) * wpScale), int2(0, 0), int2(ww, wh) - 1);
    int2 w3 = clamp(int2(float2(o3) * wpScale), int2(0, 0), int2(ww, wh) - 1);
    float hud0 = IsHudSurfMark(t_WorldPos.Load(int3(w0, 0)).w) ? 0.0 : 1.0;
    float hud1 = IsHudSurfMark(t_WorldPos.Load(int3(w1, 0)).w) ? 0.0 : 1.0;
    float hud2 = IsHudSurfMark(t_WorldPos.Load(int3(w2, 0)).w) ? 0.0 : 1.0;
    float hud3 = IsHudSurfMark(t_WorldPos.Load(int3(w3, 0)).w) ? 0.0 : 1.0;
    float hudW = hud0 + hud1 + hud2 + hud3;

    float3 s0 = HighTap(c0, t_depth.Load(int3(d0, 0)), scale) * hud0;
    float3 s1 = HighTap(c1, t_depth.Load(int3(d1, 0)), scale) * hud1;
    float3 s2 = HighTap(c2, t_depth.Load(int3(d2, 0)), scale) * hud2;
    float3 s3 = HighTap(c3, t_depth.Load(int3(d3, 0)), scale) * hud3;

    float3 avg = (hudW > 0.5) ? ((s0 + s1) + (s2 + s3)) * (2.0 / hudW) : 0.0;
    float hi = max(dot(avg, float3(1.0, 1.0, 1.0)) - g_Threshold, 0.0);
    hi *= max(g_Intensity, 0.5);
    u_Bloom[dst] = float4(avg, hi);
}
