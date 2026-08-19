#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/nrd_helpers.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"
#include "rt_irradiance_cache.h"
#include "atmosphere.h"

cbuffer CompositeParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_DenoiseApply;
    float4 g_FogParams;
    float4 g_FogColor;
    float4 g_SunDir;
    float4 g_SunColor;
    float g_AmbientScale;
    uint g_CacheSize;
    float g_CacheCellSize;
    uint g_UseDdgi;
    uint g_AddDirect;
    float g_GiWidth;
    float g_GiHeight;
    uint g_ShaftWidth;
    uint g_ShaftHeight;
    uint g_FrameIndex;
    uint g_Pad1;
    uint g_Pad2;
};

Texture2D<float4> t_DirectLighting : register(t0);
StructuredBuffer<uint4> t_Reservoir : register(t1);
Texture2D<float> t_Depth : register(t2);
Texture2D<float4> t_BaseColor : register(t3);
Texture2D<float4> t_SceneColorIn : register(t4);
Texture2D<float4> t_Normal : register(t5);
Texture2D<float4> t_NoisyDiffuse : register(t6);
Texture2D<float4> t_NoisySpecular : register(t7);
Texture2D<float4> t_Sunshafts : register(t8);
Texture2D<float4> t_DDGIAmbient : register(t9);
Texture2D<float4> t_WorldPos : register(t10);
Texture2D<float4> t_SpecReservoirA : register(t11);
Texture2D<float4> t_SpecReservoirB : register(t12);
StructuredBuffer<IrradianceCacheEntry> g_IrradianceCache : register(t13);
Texture2D<float> t_SkyOpen : register(t14);
TextureCube<float4> g_Sky0 : register(t15);
TextureCube<float4> g_Sky1 : register(t16);

RWTexture2D<float4> u_SceneColor : register(u0);
SamplerState smp_linear : register(s0);

float3 SampleSkyIncident(float3 dir, float mip)
{
    float3 d = normalize(dir);
    float3 s0 = g_Sky0.SampleLevel(smp_linear, d, mip).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, d, mip).rgb;
    float3 sky = lerp(s0, s1, saturate(g_CameraPos.w)) * g_FogColor.rgb * 0.80;
    if (dot(sky, sky) < 1e-6)
        sky = g_FogColor.rgb * 0.80;
    return sky;
}

