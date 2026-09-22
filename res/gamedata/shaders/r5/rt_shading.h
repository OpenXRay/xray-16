#ifndef RT_SHADING_H
#define RT_SHADING_H

#include "bindless_common.h"
#include "rt_common.h"
#include "rt_bsdf.h"
#include "material_eval.h"
#include "material_coverage.h"

#ifndef CLUSTERED_LIGHTING_PUNCTUAL
#define CLUSTERED_LIGHTING_PUNCTUAL
#endif
#include "shared/clustered_lighting.h"

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
TextureCube<float4> g_Sky0 : register(t5);
TextureCube<float4> g_Sky1 : register(t6);
ByteAddressBuffer g_SkinnedVB : register(t7);
ByteAddressBuffer g_SkinnedIB : register(t11);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
StructuredBuffer<float4> g_GrassMaterials : register(t30);
StructuredBuffer<uint2> g_ClusterGrid : register(t21);
StructuredBuffer<uint> g_LightIndexList : register(t22);

static const uint RT_GRASS_MATERIAL_TINT_BASE = 2u;

static const float RT_RAY_DISTANCE = 10000.0;
static const float RT_RAY_ORIGIN_OFFSET = 0.005;
#define RT_RAY_MASK_WORLD 0x01u
#define RT_RAY_MASK_HUD 0x02u

struct RTSceneParams
{
    uint identityStaticCount;
    uint terrainBatchCount;
    uint skinnedBatchStart;
    uint grassBatchStart;
    uint detailAtlasIndex;
    uint detailMeshBatchStart;
    uint staticDetailBatchStart;
    uint detailPbrIndex;
    uint detailBumpIndex;
    uint lightCount;
    uint diffuseMode;
    uint emissiveCount;
    uint maxNullEvents;
    float3 sunDir;
    float3 sunColor;
    float skyWeight;
    float skyRotation;
    float sunAngularRadius;
    uint rayMask;
    float rayDistance;
    uint clusterLights;
    uint2 clusterPixel;
};

RTSceneParams RTBuildSceneParams(uint identityStaticCount, uint terrainBatchCount,
    uint skinnedBatchStart, uint grassBatchStart, uint detailAtlasIndex,
    uint lightCount, uint diffuseMode, float4 sunDirIntensity, float4 sunColorSkyWeight,
    uint emissiveCount, uint maxNullEvents, float skyRotation, float sunAngularRadius)
{
    RTSceneParams scene;
    scene.identityStaticCount = identityStaticCount;
    scene.terrainBatchCount = terrainBatchCount;
    scene.skinnedBatchStart = skinnedBatchStart;
    scene.grassBatchStart = grassBatchStart;
    scene.detailAtlasIndex = detailAtlasIndex;
    scene.detailMeshBatchStart = grassBatchStart;
    scene.staticDetailBatchStart = 0xFFFFFFFFu;
    scene.detailPbrIndex = 0u;
    scene.detailBumpIndex = 0u;
    scene.lightCount = lightCount;
    scene.diffuseMode = diffuseMode;
    scene.emissiveCount = emissiveCount;
    scene.maxNullEvents = max(maxNullEvents, 1u);
    scene.sunDir = RTSafeNormalize(-sunDirIntensity.xyz, float3(0.0, 1.0, 0.0));
    scene.sunColor = sunColorSkyWeight.xyz * sunDirIntensity.w;
    scene.skyWeight = saturate(sunColorSkyWeight.w);
    scene.skyRotation = skyRotation;
    scene.sunAngularRadius = sunAngularRadius;
    scene.rayMask = RT_RAY_MASK_WORLD;
    scene.rayDistance = RT_RAY_DISTANCE;
    scene.clusterLights = 0u;
    scene.clusterPixel = uint2(0u, 0u);
    return scene;
}

bool IsSkinnedBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.skinnedBatchStart != 0xFFFFFFFFu && batchIdx >= scene.skinnedBatchStart &&
        !(scene.grassBatchStart != 0xFFFFFFFFu && batchIdx >= scene.grassBatchStart);
}

bool IsGrassBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.grassBatchStart != 0xFFFFFFFFu && batchIdx >= scene.grassBatchStart;
}

bool IsDetailMeshBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.detailMeshBatchStart != 0xFFFFFFFFu && batchIdx >= scene.detailMeshBatchStart &&
        !(scene.staticDetailBatchStart != 0xFFFFFFFFu && batchIdx >= scene.staticDetailBatchStart);
}

bool IsStaticDetailBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.staticDetailBatchStart != 0xFFFFFFFFu && batchIdx >= scene.staticDetailBatchStart;
}

bool IsTerrainBatch(RTSceneParams scene, uint batchIdx)
{
    return batchIdx >= scene.identityStaticCount &&
        batchIdx < scene.identityStaticCount + scene.terrainBatchCount;
}

