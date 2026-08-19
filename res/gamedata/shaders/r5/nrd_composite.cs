#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/nrd_helpers.h"
#include "shared/surface_marks.h"

cbuffer NrdCompositeParams : register(b5) {
    float4 g_Params;
    float4 g_CameraPos;
    float4 g_FogParams;
    float4 g_FogColor;
    float4x4 g_InvViewProj;
};

Texture2D<float4> t_DirectLighting : register(t0);
Texture2D<float4> t_DenoisedDiffuse : register(t1);
Texture2D<float4> t_DenoisedSpecular : register(t2);
Texture2D<float> t_Depth : register(t3);
Texture2D<float4> t_SceneColorIn : register(t4);
Texture2D<float4> t_BaseColor : register(t5);
Texture2D<float4> t_Normal : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_ClassifyWorldPos : register(t8);

RWTexture2D<float4> u_SceneColor : register(u0);
RWTexture2D<float4> u_OutDiffuse : register(u1);
RWTexture2D<float4> u_OutSpecular : register(u2);

float3 ReconstructWorldPosReverseZ(float2 uv, float depth, float4x4 invViewProj)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = mul(invViewProj, clip);
    return world.xyz / max(world.w, 1e-8);
}

float Luminance(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    float2 screenSize = g_Params.xy;
    uint method = (uint)g_Params.z;
    if (pixel.x >= (uint)screenSize.x || pixel.y >= (uint)screenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0) {
        float4 passthrough = t_SceneColorIn.Load(int3(pixel, 0));
        u_SceneColor[pixel] = passthrough;
        u_OutDiffuse[pixel] = 0;
        u_OutSpecular[pixel] = 0;
        return;
    }

    float guideMark = t_WorldPos.Load(int3(pixel, 0)).w;
    float classifyMark = t_ClassifyWorldPos.Load(int3(pixel, 0)).w;
    if (SkipRtSurfLighting(classifyMark, guideMark)) {
        u_SceneColor[pixel] = t_SceneColorIn.Load(int3(pixel, 0));
        u_OutDiffuse[pixel] = 0;
        u_OutSpecular[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / screenSize;
    float3 worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);

    float3 direct = t_DirectLighting.Load(int3(pixel, 0)).rgb;
    float4 dIn = t_DenoisedDiffuse.Load(int3(pixel, 0));
    float4 sIn = t_DenoisedSpecular.Load(int3(pixel, 0));
    float4 baseColor = t_BaseColor.Load(int3(pixel, 0));
    float4 nData = t_Normal.Load(int3(pixel, 0));
    float3 N = normalize(nData.xyz);
    float roughness = max(saturate(abs(nData.w)), MIN_ROUGHNESS);
    float3 albedo = max(baseColor.rgb, 0.0);
    float metallic = UnpackMetallicFromBaseA(baseColor.a);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 F0 = CalculateF0(albedo, metallic);

    float3 diffFactor, specFactor;
    NRD_MaterialFactors(N, V, albedo, F0, roughness, diffFactor, specFactor);

    float3 diff = (method == 0) ? NRD_YCoCgToLinear(dIn.xyz) : max(dIn.xyz, 0.0);
    float3 spec = (method == 0) ? NRD_YCoCgToLinear(sIn.xyz) : max(sIn.xyz, 0.0);
    diff = NRD_SanitizeRadiance(diff) * diffFactor;
    spec = NRD_SanitizeRadiance(spec) * specFactor;

    float giIntensity = max(g_CameraPos.w, 0.0);
    diff *= giIntensity;
    spec *= giIntensity;

    {
        float dLum = max(Luminance(direct), 0.04);
        float maxInd = dLum * 4.0 + 0.12;
        float3 ind = diff + spec;
        float iLum = Luminance(ind);
        if (iLum > maxInd) {
            float s = maxInd / iLum;
            diff *= s;
            spec *= s;
        }
    }

    float rejitter = 1.0;
    if (g_Params.w > 0.5) {
        float3 Ne = normalize(t_Normal.Load(int3(int2(pixel) + int2(1, 0), 0)).xyz);
        float3 Nw = normalize(t_Normal.Load(int3(int2(pixel) + int2(-1, 0), 0)).xyz);
        float3 Nn = normalize(t_Normal.Load(int3(int2(pixel) + int2(0, 1), 0)).xyz);
        float3 Ns = normalize(t_Normal.Load(int3(int2(pixel) + int2(0, -1), 0)).xyz);
        float edge = 1.0 - 0.25 * (saturate(dot(N, Ne)) + saturate(dot(N, Nw)) + saturate(dot(N, Nn)) + saturate(dot(N, Ns)));
        float3 L = normalize(diff + spec + 1e-4);
        float lobe = saturate(dot(N, L));
        rejitter = lerp(1.08, 0.92, saturate(edge * 2.0)) * lerp(1.05, 0.97, lobe);
        diff *= rejitter;
        spec *= rejitter;
    }

    u_OutDiffuse[pixel] = float4(diff, 1.0);
    u_OutSpecular[pixel] = float4(spec, 1.0);
    float3 ambientBase = t_SceneColorIn.Load(int3(pixel, 0)).rgb;
    float sunL = Luminance(direct);
    float giShade = saturate(1.0 - sunL * 2.5);
    giShade = lerp(0.55, 1.0, giShade);
    float3 lighting = direct + (diff + spec) * giShade;
    float dist = length(worldPos - g_CameraPos.xyz);
    float fog = saturate(dist * g_FogParams.w + g_FogParams.x);
    u_SceneColor[pixel] = float4(ambientBase + lighting * (1.0 - fog), 1.0);
}