float3 UpsampleHalfDepthAware(Texture2D<float4> tex, float2 halfSize, float2 uv, float centerDepth)
{
    float2 hs = max(halfSize, float2(1.0, 1.0));
    float2 p = uv * hs - 0.5;
    int2 i0 = int2(floor(p));
    float2 f = saturate(p - float2(i0));
    int2 maxP = int2(hs) - 1;
    int2 c00 = clamp(i0 + int2(0, 0), int2(0, 0), maxP);
    int2 c10 = clamp(i0 + int2(1, 0), int2(0, 0), maxP);
    int2 c01 = clamp(i0 + int2(0, 1), int2(0, 0), maxP);
    int2 c11 = clamp(i0 + int2(1, 1), int2(0, 0), maxP);

    float2 fullSize = g_ScreenSize;
    float d00 = RestirLoadDepth(t_Depth, uint2(c00), hs, fullSize);
    float d10 = RestirLoadDepth(t_Depth, uint2(c10), hs, fullSize);
    float d01 = RestirLoadDepth(t_Depth, uint2(c01), hs, fullSize);
    float d11 = RestirLoadDepth(t_Depth, uint2(c11), hs, fullSize);

    float3 v00 = tex.Load(int3(c00, 0)).rgb;
    float3 v10 = tex.Load(int3(c10, 0)).rgb;
    float3 v01 = tex.Load(int3(c01, 0)).rgb;
    float3 v11 = tex.Load(int3(c11, 0)).rgb;

    float w00 = (1.0 - f.x) * (1.0 - f.y) / (1e-3 + abs(d00 - centerDepth) * 80.0);
    float w10 = f.x * (1.0 - f.y) / (1e-3 + abs(d10 - centerDepth) * 80.0);
    float w01 = (1.0 - f.x) * f.y / (1e-3 + abs(d01 - centerDepth) * 80.0);
    float w11 = f.x * f.y / (1e-3 + abs(d11 - centerDepth) * 80.0);
    float wSum = w00 + w10 + w01 + w11;
    if (wSum < 1e-6) {
        float best = abs(d00 - centerDepth);
        float3 bestV = v00;
        float e10 = abs(d10 - centerDepth);
        float e01 = abs(d01 - centerDepth);
        float e11 = abs(d11 - centerDepth);
        if (e10 < best) { best = e10; bestV = v10; }
        if (e01 < best) { best = e01; bestV = v01; }
        if (e11 < best) { bestV = v11; }
        return bestV;
    }
    return (v00 * w00 + v10 * w10 + v01 * w01 + v11 * w11) / wSum;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    uint width = (uint)g_ScreenSize.x;
    uint height = (uint)g_ScreenSize.y;
    if (pixel.x >= width || pixel.y >= height)
        return;

    float4 sceneIn = t_SceneColorIn.Load(int3(pixel, 0));
    float2 uv = (float2(pixel) + 0.5) / g_ScreenSize;
    float depth = t_Depth.Load(int3(pixel, 0));
    float3 shafts = UpsampleHalfDepthAware(t_Sunshafts, max(float2(g_ShaftWidth, g_ShaftHeight), float2(1.0, 1.0)), uv, depth);

    if (depth <= 0.0 || depth >= 1.0) {
        u_SceneColor[pixel] = float4(sceneIn.rgb + shafts, 1.0);
        return;
    }

    if (IsWaterSurfMark(t_WorldPos.Load(int3(pixel, 0)).w)) {
        u_SceneColor[pixel] = float4(sceneIn.rgb + shafts, 1.0);
        return;
    }

    if (g_DenoiseApply != 0) {
        u_SceneColor[pixel] = float4(sceneIn.rgb + shafts, 1.0);
        return;
    }

    float2 giSize = max(float2(g_GiWidth, g_GiHeight), float2(1.0, 1.0));
    const bool fullGi = giSize.x >= g_ScreenSize.x && giSize.y >= g_ScreenSize.y;
    float3 direct = 0;
    if (g_AddDirect != 0)
        direct = fullGi ? t_DirectLighting.Load(int3(pixel, 0)).rgb : UpsampleHalfDepthAware(t_DirectLighting, giSize, uv, depth);
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, t_WorldPos.Load(int3(pixel, 0)), g_InvViewProj);

    float3 indirect = (fullGi ? t_NoisyDiffuse.Load(int3(pixel, 0)).rgb : UpsampleHalfDepthAware(t_NoisyDiffuse, giSize, uv, depth)) * g_GIIntensity;
    float3 specular = fullGi ? t_NoisySpecular.Load(int3(pixel, 0)).rgb : UpsampleHalfDepthAware(t_NoisySpecular, giSize, uv, depth);

    if (g_AddDirect != 0)
    {
        float dLum = max(Luminance(max(direct, 0.xxx)), 0.04);
        float maxInd = max(dLum * 3.0 + 0.4, 4.0);
        float iLum = Luminance(indirect);
        if (iLum > maxInd)
            indirect *= maxInd / iLum;
    }

    float3 ambient = 0;
    if (g_UseDdgi != 0)
        ambient = fullGi ? t_DDGIAmbient.Load(int3(pixel, 0)).rgb : UpsampleHalfDepthAware(t_DDGIAmbient, giSize, uv, depth);
    float3 lighting = direct + indirect + specular + ambient + shafts;
    if (any(isnan(lighting)))
        lighting = max(indirect + specular + ambient + shafts, 0.xxx);

    float dist = length(worldPos - g_CameraPos.xyz);
    float fog = saturate(dist * g_FogParams.w + g_FogParams.x);
    float3 outRgb = max(sceneIn.rgb + lighting * (1.0 - fog), 0.xxx);
    float farW = saturate((dist - 28.0) / 55.0);
    if (g_FogColor.w > 0.001 && any(g_FogParams.yzw > 0) && farW > 0.001) {
        float3 viewDir = normalize(worldPos - g_CameraPos.xyz);
        float3 sunDir = normalize(-g_SunDir.xyz);
        float3 Tatm, inscAtm;
        AtmosphereAerial(viewDir, dist, sunDir, g_SunColor.rgb, SampleSkyIncident(float3(0.0, 1.0, 0.0), 4.0), g_FogColor.w * farW, Tatm, inscAtm);
        outRgb = outRgb * Tatm + inscAtm;
    }
    if (any(isnan(outRgb)))
        outRgb = sceneIn.rgb;
    u_SceneColor[pixel] = float4(outRgb, sceneIn.a);
}