bool HasDetailAtlas(RTSceneParams scene)
{
    return scene.detailAtlasIndex != 0u && scene.detailAtlasIndex != INVALID_TEXTURE_INDEX;
}

bool HasDetailBump(RTSceneParams scene)
{
    return scene.detailBumpIndex != 0u && scene.detailBumpIndex != INVALID_TEXTURE_INDEX;
}

bool HasDetailPbr(RTSceneParams scene)
{
    return scene.detailPbrIndex != 0u && scene.detailPbrIndex != INVALID_TEXTURE_INDEX;
}

struct RTSceneTrace
{
    bool hit;
    float3 transmittance;
    float3 emissive;
    uint batchIdx;
    RTBatchInfo info;
    uint primitiveIndex;
    float2 barycentrics;
    float t;
    float3x4 objectToWorld;
    float coneWidth;
    float coneSpread;
    bool frontFace;
    bool exhausted;
    uint nullEvents;
};

#include "rt_material.h"
#include "rt_lighting.h"

struct RTHitClass
{
    bool opaque;
    float3 transmittance;
    float opacity;
    float3 emissive;
};

RTHitClass RTClassifyHit(RTSceneParams scene, RTSceneTrace hit, float3 direction,
    bool shadowRay, bool resolveTransmission)
{
    RTHitClass result = (RTHitClass)0;
    result.transmittance = 1.0;
    result.opacity = 1.0;
    if (IsGrassBatch(scene, hit.batchIdx))
    {
        bool wavingCard = IsDetailMeshBatch(scene, hit.batchIdx);
        if (!wavingCard && !IsStaticDetailBatch(scene, hit.batchIdx))
        {
            result.opaque = true;
            return result;
        }
        RTHitGeometry coverage = RTFetchHitCoverage(scene, hit, direction, !(shadowRay && wavingCard));
        result.opaque = RTPulledDetailOpaque(scene, coverage, wavingCard, shadowRay);
        return result;
    }
    if (IsTerrainBatch(scene, hit.batchIdx))
    {
        result.opaque = true;
        return result;
    }

    MaterialData mat = g_Materials[hit.info.materialID];
    VariantData variant = g_Variants[mat.shaderVariant];
    if (shadowRay && (variant.flags & VARIANT_FLAG_NO_SHADOW) != 0u)
        return result;
    bool water = (mat.flags & MAT_FLAG_WATER) != 0u;
    bool additive = (variant.flags & VARIANT_FLAG_ADDITIVE_EMISSION) != 0u;
    if (shadowRay && additive && !water)
        return result;

    bool alphaCoverage = MaterialHasAlphaCoverage(mat);
    RTHitGeometry coverage = (RTHitGeometry)0;
    if (alphaCoverage || (!shadowRay && additive && !water))
        coverage = RTFetchHitCoverage(scene, hit, direction, !shadowRay);
    float coverageAlpha = 1.0;
    if (alphaCoverage)
    {
        coverageAlpha = shadowRay ? MaterialRayShadowAlpha(mat, coverage.uv) :
            SampleDiffuseGrad(mat, coverage.uv, coverage.uvDx, coverage.uvDy).a;
    }
    if (MaterialAlphaTestRejects(mat, coverageAlpha))
        return result;
    if (water)
    {
        result.opaque = !shadowRay;
        if (shadowRay)
        {
            result.transmittance = 0.0;
            if (resolveTransmission)
            {
                RTHitGeometry geometry = RTFetchHitGeometry(scene, hit, direction);
                RTHitSurface surface = RTResolveHitSurface(scene, hit, geometry);
                result.transmittance = RTWaterTransmission() * (1.0 - RTWaterFresnel(surface.surface.N, -direction));
            }
        }
        return result;
    }
    if (additive)
    {
        if (!shadowRay)
            result.emissive = RTEvaluateEmitter(scene, hit.batchIdx, hit.info, coverage.uv,
                coverage.uvDx, coverage.uvDy, false);
        return result;
    }
    if (MaterialHasAlphaBlend(mat))
    {
        result.opacity = MaterialBlendOpacity(mat, coverageAlpha);
        result.transmittance = 1.0 - result.opacity;
        result.opaque = shadowRay ? result.opacity >= 1.0 : result.opacity > 0.0;
        return result;
    }
    result.opaque = true;
    return result;
}

