#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

Texture2DArray g_DiffuseAtlas : register(t10);
Texture2DArray g_NormalAtlas  : register(t11);
Texture2DArray g_DetailAtlas  : register(t12);
Texture2DArray g_PBRAtlas     : register(t13);

SamplerState g_LinearSampler : register(s0);

struct MaterialData
{
    uint diffuseSlice;
    uint normalSlice;
    uint detailSlice;
    uint pbrSlice;

    float detailScale;
    float alphaRef;
    uint flags;
    float padding1;

    float diffuseUVScaleU;
    float diffuseUVScaleV;
    float padding2;
    float padding3;
};

#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)

#define INVALID_SLICE 0xFFFFFFFF

StructuredBuffer<MaterialData> g_Materials : register(t8);

uint GetAtlasType(uint packedSlice)
{
    return packedSlice >> 24;
}

uint GetSliceIndex(uint packedSlice)
{
    return packedSlice & 0x00FFFFFF;
}

float4 SampleDiffuseScaled(uint packedSlice, float2 uv, float2 uvScale)
{
    if (packedSlice == INVALID_SLICE)
        return float4(1, 0, 1, 1);

    uint slice = GetSliceIndex(packedSlice);

    float2 atlasUV = frac(uv) * uvScale;

    return g_DiffuseAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));
}

float4 SampleDiffuse(uint packedSlice, float2 uv)
{
    return SampleDiffuseScaled(packedSlice, uv, float2(1.0, 1.0));
}

float3 SampleNormalScaled(uint packedSlice, float2 uv, float2 uvScale)
{
    if (packedSlice == INVALID_SLICE)
        return float3(0.5, 0.5, 1.0);

    uint slice = GetSliceIndex(packedSlice);

    float2 atlasUV = frac(uv) * uvScale;

    float4 sample = g_NormalAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));

    float3 normal;
    normal.xy = sample.xy * 2.0 - 1.0;
    normal.z = sqrt(saturate(1.0 - dot(normal.xy, normal.xy)));
    return normal;
}

float3 SampleNormal(uint packedSlice, float2 uv)
{
    return SampleNormalScaled(packedSlice, uv, float2(1.0, 1.0));
}

float4 SampleDetailScaled(uint packedSlice, float2 uv, float2 uvScale)
{
    if (packedSlice == INVALID_SLICE)
        return float4(0.5, 0.5, 0.5, 0.5);

    uint slice = GetSliceIndex(packedSlice);

    float2 atlasUV = frac(uv) * uvScale;

    return g_DetailAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));
}

float4 SampleDetail(uint packedSlice, float2 uv)
{
    return SampleDetailScaled(packedSlice, uv, float2(1.0, 1.0));
}

float3 SamplePBRScaled(uint packedSlice, float2 uv, float2 uvScale)
{
    if (packedSlice == INVALID_SLICE)
        return float3(0.0, 0.5, 1.0);

    uint slice = GetSliceIndex(packedSlice);

    float2 atlasUV = frac(uv) * uvScale;

    float4 sample = g_PBRAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));

    return sample.rgb;
}

float3 SamplePBR(uint packedSlice, float2 uv)
{
    return SamplePBRScaled(packedSlice, uv, float2(1.0, 1.0));
}

#endif
