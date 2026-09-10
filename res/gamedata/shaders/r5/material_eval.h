#ifndef MATERIAL_EVAL_H
#define MATERIAL_EVAL_H

struct MaterialSurface
{
    float3 albedo;
    float3 N;
    float roughness;
    float metallic;
    float ao;
    float3 emissive;
    uint shadingClass;
    float transmission;
};

float4 SampleDiffuseGrad(MaterialData mat, float2 uv, float2 uvDdx, float2 uvDdy)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);
    return GetBindlessTexture(mat.diffuseIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
}

BumpSample SampleNormalGrad(MaterialData mat, float2 uv, float2 uvDdx, float2 uvDdy)
{
    BumpSample result;
    result.normal = float3(0, 0, 1);
    result.gloss = 0.0;
    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return result;
    float4 Nu = GetBindlessTexture(mat.normalIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
    result.normal.x = Nu.a * 2.0 - 1.0;
    result.normal.y = Nu.b * 2.0 - 1.0;
    result.normal.z = sqrt(saturate(1.0 - result.normal.x * result.normal.x - result.normal.y * result.normal.y));
    result.gloss = Nu.r * Nu.r;
    return result;
}

float4 SampleDetailGrad(MaterialData mat, float2 uv, float2 uvDdx, float2 uvDdy)
{
    if (mat.detailIndex == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 0.5);
    return GetBindlessTexture(mat.detailIndex).SampleGrad(smp_linear, uv * mat.detailScale, uvDdx * mat.detailScale, uvDdy * mat.detailScale);
}

float3 SamplePBRGrad(MaterialData mat, float2 uv, float2 uvDdx, float2 uvDdy)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.5, 1.0);
    return GetBindlessTexture(mat.pbrIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy).rgb;
}

MaterialSurface EvalStandardMaterial(MaterialData mat, float3 diffuse, float2 uv, float2 uvDdx, float2 uvDdy, float3 vertexNormal, float3 tangent, float3 bitangent)
{
    MaterialSurface s;
    s.albedo = diffuse;
    s.N = normalize(vertexNormal);
    float gloss = 0.0;
    if (mat.flags & MAT_FLAG_HAS_NORMAL)
    {
        BumpSample bump = SampleNormalGrad(mat, uv, uvDdx, uvDdy);
        float3x3 TBN = float3x3(normalize(tangent), normalize(bitangent), s.N);
        s.N = normalize(mul(bump.normal, TBN));
        gloss = bump.gloss;
    }
    if (mat.flags & MAT_FLAG_HAS_DETAIL)
    {
        float4 detailSample = SampleDetailGrad(mat, uv, uvDdx, uvDdy);
        s.albedo = s.albedo * (detailSample.rgb * 2.0);
    }
    s.metallic = 0.0;
    s.roughness = 1.0 - gloss;
    s.ao = 1.0;
    if (mat.flags & MAT_FLAG_HAS_PBR)
    {
        float3 pbrSample = SamplePBRGrad(mat, uv, uvDdx, uvDdy);
        s.metallic = pbrSample.r;
        s.roughness = pbrSample.g;
        s.ao = pbrSample.b;
    }
    s.emissive = (mat.flags & MAT_FLAG_EMISSIVE) ? s.albedo * g_Variants[mat.shaderVariant].emissive : 0.0;
    bool foliage = (mat.flags & MAT_FLAG_FOLIAGE) != 0;
    s.shadingClass = foliage ? SHADING_CLASS_FOLIAGE : SHADING_CLASS_STANDARD;
    s.transmission = foliage ? foliage_params.z : 0.0;
    return s;
}

float4 SampleTerrainTextureGrad(uint index, float2 uv, float2 uvDdx, float2 uvDdy)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 1.0);
    return GetBindlessTexture(index).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
}

