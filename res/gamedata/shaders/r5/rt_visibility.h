#ifndef RT_VISIBILITY_H
#define RT_VISIBILITY_H

#include "bindless_common.h"
#include "rt_common.h"
#include "rt_grass_alpha.h"
#include "rt_particle_alpha.h"
#include "rt_material_alpha.h"

static const uint RT_VIS_MAX_SKIPS = 16;
static const uint RT_VIS_MAX_SELF = 8;
static const uint RT_VIS_GRASS_RESTARTS = 3;

bool VisIsHudBatch(uint batchIdx, uint hudSkinnedStart, uint grassBatchStart, uint particleBatchStart)
{
    return hudSkinnedStart != 0xFFFFFFFFu && batchIdx >= hudSkinnedStart &&
        (grassBatchStart == 0xFFFFFFFFu || batchIdx < grassBatchStart) &&
        (particleBatchStart == 0xFFFFFFFFu || batchIdx < particleBatchStart);
}

bool VisIsGrassBatch(uint batchIdx, uint grassBatchStart, uint particleBatchStart)
{
    return grassBatchStart != 0xFFFFFFFFu && batchIdx >= grassBatchStart &&
        (particleBatchStart == 0xFFFFFFFFu || batchIdx < particleBatchStart);
}

bool VisIsSkinnedBatch(uint batchIdx, uint skinnedBatchStart, uint grassBatchStart, uint particleBatchStart)
{
    return skinnedBatchStart != 0xFFFFFFFFu && batchIdx >= skinnedBatchStart &&
        (grassBatchStart == 0xFFFFFFFFu || batchIdx < grassBatchStart) &&
        (particleBatchStart == 0xFFFFFFFFu || batchIdx < particleBatchStart);
}

float SampleSTBN(Texture3D<float> BlueNoiseTex, uint2 pixel, uint frameIndex, uint skip)
{
    uint w = 0, h = 0, d = 0;
    BlueNoiseTex.GetDimensions(w, h, d);
    if (w < 8u || h < 8u || d < 1u)
    {
        uint hsh = pcg_hash(pixel.x + pixel.y * 198491317u + frameIndex * 747796405u + skip * 1103515245u);
        return float(hsh) * (1.0 / 4294967295.0);
    }
    uint z = (frameIndex + skip * 3u) % d;
    return BlueNoiseTex.Load(int4(int(pixel.x % w), int(pixel.y % h), int(z), 0));
}

float FoliageAlphaCut(MaterialData mat)
{
    return max(mat.alphaRef, 0.5);
}

