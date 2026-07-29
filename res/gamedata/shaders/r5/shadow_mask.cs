#define SM_6_0
#define CLUSTERED_LIGHTING_FORWARD
#define CSM_SHADOW_FORWARD
#define LOCAL_SHADOW_FORWARD
#define SHADOW_MASK_CS
#include "common.h"
#include "bindless_common.h"
#include "shared/shadow_sampling.h"
#include "shared/clustered_lighting.h"

cbuffer ShadowMaskParams : register(b5)
{
    uint2 g_maskSize;
    uint2 g_fullSize;
};

Texture2D<float> g_SceneDepth : register(t0);
RWTexture2D<float4> g_OutMask : register(u0);

float3 ReconstructWorldPos(float2 uv, float rawDepth)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 worldH = mul(m_InvVP, float4(ndc, rawDepth, 1.0));
    return worldH.xyz / max(worldH.w, 1e-5);
}

float3 ReconstructNormal(float2 uv, float3 worldPos)
{
    float2 texel = 1.0 / float2(g_fullSize);
    float dC = g_SceneDepth.SampleLevel(smp_nofilter, uv, 0).x;
    float dR = g_SceneDepth.SampleLevel(smp_nofilter, uv + float2(texel.x, 0), 0).x;
    float dU = g_SceneDepth.SampleLevel(smp_nofilter, uv + float2(0, -texel.y), 0).x;
    float3 pR = ReconstructWorldPos(uv + float2(texel.x, 0), dR);
    float3 pU = ReconstructWorldPos(uv + float2(0, -texel.y), dU);
    float3 N = normalize(cross(pR - worldPos, pU - worldPos));
    if (dot(N, N) < 1e-6)
        return float3(0, 1, 0);
    return N;
}

float EvalLocalShadowProduct(float3 worldPos, float3 N, float2 pixelPos, float linearDepth)
{
    uint numLights = (uint)cluster_params.w;
    if (numLights == 0)
        return 1.0;

    uint clusterIdx = GetClusterIndexFromPixel(pixelPos, linearDepth);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint lightCount = clusterData.y;

    float product = 1.0;
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIdx = g_LightIndexList[lightOffset + i];
        GPULightData light = g_LightData[lightIdx];
        if (light.localShadowRect.w <= 0.5)
            continue;

        float3 toLight = light.positionAndInvRangeSq.xyz - worldPos;
        float distSq = dot(toLight, toLight);
        float atten = PointLightAttenuation(distSq, light.positionAndInvRangeSq.w);
        if (atten <= 0.001f)
            continue;

        float soft = (light.spotParamsAndType.y > 0.5f)
            ? max(dev_param_1.z, 0.1)
            : max(dev_param_1.y, 0.1);
        float s = SampleLocalShadow(worldPos, N, light.spotVP, light.localShadowRect, soft);
        product *= saturate(s);
    }
    return product;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_maskSize.x || id.y >= g_maskSize.y)
        return;

    float2 uv = (float2(id.xy) + 0.5) / float2(g_maskSize);
    float rawD = g_SceneDepth.SampleLevel(smp_nofilter, uv, 0).x;
    if (rawD >= 0.9995)
    {
        g_OutMask[id.xy] = float4(1, 1, 1, 1);
        return;
    }

    float3 worldPos = ReconstructWorldPos(uv, rawD);
    float3 N = ReconstructNormal(uv, worldPos);
    float2 fullPixel = uv * float2(g_fullSize);
    float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);

    float sun = SampleCSM(worldPos, N);
    float3 L = normalize(-L_sun_dir_w);
    float contact = ContactShadow(worldPos, L);
    float localS = EvalLocalShadowProduct(worldPos, N, fullPixel, viewZ);

    g_OutMask[id.xy] = float4(saturate(sun), saturate(contact), saturate(localS), 1.0);
}
