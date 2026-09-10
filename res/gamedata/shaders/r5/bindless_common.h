// bindless_common.h
// Bindless Rendering via unbounded descriptor arrays (space1)
// Works on both D3D12 (descriptor table root param) and Vulkan (descriptor set 1)
// Must match C++ BindlessTypes.h exactly!

#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

#define INVALID_TEXTURE_INDEX 0xFFFFFFFF

// ═══════════════════════════════════════════════════════
//  BINDLESS TEXTURE ARRAY
// ═══════════════════════════════════════════════════════
// Unbounded SRV array in space1 — maps to:
//   D3D12: Root descriptor table with RegisterSpace=1
//   Vulkan: Descriptor set 1, binding 0

Texture2D g_BindlessTextures[] : register(t0, space1);

Texture2D GetBindlessTexture(uint index)
{
    return g_BindlessTextures[NonUniformResourceIndex(index)];
}

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches C++ MaterialData struct)
// ═══════════════════════════════════════════════════════

struct MaterialData
{
    uint diffuseIndex;   // Descriptor heap index
    uint normalIndex;    // Descriptor heap index
    uint detailIndex;    // Descriptor heap index
    uint pbrIndex;       // Descriptor heap index
    float detailScale;
    float alphaRef;
    uint flags;
    uint shaderVariant;
};

// Material flags
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

// ═══════════════════════════════════════════════════════
//  TERRAIN MATERIAL DATA (matches C++ TerrainMaterialData)
// ═══════════════════════════════════════════════════════
// 64 bytes - 4-layer detail blending with RGBA mask

struct TerrainMaterialData
{
    // Base textures
    uint baseAlbedoIndex;   // Level terrain base texture
    uint blendMaskIndex;    // RGBA blend mask

    // Detail color textures (4 layers)
    uint detailR_Index;     // Detail for mask.r channel
    uint detailG_Index;     // Detail for mask.g channel
    uint detailB_Index;     // Detail for mask.b channel
    uint detailA_Index;     // Detail for mask.a channel

    // Detail normal textures (4 layers)
    uint normalR_Index;     // Normal for mask.r channel
    uint normalG_Index;     // Normal for mask.g channel
    uint normalB_Index;     // Normal for mask.b channel
    uint normalA_Index;     // Normal for mask.a channel

    // Detail PBR textures (4 layers, optional)
    uint pbrR_Index;        // PBR for mask.r channel
    uint pbrG_Index;        // PBR for mask.g channel
    uint pbrB_Index;        // PBR for mask.b channel
    uint pbrA_Index;        // PBR for mask.a channel

    // Properties
    float detailScale;      // Uniform scale for all 4 detail layers
    uint flags;             // MAT_FLAG_TERRAIN, MAT_FLAG_HAS_PBR_LAYER
};

// ═══════════════════════════════════════════════════════
//  VARIANT TEXTURE DATA (matches C++ VariantTextureData)
// ═══════════════════════════════════════════════════════
// Additional textures for shader variants (up to 8 per material)
// Indexed by materialID, only valid when mat.shaderVariant > 0

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

// ═══════════════════════════════════════════════════════
//  BINDLESS BUFFERS
// ═══════════════════════════════════════════════════════

StructuredBuffer<MaterialData> g_Materials : register(t8);
StructuredBuffer<TerrainMaterialData> g_TerrainMaterials : register(t9);
StructuredBuffer<VariantTextureData> g_VariantTextures : register(t10);
StructuredBuffer<VariantData> g_Variants : register(t23);

#ifndef common_samplers_h_included
#include "common_samplers.h"
#endif

// ═══════════════════════════════════════════════════════
//  VARIANT TEXTURE SAMPLING
// ═══════════════════════════════════════════════════════

float4 SampleVariantTexture(uint materialID, uint slot, float2 uv)
{
    uint texIdx = g_VariantTextures[materialID].tex[slot];
    if (texIdx == INVALID_TEXTURE_INDEX)
        return float4(0, 0, 0, 0);
    Texture2D tex = GetBindlessTexture(texIdx);
    return tex.Sample(smp_linear, uv);
}

// ═══════════════════════════════════════════════════════
//  TEXTURE SAMPLING
// ═══════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────
//  DIFFUSE SAMPLING
// ─────────────────────────────────────────────────────

float4 SampleDiffuse(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);  // Magenta for missing

    Texture2D tex = GetBindlessTexture(mat.diffuseIndex);
    return tex.Sample(smp_linear, uv);
}

float4 SampleDiffuseLevel(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);
    return GetBindlessTexture(mat.diffuseIndex).SampleLevel(smp_linear, uv, 0);
}

// ─────────────────────────────────────────────────────
//  NORMAL SAMPLING
// ─────────────────────────────────────────────────────

struct BumpSample
{
    float3 normal;
    float gloss;
    float variance;
};

// X-Ray bump format: R=glossiness, G=normalZ, B=normalY(DX), A=normalX; mips keep the mean length
BumpSample DecodeBump(float4 Nu)
{
    float3 n = float3(Nu.a, Nu.b, Nu.g) * 2.0 - 1.0;
    float len = length(n);
    BumpSample result;
    result.normal = (len > 1e-4) ? (n / len) : float3(0.0, 0.0, 1.0);
    result.gloss = Nu.r * Nu.r;
    result.variance = (1.0 - min(len, 1.0)) / max(len, 1e-4);
    return result;
}

static const float4 BUMP_FLAT = float4(0.0, 1.0, 0.5, 0.5);

BumpSample SampleNormal(MaterialData mat, float2 uv)
{
    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return DecodeBump(BUMP_FLAT);
    return DecodeBump(GetBindlessTexture(mat.normalIndex).Sample(smp_linear, uv));
}

// ─────────────────────────────────────────────────────
//  DETAIL SAMPLING
// ─────────────────────────────────────────────────────

float4 SampleDetail(MaterialData mat, float2 uv)
{
    if (mat.detailIndex == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 0.5);  // Neutral detail

    Texture2D tex = GetBindlessTexture(mat.detailIndex);
    return tex.Sample(smp_linear, uv * mat.detailScale);
}

// ─────────────────────────────────────────────────────
//  PBR SAMPLING
// ─────────────────────────────────────────────────────

float3 SamplePBR(MaterialData mat, float2 uv)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.5, 1.0);  // Default: non-metallic, medium rough, full AO

    Texture2D tex = GetBindlessTexture(mat.pbrIndex);
    return tex.Sample(smp_linear, uv).rgb;
}

#endif // BINDLESS_COMMON_H