RTSceneTrace RTTraceRay(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    bool shadowRay, inout uint rng, float coneWidth, float coneSpread,
    float3 previousPosition, float previousPdf, bool previousDelta,
    uint rayMask = RT_RAY_MASK_WORLD)
{
    RTSceneTrace trace = (RTSceneTrace)0;
    trace.transmittance = 1.0;
    trace.coneWidth = coneWidth;
    trace.coneSpread = coneSpread;
    float rayMinDistance = 0.001;
    uint candidates = 0u;
    while (rayMinDistance < maxDistance)
    {
        if (trace.nullEvents >= scene.maxNullEvents)
        {
            trace.exhausted = true;
            trace.transmittance = 0.0;
            return trace;
        }
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = direction;
        ray.TMin = rayMinDistance;
        ray.TMax = maxDistance;
        uint4 cachedHit = uint4(0xFFFFFFFFu, 0u, 0u, 0u);
        RTHitClass cachedClassification = (RTHitClass)0;
        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, rayMask, ray);
        while (q.Proceed())
        {
            if (++candidates > scene.maxNullEvents * 64u)
            {
                q.Abort();
                trace.exhausted = true;
                trace.transmittance = 0.0;
                return trace;
            }
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                RTSceneTrace candidate = trace;
                candidate.batchIdx = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                candidate.info = g_BatchInfo[candidate.batchIdx];
                candidate.primitiveIndex = q.CandidatePrimitiveIndex();
                candidate.barycentrics = q.CandidateTriangleBarycentrics();
                candidate.t = q.CandidateTriangleRayT();
                candidate.objectToWorld = q.CandidateObjectToWorld3x4();
                RTHitClass classification = RTClassifyHit(scene, candidate, direction, shadowRay, false);
                bool commit = false;
                if (classification.opaque)
                    commit = shadowRay || classification.opacity >= 1.0 || rand_float(rng) < classification.opacity;
                else
                    commit = any(classification.transmittance < 1.0) || any(classification.emissive > 0.0);
                if (commit)
                {
                    if (!shadowRay)
                    {
                        cachedHit = uint4(q.CandidateInstanceIndex(), q.CandidateGeometryIndex(),
                            candidate.primitiveIndex, asuint(candidate.t));
                        cachedClassification = classification;
                    }
                    q.CommitNonOpaqueTriangleHit();
                }
            }
        }
        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return trace;
        trace.batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        trace.info = g_BatchInfo[trace.batchIdx];
        trace.primitiveIndex = q.CommittedPrimitiveIndex();
        trace.barycentrics = q.CommittedTriangleBarycentrics();
        trace.t = q.CommittedRayT();
        trace.objectToWorld = q.CommittedObjectToWorld3x4();
        trace.frontFace = q.CommittedTriangleFrontFace();
        RTHitClass hitClass;
        if (!shadowRay && all(cachedHit == uint4(q.CommittedInstanceIndex(), q.CommittedGeometryIndex(),
            trace.primitiveIndex, asuint(trace.t))))
        {
            hitClass = cachedClassification;
        }
        else
        {
            hitClass = RTClassifyHit(scene, trace, direction, shadowRay, true);
        }
        if (hitClass.opaque)
        {
            trace.hit = true;
            return trace;
        }
        float emissionWeight = shadowRay ? 1.0 : RTEmissionWeight(scene, trace.batchIdx, trace.info,
            trace.primitiveIndex, previousPosition, origin + direction * trace.t, previousPdf, previousDelta);
        trace.emissive += trace.transmittance * hitClass.emissive * emissionWeight;
        trace.transmittance *= hitClass.transmittance;
        if (!any(trace.transmittance > 0.0))
            return trace;
        ++trace.nullEvents;
        rayMinDistance = asfloat(asuint(trace.t) + 1u);
    }
    return trace;
}

float3 RTTraceVisibility(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    float coneWidth, float coneSpread, out bool exhausted)
{
    exhausted = false;
    if (!(maxDistance > 0.001))
        return 1.0;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDistance;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_RAY_MASK_WORLD, ray);
    float3 transmittance = 1.0;
    uint candidates = 0u;
    uint nullEvents = 0u;
    while (q.Proceed())
    {
        if (++candidates > scene.maxNullEvents * 64u)
        {
            q.Abort();
            exhausted = true;
            return 0.0;
        }
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            RTSceneTrace candidate = (RTSceneTrace)0;
            candidate.batchIdx = q.CandidateInstanceID() + q.CandidateGeometryIndex();
            candidate.info = g_BatchInfo[candidate.batchIdx];
            candidate.primitiveIndex = q.CandidatePrimitiveIndex();
            candidate.barycentrics = q.CandidateTriangleBarycentrics();
            candidate.t = q.CandidateTriangleRayT();
            candidate.objectToWorld = q.CandidateObjectToWorld3x4();
            candidate.coneWidth = coneWidth;
            candidate.coneSpread = coneSpread;
            RTHitClass classification = RTClassifyHit(scene, candidate, direction, true, true);
            if (classification.opaque)
            {
                q.CommitNonOpaqueTriangleHit();
                q.Abort();
                return 0.0;
            }
            if (any(classification.transmittance < 1.0))
            {
                transmittance *= classification.transmittance;
                if (!any(transmittance > 0.0))
                {
                    q.Abort();
                    return 0.0;
                }
                if (++nullEvents >= scene.maxNullEvents)
                {
                    q.Abort();
                    exhausted = true;
                    return 0.0;
                }
            }
        }
    }
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        return 0.0;
    return transmittance;
}

#endif
