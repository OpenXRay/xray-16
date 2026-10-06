#define SM_6_0
#define BINDLESS_NO_IMPLICIT_GRAD
#define SKY_PROBE_DIAGNOSTICS
#include "common.h"

Texture2D<float> g_Depth : register(t0);
Texture2D<float4> g_GBufferNormal : register(t2);
Texture2D<float4> g_GBufferMaterial : register(t3);
RWStructuredBuffer<uint4> u_SkyProbeInspection : register(u0);

void SkyProbeStoreSnapshot(SkyProbeDebugSnapshot snapshot)
{
    u_SkyProbeInspection[0] = snapshot.metadata;
    u_SkyProbeInspection[1] = snapshot.selection;
    u_SkyProbeInspection[2] = snapshot.gridDims;
    u_SkyProbeInspection[3] = asuint(snapshot.gridOrigin);
    u_SkyProbeInspection[4] = asuint(snapshot.camera);
    u_SkyProbeInspection[5] = asuint(snapshot.surface);
    u_SkyProbeInspection[6] = asuint(snapshot.shadingNormal);
    u_SkyProbeInspection[7] = asuint(snapshot.geometricNormal);
    u_SkyProbeInspection[8] = asuint(snapshot.material);
    u_SkyProbeInspection[9] = asuint(snapshot.view);
    u_SkyProbeInspection[10] = asuint(snapshot.biased);
    u_SkyProbeInspection[11] = asuint(snapshot.rayOrigin);
    u_SkyProbeInspection[12] = asuint(snapshot.controlOrigin);
    u_SkyProbeInspection[13] = asuint(snapshot.sampleSH);
    u_SkyProbeInspection[14] = asuint(snapshot.summary);
    u_SkyProbeInspection[15] = asuint(snapshot.sky);
    u_SkyProbeInspection[16] = asuint(snapshot.reflection);
    u_SkyProbeInspection[17] = asuint(snapshot.settings);
    u_SkyProbeInspection[18] = asuint(snapshot.lighting);
    u_SkyProbeInspection[19] = snapshot.scene;
    for (uint i = 0u; i < 8u; ++i)
    {
        uint row = SKY_PROBE_DEBUG_HEADER_ROWS + i * SKY_PROBE_DEBUG_CORNER_ROWS;
        u_SkyProbeInspection[row] = snapshot.corners[i].identity;
        u_SkyProbeInspection[row + 1u] = snapshot.corners[i].cellAndFlags;
        u_SkyProbeInspection[row + 2u] = snapshot.corners[i].rawVisibility;
        u_SkyProbeInspection[row + 3u] = snapshot.corners[i].rawDepthMean;
        u_SkyProbeInspection[row + 4u] = snapshot.corners[i].rawDepthSigma;
        u_SkyProbeInspection[row + 5u] = asuint(snapshot.corners[i].moment);
        u_SkyProbeInspection[row + 6u] = asuint(snapshot.corners[i].position);
        u_SkyProbeInspection[row + 7u] = asuint(snapshot.corners[i].offset);
        u_SkyProbeInspection[row + 8u] = asuint(snapshot.corners[i].coefficients);
        u_SkyProbeInspection[row + 9u] = asuint(snapshot.corners[i].weights);
        u_SkyProbeInspection[row + 10u] = asuint(snapshot.corners[i].sky);
        u_SkyProbeInspection[row + 11u] = asuint(snapshot.corners[i].contribution);
        for (uint r = 0u; r < 6u; ++r)
        {
            u_SkyProbeInspection[row + 12u + r * 2u] = asuint(snapshot.corners[i].rays[r].metrics);
            u_SkyProbeInspection[row + 13u + r * 2u] = snapshot.corners[i].rays[r].hit;
        }
    }
}

SkyProbeDebugRay SkyProbeControlRay(float3 origin, float3 position, uint mask, bool forceOpaque)
{
    SkyProbeDebugRay result = (SkyProbeDebugRay)0;
    float3 delta = position - origin;
    float distance = length(delta);
    result.metrics = float4(distance, -1.0, 1.0, 0.0);
    result.hit = uint4(0u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
#ifdef SKY_PROBE_RAY_QUERY
    if (distance < 1e-3)
    {
        result.hit.x = 3u;
        return result;
    }
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = delta / distance;
    ray.TMin = 0.0;
    ray.TMax = distance;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
    uint flags = forceOpaque ? RAY_FLAG_FORCE_OPAQUE : RAY_FLAG_CULL_NON_OPAQUE;
    query.TraceRayInline(g_SkyProbeTLAS, flags, mask, ray);
    while (query.Proceed())
    {
    }
    result.hit.x = 1u;
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        result.metrics.y = query.CommittedRayT();
        result.metrics.z = 0.0;
        result.metrics.w = query.CommittedTriangleFrontFace() ? 1.0 : 0.0;
        result.hit = uint4(2u, query.CommittedInstanceID(), query.CommittedGeometryIndex(), query.CommittedPrimitiveIndex());
    }
#endif
    return result;
}

