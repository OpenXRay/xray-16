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
static const float RT_WATER_TRANSMITTANCE = 0.85;

struct RTSceneParams
{
    uint identityStaticCount;
    uint terrainBatchCount;
    uint skinnedBatchStart;
    uint grassBatchStart;
    uint detailAtlasIndex;
    uint lightCount;
    uint diffuseMode;
    float3 sunDir;
    float3 sunColor;
    float skyWeight;
};

RTSceneParams RTBuildSceneParams(uint identityStaticCount, uint terrainBatchCount,
    uint skinnedBatchStart, uint grassBatchStart, uint detailAtlasIndex,
    uint lightCount, uint diffuseMode, float4 sunDirIntensity, float4 sunColorSkyWeight)
{
    RTSceneParams scene;
    scene.identityStaticCount = identityStaticCount;
    scene.terrainBatchCount = terrainBatchCount;
    scene.skinnedBatchStart = skinnedBatchStart;
    scene.grassBatchStart = grassBatchStart;
    scene.detailAtlasIndex = detailAtlasIndex;
    scene.lightCount = lightCount;
    scene.diffuseMode = diffuseMode;
    scene.sunDir = RTSafeNormalize(-sunDirIntensity.xyz, float3(0.0, 1.0, 0.0));
    scene.sunColor = sunColorSkyWeight.xyz * sunDirIntensity.w;
    scene.skyWeight = sunColorSkyWeight.w;
    return scene;
}

bool IsSkinnedBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.skinnedBatchStart > 0 && batchIdx >= scene.skinnedBatchStart &&
        !(scene.grassBatchStart > 0 && batchIdx >= scene.grassBatchStart);
}

