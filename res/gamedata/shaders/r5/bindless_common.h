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
    uint normalIndex;    // Descriptor heap index (s_bump)
    uint detailIndex;    // Descriptor heap index
    uint pbrIndex;       // Descriptor heap index
    float detailScale;
    float alphaRef;
    uint flags;
    uint shaderVariant;
    uint normalXIndex;      // bump# (s_bumpX)
    uint detailBumpIndex;   // s_detailBump
    uint detailBumpXIndex;  // s_detailBumpX
    uint tessMethod;        // 0=off, 1=PN, 2=HM, 3=PN+HM
    uint sssMapIndex;       // SSS map (R thick, G strength, B profile)
    float emissiveIntensity;
    uint lmapIndex;         // Baked hemi lightmap (lmap#N_2: a=hemi, g=sun)
    uint _pad2;
};

// Material flags
#define MAT_FLAG_ALPHA_TEST    (1 << 0)
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)
#define MAT_FLAG_TERRAIN       (1 << 6)
#define MAT_FLAG_HAS_PBR_LAYER (1 << 7)
#define MAT_FLAG_ALPHA_BLEND   (1 << 8)
#define MAT_FLAG_WATER         (1 << 9)
#define MAT_FLAG_HAS_NORMAL_X  (1 << 10)
#define MAT_FLAG_HAS_DETAIL_BUMP (1 << 11)
#define MAT_FLAG_HAS_LMAP      (1 << 12)
#define MAT_FLAG_FOLIAGE       (1 << 13) // trees / bushes / leaves — enable SSS
#define MAT_FLAG_HAS_SSS_MAP   (1 << 14) // sssMapIndex valid (R thick, G strength, B profile)
#define MAT_FLAG_MULTIPLY      (1 << 15) // DestColor*SrcColor (burns / wall stains)
#define MAT_FLAG_PARTICLE_HARD (1 << 16) // disable soft-particle depth fade
#define MAT_FLAG_GLASS         (1 << 17) // thin-glass transmission

#define SSS_PROFILE_SKIN  0.0
#define SSS_PROFILE_GRASS 1.0
#define SSS_PROFILE_GLASS 2.0

#define TESS_METHOD_OFF   0
#define TESS_METHOD_PN    1
#define TESS_METHOD_HM    2
#define TESS_METHOD_PN_HM 3

// ═══════════════════════════════════════════════════════
//  TERRAIN MATERIAL DATA (matches C++ TerrainMaterialData)
// ═══════════════════════════════════════════════════════
// 96 bytes - 4-layer detail blending with RGBA mask + dual-bump + lightmap

struct TerrainMaterialData
{
    // Base textures
    uint baseAlbedoIndex;   // Level terrain base texture
    uint blendMaskIndex;    // RGBA blend mask

    // Detail color textures (4 layers)
    uint detailR_Index;
    uint detailG_Index;
    uint detailB_Index;
    uint detailA_Index;

    // Detail normal textures (4 layers) — s_bump
    uint normalR_Index;
    uint normalG_Index;
    uint normalB_Index;
    uint normalA_Index;

    // Detail PBR textures (4 layers, optional)
    uint pbrR_Index;
    uint pbrG_Index;
    uint pbrB_Index;
    uint pbrA_Index;

    // Detail bump# (s_bumpX) for dual-bump
    uint normalXR_Index;
    uint normalXG_Index;
    uint normalXB_Index;
    uint normalXA_Index;

    // Properties
    float detailScale;
    uint flags;

    // Lightmap (s_lmap)
    uint lmapIndex;
    uint pad0;
    uint pad1;
    uint pad2;
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

// ═══════════════════════════════════════════════════════
//  BINDLESS BUFFERS
// ═══════════════════════════════════════════════════════

StructuredBuffer<MaterialData> g_Materials : register(t8);
StructuredBuffer<TerrainMaterialData> g_TerrainMaterials : register(t9);
StructuredBuffer<VariantTextureData> g_VariantTextures : register(t10);

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
    float height;
};

// Classic sload.h dual-bump: Nu.wzy + (NuE.xyz - 1), gloss = Nu.x^2
// IX-Ray PBS: Nu.wy = nxy, Nu.r = height; NuE = M/R/SSS/AO (not error@0.5)
BumpSample SampleNormal(MaterialData mat, float2 uv)
{
    BumpSample result;
    result.normal = float3(0, 0, 1);
    result.gloss = 0.0;
    result.height = 0.0;

    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return result;

    float4 Nu = GetBindlessTexture(mat.normalIndex).Sample(smp_linear, uv);
    float4 NuE = float4(0.5, 0.5, 0.5, 0.0);
    if ((mat.flags & MAT_FLAG_HAS_NORMAL_X) && mat.normalXIndex != INVALID_TEXTURE_INDEX)
        NuE = GetBindlessTexture(mat.normalXIndex).Sample(smp_linear, uv);

    float errDev = abs(NuE.x - 0.5) + abs(NuE.y - 0.5) + abs(NuE.z - 0.5);
    bool ixrayPbs = ((mat.flags & MAT_FLAG_HAS_PBR) != 0) && errDev > 0.25;

    if (ixrayPbs)
    {
        float2 nxy = Nu.wy * 2.0 - 1.0;
        result.normal = float3(nxy, sqrt(max(0.0, 1.0 - saturate(dot(nxy, nxy)))));
        result.gloss = saturate(1.0 - NuE.y);
        result.height = Nu.r;
    }
    else
    {
        result.normal = Nu.wzy + (NuE.xyz - 1.0);
        result.gloss = Nu.x * Nu.x;
        result.height = NuE.w;

        if ((mat.flags & MAT_FLAG_HAS_DETAIL_BUMP) &&
            mat.detailBumpIndex != INVALID_TEXTURE_INDEX &&
            mat.detailBumpXIndex != INVALID_TEXTURE_INDEX)
        {
            float2 duv = uv * mat.detailScale;
            float4 NDetail = GetBindlessTexture(mat.detailBumpIndex).Sample(smp_linear, duv);
            float4 NDetailX = GetBindlessTexture(mat.detailBumpXIndex).Sample(smp_linear, duv);
            result.gloss = result.gloss * NDetail.x * 2.0;
            result.normal += NDetail.wzy + NDetailX.xyz - 1.0;
        }

        result.normal.z *= 0.5;
    }
    return result;
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

float4 SamplePBRFull(MaterialData mat, float2 uv)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float4(0.0, 0.5, 1.0, 0.5);

    Texture2D tex = GetBindlessTexture(mat.pbrIndex);
    return tex.Sample(smp_linear, uv);
}

float3 SamplePBR(MaterialData mat, float2 uv)
{
    return SamplePBRFull(mat, uv).rgb;
}

void ResolveMaterialPBR(MaterialData mat, float2 uv, float gloss,
    out float metallic, out float roughness, out float ao, out float parallax)
{
    metallic = 0.0;
    roughness = saturate(1.0 - gloss);
    ao = 1.0;
    parallax = 0.5;
    if (mat.flags & MAT_FLAG_HAS_PBR)
    {
        float4 pbrSample = SamplePBRFull(mat, uv);
        metallic = pbrSample.r;
        roughness = pbrSample.g;
        ao = pbrSample.b;
        parallax = pbrSample.a;
    }
}

#endif // BINDLESS_COMMON_H
