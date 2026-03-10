#ifndef SHADOW_SAMPLING_H
#define SHADOW_SAMPLING_H

#ifdef CSM_SHADOW_FORWARD
Texture2DArray<float> g_ShadowMapArray : register(t23);
SamplerComparisonState smp_shadowcmp : register(s4);
#endif

#ifdef HUD_SHADOW_FORWARD
Texture2D<float> g_HUDShadowMap : register(t24);
#endif

float SampleCSM(float3 worldPos)
{
#ifndef CSM_SHADOW_FORWARD
    return 1.0;
#else
    float smapSize = dev_param_3.x > 0 ? dev_param_3.x : 4096.0;
    float2 texelSize = 1.0 / float2(smapSize, smapSize);

    [unroll]
    for (uint c = 0; c < 3; c++)
    {
        float4 shadowCoord = mul(shadow_matrices[c], float4(worldPos, 1.0));
        float2 shadowUV = shadowCoord.xy;
        float depth = shadowCoord.z;

        if (all(shadowUV > 0.0) && all(shadowUV < 1.0) && depth >= 0.0 && depth <= 1.0)
        {
            float shadow = 0.0;
            [unroll]
            for (int y = -1; y <= 1; y += 2) {
                [unroll]
                for (int x = -1; x <= 1; x += 2) {
                    float2 offset = float2(x, y) * texelSize * 0.5;
                    shadow += g_ShadowMapArray.SampleCmp(
                        smp_shadowcmp, float3(shadowUV + offset, c), depth);
                }
            }
            return shadow * 0.25;
        }
    }

    return 1.0;
#endif
}

float SampleHUDShadow(float3 worldPos)
{
#ifndef HUD_SHADOW_FORWARD
    return 1.0;
#else
    float4 shadowCoord = mul(shadow_matrices[3], float4(worldPos, 1.0));
    float2 shadowUV = shadowCoord.xy;
    float depth = shadowCoord.z;

    if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
        return 1.0;

    float hudSmapSize = 2048.0;
    float2 texelSize = 1.0 / float2(hudSmapSize, hudSmapSize);

    float shadow = 0.0;
    [unroll]
    for (int y = -1; y <= 1; y += 2) {
        [unroll]
        for (int x = -1; x <= 1; x += 2) {
            float2 offset = float2(x, y) * texelSize * 0.5;
            shadow += g_HUDShadowMap.SampleCmp(
                smp_shadowcmp, shadowUV + offset, depth);
        }
    }
    return shadow * 0.25;
#endif
}

#endif
