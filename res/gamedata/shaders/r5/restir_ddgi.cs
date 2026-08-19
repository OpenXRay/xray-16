#include "common.h"
#include "rt_common.h"
#include "restir_gi_common.h"

cbuffer DDGIParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_GridOrigin_Spacing;
    float4 g_GridDims_Intensity;
    float2 g_ScreenSize;
    uint g_FrameIndex;
    float g_EnvAdapt;
};

Texture2D<float> t_Depth : register(t0);
Texture2D<float4> t_Normal : register(t1);
Texture2D<float4> t_BaseColor : register(t2);
Texture2D<float4> t_DirectLighting : register(t3);
RWTexture3D<float4> u_ProbeIrradiance : register(u0);
RWTexture2D<float4> u_AmbientOut : register(u1);

float3 ProbeIndexToWorld(uint3 idx)
{
    return g_GridOrigin_Spacing.xyz + (float3(idx) + 0.5) * g_GridOrigin_Spacing.w;
}

uint3 WorldToProbeIndex(float3 worldPos)
{
    float3 local = (worldPos - g_GridOrigin_Spacing.xyz) / max(g_GridOrigin_Spacing.w, 1e-3);
    uint3 dims = (uint3)g_GridDims_Intensity.xyz;
    return uint3(
        clamp((int)local.x, 0, (int)dims.x - 1),
        clamp((int)local.y, 0, (int)dims.y - 1),
        clamp((int)local.z, 0, (int)dims.z - 1));
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
    if (depth <= 0.0) {
        u_AmbientOut[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / giSize;
    float3 worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);
    float3 N = normalize(RestirLoadTex4(t_Normal, pixel, giSize, fullSize).xyz);
    float3 albedo = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize).rgb;
    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;

    uint3 pidx = WorldToProbeIndex(worldPos);
    float3 probePos = ProbeIndexToWorld(pidx);
    float3 toProbe = normalize(probePos - worldPos);
    float weight = saturate(dot(N, toProbe) * 0.5 + 0.5);

    float3 irradiance = u_ProbeIrradiance[pidx].rgb * g_EnvAdapt;
    float3 gather = (direct * 0.15 + albedo * 0.05) * weight;
    irradiance = lerp(irradiance, gather, 0.05);
    u_ProbeIrradiance[pidx] = float4(irradiance, 1.0);

    float intensity = g_GridDims_Intensity.w;
    float3 ambient = irradiance * albedo / PI * intensity;
    u_AmbientOut[pixel] = float4(min(ambient, RESTIR_MAX_RADIANCE), 1.0);
}
