#include "common.h"
#include "rt_grass_alpha.h"

cbuffer RTVolParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_Fog;
    float2 g_ScreenSize;
    float g_FogDensity;
    float g_FogHeight;
    uint g_Steps;
    float g_FogFar;
    float g_HeightFalloff;
    float g_ShaftIntensity;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint2 g_Pad;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t0);
Texture2D<float> t_Depth : register(t1);
Texture2D<float4> t_SceneColorIn : register(t2);
Texture2D<float4> t_WorldPos : register(t3);
ByteAddressBuffer g_GrassVB : register(t5);
ByteAddressBuffer g_GrassIB : register(t6);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t7);

RWTexture2D<float4> u_SceneColor : register(u0);

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart != 0xFFFFFFFFu && batchIdx >= g_GrassBatchStart;
}

float TraceSunVisibility(float3 origin, float3 sunDir, float maxT)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = sunDir;
    ray.TMin = 0.08;
    ray.TMax = maxT;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(g_SceneTLAS,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0x03, ray);
    while (q.Proceed()) {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
            uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
            if (IsGrassBatch(candBatch) &&
                GrassTexelOpaque(g_GrassVB, g_GrassIB, g_BatchInfo, candBatch,
                    q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(), g_DetailAtlasIndex))
                q.CommitNonOpaqueTriangleHit();
        }
    }
    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0 : 1.0;
}

float InterleavedGradientNoise(float2 p)
{
    return frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float4 scene = t_SceneColorIn.Load(int3(pixel, 0));
    float intensity = max(g_ShaftIntensity, 0.0);
    if (g_Steps == 0 || intensity < 1e-4) {
        u_SceneColor[pixel] = scene;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * float2(1.0 / g_ScreenSize.x, 1.0 / g_ScreenSize.y);
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float3 cam = g_CameraPos.xyz;

    float4 farClip = mul(g_InvViewProj, float4(ndc, 1.0, 1.0));
    float3 farPos = farClip.xyz / max(farClip.w, 1e-6);
    float3 viewDir = normalize(farPos - cam);

    uint depthW = 0, depthH = 0;
    t_Depth.GetDimensions(depthW, depthH);
    uint2 depthPixel = uint2(
        clamp(uint(uv.x * float(max(depthW, 1u))), 0u, max(depthW, 1u) - 1u),
        clamp(uint(uv.y * float(max(depthH, 1u))), 0u, max(depthH, 1u) - 1u));

    float depth = t_Depth.Load(int3(depthPixel, 0));
    const bool isSky = depth >= 0.9995;

    float3 surfacePos;
    if (isSky) {
        float marchFar = min(max(g_FogFar, 900.0), 1600.0);
        surfacePos = cam + viewDir * marchFar;
    } else {
        float3 worldPos = t_WorldPos.Load(int3(depthPixel, 0)).xyz;
        if (any(abs(worldPos - cam) > 1e-3))
            surfacePos = worldPos;
        else
            surfacePos = cam + viewDir * min(max(g_FogFar, 900.0), 1600.0);
    }

    float3 toEye = cam - surfacePos;
    float dist = length(toEye);
    if (dist < 0.4) {
        u_SceneColor[pixel] = scene;
        return;
    }

    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float3 sunColor = max(g_SunColor_Fog.xyz, float3(0.15, 0.13, 0.1));
    float sunSat = g_SunColor_Fog.w;
    if (sunSat < 1e-3)
        sunSat = 1.0;

    uint steps = max(g_Steps, 1u);
    float jitter = InterleavedGradientNoise(float2(pixel));
    float coeff = (float(steps) - jitter) / float(steps * steps);
    float3 stepW = toEye * coeff;
    float density = intensity / float(steps);

    float towardSun = saturate(dot(viewDir, sunDir));
    float res = 0.0;
    if (isSky)
        res = intensity * pow(towardSun, 12.0);

    float3 cur = surfacePos;
    float sunTraceT = 2500.0;
    float cone = isSky ? pow(towardSun, 3.0) : 1.0;
    [loop] for (uint i = 0; i < steps; ++i) {
        float3 fromCam = cur - cam;
        float viewDist = length(fromCam);
        if (viewDist > 0.3)
            res += density * TraceSunVisibility(cur, sunDir, sunTraceT) * cone;
        cur += stepW;
    }

    if (!isSky)
        res *= lerp(0.35, 1.0, pow(towardSun, 4.0));

    res *= sunSat;
    float3 shafts = max(res, 0.0) * sunColor;
    u_SceneColor[pixel] = float4(scene.rgb + shafts, scene.a);
}