bool IsGrassBatch(RTSceneParams scene, uint batchIdx)
{
    return scene.grassBatchStart > 0 && batchIdx >= scene.grassBatchStart;
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

float2 GrassHitUV(RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint i0, i1, i2;
    RTLoadTriangleIndices(g_GrassIB, info, primitiveIndex, i0, i1, i2);
    return RTInterpolateUV(RTLoadGrassVertexUV(g_GrassVB, i0), RTLoadGrassVertexUV(g_GrassVB, i1),
        RTLoadGrassVertexUV(g_GrassVB, i2), barycentrics);
}

float GrassHitAlpha(RTSceneParams scene, RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    return GetBindlessTexture(scene.detailAtlasIndex).SampleLevel(smp_linear, GrassHitUV(info, primitiveIndex, barycentrics), 0).a;
}

float2 MaterialHitUV(RTSceneParams scene, uint batchIdx, RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint i0, i1, i2;
    if (IsSkinnedBatch(scene, batchIdx))
    {
        RTLoadTriangleIndices(g_SkinnedIB, info, primitiveIndex, i0, i1, i2);
        return RTInterpolateUV(RTLoadSkinnedVertexUV(g_SkinnedVB, i0), RTLoadSkinnedVertexUV(g_SkinnedVB, i1),
            RTLoadSkinnedVertexUV(g_SkinnedVB, i2), barycentrics);
    }
    RTLoadTriangleIndices(g_MegaIB, info, primitiveIndex, i0, i1, i2);
    return RTInterpolateUV(RTLoadStaticVertexUV(g_MegaVB, i0), RTLoadStaticVertexUV(g_MegaVB, i1),
        RTLoadStaticVertexUV(g_MegaVB, i2), barycentrics);
}

struct RTHitClass
{
    bool opaque;
    float transmittance;
    float opacity;
    float3 emissive;
    float4 diffuse;
};

RTHitClass RTClassifyHit(RTSceneParams scene, uint batchIdx, RTBatchInfo info, uint primitiveIndex,
    float2 barycentrics, bool shadowRay)
{
    RTHitClass result;
    result.opaque = false;
    result.transmittance = 1.0;
    result.opacity = 1.0;
    result.emissive = 0.0;
    result.diffuse = 0.0;

    if (IsGrassBatch(scene, batchIdx))
    {
        result.opaque = !HasDetailAtlas(scene) ||
            GrassHitAlpha(scene, info, primitiveIndex, barycentrics) >= RT_GRASS_ALPHA_REF;
        return result;
    }

    if (IsTerrainBatch(scene, batchIdx))
    {
        result.opaque = true;
        return result;
    }

    MaterialData mat = g_Materials[info.materialID];
    VariantData variant = g_Variants[mat.shaderVariant];
    if (shadowRay && (variant.flags & VARIANT_FLAG_NO_SHADOW) != 0)
        return result;

    if ((mat.flags & MAT_FLAG_WATER) != 0)
    {
        result.opaque = !shadowRay;
        result.transmittance = shadowRay ? RT_WATER_TRANSMITTANCE : 1.0;
        return result;
    }

    result.diffuse = SampleDiffuseLevel(mat, MaterialHitUV(scene, batchIdx, info, primitiveIndex, barycentrics));
    if ((mat.flags & MAT_FLAG_ALPHA_TEST) != 0 && result.diffuse.a < mat.alphaRef)
        return result;

    if ((variant.flags & VARIANT_FLAG_ADDITIVE_EMISSION) != 0)
    {
        if (!shadowRay)
        {
            float coverage = (variant.flags & VARIANT_FLAG_EMISSION_ALPHA) != 0 ? saturate(result.diffuse.a) : 1.0;
            result.emissive = result.diffuse.rgb * variant.emissive * coverage;
        }
        return result;
    }

    if ((mat.flags & MAT_FLAG_ALPHA_BLEND) != 0)
    {
        result.opacity = saturate(result.diffuse.a);
        result.transmittance = 1.0 - result.opacity;
        result.opaque = shadowRay ? result.opacity >= 1.0 : result.opacity > 0.0;
        return result;
    }

    result.opaque = true;
    return result;
}

struct RTSceneTrace
{
    bool hit;
    float transmittance;
    float3 emissive;
    uint batchIdx;
    RTBatchInfo info;
    uint primitiveIndex;
    float2 barycentrics;
    float t;
    float3x4 objectToWorld;
    float4 diffuse;
};

RTSceneTrace RTTraceRay(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    bool shadowRay, inout uint rng)
{
    RTSceneTrace trace;
    trace.hit = false;
    trace.transmittance = 1.0;
    trace.emissive = 0.0;
    trace.batchIdx = 0;
    trace.primitiveIndex = 0;
    trace.barycentrics = 0.0;
    trace.t = 0.0;
    trace.diffuse = 0.0;

    float rayMinDistance = 0.001;
    while (rayMinDistance < maxDistance)
    {
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = direction;
        ray.TMin = rayMinDistance;
        ray.TMax = maxDistance;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_FORCE_NON_OPAQUE> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
        while (q.Proceed())
        {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                uint candidateBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                RTHitClass candidate = RTClassifyHit(scene, candidateBatch, g_BatchInfo[candidateBatch],
                    q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(), shadowRay);
                if (candidate.opaque || candidate.transmittance < 1.0 || any(candidate.emissive > 0.0))
                    q.CommitNonOpaqueTriangleHit();
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return trace;

        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo info = g_BatchInfo[batchIdx];
        uint primitiveIndex = q.CommittedPrimitiveIndex();
        float2 barycentrics = q.CommittedTriangleBarycentrics();
        float t = q.CommittedRayT();
        RTHitClass hitClass = RTClassifyHit(scene, batchIdx, info, primitiveIndex, barycentrics, shadowRay);

        bool accept = hitClass.opaque;
        if (accept && hitClass.opacity < 1.0)
            accept = rand_float(rng) < hitClass.opacity;
        if (accept)
        {
            trace.hit = true;
            trace.batchIdx = batchIdx;
            trace.info = info;
            trace.primitiveIndex = primitiveIndex;
            trace.barycentrics = barycentrics;
            trace.t = t;
            trace.objectToWorld = q.CommittedObjectToWorld3x4();
            trace.diffuse = hitClass.diffuse;
            return trace;
        }

        if (!hitClass.opaque)
        {
            trace.emissive += trace.transmittance * hitClass.emissive;
            trace.transmittance *= hitClass.transmittance;
            if (trace.transmittance <= 0.0)
                return trace;
        }

        rayMinDistance = asfloat(asuint(t) + 1u);
    }

    return trace;
}

float RTTraceVisibility(RTSceneParams scene, float3 origin, float3 direction, float maxDistance)
{
    uint rng = 0;
    RTSceneTrace trace = RTTraceRay(scene, origin, direction, maxDistance, true, rng);
    return trace.hit ? 0.0 : trace.transmittance;
}

struct RTHitGeometry
{
    float2 uv;
    float3 normal;
    float3 geoNormal;
    float3 tangent;
    float3 bitangent;
};

RTHitGeometry RTFetchHitGeometry(RTSceneParams scene, RTSceneTrace trace, float3 rayDirection)
{
    RTHitGeometry geometry;

    uint i0, i1, i2;
    RTTriangleVertex v0, v1, v2;
    if (IsGrassBatch(scene, trace.batchIdx))
    {
        RTLoadTriangleIndices(g_GrassIB, trace.info, trace.primitiveIndex, i0, i1, i2);
        v0 = RTLoadGrassVertex(g_GrassVB, i0);
        v1 = RTLoadGrassVertex(g_GrassVB, i1);
        v2 = RTLoadGrassVertex(g_GrassVB, i2);
    }
    else if (IsSkinnedBatch(scene, trace.batchIdx))
    {
        RTLoadTriangleIndices(g_SkinnedIB, trace.info, trace.primitiveIndex, i0, i1, i2);
        v0 = RTLoadSkinnedVertex(g_SkinnedVB, i0);
        v1 = RTLoadSkinnedVertex(g_SkinnedVB, i1);
        v2 = RTLoadSkinnedVertex(g_SkinnedVB, i2);
    }
    else
    {
        RTLoadTriangleIndices(g_MegaIB, trace.info, trace.primitiveIndex, i0, i1, i2);
        v0 = RTLoadStaticVertex(g_MegaVB, i0);
        v1 = RTLoadStaticVertex(g_MegaVB, i1);
        v2 = RTLoadStaticVertex(g_MegaVB, i2);
    }

    RTShadingVertex vertex = RTInterpolateTriangleVertex(v0, v1, v2, trace.barycentrics);
    bool worldSpaceVertices = IsGrassBatch(scene, trace.batchIdx) || IsSkinnedBatch(scene, trace.batchIdx);
    float3 p0 = worldSpaceVertices ? v0.position : TransformPointToWorld(v0.position, trace.objectToWorld);
    float3 p1 = worldSpaceVertices ? v1.position : TransformPointToWorld(v1.position, trace.objectToWorld);
    float3 p2 = worldSpaceVertices ? v2.position : TransformPointToWorld(v2.position, trace.objectToWorld);
    float3 geoNormal = RTSafeNormalize(cross(p1 - p0, p2 - p0), float3(0.0, 1.0, 0.0));

    if (dot(geoNormal, rayDirection) > 0.0)
        geoNormal = -geoNormal;

    float3 normal;
    if (RTPackedVectorValid(vertex.normal))
        normal = worldSpaceVertices ? vertex.normal : TransformNormalToWorld(vertex.normal, trace.objectToWorld);
    else
        normal = geoNormal;

    normal = RTSafeNormalize(normal, geoNormal);
    if (dot(normal, geoNormal) < 0.0)
        normal = -normal;
    if (dot(normal, rayDirection) >= 0.0)
        normal = geoNormal;

    float3 tangent = vertex.tangent;
    float3 bitangent = vertex.bitangent;
    if (vertex.authoredBasis && RTPackedVectorValid(tangent) && RTPackedVectorValid(bitangent))
    {
        if (!worldSpaceVertices)
        {
            float3x3 model = RTModelTransform(trace.objectToWorld);
            tangent = mul(model, tangent);
            bitangent = mul(model, bitangent);
        }
    }
    else
    {
        RTUVDerivedBasis(p0, p1, p2, v0.uv, v1.uv, v2.uv, tangent, bitangent);
        tangent -= normal * dot(normal, tangent);
        bitangent -= normal * dot(normal, bitangent);
    }

    float3 basisTangent, basisBitangent;
    RTNormalizedBasis(normal, tangent, bitangent, basisTangent, basisBitangent);

    geometry.uv = vertex.uv;
    geometry.normal = normal;
    geometry.geoNormal = geoNormal;
    geometry.tangent = basisTangent;
    geometry.bitangent = basisBitangent;
    return geometry;
}

struct RTHitSurface
{
    MaterialSurface surface;
    uint flags;
};

RTHitSurface RTResolveHitSurface(RTSceneParams scene, RTSceneTrace trace, RTHitGeometry geometry)
{
    RTHitSurface result;
    result.flags = 0;
    result.surface.shadingClass = SHADING_CLASS_STANDARD;
    result.surface.transmission = 0.0;
    result.surface.emissive = 0.0;

    if (IsGrassBatch(scene, trace.batchIdx))
    {
        if (HasDetailAtlas(scene))
        {
            float4 texel = GetBindlessTexture(scene.detailAtlasIndex).SampleLevel(smp_linear, geometry.uv, 0);
            result.surface.albedo = texel.rgb;
        }
        else
        {
            result.surface.albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), 1.0 - geometry.uv.y);
        }
        result.surface.N = geometry.normal;
        result.surface.roughness = 1.0;
        result.surface.metallic = 0.0;
        result.surface.ao = 1.0;
        result.surface.shadingClass = SHADING_CLASS_FOLIAGE;
        result.surface.transmission = foliage_params.x;
        return result;
    }

    if (IsTerrainBatch(scene, trace.batchIdx))
    {
        result.surface = EvalTerrainMaterial(g_TerrainMaterials[trace.info.materialID], geometry.uv, 0.0, 0.0,
            geometry.normal, geometry.tangent, geometry.bitangent);
        return result;
    }

    MaterialData mat = g_Materials[trace.info.materialID];
    result.flags = mat.flags;
    result.surface = EvalStandardMaterial(mat, trace.diffuse.rgb, geometry.uv, 0.0, 0.0,
        geometry.normal, geometry.tangent, geometry.bitangent);
    return result;
}

