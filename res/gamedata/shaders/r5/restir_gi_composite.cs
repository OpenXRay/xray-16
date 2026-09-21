#include "restir_gi_common.h"

cbuffer CompositeParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_DiffuseMode;
};

Texture2D<float4> t_DirectLighting : register(t0);
Texture2D<float4> t_ReservoirA : register(t1);
Texture2D<float4> t_ReservoirB : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_BaseColor : register(t5);
Texture2D<float4> t_Normal : register(t8);
Texture2D<float2> t_Material : register(t17);

RWTexture2D<float4> u_SceneColor : register(u0);
RWTexture2D<float4> u_IndirectLighting : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    if (depth <= 0.0 || dot(normalData.xyz, normalData.xyz) < 0.25) {
        return;
    }

    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;

    GIReservoir r = UnpackReservoir(
        t_ReservoirA.Load(int3(pixel, 0)),
        t_ReservoirB.Load(int3(pixel, 0))
    );

    float3 indirect = 0;
    if (IsReservoirValid(r) && r.W > 0) {
        float2 giUV = (float2(pixel) + 0.5) / g_ScreenSize;
        float4 giClip = float4(giUV.x * 2.0 - 1.0, 1.0 - giUV.y * 2.0, depth, 1.0);
        if (depth >= 0.9)
            giClip.z = (depth - 0.9) * 10.0;
        float4 giWorld = mul(g_InvViewProj, giClip);
        float3 worldPos = giWorld.xyz / giWorld.w;

        MaterialSurface primary = GBufferMaterialSurface(normalData, t_BaseColor.Load(int3(pixel, 0)), t_Material.Load(int3(pixel, 0)));
        float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, primary.N);

        indirect = GITargetRadiance(primary, V, worldPos, r.samplePos, r.Lo, g_DiffuseMode) * r.W;
        indirect = min(indirect, RESTIR_MAX_RADIANCE);
    }

    indirect = (u_IndirectLighting[pixel].rgb + indirect) * g_GIIntensity;
    u_IndirectLighting[pixel] = float4(indirect, 1.0);
    float3 emission = u_SceneColor[pixel].rgb;
    u_SceneColor[pixel] = float4(emission + direct + indirect, 1.0);
}
