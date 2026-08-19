// wet/ssfx_ripples.h — SSFX ripples, Slang-safe (no early returns)
#ifndef WET_SSFX_RIPPLES_H
#define WET_SSFX_RIPPLES_H

static const float3 SSFX_ripples_speed = float3(1.05f, 1.31f, 1.58f);
static const float4 SSFX_ripples_offset = float4(0.5f, 0.25f, 0.31f, 0.5f);
static const float SSFX_ripples_PI = 3.141592f;

float hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float2 ssfx_process_ripples(float4 ripples, float3 setup, float time)
{
    float2 ripples_N = ripples.yz * 2.0 - 1.0;
    float RFrac = frac(ripples.w + time * setup.x);
    float TimeFrac = RFrac - 1.0 + ripples.x;
    float RFreq = clamp(TimeFrac * setup.z, 0.0, 4.0);
    float FinalFactor = saturate(0.7 - RFrac) * ripples.x * sin(RFreq * SSFX_ripples_PI);
    FinalFactor *= saturate(1.0 - RFreq * 0.25);
    ripples_N *= FinalFactor * setup.y;
    return ripples_N;
}

// depth = distance to eye. Fade instead of early-return (Slang SPIR-V crash).
float2 ssfx_rain_ripples(Texture2D ripples_tex, SamplerState smp, float2 uvs, float3 setup, float depth, float time)
{
    float fade = saturate((15.0 - depth) * 0.0666);

    float4 Layer0 = ripples_tex.SampleLevel(smp, uvs, 0);
    float4 Layer1 = ripples_tex.SampleLevel(smp, uvs * 0.61 + SSFX_ripples_offset.xy, 0);
    float4 Layer2 = ripples_tex.SampleLevel(smp, uvs * 0.87 + SSFX_ripples_offset.zw, 0);

    float2 result =
        ssfx_process_ripples(Layer0, float3(SSFX_ripples_speed.x * setup.x, setup.yz), time) +
        ssfx_process_ripples(Layer1, float3(SSFX_ripples_speed.y * setup.x, setup.yz), time) +
        ssfx_process_ripples(Layer2, float3(SSFX_ripples_speed.z * setup.x, setup.yz), time);

    result *= fade;
    return clamp(result, float2(-1.0, -1.0), float2(1.0, 1.0));
}

#endif
