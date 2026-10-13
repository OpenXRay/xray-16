#ifndef SUN_SHADOW_H
#define SUN_SHADOW_H

#ifdef SUN_SHADOW_RECEIVER

Texture2D<float4> g_SunShadowMask : register(t29);

float2 VSMMaskShadow(float3 worldPos, float4 svPosition, bool hudReceiver = false)
{
    float2 pixel;
    if (svPosition.w != 0.0)
    {
        pixel = svPosition.xy;
    }
    else
    {
        float3 presentationPos = hudReceiver ? mul(m_HudWarp, float4(worldPos, 1.0)).xyz : worldPos;
        float4 c = mul(m_VP, float4(presentationPos, 1.0));
        float2 ndc = c.xy / max(c.w, 1e-6);
        pixel = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5) * screen_res.xy;
    }
    float4 mask = g_SunShadowMask.Load(int3(int2(pixel), 0));
    return float2(saturate(mask.r), max(mask.b, 0.0));
}

float2 SunShadow(float3 worldPos, float4 svPosition, bool hudReceiver = false)
{
    if (cascade_splits.x < 0.5)
        return float2(1.0, 0.0);
    return VSMMaskShadow(worldPos, svPosition, hudReceiver);
}

float SunVisibility(float3 worldPos, bool hudReceiver = false)
{
    return SunShadow(worldPos, float4(0.0, 0.0, 0.0, 0.0), hudReceiver).x;
}

float3 SunShadowDebugColor(float3 color, float3 worldPos, bool hudReceiver = false)
{
    return SunVisibility(worldPos, hudReceiver).xxx;
}

#else

float2 SunShadow(float3 worldPos, float4 svPosition, bool hudReceiver = false)
{
    return float2(1.0, 0.0);
}

float SunVisibility(float3 worldPos, bool hudReceiver = false)
{
    return 1.0;
}

#endif

#endif
