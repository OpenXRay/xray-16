#include "common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"

cbuffer DlssRrGuideParams : register(b5) {
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_Pad;
};

Texture2D<float4> g_BaseColor : register(t0);
Texture2D<float4> g_Normal : register(t1);
Texture2D<float4> g_WorldPos : register(t2);
Texture2D<float4> g_NoisySpecular : register(t3);
Texture2D<float> g_Depth : register(t4);

RWTexture2D<float4> u_DiffuseAlbedo : register(u0);
RWTexture2D<float4> u_SpecularAlbedo : register(u1);
RWTexture2D<float> u_SpecularHitDist : register(u2);

float3 EnvBRDFApprox2(float3 specularColor, float alpha, float NoV)
{
    NoV = abs(NoV);
    float4 X = float4(1.0, NoV, NoV * NoV, NoV * NoV * NoV);
    float4 Y = float4(1.0, alpha, alpha * alpha, alpha * alpha * alpha);
    float2x2 M1 = float2x2(0.99044, -1.28514, 1.29678, -0.755907);
    float3x3 M2 = float3x3(1.0, 2.92338, 59.4188, 20.3225, -27.0302, 222.592, 121.563, 626.13, 316.627);
    float2x2 M3 = float2x2(0.0365463, 3.32707, 9.0632, -9.04756);
    float3x3 M4 = float3x3(1.0, 3.59685, -1.36772, 9.04401, -16.3174, 9.22949, 5.56589, 19.7886, -20.2123);
    float bias = dot(mul(M1, X.xy), Y.xy) * rcp(dot(mul(M2, X.xyw), Y.xyw));
    float scale = dot(mul(M3, X.xy), Y.xy) * rcp(dot(mul(M4, X.xzw), Y.xyw));
    bias *= saturate(specularColor.g * 50.0);
    return mad(specularColor, max(0.0, scale), max(0.0, bias));
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixel = dtid.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = g_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0)
    {
        u_DiffuseAlbedo[pixel] = float4(1.0, 1.0, 1.0, 1.0);
        u_SpecularAlbedo[pixel] = 0;
        u_SpecularHitDist[pixel] = 0;
        return;
    }

    float4 base = g_BaseColor.Load(int3(pixel, 0));
    float4 nrm = g_Normal.Load(int3(pixel, 0));
    float3 worldPos = g_WorldPos.Load(int3(pixel, 0)).xyz;
    float3 N = normalize(nrm.xyz);
    float roughness = saturate(nrm.w);
    float metallic = UnpackMetallicFromBaseA(base.a);
    float3 albedo = max(base.rgb, 0.0);
    float3 F0 = CalculateF0(albedo, metallic);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float NoV = saturate(dot(N, V));
    float alpha = roughness * roughness;

    float3 diffAlb = albedo * (1.0 - metallic);
    float3 specAlb = EnvBRDFApprox2(F0, alpha, NoV);
    float specHit = max(g_NoisySpecular.Load(int3(pixel, 0)).a, 0.0);

    u_DiffuseAlbedo[pixel] = float4(diffAlb, 1.0);
    u_SpecularAlbedo[pixel] = float4(specAlb, 1.0);
    u_SpecularHitDist[pixel] = specHit;
}
