#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "restir_gi_common.h"

cbuffer CompositeParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_Pad;
};

Texture2D<float4> t_DirectLighting : register(t0);
Texture2D<float4> t_ReservoirA : register(t1);
Texture2D<float4> t_ReservoirB : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_Normal : register(t4);
Texture2D<float4> t_BaseColor : register(t5);
Texture2D<float4> t_SceneColorIn : register(t6);
Texture2D<float4> t_WorldPos : register(t7);

RWTexture2D<float4> u_SceneColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0) {
        u_SceneColor[pixel] = t_SceneColorIn.Load(int3(pixel, 0));
        return;
    }

    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;

    GIReservoir r = UnpackReservoir(
        t_ReservoirA.Load(int3(pixel, 0)),
        t_ReservoirB.Load(int3(pixel, 0))
    );

    float3 indirect = 0;
    if (IsReservoirValid(r) && r.W > 0) {
        float3 worldPos = t_WorldPos.Load(int3(pixel, 0)).xyz;
        float4 normalData = t_Normal.Load(int3(pixel, 0));
        float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));

        float3 N = normalize(normalData.xyz);
        float roughness = normalData.w;
        float3 albedo = baseColorData.rgb;
        float metallic = baseColorData.a;

        float3 wi = normalize(r.samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        float3 brdfCos = kD * albedo / PI * cosTheta;

        indirect = r.Lo * brdfCos * r.W;
        indirect = min(indirect, RESTIR_MAX_RADIANCE);
        indirect *= g_GIIntensity;
    }

    float3 finalColor = direct + indirect;
    u_SceneColor[pixel] = float4(finalColor, 1.0);
}
