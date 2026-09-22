#ifndef RT_MATERIAL_H
#define RT_MATERIAL_H

#include "rt_common.h"
#include "material_coverage.h"
#include "shared/detail_alpha.h"
#include "detail_blade_material.h"
#include "detail_blade_ray.h"

struct RTHitGeometry
{
    float2 uv;
    float2 uvDx;
    float2 uvDy;
    float3 normal;
    float3 geoNormal;
    float3 tangent;
    float3 bitangent;
    float3 position;
    float bladeVariance;
    uint bladeObjectId;
    float bladeHash;
    bool proceduralBlade;
};

bool RTBatchVerticesInWorldSpace(RTSceneParams scene, uint batchIdx)
{
    return IsGrassBatch(scene, batchIdx) || IsSkinnedBatch(scene, batchIdx);
}

bool RTGrassBatchProcedural(RTSceneParams scene, uint batchIdx)
{
    return IsGrassBatch(scene, batchIdx) && !IsDetailMeshBatch(scene, batchIdx) && !IsStaticDetailBatch(scene, batchIdx);
}

void RTLoadBatchTriangleVertices(RTSceneParams scene, uint batchIdx, RTBatchInfo info, uint primitiveIndex,
    out RTTriangleVertex v0, out RTTriangleVertex v1, out RTTriangleVertex v2)
{
    uint i0, i1, i2;
    if (IsGrassBatch(scene, batchIdx))
    {
        bool procedural = RTGrassBatchProcedural(scene, batchIdx);
        RTLoadTriangleIndices(g_GrassIB, info, primitiveIndex, i0, i1, i2);
        v0 = RTLoadGrassVertex(g_GrassVB, i0, procedural);
        v1 = RTLoadGrassVertex(g_GrassVB, i1, procedural);
        v2 = RTLoadGrassVertex(g_GrassVB, i2, procedural);
        return;
    }

    if (IsSkinnedBatch(scene, batchIdx))
    {
        RTLoadTriangleIndices(g_SkinnedIB, info, primitiveIndex, i0, i1, i2);
        v0 = RTLoadSkinnedVertex(g_SkinnedVB, i0);
        v1 = RTLoadSkinnedVertex(g_SkinnedVB, i1);
        v2 = RTLoadSkinnedVertex(g_SkinnedVB, i2);
        return;
    }

    RTLoadTriangleIndices(g_MegaIB, info, primitiveIndex, i0, i1, i2);
    v0 = RTLoadStaticVertex(g_MegaVB, i0);
    v1 = RTLoadStaticVertex(g_MegaVB, i1);
    v2 = RTLoadStaticVertex(g_MegaVB, i2);
}

void RTLoadWorldTriangle(RTSceneParams scene, uint batchIdx, RTBatchInfo info, uint primitiveIndex,
    float3x4 objectToWorld, out RTTriangleVertex v0, out RTTriangleVertex v1, out RTTriangleVertex v2)
{
    RTLoadBatchTriangleVertices(scene, batchIdx, info, primitiveIndex, v0, v1, v2);
    if (RTBatchVerticesInWorldSpace(scene, batchIdx))
        return;

    v0.position = TransformPointToWorld(v0.position, objectToWorld);
    v1.position = TransformPointToWorld(v1.position, objectToWorld);
    v2.position = TransformPointToWorld(v2.position, objectToWorld);
}

