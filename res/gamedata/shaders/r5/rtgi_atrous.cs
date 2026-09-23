#include "rtgi_reconstruct_common.h"

cbuffer RTGIFilterParams : register(b5)
{
    uint g_Width, g_Height, g_StepSize, g_Iteration;
    float g_PhiColor, g_PhiNormal, g_FilterPad0, g_FilterPad1;
};

Texture2D<float4> t_Diffuse : register(t0);
Texture2D<float4> t_Specular : register(t1);
Texture2D<float4> t_NormalRoughness : register(t2);
Texture2D<float> t_DepthGuide : register(t3);

RWTexture2D<float4> u_Diffuse : register(u0);
RWTexture2D<float4> u_Specular : register(u1);

static const float RTGI_ATROUS_KERNEL[3] = { 0.375, 0.25, 0.0625 };

bool RTGIAtrousInside(int2 pixel)
{
    return pixel.x >= 0 && pixel.y >= 0 && pixel.x < int(g_Width) && pixel.y < int(g_Height);
}

float RTGIAtrousGuide(int2 pixel)
{
    return RTGIAtrousInside(pixel) ? t_DepthGuide.Load(int3(pixel, 0)) : 0.0;
}

float2 RTGIAtrousVarianceCenter(int2 pixel)
{
    float2 sum = 0.0;
    float sumWeight = 0.0;
    for (int yy = -1; yy <= 1; ++yy)
    {
        for (int xx = -1; xx <= 1; ++xx)
        {
            int2 tap = pixel + int2(xx, yy);
            if (!RTGIAtrousInside(tap))
                continue;
            float weight = (xx == 0 ? 0.5 : 0.25) * (yy == 0 ? 0.5 : 0.25);
            sum += weight * float2(t_Diffuse.Load(int3(tap, 0)).a, t_Specular.Load(int3(tap, 0)).a);
            sumWeight += weight;
        }
    }
    return sumWeight > 0.0 ? sum / sumWeight : 0.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    int2 pixel = int2(dispatchID.xy);
    if (!RTGIAtrousInside(pixel))
        return;

    float4 diffuse = t_Diffuse.Load(int3(pixel, 0));
    float4 specular = t_Specular.Load(int3(pixel, 0));
    float guide = t_DepthGuide.Load(int3(pixel, 0));
    if (guide == 0.0)
    {
        u_Diffuse[pixel] = diffuse;
        u_Specular[pixel] = specular;
        return;
    }

    float4 normalRoughness = t_NormalRoughness.Load(int3(pixel, 0));
    float3 N = normalize(normalRoughness.xyz);
    float roughness = normalRoughness.w;
    float depth = abs(guide);
    int step = int(max(g_StepSize, 1u));
    float2 varianceCenter = RTGIAtrousVarianceCenter(pixel);
    float2 phiLum = g_PhiColor * sqrt(max(varianceCenter, 0.0) + 1e-10);
    float gradient = RTGIReconDepthGradient(guide, RTGIAtrousGuide(pixel + int2(-1, 0)), RTGIAtrousGuide(pixel + int2(1, 0)),
        RTGIAtrousGuide(pixel + int2(0, -1)), RTGIAtrousGuide(pixel + int2(0, 1)));
    float phiDepth = max(gradient, 1e-8) * float(step);
    float diffuseLum = RTGIReconSignalLuminance(diffuse.rgb);
    float specularLum = RTGIReconSignalLuminance(specular.rgb);

    float3 sumDiffuse = 0.0;
    float3 sumSpecular = 0.0;
    float sumDiffuseVariance = 0.0;
    float sumSpecularVariance = 0.0;
    float sumWeightDiffuse = 0.0;
    float sumWeightSpecular = 0.0;
    for (int yy = -2; yy <= 2; ++yy)
    {
        for (int xx = -2; xx <= 2; ++xx)
        {
            int2 tap = pixel + int2(xx, yy) * step;
            if (!RTGIAtrousInside(tap))
                continue;
            float tapGuide = t_DepthGuide.Load(int3(tap, 0));
            if (!RTGIReconGuideCompatible(guide, tapGuide))
                continue;
            float4 tapDiffuse = t_Diffuse.Load(int3(tap, 0));
            float4 tapSpecular = t_Specular.Load(int3(tap, 0));
            float4 tapNormalRoughness = t_NormalRoughness.Load(int3(tap, 0));
            if (!all(isfinite(tapDiffuse)) || !all(isfinite(tapSpecular)) || !all(isfinite(tapNormalRoughness)) ||
                dot(tapNormalRoughness.xyz, tapNormalRoughness.xyz) < 0.25)
                continue;
            float kernel = RTGI_ATROUS_KERNEL[abs(xx)] * RTGI_ATROUS_KERNEL[abs(yy)];
            float weightNormal = pow(saturate(dot(N, normalize(tapNormalRoughness.xyz))), g_PhiNormal);
            float weightDepth = abs(depth - abs(tapGuide)) / (phiDepth * length(float2(xx, yy)) + RTGI_RECON_WEIGHT_EPSILON);
            float weightDiffuse = exp(-weightDepth - abs(diffuseLum - RTGIReconSignalLuminance(tapDiffuse.rgb)) /
                (phiLum.x + RTGI_RECON_WEIGHT_EPSILON)) * weightNormal * kernel;
            float weightSpecular = exp(-weightDepth - abs(specularLum - RTGIReconSignalLuminance(tapSpecular.rgb)) /
                (phiLum.y + RTGI_RECON_WEIGHT_EPSILON)) * weightNormal * kernel *
                exp(-abs(roughness - tapNormalRoughness.w) * RTGI_RECON_ROUGHNESS_SCALE);
            sumDiffuse += tapDiffuse.rgb * weightDiffuse;
            sumDiffuseVariance += tapDiffuse.a * weightDiffuse * weightDiffuse;
            sumWeightDiffuse += weightDiffuse;
            sumSpecular += tapSpecular.rgb * weightSpecular;
            sumSpecularVariance += tapSpecular.a * weightSpecular * weightSpecular;
            sumWeightSpecular += weightSpecular;
        }
    }

    float4 outDiffuse = diffuse;
    float4 outSpecular = specular;
    if (sumWeightDiffuse > RTGI_RECON_WEIGHT_EPSILON)
        outDiffuse = float4(sumDiffuse / sumWeightDiffuse, sumDiffuseVariance / (sumWeightDiffuse * sumWeightDiffuse));
    if (sumWeightSpecular > RTGI_RECON_WEIGHT_EPSILON)
        outSpecular = float4(sumSpecular / sumWeightSpecular, sumSpecularVariance / (sumWeightSpecular * sumWeightSpecular));
    if (!all(isfinite(outDiffuse)))
        outDiffuse = diffuse;
    if (!all(isfinite(outSpecular)))
        outSpecular = specular;
    u_Diffuse[pixel] = outDiffuse;
    u_Specular[pixel] = outSpecular;
}