float3 SampleTerrainNormalGrad(uint index, float2 uv, float2 uvDdx, float2 uvDdy)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.0, 1.0);
    float4 Nu = GetBindlessTexture(index).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
    float3 normal;
    normal.x = Nu.a * 2.0 - 1.0;
    normal.y = Nu.b * 2.0 - 1.0;
    normal.z = sqrt(saturate(1.0 - normal.x * normal.x - normal.y * normal.y));
    return normal;
}

MaterialSurface EvalTerrainMaterial(TerrainMaterialData mat, float2 uv, float2 uvDdx, float2 uvDdy, float3 vertexNormal, float3 tangent, float3 bitangent)
{
    float2 detailUV = uv * mat.detailScale;
    float2 detailDdx = uvDdx * mat.detailScale;
    float2 detailDdy = uvDdy * mat.detailScale;

    float4 baseSample = SampleTerrainTextureGrad(mat.baseAlbedoIndex, uv, uvDdx, uvDdy);
    float4 mask = SampleTerrainTextureGrad(mat.blendMaskIndex, uv, uvDdx, uvDdy);
    float maskSum = dot(mask, float4(1, 1, 1, 1));
    if (maskSum > 0.001)
        mask = mask / maskSum;
    else
        mask = float4(0.25, 0.25, 0.25, 0.25);

    float3 detailR = SampleTerrainTextureGrad(mat.detailR_Index, detailUV, detailDdx, detailDdy).rgb;
    float3 detailG = SampleTerrainTextureGrad(mat.detailG_Index, detailUV, detailDdx, detailDdy).rgb;
    float3 detailB = SampleTerrainTextureGrad(mat.detailB_Index, detailUV, detailDdx, detailDdy).rgb;
    float3 detailA = SampleTerrainTextureGrad(mat.detailA_Index, detailUV, detailDdx, detailDdy).rgb;
    float3 blendedDetail = detailR * mask.r + detailG * mask.g + detailB * mask.b + detailA * mask.a;

    MaterialSurface s;
    s.albedo = baseSample.rgb * blendedDetail * 2.0;

    float3 N = normalize(vertexNormal);
    float3 normalR = SampleTerrainNormalGrad(mat.normalR_Index, detailUV, detailDdx, detailDdy);
    float3 normalG = SampleTerrainNormalGrad(mat.normalG_Index, detailUV, detailDdx, detailDdy);
    float3 normalB = SampleTerrainNormalGrad(mat.normalB_Index, detailUV, detailDdx, detailDdy);
    float3 normalA = SampleTerrainNormalGrad(mat.normalA_Index, detailUV, detailDdx, detailDdy);
    float3 blendedNormal = normalize(normalR * mask.r + normalG * mask.g + normalB * mask.b + normalA * mask.a);
    float3x3 TBN = float3x3(normalize(tangent), normalize(bitangent), N);
    s.N = normalize(mul(blendedNormal, TBN));

    s.metallic = 0.0;
    s.roughness = 0.5;
    s.ao = 1.0;
    if (mat.flags & MAT_FLAG_HAS_PBR_LAYER)
    {
        float3 pbrR = SampleTerrainTextureGrad(mat.pbrR_Index, detailUV, detailDdx, detailDdy).rgb;
        float3 pbrG = SampleTerrainTextureGrad(mat.pbrG_Index, detailUV, detailDdx, detailDdy).rgb;
        float3 pbrB = SampleTerrainTextureGrad(mat.pbrB_Index, detailUV, detailDdx, detailDdy).rgb;
        float3 pbrA = SampleTerrainTextureGrad(mat.pbrA_Index, detailUV, detailDdx, detailDdy).rgb;
        float3 blendedPBR = pbrR * mask.r + pbrG * mask.g + pbrB * mask.b + pbrA * mask.a;
        s.metallic = blendedPBR.r;
        s.roughness = blendedPBR.g;
        s.ao = blendedPBR.b;
    }
    s.emissive = 0.0;
    s.shadingClass = SHADING_CLASS_STANDARD;
    s.transmission = 0.0;
    return s;
}

#endif
