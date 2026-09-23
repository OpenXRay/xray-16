#include "rtgi_reconstruct_common.h"

cbuffer RTGICompositeParams : register(b5)
{
    uint width, height, remodulate, pad1;
};

Texture2D<float4> t_Diffuse : register(t0);
Texture2D<float4> t_Specular : register(t1);
Texture2D<float4> t_Emission : register(t2);
Texture2D<float4> t_AlbedoMetallic : register(t3);

RWTexture2D<float4> u_SceneColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= width || pixel.y >= height)
        return;

    float3 emission = t_Emission.Load(int3(pixel, 0)).rgb;
    float3 diffuse = t_Diffuse.Load(int3(pixel, 0)).rgb;
    float3 specular = t_Specular.Load(int3(pixel, 0)).rgb;
    if (!all(isfinite(emission)))
        emission = 0.0;
    if (!all(isfinite(diffuse)))
        diffuse = 0.0;
    if (!all(isfinite(specular)))
        specular = 0.0;
    if (remodulate != 0u)
    {
        float4 albedoMetallic = t_AlbedoMetallic.Load(int3(pixel, 0));
        if (!all(isfinite(albedoMetallic)))
            albedoMetallic = 0.0;
        diffuse *= RTGIReconDiffuseFactor(albedoMetallic);
        specular *= RTGIReconSpecularFactor(albedoMetallic);
    }

    float3 color = emission + diffuse + specular;
    if (!all(isfinite(color)))
        color = 0.0;
    u_SceneColor[pixel] = float4(color, 1.0);
}
