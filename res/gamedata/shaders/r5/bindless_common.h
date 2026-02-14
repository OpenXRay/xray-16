#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

#define INVALID_TEXTURE_INDEX 0xFFFFFFFF

Texture2D GetBindlessTexture(uint index)
{
    DescriptorHandle<Texture2D> handle = DescriptorHandle<Texture2D>(uint2(index, 0));
    return *nonuniform(handle);
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
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)
#define MAT_FLAG_TERRAIN       (1 << 6)
#define MAT_FLAG_HAS_PBR_LAYER (1 << 7)
#define MAT_FLAG_ALPHA_BLEND   (1 << 8)

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

StructuredBuffer<MaterialData> g_Materials : register(t8);
StructuredBuffer<TerrainMaterialData> g_TerrainMaterials : register(t9);
StructuredBuffer<VariantTextureData> g_VariantTextures : register(t10);

SamplerState g_LinearSampler : register(s0);

float4 SampleVariantTexture(uint materialID, uint slot, float2 uv)
{
    uint texIdx = g_VariantTextures[materialID].tex[slot];
    if (texIdx == INVALID_TEXTURE_INDEX)
        return float4(0, 0, 0, 0);
    Texture2D tex = GetBindlessTexture(texIdx);
    return tex.Sample(g_LinearSampler, uv);
}

float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);

    Texture2D tex = GetBindlessTexture(mat.diffuseIndex);
    return tex.Sample(g_LinearSampler, uv);
}

struct BumpSample
{
    float3 normal;
    float gloss;
};

BumpSample SampleNormal(MaterialData mat, float2 uv)
{
    BumpSample result;
    result.normal = float3(0, 0, 1);
    result.gloss = 0.0;

    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return result;

    Texture2D tex = GetBindlessTexture(mat.normalIndex);
    float4 Nu = tex.Sample(g_LinearSampler, uv);

    result.normal.x = Nu.a * 2.0 - 1.0;
    result.normal.y = Nu.b * 2.0 - 1.0;
    result.normal.z = sqrt(saturate(1.0 - result.normal.x * result.normal.x - result.normal.y * result.normal.y));
    result.gloss = Nu.r * Nu.r;
    return result;
}

float4 SampleDetail(MaterialData mat, float2 uv)
{
    if (mat.detailIndex == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 0.5);

    Texture2D tex = GetBindlessTexture(mat.detailIndex);
    return tex.Sample(g_LinearSampler, uv * mat.detailScale);
}

float3 SamplePBR(MaterialData mat, float2 uv)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.5, 1.0);

    Texture2D tex = GetBindlessTexture(mat.pbrIndex);
    return tex.Sample(g_LinearSampler, uv).rgb;
}

#endif
