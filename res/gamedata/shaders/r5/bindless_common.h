#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

#define INVALID_TEXTURE_INDEX 0xFFFFFFFF

Texture2D g_BindlessTextures[] : register(t0, space1);

Texture2D GetBindlessTexture(uint index)
{
    return g_BindlessTextures[NonUniformResourceIndex(index)];
}

struct MaterialData
{
    uint diffuseIndex;
    uint normalIndex;
    uint detailIndex;
    uint pbrIndex;
    float detailScale;
    float alphaRef;
    uint flags;
    uint shaderVariant;
};

#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)
#define MAT_FLAG_TERRAIN       (1 << 6)
#define MAT_FLAG_HAS_PBR_LAYER (1 << 7)
#define MAT_FLAG_ALPHA_BLEND   (1 << 8)
#define MAT_FLAG_WATER         (1 << 9)
#define MAT_FLAG_FOLIAGE       (1 << 10)

struct TerrainMaterialData
{
    uint baseAlbedoIndex;
    uint blendMaskIndex;

    uint detailR_Index;
    uint detailG_Index;
    uint detailB_Index;
    uint detailA_Index;

    uint normalR_Index;
    uint normalG_Index;
    uint normalB_Index;
    uint normalA_Index;

    uint pbrR_Index;
    uint pbrG_Index;
    uint pbrB_Index;
    uint pbrA_Index;

    float detailScale;
    uint flags;
};

struct VariantTextureData
{
    uint tex[8];
};

struct VariantData
{
    float emissive;
    uint flags;
    uint packed;
    float fadeScale;
};

#define VARIANT_FADE_NONE   0u
#define VARIANT_FADE_EDGE   1u
#define VARIANT_FADE_FACING 2u
#define VARIANT_COLOR_LIT             0u
#define VARIANT_COLOR_UNLIT           1u
#define VARIANT_COLOR_ALPHA_ONLY      2u
#define VARIANT_COLOR_UNLIT_ALPHA_FADE 3u
#define VARIANT_COLOR_NONE            4u
#define VariantFadeMode(v)  ((v).packed & 0xFFu)
#define VariantColorMode(v) (((v).packed >> 8) & 0xFFu)

#define VARIANT_FLAG_FOG           (1 << 0)
#define VARIANT_FLAG_DISTORT       (1 << 1)
#define VARIANT_FLAG_TRANSPARENT   (1 << 2)
#define VARIANT_FLAG_BACK_TO_FRONT (1 << 3)
#define VARIANT_FLAG_EMISSIVE      (1 << 4)
#define VARIANT_FLAG_NO_SHADOW     (1 << 5)

StructuredBuffer<MaterialData> g_Materials : register(t8);
StructuredBuffer<TerrainMaterialData> g_TerrainMaterials : register(t9);
StructuredBuffer<VariantTextureData> g_VariantTextures : register(t10);
StructuredBuffer<VariantData> g_Variants : register(t23);

#ifndef common_samplers_h_included
#include "common_samplers.h"
#endif

float4 SampleVariantTexture(uint materialID, uint slot, float2 uv)
{
    uint texIdx = g_VariantTextures[materialID].tex[slot];
    if (texIdx == INVALID_TEXTURE_INDEX)
        return float4(0, 0, 0, 0);
    Texture2D tex = GetBindlessTexture(texIdx);
    return tex.Sample(smp_linear, uv);
}

float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);

    Texture2D tex = GetBindlessTexture(mat.diffuseIndex);
    return tex.Sample(smp_linear, uv);
}

float4 SampleDiffuseLevel(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);
    return GetBindlessTexture(mat.diffuseIndex).SampleLevel(smp_linear, uv, 0);
}

struct BumpSample
{
    float3 normal;
    float gloss;
};

BumpSample DecodeBump(float4 Nu)
{
    BumpSample result;
    result.normal.x = Nu.a * 2.0 - 1.0;
    result.normal.y = Nu.b * 2.0 - 1.0;
    result.normal.z = sqrt(saturate(1.0 - result.normal.x * result.normal.x - result.normal.y * result.normal.y));
    result.gloss = Nu.r * Nu.r;
    return result;
}

BumpSample SampleNormal(MaterialData mat, float2 uv)
{
    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return DecodeBump(float4(0.0, 0.0, 0.5, 0.5));
    return DecodeBump(GetBindlessTexture(mat.normalIndex).Sample(smp_linear, uv));
}

float4 SampleDetail(MaterialData mat, float2 uv)
{
    if (mat.detailIndex == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 0.5);

    Texture2D tex = GetBindlessTexture(mat.detailIndex);
    return tex.Sample(smp_linear, uv * mat.detailScale);
}

float3 SamplePBR(MaterialData mat, float2 uv)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.5, 1.0);

    Texture2D tex = GetBindlessTexture(mat.pbrIndex);
    return tex.Sample(smp_linear, uv).rgb;
}

#endif
