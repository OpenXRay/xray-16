#ifndef RT_VISIBILITY_H
#define RT_VISIBILITY_H

#include "bindless_common.h"
#include "rt_common.h"
#include "rt_grass_alpha.h"
#include "rt_particle_alpha.h"

static const uint RT_VIS_MAX_SKIPS = 8;

float TraceVisibilityAtten(
    RaytracingAccelerationStructure tlas,
    StructuredBuffer<RTBatchInfo> batchInfo,
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    ByteAddressBuffer particleVB,
    ByteAddressBuffer particleIB,
    float3 origin,
    float3 dir,
    float tMax,
    uint identityStaticCount,
    uint terrainBatchCount,
    uint skinnedBatchStart,
    uint grassBatchStart,
    uint particleBatchStart,
    uint detailAtlasIndex,
    bool nearSkinnedOccludes,
    float skinnedSelfMax)
{
    float atten = 1.0;
    float3 shadowOrigin = origin;
    float3 shadowDir = normalize(dir);
    float remain = tMax;
    const float selfSkip = nearSkinnedOccludes ? 0.004 : 0.02;
    const float skinSelf = max(skinnedSelfMax, 0.0);

    for (uint si = 0; si < RT_VIS_MAX_SKIPS; si++) {
        RayDesc ray;
        ray.Origin = shadowOrigin;
        ray.Direction = shadowDir;
        ray.TMin = 0.001;
        ray.TMax = remain;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(tlas, RAY_FLAG_NONE, 0xFF, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (grassBatchStart != 0xFFFFFFFFu && candBatch >= grassBatchStart &&
                    (particleBatchStart == 0xFFFFFFFFu || candBatch < particleBatchStart) &&
                    GrassTexelOpaque(grassVB, grassIB, batchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(), detailAtlasIndex))
                    q.CommitNonOpaqueTriangleHit();
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            break;

        float tHit0 = q.CommittedRayT();
        if (tHit0 < selfSkip) {
            shadowOrigin = shadowOrigin + shadowDir * (tHit0 + 0.002);
            remain -= (tHit0 + 0.002);
            if (remain <= 0.001) break;
            continue;
        }

        uint sBatchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo sInfo = batchInfo[sBatchIdx];

        bool isGrass = grassBatchStart != 0xFFFFFFFFu && sBatchIdx >= grassBatchStart &&
            (particleBatchStart == 0xFFFFFFFFu || sBatchIdx < particleBatchStart);
        if (isGrass) {
            float tHit = q.CommittedRayT();
            if (tHit < 0.1) {
                shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
                remain -= (tHit + 0.002);
                if (remain <= 0.001) break;
                continue;
            }
            atten *= (detailAtlasIndex > 0) ? 0.88 : 0.94;
            if (atten < 0.18) { atten = 0.18; break; }
            shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
            remain -= (tHit + 0.002);
            if (remain <= 0.001) break;
            continue;
        }

        MaterialData sMat = g_Materials[sInfo.materialID];

        bool isParticle = particleBatchStart != 0xFFFFFFFFu && sBatchIdx >= particleBatchStart;
        if (isParticle) {
            float tHit = q.CommittedRayT() + 0.002;
            if (sMat.flags & MAT_FLAG_EMISSIVE) {
                shadowOrigin = shadowOrigin + shadowDir * tHit;
                remain -= tHit;
                if (remain <= 0.001) break;
                continue;
            }
            float2 sUV = GetParticleHitUV(particleVB, particleIB, sInfo,
                q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());
            float2 d = sUV * 2.0 - 1.0;
            float soft = saturate(1.0 - dot(d, d));
            soft *= soft;
            atten *= (1.0 - 0.65 * soft);
            if (atten < 0.2) { atten = 0.2; break; }
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) break;
            continue;
        }

        if (sMat.flags & MAT_FLAG_EMISSIVE) {
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) break;
            continue;
        }

        if (sMat.flags & MAT_FLAG_WATER) {
            if (nearSkinnedOccludes) {
                atten = 0;
                break;
            }
            atten *= 0.85;
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) { atten = 0; break; }
            continue;
        }

        bool isTerrain = sBatchIdx >= identityStaticCount &&
            sBatchIdx < identityStaticCount + terrainBatchCount;
        if (isTerrain) { atten = 0; break; }

        bool isSkinned = skinnedBatchStart != 0xFFFFFFFFu && sBatchIdx >= skinnedBatchStart &&
            (grassBatchStart == 0xFFFFFFFFu || sBatchIdx < grassBatchStart) &&
            (particleBatchStart == 0xFFFFFFFFu || sBatchIdx < particleBatchStart);
        if (isSkinned) {
            if (tHit0 < skinSelf) {
                shadowOrigin = shadowOrigin + shadowDir * (tHit0 + 0.002);
                remain -= (tHit0 + 0.002);
                if (remain <= 0.001) break;
                continue;
            }
            atten = 0;
            break;
        }

        float2 sUV = GetHitUV(megaVB, megaIB, sInfo, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());
        float4 sDiffuse = SampleDiffuseLevel(sMat, sUV);

        if ((sMat.flags & MAT_FLAG_ALPHA_TEST) && sDiffuse.a < sMat.alphaRef) {
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) { atten = 0; break; }
            continue;
        }
        if ((sMat.flags & MAT_FLAG_ALPHA_BLEND) && sDiffuse.a < 0.5) {
            atten *= (1.0 - sDiffuse.a);
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) { atten = 0; break; }
            continue;
        }

        atten = 0;
        break;
    }
    return atten;
}

bool TraceVisibilityClear(
    RaytracingAccelerationStructure tlas,
    StructuredBuffer<RTBatchInfo> batchInfo,
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    ByteAddressBuffer particleVB,
    ByteAddressBuffer particleIB,
    float3 origin,
    float3 target,
    uint identityStaticCount,
    uint terrainBatchCount,
    uint skinnedBatchStart,
    uint grassBatchStart,
    uint particleBatchStart,
    uint detailAtlasIndex)
{
    float3 dir = target - origin;
    float dist = length(dir);
    if (dist < 1e-4)
        return false;
    float atten = TraceVisibilityAtten(
        tlas, batchInfo, megaVB, megaIB, grassVB, grassIB, particleVB, particleIB,
        origin, dir / dist, max(dist - 0.02, 0.001),
        identityStaticCount, terrainBatchCount, skinnedBatchStart, grassBatchStart,
        particleBatchStart, detailAtlasIndex, true, 0.0);
    return atten > 0.15;
}

#endif
