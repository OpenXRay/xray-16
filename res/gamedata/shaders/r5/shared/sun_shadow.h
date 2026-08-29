#ifndef SUN_SHADOW_H
#define SUN_SHADOW_H

#ifdef SUN_SHADOW_RECEIVER

Texture2D<float> g_SunShadowFar : register(t26);
Texture2D<float> g_SunShadowCasc0 : register(t27);

static const float kSunCascBias0 = 0.0004;
static const float kSunFarBias = 0.0015;

float CascTap(Texture2D<float> smap, float2 uv, float ref, float2 invSize)
{
    float2 sz = 1.0 / invSize;
    float2 t = uv * sz - 0.5;
    float2 f = frac(t);
    float4 d = smap.GatherRed(smp_nofilter, (floor(t) + 1.0) * invSize);
    float4 c = step(d, ref.xxxx);
    return lerp(lerp(c.w, c.z, f.x), lerp(c.x, c.y, f.x), f.y);
}

float CascSample(Texture2D<float> smap, float4x4 vp, float3 worldPos, float bias, float2 invSize)
{
    float3 n = mul(vp, float4(worldPos, 1.0)).xyz;
    float2 uv = float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
    if (uv.x < 0.01 || uv.x > 0.99 || uv.y < 0.01 || uv.y > 0.99 || n.z <= 0.0 || n.z >= 1.0)
        return -1.0;
    float ref = n.z + bias;
    return 0.25 * (CascTap(smap, uv + float2(-0.5, -0.5) * invSize, ref, invSize)
                 + CascTap(smap, uv + float2( 0.5, -0.5) * invSize, ref, invSize)
                 + CascTap(smap, uv + float2(-0.5,  0.5) * invSize, ref, invSize)
                 + CascTap(smap, uv + float2( 0.5,  0.5) * invSize, ref, invSize));
}

float SunShadowFar(float3 worldPos)
{
    float4 n = mul(shadow_matrices[2], float4(worldPos, 1.0));
    float2 uv = float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || n.z <= 0.0 || n.z >= 1.0)
        return 1.0;

    float ref = n.z + kSunFarBias;
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
    if (cascade_splits.y > 0.0)
    {
        float s = CascSample(g_SunShadowCasc0, shadow_matrices[0], worldPos, kSunCascBias0, float2(cascade_splits.y, cascade_splits.y));
        if (s >= 0.0)
            return s;
    }
    return SunShadowFar(worldPos);
}

uint SunShadowZone(float3 worldPos)
{
    if (cascade_splits.y > 0.0)
    {
        float3 n = mul(shadow_matrices[0], float4(worldPos, 1.0)).xyz;
        float2 uv = float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
        if (uv.x >= 0.01 && uv.x <= 0.99 && uv.y >= 0.01 && uv.y <= 0.99 && n.z > 0.0 && n.z < 1.0)
            return 0u;
    }
    float4 f = mul(shadow_matrices[2], float4(worldPos, 1.0));
    float2 fuv = float2(f.x * 0.5 + 0.5, 0.5 - f.y * 0.5);
    if (all(fuv >= 0.0) && all(fuv <= 1.0) && f.z > 0.0 && f.z < 1.0)
        return 1u;
    return 2u;
}

float3 SunShadowDebugColor(float3 color, float3 worldPos)
{
    float vis = SunVisibility(worldPos);
    if (dev_param_3.y > 1.5)
    {
        uint zone = SunShadowZone(worldPos);
        float3 tint = zone == 0u ? float3(0.2, 1.0, 0.2) : (zone == 1u ? float3(1.0, 0.6, 0.2) : float3(0.3, 0.3, 1.0));
        return lerp(color, tint, 0.45) * (0.35 + 0.65 * vis);
    }
    return vis.xxx;
}

#else

float SunVisibility(float3 worldPos)
{
    return 1.0;
}

#endif

#endif