RTHitGeometry RTFetchHitGeometry(RTSceneParams scene, RTSceneTrace trace, float3 rayDirection)
{
    RTHitGeometry geometry;

    RTTriangleVertex v0, v1, v2;
    RTLoadWorldTriangle(scene, trace.batchIdx, trace.info, trace.primitiveIndex, trace.objectToWorld, v0, v1, v2);

    bool worldSpaceVertices = RTBatchVerticesInWorldSpace(scene, trace.batchIdx);
    RTShadingVertex vertex = RTInterpolateTriangleVertex(v0, v1, v2, trace.barycentrics);
    float3 p0 = v0.position;
    float3 p1 = v1.position;
    float3 p2 = v2.position;
    float3 geoNormal = RTSafeNormalize(cross(p1 - p0, p2 - p0), float3(0.0, 1.0, 0.0));

    if (dot(geoNormal, rayDirection) > 0.0)
        geoNormal = -geoNormal;

    bool pulledWavingCard = IsDetailMeshBatch(scene, trace.batchIdx);
    bool pulledStaticPatch = IsStaticDetailBatch(scene, trace.batchIdx);

    float3 uvTangent, uvBitangent;
    RTUVDerivedBasis(p0, p1, p2, v0.uv, v1.uv, v2.uv, uvTangent, uvBitangent);

    float2 uvDx, uvDy;
    RTUVFootprintFromRayCone(uvTangent, uvBitangent, geoNormal, rayDirection, trace.t,
        trace.coneWidth, trace.coneSpread, uvDx, uvDy);

    float bladeVariance = 0.0;
    float3 normal;
    if (vertex.proceduralBlade)
    {
        RTBladeShading blade = RTResolveBladeShading(v0, v1, v2, vertex, uvDx, uvDy, -rayDirection);
        normal = blade.normal;
        bladeVariance = blade.variance;
    }
    else
    {
        if (pulledWavingCard)
            normal = geoNormal;
        else if (RTPackedVectorValid(vertex.normal))
            normal = worldSpaceVertices ? vertex.normal : TransformNormalToWorld(vertex.normal, trace.objectToWorld);
        else
            normal = geoNormal;
    }

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
        tangent = uvTangent - normal * dot(normal, uvTangent);
        bitangent = uvBitangent - normal * dot(normal, uvBitangent);
    }

    float3 basisTangent, basisBitangent;
    if (pulledStaticPatch || pulledWavingCard)
    {
        float3 up = pulledStaticPatch ? float3(0.0, 1.0, 0.0) : geoNormal;
        basisTangent = uvTangent - up * dot(up, uvTangent);
        float tangentLength = length(basisTangent);
        basisTangent = tangentLength > 1e-8 ? basisTangent / tangentLength : 0.0;
        basisBitangent = uvBitangent - up * dot(up, uvBitangent) - basisTangent * dot(basisTangent, uvBitangent);
        float bitangentLength = length(basisBitangent);
        basisBitangent = bitangentLength > 1e-8 ? basisBitangent / bitangentLength : 0.0;
    }
    else
    {
        RTNormalizedBasis(normal, tangent, bitangent, basisTangent, basisBitangent);
    }

    geometry.uv = vertex.uv;
    geometry.uvDx = uvDx;
    geometry.uvDy = uvDy;
    geometry.normal = normal;
    geometry.geoNormal = geoNormal;
    geometry.tangent = basisTangent;
    geometry.bitangent = basisBitangent;
    geometry.position = RTInterpolatePoint(p0, p1, p2, trace.barycentrics);
    geometry.bladeVariance = bladeVariance;
    geometry.bladeObjectId = vertex.bladeObjectId;
    geometry.bladeHash = vertex.bladeHash;
    geometry.proceduralBlade = vertex.proceduralBlade;
    return geometry;
}

struct RTHitSurface
{
    MaterialSurface surface;
    uint flags;
};

float RTPulledDetailAlphaRef(bool wavingCard)
{
    return wavingCard ? DETAIL_PULLED_CARD_ALPHA_REF : DETAIL_STATIC_PATCH_ALPHA_REF;
}

bool RTPulledDetailOpaque(RTSceneParams scene, RTHitGeometry geometry, bool wavingCard, bool shadowRay)
{
    if (!HasDetailAtlas(scene))
        return true;

    if (shadowRay && wavingCard)
        return GetBindlessTexture(scene.detailAtlasIndex).SampleLevel(smp_linear, geometry.uv, 0.0).a >= RTPulledDetailAlphaRef(true);

    return GetBindlessTexture(scene.detailAtlasIndex).SampleGrad(smp_linear, geometry.uv,
        geometry.uvDx, geometry.uvDy).a >= RTPulledDetailAlphaRef(wavingCard);
}

RTHitSurface RTResolvePulledDetail(RTSceneParams scene, RTHitGeometry geometry, bool wavingCard)
{
    RTHitSurface result;
    result.flags = 0;
    result.surface.shadingClass = wavingCard ? SHADING_CLASS_FOLIAGE : SHADING_CLASS_STANDARD;
    result.surface.transmission = wavingCard ? foliage_params.y : 0.0;
    result.surface.emissive = 0.0;
    result.surface.albedo = HasDetailAtlas(scene)
        ? GetBindlessTexture(scene.detailAtlasIndex).SampleGrad(smp_linear, geometry.uv,
            geometry.uvDx, geometry.uvDy).rgb
        : float3(0.0, 0.0, 0.0);

    float3 baseNormal = wavingCard ? geometry.geoNormal : float3(0.0, 1.0, 0.0);
    float3 N = baseNormal;
    float gloss = 0.0;
    float variance = 0.0;
    if (HasDetailBump(scene))
    {
        BumpSample bump = DecodeBump(GetBindlessTexture(scene.detailBumpIndex).SampleGrad(smp_linear,
            geometry.uv, geometry.uvDx, geometry.uvDy));
        N = RTSafeNormalize(mul(bump.normal, float3x3(geometry.tangent, geometry.bitangent, baseNormal)), baseNormal);
        gloss = bump.gloss;
        variance = bump.variance;
    }

    float metallic = 0.0;
    float roughness = 1.0 - gloss;
    float ao = 1.0;
    if (HasDetailPbr(scene))
    {
        float4 pbr = GetBindlessTexture(scene.detailPbrIndex).SampleGrad(smp_linear, geometry.uv,
            geometry.uvDx, geometry.uvDy);
        metallic = pbr.r;
        roughness = pbr.g;
        ao = pbr.b;
    }

    result.surface.N = N;
    result.surface.roughness = RoughnessWithVariance(roughness, variance);
    result.surface.metallic = metallic;
    result.surface.ao = ao;
    return result;
}

