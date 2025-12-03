// bindless_common.h
// 2D Grid-Packing Bindless Rendering
// Uses 4096×4096 pages with 128px cell-based allocation
// Must match C++ BindlessTypes.h exactly!

#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

// ═══════════════════════════════════════════════════════
//  CONFIGURATION (must match C++ constants)
// ═══════════════════════════════════════════════════════

#define ATLAS_PAGE_SIZE 4096
#define ATLAS_CELL_SIZE 128
#define ATLAS_GRID_SIZE 32  // 4096 / 128

// ═══════════════════════════════════════════════════════
//  TEXTURE ARRAYS (4 total - one per type)
// ═══════════════════════════════════════════════════════
// Each array slice is a 4096×4096 page containing multiple textures

Texture2DArray g_DiffuseAtlas : register(t10);  // Base color / albedo
Texture2DArray g_NormalAtlas  : register(t11);  // Normal maps
Texture2DArray g_DetailAtlas  : register(t12);  // Detail textures
Texture2DArray g_PBRAtlas     : register(t13);  // Metallic/Roughness/AO

SamplerState g_LinearSampler : register(s0);

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches C++ MaterialData struct)
// ═══════════════════════════════════════════════════════
// Uses packed atlas allocations - 2× uint per texture

struct MaterialData
{
    // Diffuse allocation (packed)
    uint diffuseAtlasLow;   // pageIndex (16 bits) | cellX (16 bits)
    uint diffuseAtlasHigh;  // cellY (16 bits) | cellsWide (8 bits) | cellsTall (8 bits)

    // Normal allocation (packed)
    uint normalAtlasLow;
    uint normalAtlasHigh;

    // Detail allocation (packed)
    uint detailAtlasLow;
    uint detailAtlasHigh;

    // PBR allocation (packed)
    uint pbrAtlasLow;
    uint pbrAtlasHigh;

    // Material properties
    float detailScale;
    float alphaRef;
    uint flags;
    float padding1;
};

// Material flags
#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)

// Invalid allocation marker (pageIndex = 0xFFFF)
#define INVALID_PAGE 0xFFFF

// ═══════════════════════════════════════════════════════
//  BINDLESS BUFFERS
// ═══════════════════════════════════════════════════════

StructuredBuffer<MaterialData> g_Materials : register(t8);

// ═══════════════════════════════════════════════════════
//  ATLAS RECT (unpacked allocation for sampling)
// ═══════════════════════════════════════════════════════

struct AtlasRect
{
    uint pageIndex;   // Which page (slice) in the Texture2DArray
    float2 uvMin;     // Top-left corner in UV space [0, 1]
    float2 uvSize;    // Size in UV space
};

// ═══════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════

// Unpack allocation from 2× uint to AtlasRect
// Packing layout:
//   Low:  pageIndex (16 bits) | cellX (16 bits)
//   High: cellY (16 bits) | cellsWide (8 bits) | cellsTall (8 bits)
AtlasRect UnpackAtlasRect(uint low, uint high)
{
    AtlasRect r;

    r.pageIndex = low & 0xFFFF;

    uint cellX = (low >> 16) & 0xFFFF;
    uint cellY = high & 0xFFFF;
    uint cellsWide = (high >> 16) & 0xFF;
    uint cellsTall = (high >> 24) & 0xFF;

    // Convert cell coordinates to UV space
    // Cell size = 128px, page size = 4096px
    // UV per cell = 128/4096 = 0.03125
    float cellToUV = (float)ATLAS_CELL_SIZE / (float)ATLAS_PAGE_SIZE;

    r.uvMin = float2(cellX, cellY) * cellToUV;
    r.uvSize = float2(cellsWide, cellsTall) * cellToUV;

    return r;
}

// Check if allocation is valid
bool IsValidAllocation(uint low)
{
    return (low & 0xFFFF) != INVALID_PAGE;
}

// ═══════════════════════════════════════════════════════
//  TEXTURE SAMPLING
// ═══════════════════════════════════════════════════════

// Sample from atlas with proper UV mapping
// Uses half-texel inset at edges to prevent bleeding
float4 SampleAtlasSafe(Texture2DArray atlas, AtlasRect r, float2 uv)
{
    // Half-texel inset to prevent sampling outside allocated region
    float halfTexel = 0.5 / (float)ATLAS_PAGE_SIZE;

    // Map input UV [0,1] to allocated region
    // frac(uv) handles texture wrapping
    float2 localUV = frac(uv);

    // Lerp between uvMin + halfTexel and uvMin + uvSize - halfTexel
    float2 safeMin = r.uvMin + halfTexel;
    float2 safeMax = r.uvMin + r.uvSize - halfTexel;
    float2 atlasUV = lerp(safeMin, safeMax, localUV);

    return atlas.Sample(g_LinearSampler, float3(atlasUV, r.pageIndex));
}

// ─────────────────────────────────────────────────────
//  DIFFUSE SAMPLING
// ─────────────────────────────────────────────────────

float4 SampleDiffuse(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float4(1, 0, 1, 1);  // Magenta for missing

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    return SampleAtlasSafe(g_DiffuseAtlas, r, uv);
}

// Convenience overload using MaterialData
float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    return SampleDiffuse(mat.diffuseAtlasLow, mat.diffuseAtlasHigh, uv);
}

// ─────────────────────────────────────────────────────
//  NORMAL SAMPLING
// ─────────────────────────────────────────────────────

float3 SampleNormal(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float3(0.5, 0.5, 1.0);  // Flat normal

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    float4 sample = SampleAtlasSafe(g_NormalAtlas, r, uv);

    // Decode normal (RG -> XYZ, reconstruct Z)
    float3 normal;
    normal.xy = sample.xy * 2.0 - 1.0;
    normal.z = sqrt(saturate(1.0 - dot(normal.xy, normal.xy)));
    return normal;
}

// Convenience overload using MaterialData
float3 SampleNormal(MaterialData mat, float2 uv)
{
    return SampleNormal(mat.normalAtlasLow, mat.normalAtlasHigh, uv);
}

// ─────────────────────────────────────────────────────
//  DETAIL SAMPLING
// ─────────────────────────────────────────────────────

float4 SampleDetail(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float4(0.5, 0.5, 0.5, 0.5);  // Neutral detail

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    return SampleAtlasSafe(g_DetailAtlas, r, uv);
}

// Convenience overload using MaterialData
float4 SampleDetail(MaterialData mat, float2 uv, float detailScale)
{
    return SampleDetail(mat.detailAtlasLow, mat.detailAtlasHigh, uv * detailScale);
}

// ─────────────────────────────────────────────────────
//  PBR SAMPLING
// ─────────────────────────────────────────────────────

float3 SamplePBR(uint atlasLow, uint atlasHigh, float2 uv)
{
    if (!IsValidAllocation(atlasLow))
        return float3(0.0, 0.5, 1.0);  // Default: non-metallic, medium rough, full AO

    AtlasRect r = UnpackAtlasRect(atlasLow, atlasHigh);
    float4 sample = SampleAtlasSafe(g_PBRAtlas, r, uv);

    // R = metallic, G = roughness, B = AO
    return sample.rgb;
}

// Convenience overload using MaterialData
float3 SamplePBR(MaterialData mat, float2 uv)
{
    return SamplePBR(mat.pbrAtlasLow, mat.pbrAtlasHigh, uv);
}

#endif // BINDLESS_COMMON_H
