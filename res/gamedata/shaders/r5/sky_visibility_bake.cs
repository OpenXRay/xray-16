#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_shading.h"
#include "shared/sky_visibility.h"

cbuffer SkyVisibilityBakeParams
{
    float4 g_SkyGridOrigin;
    uint4 g_SkyGridDims;
    uint g_SkyBakeFirst, g_SkyBakeCount, g_SkyBakeRays, g_SkyBakeMaxNullEvents;
    float g_SkyBakeRayDistance, g_SkyBakeBackfaceLimit; uint g_SkyIdentityStaticCount, g_SkyTerrainBatchCount;
    uint g_SkySkinnedBatchStart, g_SkyGrassBatchStart, g_SkyDetailAtlasIndex, g_SkyDetailMeshBatchStart;
    uint g_SkyStaticDetailBatchStart, g_SkyDetailPbrIndex, g_SkyDetailBumpIndex, g_SkyBakePad0;
};

RWStructuredBuffer<SkyProbeRecord> u_SkyProbes;

RTSceneParams SkyBakeScene()
{
    RTSceneParams scene = RTBuildSceneParams(g_SkyIdentityStaticCount, g_SkyTerrainBatchCount,
        g_SkySkinnedBatchStart, g_SkyGrassBatchStart, g_SkyDetailAtlasIndex, 0u, 0u,
        float4(0.0, -1.0, 0.0, 0.0), float4(0.0, 0.0, 0.0, 0.0), 0u, g_SkyBakeMaxNullEvents, 0.0);
    scene.detailMeshBatchStart = g_SkyDetailMeshBatchStart;
    scene.staticDetailBatchStart = g_SkyStaticDetailBatchStart;
    scene.detailPbrIndex = g_SkyDetailPbrIndex;
    scene.detailBumpIndex = g_SkyDetailBumpIndex;
    scene.rayDistance = g_SkyBakeRayDistance;
    scene.rayMask = RT_RAY_MASK_STATIC;
    return scene;
}

float3 SkyBakeDirection(uint index, uint count)
{
    float y = 1.0 - (2.0 * float(index) + 1.0) / float(count);
    float r = sqrt(saturate(1.0 - y * y));
    float phi = float(index) * 2.39996323;
    return float3(r * cos(phi), y, r * sin(phi));
}

bool SkyBakeSingleSided(RTSceneParams scene, RTSceneTrace trace)
{
    if (IsTerrainBatch(scene, trace.batchIdx))
        return true;
    return !MaterialHasAlphaCoverage(g_Materials[trace.info.materialID]);
}

bool SkyBakeBackface(RTSceneParams scene, RTSceneTrace trace, float3 direction)
{
    RTTriangleVertex v0, v1, v2;
    RTLoadWorldTriangle(scene, trace.batchIdx, trace.info, trace.primitiveIndex, trace.objectToWorld, v0, v1, v2);
    RTShadingVertex vertex = RTInterpolateTriangleVertex(v0, v1, v2, trace.barycentrics);
    if (!RTPackedVectorValid(vertex.normal))
        return !trace.frontFace;
    float3 normal = RTBatchVerticesInWorldSpace(scene, trace.batchIdx) ? vertex.normal :
        TransformNormalToWorld(vertex.normal, trace.objectToWorld);
    return dot(normal, direction) > 0.0;
}

struct SkyBakeHit
{
    bool hit;
    bool backface;
    float transmittance;
    float t;
};

