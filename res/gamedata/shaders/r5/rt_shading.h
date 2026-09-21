#ifndef RT_SHADING_H
#define RT_SHADING_H

#include "bindless_common.h"
#include "rt_common.h"
#include "rt_bsdf.h"
#include "material_eval.h"

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

static const float RT_RAY_DISTANCE = 10000.0;
static const float RT_RAY_ORIGIN_OFFSET = 0.005;
static const float RT_GRASS_ALPHA_REF = 0.3;

struct RTSceneParams
{
    uint identityStaticCount;
    uint terrainBatchCount;
    uint skinnedBatchStart;
    uint grassBatchStart;
    uint detailAtlasIndex;
    uint lightCount;
    uint diffuseMode;
    uint emissiveCount;
    uint maxNullEvents;
    float3 sunDir;
    float3 sunColor;
    float skyWeight;
    float skyRotation;
    float sunAngularRadius;
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
    scene.lightCount = lightCount;
    scene.diffuseMode = diffuseMode;
    scene.emissiveCount = emissiveCount;
    scene.maxNullEvents = max(maxNullEvents, 1u);
    scene.sunDir = RTSafeNormalize(-sunDirIntensity.xyz, float3(0.0, 1.0, 0.0));
    scene.sunColor = sunColorSkyWeight.xyz * sunDirIntensity.w;
    scene.skyWeight = saturate(sunColorSkyWeight.w);
    scene.skyRotation = skyRotation;
    scene.sunAngularRadius = sunAngularRadius;
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

bool IsTerrainBatch(RTSceneParams scene, uint batchIdx)
{
    return batchIdx >= scene.identityStaticCount &&
        batchIdx < scene.identityStaticCount + scene.terrainBatchCount;
}

bool HasDetailAtlas(RTSceneParams scene)
{
    return scene.detailAtlasIndex != 0u && scene.detailAtlasIndex != INVALID_TEXTURE_INDEX;
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
    float4 diffuse;
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
    float4 diffuse;
};

RTHitClass RTClassifyHit(RTSceneParams scene, RTSceneTrace hit, float3 direction, bool shadowRay)
{
    RTHitClass result = (RTHitClass)0;
    result.transmittance = 1.0;
    result.opacity = 1.0;
    RTHitGeometry geometry = RTFetchHitGeometry(scene, hit, direction);
    if (IsGrassBatch(scene, hit.batchIdx))
    {
        result.opaque = !HasDetailAtlas(scene) || GetBindlessTexture(scene.detailAtlasIndex)
            .SampleGrad(smp_linear, geometry.uv, geometry.uvDx, geometry.uvDy).a >= RT_GRASS_ALPHA_REF;
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
    result.diffuse = SampleDiffuseGrad(mat, geometry.uv, geometry.uvDx, geometry.uvDy);
    if ((mat.flags & MAT_FLAG_ALPHA_TEST) != 0u && result.diffuse.a < mat.alphaRef)
        return result;
    if ((mat.flags & MAT_FLAG_WATER) != 0u)
    {
        result.opaque = !shadowRay;
        if (shadowRay)
        {
            RTHitSurface water = RTResolveHitSurface(scene, hit, geometry);
            result.transmittance = RTWaterTransmission() * (1.0 - RTWaterFresnel(water.surface.N, -direction));
        }
        return result;
    }
    if ((variant.flags & VARIANT_FLAG_ADDITIVE_EMISSION) != 0u)
    {
        if (!shadowRay)
            result.emissive = RTEvaluateEmitter(scene, hit.batchIdx, hit.info, geometry.uv,
                geometry.uvDx, geometry.uvDy, false);
        return result;
    }
    if ((mat.flags & MAT_FLAG_ALPHA_BLEND) != 0u)
    {
        result.opacity = saturate(result.diffuse.a);
        result.transmittance = 1.0 - result.opacity;
        result.opaque = shadowRay ? result.opacity >= 1.0 : result.opacity > 0.0;
        return result;
    }
    result.opaque = true;
    return result;
}

RTSceneTrace RTTraceRay(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    bool shadowRay, inout uint rng, float coneWidth, float coneSpread,
    float3 previousPosition, float previousPdf, bool previousDelta)
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
        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_FORCE_NON_OPAQUE> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
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
                RTHitClass classification = RTClassifyHit(scene, candidate, direction, shadowRay);
                if (classification.opaque || any(classification.transmittance < 1.0) || any(classification.emissive > 0.0))
                    q.CommitNonOpaqueTriangleHit();
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
        RTHitClass hitClass = RTClassifyHit(scene, trace, direction, shadowRay);
        bool accept = hitClass.opaque;
        if (accept && hitClass.opacity < 1.0)
            accept = rand_float(rng) < hitClass.opacity;
        if (accept)
        {
            trace.hit = true;
            trace.diffuse = hitClass.diffuse;
            return trace;
        }
        if (!hitClass.opaque)
        {
            float emissionWeight = shadowRay ? 1.0 : RTEmissionWeight(scene, trace.batchIdx, trace.info,
                trace.primitiveIndex, previousPosition, origin + direction * trace.t, previousPdf, previousDelta);
            trace.emissive += trace.transmittance * hitClass.emissive * emissionWeight;
            trace.transmittance *= hitClass.transmittance;
            if (!any(trace.transmittance > 0.0))
                return trace;
        }
        ++trace.nullEvents;
        rayMinDistance = asfloat(asuint(trace.t) + 1u);
    }
    return trace;
}

float3 RTTraceVisibility(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    float coneWidth, float coneSpread, out bool exhausted)
{
    uint rng = 0u;
    RTSceneTrace trace = RTTraceRay(scene, origin, direction, maxDistance, true, rng,
        coneWidth, coneSpread, origin, 0.0, true);
    exhausted = trace.exhausted;
    return trace.hit ? float3(0.0, 0.0, 0.0) : trace.transmittance;
}

#endif
