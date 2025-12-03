#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

#define ATLAS_PAGE_SIZE 4096
#define ATLAS_CELL_SIZE 128
#define ATLAS_GRID_SIZE 32

Texture2DArray g_DiffuseAtlas : register(t10);
Texture2DArray g_NormalAtlas  : register(t11);
Texture2DArray g_DetailAtlas  : register(t12);
Texture2DArray g_PBRAtlas     : register(t13);

SamplerState g_LinearSampler : register(s0);

struct MaterialData
{
    uint diffuseAtlasLow;
    uint diffuseAtlasHigh;

    uint normalAtlasLow;
    uint normalAtlasHigh;

    uint detailAtlasLow;
    uint detailAtlasHigh;

    uint pbrAtlasLow;
    uint pbrAtlasHigh;

    float detailScale;
    float alphaRef;
    uint flags;
    float padding1;
};

#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)

#define INVALID_PAGE 0xFFFF

StructuredBuffer<MaterialData> g_Materials : register(t8);

struct AtlasRect
{
    uint pageIndex;
    float2 uvMin;
    float2 uvSize;
};

AtlasRect UnpackAtlasRect(uint low, uint high)
{
    AtlasRect r;

    r.pageIndex = low & 0xFFFF;

    uint cellX = (low >> 16) & 0xFFFF;
    uint cellY = high & 0xFFFF;
    uint cellsWide = (high >> 16) & 0xFF;
    uint cellsTall = (high >> 24) & 0xFF;

    float cellToUV = (float)ATLAS_CELL_SIZE / (float)ATLAS_PAGE_SIZE;

    r.uvMin = float2(cellX, cellY) * cellToUV;
    r.uvSize = float2(cellsWide, cellsTall) * cellToUV;

    return r;
}

bool IsValidAllocation(uint low)
{
    return (low & 0xFFFF) != INVALID_PAGE;
}

float4 SampleAtlasSafe(Texture2DArray atlas, AtlasRect r, float2 uv)
{
    float halfTexel = 0.5 / (float)ATLAS_PAGE_SIZE;

    float2 localUV = frac(uv);

    float2 safeMin = r.uvMin + halfTexel;
    float2 safeMax = r.uvMin + r.uvSize - halfTexel;
    float2 atlasUV = lerp(safeMin, safeMax, localUV);

    return atlas.Sample(g_LinearSampler, float3(atlasUV, r.pageIndex));
}

float4 SampleDiffuse(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float4(1, 0, 1, 1);

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    return SampleAtlasSafe(g_DiffuseAtlas, r, uv);
}

float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    return SampleDiffuse(mat.diffuseAtlasLow, mat.diffuseAtlasHigh, uv);
}

float3 SampleNormal(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float3(0.5, 0.5, 1.0);

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    float4 sample = SampleAtlasSafe(g_NormalAtlas, r, uv);

    float3 normal;
    normal.xy = sample.xy * 2.0 - 1.0;
    normal.z = sqrt(saturate(1.0 - dot(normal.xy, normal.xy)));
    return normal;
}

float3 SampleNormal(MaterialData mat, float2 uv)
{
    return SampleNormal(mat.normalAtlasLow, mat.normalAtlasHigh, uv);
}

float4 SampleDetail(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float4(0.5, 0.5, 0.5, 0.5);

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    return SampleAtlasSafe(g_DetailAtlas, r, uv);
}

float4 SampleDetail(MaterialData mat, float2 uv, float detailScale)
{
    return SampleDetail(mat.detailAtlasLow, mat.detailAtlasHigh, uv * detailScale);
}

float3 SamplePBR(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float3(0.0, 0.5, 1.0);

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    float4 sample = SampleAtlasSafe(g_PBRAtlas, r, uv);

    return sample.rgb;
}

float3 SamplePBR(MaterialData mat, float2 uv)
{
    return SamplePBR(mat.pbrAtlasLow, mat.pbrAtlasHigh, uv);
}

#endif