float3 SampleRTSky(RTSceneParams scene, float3 direction)
{
    float3 sky0 = g_Sky0.SampleLevel(smp_linear, direction, 0).rgb;
    float3 sky1 = g_Sky1.SampleLevel(smp_linear, direction, 0).rgb;
    return lerp(sky0, sky1, scene.skyWeight);
}

float3 RTDirectLighting(RTSceneParams scene, MaterialSurface surface, float3 position, float3 geoNormal, float3 V)
{
    float3 total = 0.0;
    bool foliage = surface.shadingClass == SHADING_CLASS_FOLIAGE;

    RTBSDFTerms sunTerms = RTEvaluateBSDFTerms(surface, V, scene.sunDir, scene.sunColor, scene.diffuseMode);
    if ((foliage || dot(scene.sunDir, geoNormal) > 0.0) &&
        (any(sunTerms.bsdf > 0.0) || any(sunTerms.transmission > 0.0)))
    {
        float side = dot(scene.sunDir, geoNormal) >= 0.0 ? 1.0 : -1.0;
        float3 origin = position + geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
        total += RTCombineBSDFTerms(sunTerms, RTTraceVisibility(scene, origin, scene.sunDir, RT_RAY_DISTANCE));
    }

    for (uint i = 0; i < scene.lightCount; ++i)
    {
        GPULightData light = g_LightData[i];
        float3 L;
        float dist;
        float atten = PunctualLightAttenuation(light, position, L, dist);
        if (atten <= 0.001 || (!foliage && dot(L, geoNormal) <= 0.0))
            continue;

        RTBSDFTerms terms = RTEvaluateBSDFTerms(surface, V, L, light.colorAndRange.xyz * atten, scene.diffuseMode);
        if (!any(terms.bsdf > 0.0) && !any(terms.transmission > 0.0))
            continue;

        float side = dot(L, geoNormal) >= 0.0 ? 1.0 : -1.0;
        float3 origin = position + geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
        float3 toLight = light.positionAndInvRangeSq.xyz - origin;
        float lightDistance = length(toLight);
        float visibility = 1.0;
        if (lightDistance > 0.002)
            visibility = RTTraceVisibility(scene, origin, toLight / lightDistance, lightDistance - 0.001);
        total += RTCombineBSDFTerms(terms, visibility);
    }

    return total;
}

#endif
