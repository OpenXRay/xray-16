#include "rt_irradiance_cache.h"
#include "vol_fog_common.h"
#include "vol_fog_params.h"
#include "shared/clustered_lighting.h"
#include "rt_common.h"

RaytracingAccelerationStructure g_SceneTLAS : register(t2);
StructuredBuffer<GPULightData> g_Lights : register(t3);
Texture3D<float> t_BlueNoise : register(t4);
Texture3D<float4> t_PrevLighting : register(t5);

Texture3D<float4> t_Density : register(t0);
StructuredBuffer<IrradianceCacheEntry> t_IrradianceCache : register(t1);
TextureCube<float4> g_Sky0 : register(t6);
TextureCube<float4> g_Sky1 : register(t7);

RWTexture3D<float4> u_Lighting : register(u0);
SamplerState smp_linear : register(s0);

float3 SampleSkyIncident(float3 dir, float mip)
{
    float3 d = normalize(dir);
    float3 s0 = g_Sky0.SampleLevel(smp_linear, d, mip).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, d, mip).rgb;
    float3 sky = lerp(s0, s1, saturate(g_SkyColor.w)) * g_SkyColor.rgb * 0.80;
    if (dot(sky, sky) < 1e-6)
        sky = g_HemiColor.rgb;
    return sky;
}

float SampleSTBN3(uint3 id)
{
    uint w = 0, h = 0, d = 0;
    t_BlueNoise.GetDimensions(w, h, d);
    if (w < 4u || h < 4u || d < 1u)
        return 0.5;
    uint z = (g_FrameIndex + id.z * 3u) % max(d, 1u);
    return t_BlueNoise.Load(int4(int(id.x % w), int(id.y % h), int(z), 0));
}

float TraceOpenVis(float3 origin, float3 dir, float tMax)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.35;
    ray.TMax = tMax;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_MASK_SHADOW_MAPPED, ray);
    while (q.Proceed()) {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            continue;
    }
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
}

float2 ProjectToUv(float4x4 viewProj, float3 worldPos)
{
    float4 clip = mul(viewProj, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(abs(clip.w), 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

[numthreads(8, 8, 4)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= VOL_FOG_W || id.y >= VOL_FOG_H || id.z >= VOL_FOG_D)
        return;

    float4 dens = t_Density[id];
    float density = dens.a;
    if (density <= 1e-5) {
        u_Lighting[id] = 0;
        return;
    }

    float3 worldPos = VolFogFroxelWorldPos(id, g_InvViewProj, g_CameraPos.xyz, g_ZNear, g_ZFar);
    float3 visOrigin = worldPos + float3(0.0, 0.4, 0.0);
    float jitter = SampleSTBN3(id);
    worldPos += (jitter * 2.0 - 1.0) * 0.04;
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 albedo = saturate(dens.rgb);
    float3 L = normalize(-g_SunDir.xyz);
    float sunVis = 1.0;
    float skyVis = 1.0;
    if (g_EnableRT != 0) {
        skyVis = TraceOpenVis(visOrigin, float3(0.0, 1.0, 0.0), 28.0);
        if (skyVis > 0.5) {
            float tiltA = TraceOpenVis(visOrigin, normalize(float3(0.22, 1.0, 0.0)), 22.0);
            float tiltB = TraceOpenVis(visOrigin, normalize(float3(-0.22, 1.0, 0.0)), 22.0);
            if (tiltA + tiltB < 0.5)
                skyVis = 0.0;
        }
        if (g_EnableSun != 0)
            sunVis = TraceOpenVis(visOrigin + L * 0.15, L, 400.0);
    }

    if (skyVis < 0.5)
        density *= sunVis;

    float3 inscatt = 0.0;
    float dynamicW = 0.0;
    if (skyVis > 0.5)
        inscatt = SampleSkyIncident(float3(0.0, 1.0, 0.0), 4.0) * albedo;

    if (g_EnableSun != 0 && g_SunColor.w > 0.0) {
        float3 sun = g_SunColor.rgb;
        float sunLum = max(max(sun.r, sun.g), sun.b);
        if (sunLum > 2.4)
            sun *= 2.4 / sunLum;
        inscatt += sun * sunVis * albedo * g_SunColor.w;
    }

    if (g_EnableLights != 0 && g_NumLights > 0) {
        uint maxL = min(g_EnableLights, 8u);
        uint counted = 0;
        float bestSpot = 0;
        uint bestSpotIdx = 0xFFFFFFFFu;
        for (uint li = 0; li < g_NumLights && counted < maxL; li++) {
            GPULightData light = g_Lights[li];
            float3 lpos = light.positionAndInvRangeSq.xyz;
            float3 toL = lpos - worldPos;
            float distSq = dot(toL, toL);
            float invRangeSq = light.positionAndInvRangeSq.w;
            if (distSq * invRangeSq > 1.0)
                continue;
            float dist = sqrt(max(distSq, 1e-5));
            float att = PointLightAttenuation(distSq, invRangeSq, light.spotParamsAndType.w);
            float isSpot = light.spotParamsAndType.y;
            if (isSpot > 0.5) {
                att *= SpotLightAttenuation(toL, light.directionAndSpotScale.xyz,
                    light.directionAndSpotScale.w, light.spotParamsAndType.x);
                float camDist = length(lpos - g_CameraPos.xyz);
                bool doSpot = (g_SpotMode == 2) || (g_SpotMode == 1 && li == g_PlayerLight);
                if (doSpot && att > 0.01 && camDist < 25.0) {
                    if (att > bestSpot) {
                        bestSpot = att;
                        bestSpotIdx = li;
                    }
                    dynamicW = max(dynamicW, saturate(att));
                }
            }
            float3 ldir = toL / dist;
            float phase = min(VolFogHenyeyGreenstein(dot(V, ldir), g_FogTune2.x), 1.25);
            inscatt += light.colorAndRange.xyz * att * phase * albedo;
            counted++;
        }
        if (bestSpotIdx != 0xFFFFFFFFu && g_EnableRT != 0) {
            GPULightData sl = g_Lights[bestSpotIdx];
            float3 toS = sl.positionAndInvRangeSq.xyz - visOrigin;
            float d = length(toS);
            float vis = TraceOpenVis(visOrigin, toS / max(d, 1e-4), max(d - 0.08, 0.05));
            inscatt *= lerp(1.0, vis, saturate(bestSpot * 2.0));
        }
    }

    if (g_EnableGI != 0 && g_FogTune2.z > 0.5) {
        uint cacheSize = (uint)g_FogTune2.w;
        if (cacheSize > 0) {
            uint h = IrradianceCacheHash(worldPos, 0.75, cacheSize);
            IrradianceCacheEntry e = t_IrradianceCache[h];
            if (e.stamp != 0)
                inscatt += min(e.irradiance, 1.0) * 0.02;
        }
    }

    inscatt = min(inscatt, 1.8);
    float4 curr = float4(inscatt * density, density);

    if (g_EnableTemporal != 0) {
        float2 prevUV = ProjectToUv(g_PrevViewProj, worldPos);
        float viewZ = length(worldPos - g_CameraPos.xyz);
        float tz = VolFogViewToT(max(viewZ, g_ZNear), g_ZNear, g_ZFar);
        if (all(prevUV >= 0.0) && all(prevUV < 1.0) && tz > 0.0 && tz < 1.0) {
            float4 hist = t_PrevLighting.SampleLevel(smp_linear, float3(prevUV, tz), 0);
            float hw = lerp(0.86, 0.35, dynamicW);
            curr = lerp(curr, hist, hw);
        }
    }

    u_Lighting[id] = curr;
}
