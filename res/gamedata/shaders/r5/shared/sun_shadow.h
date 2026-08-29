#ifndef SUN_SHADOW_H
#define SUN_SHADOW_H

#ifdef SUN_SHADOW_RECEIVER

Texture2D<float> g_SunShadowFar : register(t26);

float SunShadowFar(float3 worldPos)
{
    float4 n = mul(shadow_matrices[2], float4(worldPos, 1.0));
    float2 uv = float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || n.z <= 0.0 || n.z >= 1.0)
        return 1.0;

    float ref = n.z + cascade_splits.y;
    float2 texel = float2(cascade_splits.z, cascade_splits.z);
    float sum = 0.0;
    sum += step(g_SunShadowFar.SampleLevel(smp_nofilter, uv + float2(-0.75, -0.75) * texel, 0), ref);
    sum += step(g_SunShadowFar.SampleLevel(smp_nofilter, uv + float2( 0.75, -0.75) * texel, 0), ref);
    sum += step(g_SunShadowFar.SampleLevel(smp_nofilter, uv + float2(-0.75,  0.75) * texel, 0), ref);
    sum += step(g_SunShadowFar.SampleLevel(smp_nofilter, uv + float2( 0.75,  0.75) * texel, 0), ref);
    return sum * 0.25;
}

float SunVisibility(float3 worldPos)
{
    if (cascade_splits.x < 0.5)
        return 1.0;
    return SunShadowFar(worldPos);
}

#else

float SunVisibility(float3 worldPos)
{
    return 1.0;
}

#endif

#endif