SkyBakeHit SkyBakeTrace(RTSceneParams scene, float3 origin, float3 direction)
{
    SkyBakeHit result;
    result.hit = false;
    result.backface = false;
    result.transmittance = 1.0;
    result.t = g_SkyBakeRayDistance;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = g_SkyBakeRayDistance;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_RAY_MASK_STATIC, ray);
    float3 transmittance = 1.0;
    uint candidates = 0u;
    while (q.Proceed())
    {
        if (++candidates > scene.maxNullEvents * 64u)
        {
            q.Abort();
            result.hit = true;
            return result;
        }
        if (q.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE)
            continue;
        RTSceneTrace candidate = (RTSceneTrace)0;
        candidate.batchIdx = q.CandidateInstanceID() + q.CandidateGeometryIndex();
        candidate.info = g_BatchInfo[candidate.batchIdx];
        candidate.primitiveIndex = q.CandidatePrimitiveIndex();
        candidate.barycentrics = q.CandidateTriangleBarycentrics();
        candidate.t = q.CandidateTriangleRayT();
        candidate.objectToWorld = q.CandidateObjectToWorld3x4();
        RTHitClass classification = RTClassifyHit(scene, candidate, direction, true, false);
        if (classification.opaque)
            q.CommitNonOpaqueTriangleHit();
        else
            transmittance *= classification.transmittance;
    }
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        RTSceneTrace trace = (RTSceneTrace)0;
        trace.batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        trace.info = g_BatchInfo[trace.batchIdx];
        trace.primitiveIndex = q.CommittedPrimitiveIndex();
        trace.barycentrics = q.CommittedTriangleBarycentrics();
        trace.t = q.CommittedRayT();
        trace.objectToWorld = q.CommittedObjectToWorld3x4();
        trace.frontFace = q.CommittedTriangleFrontFace();
        result.hit = true;
        result.t = trace.t;
        result.backface = SkyBakeSingleSided(scene, trace) && SkyBakeBackface(scene, trace, direction);
        return result;
    }
    result.transmittance = Luminance(transmittance);
    return result;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (dispatchID.x >= g_SkyBakeCount)
        return;
    uint probe = g_SkyBakeFirst + dispatchID.x;
    if (probe >= g_SkyGridDims.w)
        return;

    uint3 cell = uint3(probe % g_SkyGridDims.x, (probe / g_SkyGridDims.x) % g_SkyGridDims.y,
        probe / (g_SkyGridDims.x * g_SkyGridDims.y));
    float spacing = g_SkyGridOrigin.w;
    float limit = SKY_PROBE_OFFSET_RANGE * spacing;
    float3 grid = g_SkyGridOrigin.xyz + float3(cell) * spacing;
    RTSceneParams scene = SkyBakeScene();
    uint rays = max(g_SkyBakeRays, 1u);
    float depthRange = SKY_PROBE_DEPTH_RANGE * spacing;

    float3 offset = 0.0;
    bool valid = false;
    float3 validOffset = 0.0;
    float validVisible = 0.0;
    float3 validDirection = 0.0;
    float validMean[SKY_PROBE_DEPTH_TEXELS];
    float validMeanSq[SKY_PROBE_DEPTH_TEXELS];
    [unroll]
    for (uint k = 0u; k < SKY_PROBE_DEPTH_TEXELS; ++k)
    {
        validMean[k] = 1.0;
        validMeanSq[k] = 1.0;
    }
    uint failure = SKY_PROBE_FAIL_STUCK;
    for (uint attempt = 0u; attempt < 3u; ++attempt)
    {
        float visible = 0.0;
        float3 visibleDirection = 0.0;
        uint backfaces = 0u;
        float closestBack = 1e30;
        float3 closestBackDirection = 0.0;
        float closestFront = 1e30;
        float3 closestFrontDirection = 0.0;
        float depthMean[SKY_PROBE_DEPTH_TEXELS];
        float depthMeanSq[SKY_PROBE_DEPTH_TEXELS];
        float depthWeight[SKY_PROBE_DEPTH_TEXELS];
        [unroll]
        for (uint k = 0u; k < SKY_PROBE_DEPTH_TEXELS; ++k)
        {
            depthMean[k] = 0.0;
            depthMeanSq[k] = 0.0;
            depthWeight[k] = 0.0;
        }
        float3 position = grid + offset;
        for (uint i = 0u; i < rays; ++i)
        {
            float3 direction = SkyBakeDirection(i, rays);
            SkyBakeHit trace = SkyBakeTrace(scene, position, direction);
            float depth = 1.0;
            if (!trace.hit)
            {
                visible += trace.transmittance;
                visibleDirection += trace.transmittance * direction;
            }
            else if (trace.backface)
            {
                depth = saturate(trace.t * SKY_PROBE_DEPTH_BACKFACE_SCALE / depthRange);
                ++backfaces;
                if (trace.t < closestBack)
                {
                    closestBack = trace.t;
                    closestBackDirection = direction;
                }
            }
            else
            {
                depth = saturate(trace.t / depthRange);
                if (trace.t < closestFront)
                {
                    closestFront = trace.t;
                    closestFrontDirection = direction;
                }
            }
            [unroll]
            for (uint k = 0u; k < SKY_PROBE_DEPTH_TEXELS; ++k)
            {
                float w = pow(saturate(dot(SkyProbeDepthTexelDirection(k), direction)), SKY_PROBE_DEPTH_SHARPNESS);
                depthMean[k] += w * depth;
                depthMeanSq[k] += w * depth * depth;
                depthWeight[k] += w;
            }
        }

        if (float(backfaces) > g_SkyBakeBackfaceLimit * float(rays))
        {
            if (closestBack > 2.0 * limit)
            {
                failure = SKY_PROBE_FAIL_UNREACHABLE;
                break;
            }
            float3 moved = clamp(offset + closestBackDirection * (closestBack + 0.05 * spacing), -limit, limit);
            if (all(abs(moved - offset) <= 1e-3))
                break;
            offset = moved;
            continue;
        }
        valid = true;
        validOffset = offset;
        validVisible = visible;
        validDirection = visibleDirection;
        [unroll]
        for (uint k = 0u; k < SKY_PROBE_DEPTH_TEXELS; ++k)
        {
            bool covered = depthWeight[k] > 1e-6;
            validMean[k] = covered ? depthMean[k] / depthWeight[k] : 1.0;
            validMeanSq[k] = covered ? depthMeanSq[k] / depthWeight[k] : 1.0;
        }
        float minimumFront = 0.1 * spacing;
        if (closestFront >= minimumFront || attempt == 2u)
            break;
        float3 pushed = clamp(offset - closestFrontDirection * (minimumFront - closestFront), -limit, limit);
        if (all(abs(pushed - offset) <= 1e-3))
            break;
        offset = pushed;
    }
    if (valid)
        offset = validOffset;

    float weight = 12.5663706 / float(rays);
    float c0 = validVisible * weight * SKY_VISIBILITY_SH_Y0;
    float3 c1 = validDirection * weight * SKY_VISIBILITY_SH_Y1;
    uint flags = any(abs(offset) > 1e-3) ? SKY_PROBE_FLAG_RELOCATED : 0u;
    if (!valid)
    {
        c0 = -1.0;
        c1 = 0.0;
        flags |= failure << SKY_PROBE_FAIL_SHIFT;
    }

    float sigma[SKY_PROBE_DEPTH_TEXELS];
    [unroll]
    for (uint k = 0u; k < SKY_PROBE_DEPTH_TEXELS; ++k)
        sigma[k] = sqrt(max(validMeanSq[k] - validMean[k] * validMean[k], 0.0)) * 2.0;

    SkyProbeRecord record;
    record.visibility = uint4(SkyVisibilityPack(c0, c1), SkyProbePackOffset(offset, spacing), flags);
    record.depthMean = uint4(
        SkyProbePackUnorm4(float4(validMean[0], validMean[1], validMean[2], validMean[3])),
        SkyProbePackUnorm4(float4(validMean[4], validMean[5], validMean[6], validMean[7])),
        SkyProbePackUnorm4(float4(validMean[8], validMean[9], validMean[10], validMean[11])),
        SkyProbePackUnorm4(float4(validMean[12], validMean[13], validMean[14], validMean[15])));
    record.depthSigma = uint4(
        SkyProbePackUnorm4(float4(sigma[0], sigma[1], sigma[2], sigma[3])),
        SkyProbePackUnorm4(float4(sigma[4], sigma[5], sigma[6], sigma[7])),
        SkyProbePackUnorm4(float4(sigma[8], sigma[9], sigma[10], sigma[11])),
        SkyProbePackUnorm4(float4(sigma[12], sigma[13], sigma[14], sigma[15])));
    u_SkyProbes[probe] = record;
}
