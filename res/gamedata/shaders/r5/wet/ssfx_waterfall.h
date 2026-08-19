// wet/ssfx_waterfall.h — SSFX/CoP waterfall + sparse droplet streaks
#ifndef WET_SSFX_WATERFALL_H
#define WET_SSFX_WATERFALL_H

#include "wet/ssfx_ripples.h" // hash22

static const float4 ssfx_wetsurfaces_1 = float4(0.90f, 1.50f, 0.20f, 1.00f);
static const float4 ssfx_wetsurfaces_2 = float4(0.75f, 1.50f, 0.20f, 0.55f);

float3 ssfx_RainPos(float3 worldPos, float3 eyePos)
{
    return worldPos - eyePos;
}

float3 ssfx_GetWaterFall(Texture2D s_texture, SamplerState smp, float2 tc, float rainInt, float time)
{
    float col_num = 50.0;
    float col_scale = 1.0 / col_num;
    float2 tc_ori = tc * ssfx_wetsurfaces_2.x;
    float2 col_tc = float2(frac(tc_ori.x * col_num), tc_ori.y);
    float col_id = ceil(tc_ori.x * col_num);

    float col_offset = hash22(float2(col_id, 1.0));
    col_offset = (col_offset < ssfx_wetsurfaces_2.z) ? (col_offset * 3.0) : col_offset;
    col_offset += time * col_offset * max(rainInt, 0.001) * ssfx_wetsurfaces_2.y;

    float3 water = s_texture.SampleLevel(
        smp,
        col_tc * float2(col_scale, 1.0) + float2(col_scale * col_id, col_offset),
        0).xyz;

    water = water.xzy * 2.0 - 1.0;
    water *= ssfx_wetsurfaces_2.w;
    return water;
}

float3 ssfx_GetWaterNMap(Texture2D s_texture, SamplerState smp, float2 tc)
{
    float3 water = s_texture.SampleLevel(smp, tc, 0).xyz;
    water = (water.xzy - 0.5) * 2.0;
    water *= 0.3;
    water.y = 0.0;
    return water;
}

// Raw water_normal grain — use NM xy variance (luma of DXT normals is often flat)
float ssfx_FallFlowRaw(Texture2D s_texture, SamplerState smp, float2 tc)
{
    float3 s = s_texture.SampleLevel(smp, tc, 0).xyz;
    float3 n = s.xzy * 2.0 - 1.0;
    // Horizontal/vertical lobes of the normal map = visible micro-flow grain
    float flow = abs(n.x) * 1.6 + abs(n.z) * 1.2;
    flow += abs(s.x - 0.5) * 0.8;
    return saturate(flow);
}

float2 ssfx_WallWeightsSoft(float3 worldN)
{
    return abs(worldN.xz);
}

float2 ssfx_WallWeightsXZ(float3 worldN)
{
    float2 w = saturate(abs(worldN.xz) - 0.15);
    w *= float2(w.x > w.y ? 1.0 : 0.0, w.x < w.y ? 1.0 : 0.0);
    return w;
}

// Sparse tear-shaped drips (not noise, not infinite bars). No lateral wobble.
float ssfx_DropletStreak(float2 tc, float rainInt, float time)
{
    float col_num = 40.0;
    float2 t = tc * 0.72;
    float col_id = floor(t.x * col_num);
    float x = frac(t.x * col_num) - 0.5;

    float seed = hash22(float2(col_id, 7.0));
    float active = step(0.22, seed); // denser — fewer empty columns
    float speed = lerp(0.45, 1.35, seed) * max(rainInt, 0.2);
    float y = t.y + time * speed + seed * 3.0;

    float body = exp(-x * x * 70.0); // slightly wider

    float cell = frac(y * 0.62 + seed * 1.7);
    float head = saturate(1.0 - abs(cell - 0.22) * 6.5);
    head = head * head;
    float tail = saturate(1.0 - abs(cell - 0.55) * 3.2) * 0.4;
    float tear = saturate(head + tail);

    return body * tear * active;
}

// Streak from waterfall NM peaks only (suppresses noisy midtones)
float ssfx_FallPeak(float3 fallNM)
{
    float ax = abs(fallNM.x);
    float az = abs(fallNM.z);
    // Keep only strong lobes → droplet-like, not grain
    float peak = saturate(ax * 5.0 - 0.35);
    peak = max(peak, saturate(az * 4.0 - 0.4));
    return peak * peak;
}

#endif
