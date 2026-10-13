#include "rtgi_reconstruct_common.h"

cbuffer RTGIFilterParams
{
    uint g_Width, g_Height, g_StepSize, g_Iteration;
    float g_PhiColor, g_PhiNormal, g_FilterPad0, g_FilterPad1;
};

Texture2D<float4> t_HistoryDiffuse;
Texture2D<float4> t_HistorySpecular;
Texture2D<float4> t_Moments;
Texture2D<float4> t_NormalRoughness;
Texture2D<float4> t_SurfaceData;

RWTexture2D<float4> u_Diffuse;
RWTexture2D<float4> u_Specular;
RWTexture2D<float> u_DepthGuide;

struct RTGIVarianceSample
{
    float4 diffuse;
    float4 specular;
    float4 moments;
    float4 normalRoughness;
    float guide;
    bool valid;
};

RTGIVarianceSample RTGIVarianceLoad(int2 pixel)
{
    RTGIVarianceSample result;
    result.diffuse = t_HistoryDiffuse.Load(int3(pixel, 0));
    result.specular = t_HistorySpecular.Load(int3(pixel, 0));
    result.moments = t_Moments.Load(int3(pixel, 0));
    result.normalRoughness = t_NormalRoughness.Load(int3(pixel, 0));
    float4 surfaceData = t_SurfaceData.Load(int3(pixel, 0));
    result.valid = result.diffuse.a != 0.0 && all(isfinite(result.diffuse)) && all(isfinite(result.specular)) &&
        all(isfinite(result.moments)) && all(isfinite(result.normalRoughness)) && all(isfinite(surfaceData)) &&
        dot(result.normalRoughness.xyz, result.normalRoughness.xyz) >= 0.25;
    result.guide = RTGIReconDepthGuide(surfaceData, result.valid);
    result.valid = result.valid && result.guide != 0.0;
    return result;
}

float RTGIVarianceGuide(int2 pixel)
{
    if (pixel.x < 0 || pixel.y < 0 || pixel.x >= int(g_Width) || pixel.y >= int(g_Height))
        return 0.0;
    return RTGIVarianceLoad(pixel).guide;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    int2 pixel = int2(dispatchID.xy);
    if (pixel.x >= int(g_Width) || pixel.y >= int(g_Height))
        return;

    RTGIVarianceSample center = RTGIVarianceLoad(pixel);
    u_DepthGuide[pixel] = center.guide;
    if (!center.valid)
    {
        u_Diffuse[pixel] = 0.0;
        u_Specular[pixel] = 0.0;
        return;
    }

    float diffuseLength = max(RTGIReconHistoryLength(center.diffuse.a), 1.0);
    float specularLength = max(center.specular.a, 1.0);
    float4 outDiffuse = float4(center.diffuse.rgb, max(center.moments.y - center.moments.x * center.moments.x, 0.0));
    float4 outSpecular = float4(center.specular.rgb, max(center.moments.w - center.moments.z * center.moments.z, 0.0));
    bool diffuseSpatial = diffuseLength < RTGI_RECON_VARIANCE_HISTORY;
    bool specularSpatial = specularLength < RTGI_RECON_VARIANCE_HISTORY;
    if (diffuseSpatial || specularSpatial)
    {
        float3 N = normalize(center.normalRoughness.xyz);
        float roughness = center.normalRoughness.w;
        float depth = abs(center.guide);
        float gradient = RTGIReconDepthGradient(center.guide, RTGIVarianceGuide(pixel + int2(-1, 0)),
            RTGIVarianceGuide(pixel + int2(1, 0)), RTGIVarianceGuide(pixel + int2(0, -1)), RTGIVarianceGuide(pixel + int2(0, 1)));
        float phiDepth = max(gradient, 1e-8) * 3.0;
        float diffuseLum = RTGIReconSignalLuminance(center.diffuse.rgb);
        float specularLum = RTGIReconSignalLuminance(center.specular.rgb);

        float3 sumDiffuse = 0.0;
        float3 sumSpecular = 0.0;
        float2 sumDiffuseMoments = 0.0;
        float2 sumSpecularMoments = 0.0;
        float sumWeightDiffuse = 0.0;
        float sumWeightSpecular = 0.0;
        for (int yy = -3; yy <= 3; ++yy)
        {
            for (int xx = -3; xx <= 3; ++xx)
            {
                int2 tap = pixel + int2(xx, yy);
                if (tap.x < 0 || tap.y < 0 || tap.x >= int(g_Width) || tap.y >= int(g_Height))
                    continue;
                RTGIVarianceSample neighbor = RTGIVarianceLoad(tap);
                if (!neighbor.valid || !RTGIReconGuideCompatible(center.guide, neighbor.guide))
                    continue;
                float weightNormal = pow(saturate(dot(N, normalize(neighbor.normalRoughness.xyz))), g_PhiNormal);
                float weightDepth = abs(depth - abs(neighbor.guide)) / (phiDepth * length(float2(xx, yy)) + RTGI_RECON_WEIGHT_EPSILON);
                float weightDiffuse = exp(-weightDepth - abs(diffuseLum - RTGIReconSignalLuminance(neighbor.diffuse.rgb)) / g_PhiColor) * weightNormal;
                float weightSpecular = exp(-weightDepth - abs(specularLum - RTGIReconSignalLuminance(neighbor.specular.rgb)) / g_PhiColor) *
                    weightNormal * exp(-abs(roughness - neighbor.normalRoughness.w) * RTGI_RECON_ROUGHNESS_SCALE);
                sumDiffuse += neighbor.diffuse.rgb * weightDiffuse;
                sumDiffuseMoments += neighbor.moments.xy * weightDiffuse;
                sumWeightDiffuse += weightDiffuse;
                sumSpecular += neighbor.specular.rgb * weightSpecular;
                sumSpecularMoments += neighbor.moments.zw * weightSpecular;
                sumWeightSpecular += weightSpecular;
            }
        }
        if (diffuseSpatial && sumWeightDiffuse > RTGI_RECON_WEIGHT_EPSILON)
        {
            float2 moments = sumDiffuseMoments / sumWeightDiffuse;
            outDiffuse = float4(sumDiffuse / sumWeightDiffuse,
                max(moments.y - moments.x * moments.x, 0.0) * (RTGI_RECON_VARIANCE_HISTORY / diffuseLength));
        }
        if (specularSpatial && sumWeightSpecular > RTGI_RECON_WEIGHT_EPSILON)
        {
            float2 moments = sumSpecularMoments / sumWeightSpecular;
            outSpecular = float4(sumSpecular / sumWeightSpecular,
                max(moments.y - moments.x * moments.x, 0.0) * (RTGI_RECON_VARIANCE_HISTORY / specularLength));
        }
    }

    if (!all(isfinite(outDiffuse)))
        outDiffuse = float4(center.diffuse.rgb, 0.0);
    if (!all(isfinite(outSpecular)))
        outSpecular = float4(center.specular.rgb, 0.0);
    u_Diffuse[pixel] = outDiffuse;
    u_Specular[pixel] = outSpecular;
}