float SkyProbeUnclampedResponse(float c0, float3 c1, float3 direction)
{
    return SKY_VISIBILITY_SH_Y0 * c0 + (2.0 / 3.0) * SKY_VISIBILITY_SH_Y1 * dot(c1, direction);
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    SkyProbeDebugSnapshot snapshot = (SkyProbeDebugSnapshot)0;
    uint2 pixel = uint2(screen_res.xy * 0.5);
    snapshot.metadata = uint4(SKY_PROBE_DEBUG_VERSION, g_SkyInspectCapture.x, SKY_PROBE_DEBUG_PRODUCTION_MOMENTS, 0u);
    snapshot.selection = uint4(pixel, uint2(screen_res.xy));
    uint3 gridDims = uint3(sky_probe_dims.xyz);
    snapshot.gridDims = uint4(gridDims, gridDims.x * gridDims.y * gridDims.z);
    snapshot.gridOrigin = sky_probe_origin;
    snapshot.camera = float4(eye_position, float(g_SkyInspectCapture.w));
    snapshot.settings = g_SkyInspectSettings;
    snapshot.lighting = g_SkyInspectLighting;
    snapshot.scene = g_SkyInspectScene;
    snapshot.scene.w = g_SkyInspectCapture.y;
    if (sky_probe_dims.w < 0.5 || sky_probe_origin.w <= 0.0)
    {
        SkyProbeStoreSnapshot(snapshot);
        return;
    }

    float depth = g_Depth.Load(int3(pixel, 0));
    float4 normal = g_GBufferNormal.Load(int3(pixel, 0));
    float4 material = g_GBufferMaterial.Load(int3(pixel, 0));
    snapshot.surface.w = depth;
    snapshot.shadingNormal = normal;
    snapshot.material = material;
    if (dot(normal.xyz, normal.xyz) < 0.25 || !all(isfinite(normal)) || !isfinite(depth))
    {
        snapshot.metadata.w = depth <= 0.0 ? 1u : 3u;
        SkyProbeStoreSnapshot(snapshot);
        return;
    }

    bool hud = GBufferIsHud(material);
    float3 position = reconstruct_world_pos(float2(pixel) + 0.5, depth, hud);
    float3 shadingNormal = normalize(normal.xyz);
    float3 geometricNormal = GBufferGeometricNormal(material, shadingNormal);
    snapshot.surface = float4(position, depth);
    if (!all(isfinite(position)) || !all(isfinite(geometricNormal)))
    {
        snapshot.metadata.w = 3u;
        SkyProbeStoreSnapshot(snapshot);
        return;
    }
    float3 view = normalize(eye_position - position);
    float3 biased = SkyProbeBiasedPosition(position, geometricNormal, view);
    float3 rayOrigin = SkyProbeRayOrigin(position, geometricNormal, view);
    float3 controlOrigin = position + geometricNormal * 0.001;
    float roughness = abs(normal.w);
    float3 reflection = normalize(lerp(reflect(-view, shadingNormal), shadingNormal, roughness * roughness));
    float3 gridPosition = (biased - sky_probe_origin.xyz) / sky_probe_origin.w;
    bool gridClamped = any(gridPosition < 0.0) || any(gridPosition > sky_probe_dims.xyz - 1.0);
    snapshot.metadata.w = hud ? 2u : 4u;
    snapshot.shadingNormal = float4(shadingNormal, roughness);
    snapshot.geometricNormal = float4(geometricNormal, dot(geometricNormal, view));
    snapshot.view = float4(view, length(eye_position - position));
    snapshot.biased = float4(biased, length(biased - position));
    snapshot.rayOrigin = float4(rayOrigin, length(rayOrigin - position));
    snapshot.controlOrigin = float4(controlOrigin, 0.001);
    snapshot.reflection = float4(reflection, gridClamped ? 1.0 : 0.0);

    SkyProbeVisibility visibility = SkyProbeSample(position, geometricNormal, view);
    snapshot.sampleSH = float4(visibility.c0, visibility.c1);
    snapshot.summary.z = visibility.confidence;
    snapshot.sky = float4(SkyProbeVisibilityToward(visibility, shadingNormal),
        SkyProbeVisibilityToward(visibility, geometricNormal), SkyProbeVisibilityToward(visibility, -shadingNormal),
        SkyProbeVisibilityToward(visibility, reflection));
    float floorLimitedWeight = 0.0;
    uint3 dims = uint3(sky_probe_dims.xyz);
    for (uint i = 0u; i < 8u; ++i)
    {
        SkyProbeCorner corner = SkyProbeEvaluateCorner(position, geometricNormal, biased, i);
        SkyProbeDebugCorner diagnostic = (SkyProbeDebugCorner)0;
        uint3 cell = uint3(corner.index % dims.x, (corner.index / dims.x) % dims.y, corner.index / (dims.x * dims.y));
        SkyProbeRecord record = g_SkyProbes[corner.index];
        float c0;
        float3 c1;
        SkyVisibilityUnpack(record.visibility.xy, c0, c1);
        float3 offset = SkyVisibilityBaked(c0) ? SkyProbeUnpackOffset(record.visibility.z, sky_probe_origin.w) : 0.0;
        float3 probePosition = sky_probe_origin.xyz + float3(cell) * sky_probe_origin.w + offset;
        diagnostic.identity = uint4(corner.index, corner.sampleState, cell.xy);
        diagnostic.cellAndFlags = uint4(cell.z, record.visibility.w,
            (record.visibility.w >> SKY_PROBE_FAIL_SHIFT) & 3u, 0u);
        diagnostic.rawVisibility = record.visibility;
        diagnostic.rawDepthMean = record.depthMean;
        diagnostic.rawDepthSigma = record.depthSigma;
        diagnostic.moment = corner.moment;
        diagnostic.position = float4(probePosition, length(probePosition - position));
        diagnostic.offset = float4(offset, length(offset));
        diagnostic.coefficients = float4(c0, c1);
        diagnostic.weights = float4(corner.trilinearWeight, corner.facingWeight, corner.visibility, corner.weight);
        diagnostic.contribution.w = corner.preCubicWeight;
        if (corner.valid)
        {
            diagnostic.sky = float4(SkyProbeUnclampedResponse(c0, c1, shadingNormal),
                SkyVisibilityCosine(c0, c1, shadingNormal), SkyProbeUnclampedResponse(c0, c1, geometricNormal),
                SkyVisibilityCosine(c0, c1, reflection));
            diagnostic.rays[0] = SkyProbeControlRay(rayOrigin, probePosition, SKY_PROBE_RAY_MASK_STATIC, false);
            diagnostic.rays[1] = SkyProbeControlRay(controlOrigin, probePosition, SKY_PROBE_RAY_MASK_STATIC, false);
            diagnostic.rays[2] = SkyProbeControlRay(rayOrigin, probePosition, SKY_PROBE_RAY_MASK_STATIC, true);
            diagnostic.rays[3] = SkyProbeControlRay(controlOrigin, probePosition, SKY_PROBE_RAY_MASK_STATIC, true);
            diagnostic.rays[4] = SkyProbeControlRay(rayOrigin, probePosition, 0x01u, true);
            diagnostic.rays[5] = SkyProbeControlRay(controlOrigin, probePosition, 0x01u, true);
            snapshot.summary.x += corner.weight;
            snapshot.summary.y = max(snapshot.summary.y, corner.visibility);
            if (corner.visibility < SKY_PROBE_VISIBILITY_FLOOR)
                floorLimitedWeight += corner.weight;
        }
        snapshot.corners[i] = diagnostic;
    }
    if (snapshot.summary.x > 1e-12)
    {
        snapshot.summary.w = floorLimitedWeight / snapshot.summary.x;
        for (uint i = 0u; i < 8u; ++i)
        {
            float normalizedWeight = snapshot.corners[i].weights.w / snapshot.summary.x;
            snapshot.corners[i].contribution.x = normalizedWeight;
            snapshot.corners[i].contribution.y = normalizedWeight * snapshot.corners[i].sky.x * visibility.confidence;
            snapshot.corners[i].contribution.z =
                snapshot.corners[i].weights.z < SKY_PROBE_VISIBILITY_FLOOR ? normalizedWeight : 0.0;
        }
    }
    SkyProbeStoreSnapshot(snapshot);
}
