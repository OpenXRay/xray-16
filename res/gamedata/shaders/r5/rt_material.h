#ifndef RT_MATERIAL_H
#define RT_MATERIAL_H

struct RTHitGeometry
{
    float2 uv;
    float2 uvDx;
    float2 uvDy;
    float3 normal;
    float3 geoNormal;
    float3 tangent;
    float3 bitangent;
};

bool RTBatchVerticesInWorldSpace(RTSceneParams scene, uint batchIdx)
{
    return IsGrassBatch(scene, batchIdx) || IsSkinnedBatch(scene, batchIdx);
}

void RTLoadBatchTriangleVertices(RTSceneParams scene, uint batchIdx, RTBatchInfo info, uint primitiveIndex,
    out RTTriangleVertex v0, out RTTriangleVertex v1, out RTTriangleVertex v2)
{
    uint i0, i1, i2;
    if (IsGrassBatch(scene, batchIdx))
    {
        RTLoadTriangleIndices(g_GrassIB, info, primitiveIndex, i0, i1, i2);
        v0 = RTLoadGrassVertex(g_GrassVB, i0);
        v1 = RTLoadGrassVertex(g_GrassVB, i1);
        v2 = RTLoadGrassVertex(g_GrassVB, i2);
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

    float3 uvTangent, uvBitangent;
    RTUVDerivedBasis(p0, p1, p2, v0.uv, v1.uv, v2.uv, uvTangent, uvBitangent);

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
    if (IsStaticDetailBatch(scene, trace.batchIdx))
    {
        float3 up = float3(0.0, 1.0, 0.0);
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
    geometry.normal = normal;
    geometry.geoNormal = geoNormal;
    geometry.tangent = basisTangent;
    geometry.bitangent = basisBitangent;
    RTUVFootprintFromRayCone(uvTangent, uvBitangent, geoNormal, rayDirection, trace.t,
        trace.coneWidth, trace.coneSpread, geometry.uvDx, geometry.uvDy);
    return geometry;
}

struct RTHitSurface
{
    MaterialSurface surface;
    uint flags;
};

RTHitSurface RTResolveStaticDetail(RTSceneParams scene, RTHitGeometry geometry)
{
    RTHitSurface result;
    result.flags = 0;
    result.surface.shadingClass = SHADING_CLASS_STANDARD;
    result.surface.transmission = 0.0;
    result.surface.emissive = 0.0;
    result.surface.albedo = HasDetailAtlas(scene)
        ? GetBindlessTexture(scene.detailAtlasIndex).SampleGrad(smp_linear, geometry.uv,
            geometry.uvDx, geometry.uvDy).rgb
        : float3(0.0, 0.0, 0.0);

    float3 worldUp = float3(0.0, 1.0, 0.0);
    float3 N = worldUp;
    float gloss = 0.0;
    float variance = 0.0;
    if (HasDetailBump(scene))
    {
        BumpSample bump = DecodeBump(GetBindlessTexture(scene.detailBumpIndex).SampleGrad(smp_linear,
            geometry.uv, geometry.uvDx, geometry.uvDy));
        N = RTSafeNormalize(mul(bump.normal, float3x3(geometry.tangent, geometry.bitangent, worldUp)), worldUp);
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
            return RTResolveStaticDetail(scene, geometry);

        if (IsDetailMeshBatch(scene, trace.batchIdx) && HasDetailAtlas(scene))
        {
            float4 texel = GetBindlessTexture(scene.detailAtlasIndex).SampleGrad(smp_linear, geometry.uv,
                geometry.uvDx, geometry.uvDy);
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
    if ((mat.flags & MAT_FLAG_ALPHA_TEST) != 0 && diffuseSample.a < mat.alphaRef)
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
