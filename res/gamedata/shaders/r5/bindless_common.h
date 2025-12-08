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
    float padding;
};

#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)

StructuredBuffer<MaterialData> g_Materials : register(t8);

SamplerState g_LinearSampler : register(s0);

float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);

    Texture2D tex = GetBindlessTexture(mat.diffuseIndex);
    return tex.Sample(g_LinearSampler, uv);
}

float3 SampleNormal(MaterialData mat, float2 uv)
{
    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return float3(0.5, 0.5, 1.0);

    Texture2D tex = GetBindlessTexture(mat.normalIndex);
    float4 sample = tex.Sample(g_LinearSampler, uv);

    float3 normal;
    normal.x = sample.a * 2.0 - 1.0;
    normal.y = sample.b * 2.0 - 1.0;
    normal.z = sample.g * 2.0 - 1.0;
    return normal;
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