RTHitSurface RTResolveBladeMaterial(RTHitGeometry geometry)
{
    RTHitSurface result;
    result.flags = 0;

    float4 base = g_GrassMaterials[0];
    float4 tip = g_GrassMaterials[1];
    float3 tint = g_GrassMaterials[RT_GRASS_MATERIAL_TINT_BASE + geometry.bladeObjectId].rgb;
    float4 vein = BLADE_NEUTRAL_VEIN;
    float heightParam = 1.0 - geometry.uv.y;
    float widthPercent = geometry.uv.x;

    float3 albedo = BladeBaseAlbedo(base.rgb, tip.rgb, tint, base.w, geometry.bladeHash, heightParam);
    albedo = BladeVeinAlbedo(albedo, vein, vein.a);
    result.surface.albedo = BladeDistanceFade(albedo, base.rgb, tip.rgb, geometry.position);
    result.surface.N = geometry.normal;
    result.surface.roughness = BladeRoughness(heightParam, vein.a, geometry.bladeVariance);
    result.surface.metallic = 0.0;
    result.surface.ao = BladeAmbientOcclusion(heightParam, widthPercent, vein.a);
    result.surface.emissive = 0.0;
    result.surface.shadingClass = SHADING_CLASS_FOLIAGE;
    result.surface.transmission = foliage_params.x;
    return result;
}

RTHitSurface RTResolveHitSurface(RTSceneParams scene, RTSceneTrace trace, RTHitGeometry geometry)
{
    RTHitSurface result;
    result.flags = 0;
    result.surface.shadingClass = SHADING_CLASS_STANDARD;
    result.surface.transmission = 0.0;
    result.surface.emissive = 0.0;

    if (IsGrassBatch(scene, trace.batchIdx))
    {
        if (IsStaticDetailBatch(scene, trace.batchIdx))
            return RTResolvePulledDetail(scene, geometry, false);

        if (IsDetailMeshBatch(scene, trace.batchIdx))
            return RTResolvePulledDetail(scene, geometry, true);

        return RTResolveBladeMaterial(geometry);
    }

    if (IsTerrainBatch(scene, trace.batchIdx))
    {
        result.surface = EvalTerrainMaterial(g_TerrainMaterials[trace.info.materialID], geometry.uv,
            geometry.uvDx, geometry.uvDy, geometry.normal, geometry.tangent, geometry.bitangent);
        return result;
    }

    MaterialData mat = g_Materials[trace.info.materialID];
    result.flags = mat.flags;
    float3 albedo = SampleDiffuseGrad(mat, geometry.uv, geometry.uvDx, geometry.uvDy).rgb;
    result.surface = EvalStandardMaterial(mat, albedo, geometry.uv, geometry.uvDx, geometry.uvDy,
        geometry.normal, geometry.tangent, geometry.bitangent);
    return result;
}

float3 RTEvaluateEmitter(RTSceneParams scene, uint batchIdx, RTBatchInfo info, float2 uv, float2 uvDx,
    float2 uvDy, bool sampledLight)
{
    if (IsGrassBatch(scene, batchIdx) || IsTerrainBatch(scene, batchIdx))
        return 0.0;

    MaterialData mat = g_Materials[info.materialID];
    VariantData variant = g_Variants[mat.shaderVariant];
    bool additive = (variant.flags & VARIANT_FLAG_ADDITIVE_EMISSION) != 0;
    bool variantEmissive = (variant.flags & VARIANT_FLAG_EMISSIVE) != 0;
    if (!additive && !variantEmissive && (mat.flags & MAT_FLAG_EMISSIVE) == 0)
        return 0.0;

    float4 diffuseSample = SampleDiffuseGrad(mat, uv, uvDx, uvDy);
    if (MaterialAlphaTestRejects(mat, diffuseSample.a))
        return 0.0;

    float3 albedo = diffuseSample.rgb;
    if ((mat.flags & MAT_FLAG_HAS_DETAIL) != 0)
        albedo *= SampleDetailGrad(mat, uv, uvDx, uvDy).rgb * 2.0;

    float coverage = 1.0;
    if (additive)
    {
        if ((variant.flags & VARIANT_FLAG_EMISSION_ALPHA) != 0)
            coverage = saturate(diffuseSample.a);
    }
    else if (sampledLight && (mat.flags & MAT_FLAG_ALPHA_BLEND) != 0)
    {
        coverage = saturate(diffuseSample.a);
    }

    return albedo * variant.emissive * coverage;
}

#endif