bool VisMegaAlphaKeep(
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    MaterialData mat = g_Materials[info.materialID];
    if ((mat.flags & MAT_FLAG_EMISSIVE) != 0)
        return false;
    if ((mat.flags & MAT_FLAG_WATER) != 0)
        return false;
    float2 uv = GetHitUV(megaVB, megaIB, info, primIdx, bary);
    float4 diffuse = SampleDiffuseAlpha(mat, uv);
    if ((mat.flags & MAT_FLAG_FOLIAGE) != 0 ||
        ((mat.flags & MAT_FLAG_ALPHA_TEST) != 0 && (mat.flags & MAT_FLAG_ALPHA_BLEND) == 0))
        return diffuse.a >= FoliageAlphaCut(mat);
    return MaterialDiffuseOpaque(mat, diffuse);
}

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
    uint instanceMask,
    uint identityStaticCount,
    uint terrainBatchCount,
    uint skinnedBatchStart,
    uint grassBatchStart,
    uint particleBatchStart,
    uint detailAtlasIndex,
    bool nearSkinnedOccludes,
    float skinnedSelfMax,
    uint hudSkinnedStart,
    Texture3D<float> BlueNoiseTex,
    uint2 noisePixel,
    uint frameIndex)
{
    float atten = 1.0;
    float3 shadowOrigin = origin;
    float3 shadowDir = normalize(dir);
    float remain = tMax;
    const float selfSkip = nearSkinnedOccludes ? 0.004 : 0.02;
    const float skinSelf = max(skinnedSelfMax, 0.0);
    uint skips = 0;
    uint selfs = 0;
    while (skips < RT_VIS_MAX_SKIPS) {
        RayDesc ray;
        ray.Origin = shadowOrigin;
        ray.Direction = shadowDir;
        ray.TMin = 0.001;
        ray.TMax = remain;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(tlas, RAY_FLAG_NONE, instanceMask, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (VisIsHudBatch(candBatch, hudSkinnedStart, grassBatchStart, particleBatchStart))
                    continue;
                bool isParticle = particleBatchStart != 0xFFFFFFFFu && candBatch >= particleBatchStart;
                if (isParticle)
                    continue;
                if (VisIsGrassBatch(candBatch, grassBatchStart, particleBatchStart)) {
                    if (GrassTexelOpaque(grassVB, grassIB, batchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(),
                            detailAtlasIndex))
                        q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                if (VisIsSkinnedBatch(candBatch, skinnedBatchStart, grassBatchStart, particleBatchStart)) {
                    q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                if (VisMegaAlphaKeep(megaVB, megaIB, batchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))
                    q.CommitNonOpaqueTriangleHit();
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            break;

        float tHit0 = q.CommittedRayT();
        if (tHit0 < selfSkip) {
            float step = (selfs >= 3u) ? 0.04 : 0.002;
            shadowOrigin = shadowOrigin + shadowDir * (tHit0 + step);
            remain -= (tHit0 + step);
            selfs++;
            if (remain <= 0.001 || selfs >= RT_VIS_MAX_SELF)
                break;
            continue;
        }

        uint sBatchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo sInfo = batchInfo[sBatchIdx];

        if (VisIsHudBatch(sBatchIdx, hudSkinnedStart, grassBatchStart, particleBatchStart)) {
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            if (remain <= 0.001) break;
            continue;
        }

        if (VisIsGrassBatch(sBatchIdx, grassBatchStart, particleBatchStart)) {
            atten = 0;
            break;
        }

        MaterialData sMat = g_Materials[sInfo.materialID];

        bool isParticle = particleBatchStart != 0xFFFFFFFFu && sBatchIdx >= particleBatchStart;
        if (isParticle) {
            float tHit = q.CommittedRayT() + 0.002;
            if ((sMat.flags & MAT_FLAG_EMISSIVE) != 0) {
                shadowOrigin = shadowOrigin + shadowDir * tHit;
                remain -= tHit;
                skips++;
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
            skips++;
            if (remain <= 0.001) break;
            continue;
        }

        if ((sMat.flags & MAT_FLAG_EMISSIVE) != 0) {
            float tHit = q.CommittedRayT();
            if (tHit > remain * 0.82) {
                shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
                remain -= (tHit + 0.002);
                skips++;
                if (remain <= 0.001) break;
                continue;
            }
            atten = 0;
            break;
        }

        if ((sMat.flags & MAT_FLAG_WATER) != 0) {
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            skips++;
            if (remain <= 0.001) break;
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
        float4 sDiffuse = SampleDiffuseAlpha(sMat, sUV);

        if ((sMat.flags & MAT_FLAG_FOLIAGE) != 0 ||
            ((sMat.flags & MAT_FLAG_ALPHA_TEST) != 0 && (sMat.flags & MAT_FLAG_ALPHA_BLEND) == 0)) {
            atten = 0;
            break;
        }

        if (!MaterialDiffuseOpaque(sMat, sDiffuse)) {
            if ((sMat.flags & MAT_FLAG_ALPHA_BLEND) != 0)
                atten *= (1.0 - sDiffuse.a);
            float tHit = q.CommittedRayT() + 0.002;
            shadowOrigin = shadowOrigin + shadowDir * tHit;
            remain -= tHit;
            skips++;
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
    uint detailAtlasIndex,
    uint hudSkinnedStart,
    Texture3D<float> BlueNoiseTex,
    uint2 noisePixel,
    uint frameIndex)
{
    float3 dir = target - origin;
    float dist = length(dir);
    if (dist < 1e-4)
        return false;
    float atten = TraceVisibilityAtten(
        tlas, batchInfo, megaVB, megaIB, grassVB, grassIB, particleVB, particleIB,
        origin, dir / dist, max(dist - 0.02, 0.001), RT_MASK_SHADOW,
        identityStaticCount, terrainBatchCount, skinnedBatchStart, grassBatchStart,
        particleBatchStart, detailAtlasIndex, true, 0.0, hudSkinnedStart,
        BlueNoiseTex, noisePixel, frameIndex);
    return atten > 0.15;
}

float EvaluateSunVisibilityWithGrass(
    RaytracingAccelerationStructure tlas,
    StructuredBuffer<RTBatchInfo> batchInfo,
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    float3 origin,
    float3 sunDir,
    float tMax,
    uint identityStaticCount,
    uint terrainBatchCount,
    uint skinnedBatchStart,
    uint grassBatchStart,
    uint particleBatchStart,
    uint detailAtlasIndex,
    uint hudSkinnedStart,
    Texture3D<float> BlueNoiseTex,
    uint2 pixel,
    uint frameIndex,
    uint instanceMask)
{
    float3 shadowOrigin = origin;
    float3 shadowDir = normalize(sunDir);
    float remain = max(tMax, 0.001);
    uint skips = 0;

    while (skips < RT_VIS_GRASS_RESTARTS) {
        RayDesc ray;
        ray.Origin = shadowOrigin;
        ray.Direction = shadowDir;
        ray.TMin = 0.001;
        ray.TMax = remain;

        RayQuery<RAY_FLAG_NONE> q;
        q.TraceRayInline(tlas, RAY_FLAG_NONE, instanceMask, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (VisIsHudBatch(candBatch, hudSkinnedStart, grassBatchStart, particleBatchStart))
                    continue;
                if (VisIsGrassBatch(candBatch, grassBatchStart, particleBatchStart)) {
                    if (GrassTexelOpaque(grassVB, grassIB, batchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(),
                            detailAtlasIndex))
                        q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                if (VisIsSkinnedBatch(candBatch, skinnedBatchStart, grassBatchStart, particleBatchStart)) {
                    q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                if (VisMegaAlphaKeep(megaVB, megaIB, batchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))
                    q.CommitNonOpaqueTriangleHit();
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return 1.0;

        float tHit = q.CommittedRayT();
        uint sBatchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();

        if (VisIsHudBatch(sBatchIdx, hudSkinnedStart, grassBatchStart, particleBatchStart)) {
            shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
            remain -= (tHit + 0.002);
            if (remain <= 0.001)
                return 1.0;
            skips++;
            continue;
        }

        if (VisIsGrassBatch(sBatchIdx, grassBatchStart, particleBatchStart)) {
            return 0.0;
        }

        RTBatchInfo sInfo = batchInfo[sBatchIdx];
        MaterialData sMat = g_Materials[sInfo.materialID];

        if ((sMat.flags & MAT_FLAG_WATER) != 0 || (sMat.flags & MAT_FLAG_EMISSIVE) != 0) {
            shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
            remain -= (tHit + 0.002);
            if (remain <= 0.001)
                return 1.0;
            skips++;
            continue;
        }

        bool isSkinned = skinnedBatchStart != 0xFFFFFFFFu && sBatchIdx >= skinnedBatchStart &&
            (grassBatchStart == 0xFFFFFFFFu || sBatchIdx < grassBatchStart) &&
            (particleBatchStart == 0xFFFFFFFFu || sBatchIdx < particleBatchStart);
        if (isSkinned && tHit < 0.04) {
            shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
            remain -= (tHit + 0.002);
            if (remain <= 0.001)
                return 1.0;
            skips++;
            continue;
        }

        if (sBatchIdx >= identityStaticCount &&
            sBatchIdx < identityStaticCount + terrainBatchCount)
            return 0.0;

        float2 sUV = GetHitUV(megaVB, megaIB, sInfo, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());
        float4 sDiffuse = SampleDiffuseAlpha(sMat, sUV);
        if ((sMat.flags & MAT_FLAG_FOLIAGE) != 0 ||
            ((sMat.flags & MAT_FLAG_ALPHA_TEST) != 0 && (sMat.flags & MAT_FLAG_ALPHA_BLEND) == 0))
            return 0.0;
        if (!MaterialDiffuseOpaque(sMat, sDiffuse)) {
            shadowOrigin = shadowOrigin + shadowDir * (tHit + 0.002);
            remain -= (tHit + 0.002);
            if (remain <= 0.001)
                return 1.0;
            skips++;
            continue;
        }
        return 0.0;
    }
    return 1.0;
}

#endif
