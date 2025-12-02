// bindless_common.h
// Simplified bindless rendering - just 4 texture arrays
// Must match C++ BindlessTypes.h exactly!

#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

// ═══════════════════════════════════════════════════════
//  TEXTURE ARRAYS (4 total - one per type)
// ═══════════════════════════════════════════════════════
// All textures resized to same size, RGBA8 format
// Much cleaner than 72 separate arrays!

Texture2DArray g_DiffuseAtlas : register(t10);  // Base color / albedo
Texture2DArray g_NormalAtlas  : register(t11);  // Normal maps
Texture2DArray g_DetailAtlas  : register(t12);  // Detail textures
Texture2DArray g_PBRAtlas     : register(t13);  // Metallic/Roughness/AO

SamplerState g_LinearSampler : register(s0);

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches C++ MaterialData struct)
// ═══════════════════════════════════════════════════════

struct MaterialData
{
    // Texture slice references (type in high 8 bits, slice in low 24 bits)
    uint diffuseSlice;
    uint normalSlice;
    uint detailSlice;
    uint pbrSlice;

    // Material properties
    float detailScale;
    float alphaRef;
    uint flags;
    float padding1;

    // UV scale factors for textures smaller than atlas size
    float diffuseUVScaleU;
    float diffuseUVScaleV;
    float padding2;
    float padding3;
};

// Material flags
#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)

// Invalid slice marker
#define INVALID_SLICE 0xFFFFFFFF

// ═══════════════════════════════════════════════════════
//  BINDLESS BUFFERS
// ═══════════════════════════════════════════════════════

StructuredBuffer<MaterialData> g_Materials : register(t8);

// ═══════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════

// Extract atlas type (0-3) from packed slice reference
uint GetAtlasType(uint packedSlice)
{
    return packedSlice >> 24;
}

// Extract slice index from packed slice reference
uint GetSliceIndex(uint packedSlice)
{
    return packedSlice & 0x00FFFFFF;
}

// ═══════════════════════════════════════════════════════
//  TEXTURE SAMPLING
// ═══════════════════════════════════════════════════════
// Simple and clean - just 4 arrays to choose from!

float4 SampleDiffuseScaled(uint packedSlice, float2 uv, float2 uvScale)
{
    if (packedSlice == INVALID_SLICE)
        return float4(1, 0, 1, 1); // Magenta for missing

    uint slice = GetSliceIndex(packedSlice);

    // uvScale is a DIVISOR (e.g., 0.0625 for 64x64 texture in 1024 atlas)
    // frac(uv) wraps to [0,1], then * uvScale maps to texture region
    // Texture is tiled in atlas, so filtering at edges samples valid data
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
        return float3(0.5, 0.5, 1.0); // Flat normal

    uint slice = GetSliceIndex(packedSlice);

    // uvScale is a DIVISOR - texture is tiled in atlas for seamless filtering
    float2 atlasUV = frac(uv) * uvScale;

    float4 sample = g_NormalAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));

    // Decode normal (RG -> XYZ, reconstruct Z)
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
        return float4(0.5, 0.5, 0.5, 0.5); // Neutral detail

    uint slice = GetSliceIndex(packedSlice);

    // uvScale is a DIVISOR - texture is tiled in atlas for seamless filtering
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
        return float3(0.0, 0.5, 1.0); // Default: non-metallic, medium rough, full AO

    uint slice = GetSliceIndex(packedSlice);

    // uvScale is a DIVISOR - texture is tiled in atlas for seamless filtering
    float2 atlasUV = frac(uv) * uvScale;

    float4 sample = g_PBRAtlas.Sample(g_LinearSampler, float3(atlasUV, slice));

    // R = metallic, G = roughness, B = AO
    return sample.rgb;
}

float3 SamplePBR(uint packedSlice, float2 uv)
{
    return SamplePBRScaled(packedSlice, uv, float2(1.0, 1.0));
}

#endif // BINDLESS_COMMON_H
