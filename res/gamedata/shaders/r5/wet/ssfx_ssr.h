// wet/ssfx_ssr.h — conservative wet SSR (world march + SceneReflection)
#ifndef WET_SSFX_SSR_H
#define WET_SSFX_SSR_H

#ifndef WET_SSR_STEPS
#define WET_SSR_STEPS 16
#endif
#ifndef WET_SSR_MAXDIST
#define WET_SSR_MAXDIST 48.0
#endif
#ifndef WET_SSR_THICK
#define WET_SSR_THICK 0.45
#endif

float2 WetWorldToUv(float3 worldPos, float4x4 vp, out float clipW)
{
    float4 clip = mul(vp, float4(worldPos, 1.0));
    clipW = clip.w;
    float2 ndc = clip.xy / max(clip.w, 1e-4);
    // Vulkan/Metal: Y flip like rest of r5
    return float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

float WetBorderAtten(float2 uv)
{
    float b = min(min(uv.x, uv.y), min(1.0 - uv.x, 1.0 - uv.y));
    return saturate(b / 0.08);
}

// Returns rgb=reflection, a=confidence (0 = miss — do not darken/blacken)
float4 WetSSR(
    Texture2D sceneColor,
    Texture2D worldPosTex,
    SamplerState smpColor,
    SamplerState smpPos,
    float2 uv0,
    float3 worldPos,
    float3 N,
    float3 eyePos,
    float4x4 vp)
{
    float3 V = normalize(worldPos - eyePos);
    float3 R = reflect(V, normalize(N));
    // Skip downward rays into ground (common black/noise source)
    if (R.y < -0.35)
        return float4(0, 0, 0, 0);

    R = normalize(R);
    float2 hitUV = uv0;
    float conf = 0.0;
    bool hit = false;

    float tPrev = 0.15;
    [loop]
    for (int i = 1; i <= WET_SSR_STEPS; ++i)
    {
        float u = float(i) / float(WET_SSR_STEPS);
        float t = WET_SSR_MAXDIST * u * u;
        float3 march = worldPos + R * t;

        float w;
        float2 uv = WetWorldToUv(march, vp, w);
        if (w <= 1e-3)
            break;
        if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0)
            break;

        float3 samplePos = worldPosTex.SampleLevel(smpPos, uv, 0).xyz;
        float sampleLen = length(samplePos);
        // Empty / sky
        if (sampleLen < 0.05)
        {
            tPrev = t;
            continue;
        }

        float dist = length(samplePos - march);
        // Behind surface along view? Prefer closer geometry hit
        float toward = dot(normalize(samplePos - worldPos), R);
        if (toward > 0.15 && dist < WET_SSR_THICK * (1.0 + t * 0.02))
        {
            hitUV = uv;
            conf = saturate(1.0 - dist / max(WET_SSR_THICK, 0.001));
            conf *= WetBorderAtten(uv);
            // Avoid self-hit at start
            conf *= saturate((t - 0.35) * 2.0);
            hit = conf > 0.05;
            if (hit)
                break;
        }
        tPrev = t;
    }

    if (!hit)
        return float4(0, 0, 0, 0);

    float3 refl = sceneColor.SampleLevel(smpColor, hitUV, 0).rgb;
    // Reject near-black "hits" (former fullscreen SSR failure mode)
    float lum = dot(refl, float3(0.299, 0.587, 0.114));
    conf *= saturate(lum * 8.0);
    return float4(refl, conf);
}

#endif
