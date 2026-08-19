#include "vol_fog_common.h"
#include "vol_fog_params.h"
#include "shared/surface_marks.h"
#include "atmosphere.h"

Texture2D<float> t_Depth : register(t0);
Texture3D<float4> t_Accum : register(t1);
Texture2D<float4> t_WorldPos : register(t2);
TextureCube<float4> g_Sky0 : register(t3);
TextureCube<float4> g_Sky1 : register(t4);
RWTexture2D<float4> u_SceneColor : register(u0);

SamplerState smp_linear : register(s0);

float3 SampleSkyIncident(float3 dir, float mip)
{
    float3 d = normalize(dir);
    float3 s0 = g_Sky0.SampleLevel(smp_linear, d, mip).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, d, mip).rgb;
    float3 sky = lerp(s0, s1, saturate(g_SkyColor.w)) * g_SkyColor.rgb * 0.80;
    if (dot(sky, sky) < 1e-6)
        sky = g_HemiColor.rgb;
    return sky;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float2 uv = (float2(pixel) + 0.5) / g_ScreenSize;
    float4 wpSamp = t_WorldPos.Load(int3(pixel, 0));
    const bool isHud = IsHudSurfMark(wpSamp.w);
    const bool isSky = (depth <= 1e-7) && !isHud;

    float4 clipFar = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    float4 worldFarH = mul(g_InvViewProj, clipFar);
    float3 worldFar = worldFarH.xyz / max(worldFarH.w, 1e-8);
    float3 viewDir = normalize(worldFar - g_CameraPos.xyz);

    float t = 1.0;
    if (isHud)
    {
        float hudZ = length(wpSamp.xyz - g_CameraPos.xyz);
        if (hudZ < 0.2)
            hudZ = 2.0;
        hudZ = clamp(hudZ, 1.25, 3.5);
        t = VolFogViewToT(hudZ, g_ZNear, g_ZFar);
    }
    else if (!isSky)
    {
        float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
        float4 world = mul(g_InvViewProj, clip);
        float3 worldPos = world.xyz / max(world.w, 1e-8);
        float viewZ = length(worldPos - g_CameraPos.xyz);
        t = VolFogViewToT(max(viewZ, g_ZNear), g_ZNear, g_ZFar);
    }

    float4 acc = 0;
    float wsum = 0;
    int2 maxP = int2(g_ScreenSize) - 1;
    [unroll]
    for (int oy = -1; oy <= 1; oy++) {
        [unroll]
        for (int ox = -1; ox <= 1; ox++) {
            int2 np = clamp(int2(pixel) + int2(ox, oy), int2(0, 0), maxP);
            float dn = t_Depth.Load(int3(np, 0));
            float2 uvn = (float2(np) + 0.5) / g_ScreenSize;
            float4 s = t_Accum.SampleLevel(smp_linear, float3(uvn, t), 0);
            float rel = abs(dn - depth) / max(max(abs(dn), abs(depth)), 1e-5);
            float w = (ox == 0 && oy == 0) ? 1.5 : 0.55;
            w *= saturate(1.0 - rel * 14.0);
            bool nSky = dn <= 1e-7;
            if (nSky != isSky)
                w *= 0.04;
            acc += s * w;
            wsum += w;
        }
    }
    acc /= max(wsum, 1e-4);
    float T = saturate(acc.a);
    float3 inscatt = acc.rgb;

    float fogStrength = isHud ? 0.0 : 1.0;
    if (isSky)
    {
        float zenith = saturate(viewDir.y);
        float horizon = saturate(1.0 - abs(viewDir.y));
        fogStrength = lerp(0.55, 1.0, horizon);
        fogStrength *= lerp(1.0, 0.65, zenith * zenith);
    }

    float outT = lerp(1.0, T, fogStrength);
    float3 outInsc = inscatt * fogStrength;

    float4 color = u_SceneColor[pixel];
    color.rgb = color.rgb * outT + outInsc;
    if (g_EnableAtmosphere != 0 && g_AtmosphereStrength > 0.001 && isSky) {
        float3 sunDir = normalize(-g_SunDir.xyz);
        float3 Tatm, inscAtm;
        AtmosphereAerial(viewDir, 120.0, sunDir, g_SunColor.rgb, SampleSkyIncident(float3(0.0, 1.0, 0.0), 4.0), g_AtmosphereStrength, Tatm, inscAtm);
        color.rgb = color.rgb * Tatm + inscAtm;
    }
    u_SceneColor[pixel] = color;
}
