// bindless_common.h
// SM6.6 Bindless Rendering using Slang DescriptorHandle
// Uses descriptor indices for true bindless texture access
// Must match C++ BindlessTypes.h exactly!

#ifndef BINDLESS_COMMON_H
#define BINDLESS_COMMON_H

// ═══════════════════════════════════════════════════════
//  SM6 BINDLESS CONFIGURATION
// ═══════════════════════════════════════════════════════

// Invalid texture index marker
#define INVALID_TEXTURE_INDEX 0xFFFFFFFF

// ═══════════════════════════════════════════════════════
//  SLANG BINDLESS TEXTURE ACCESS
// ═══════════════════════════════════════════════════════
// Uses Slang's DescriptorHandle<T> which translates to
// ResourceDescriptorHeap[index] when compiled to HLSL/DXIL
// For SPIRV/Vulkan: translates to unbounded descriptor arrays

// Fetch texture from descriptor heap using index
// nonuniform() handles divergent access (like NonUniformResourceIndex in HLSL)
Texture2D GetBindlessTexture(uint index)
{
    // Construct handle from uint2 (x = texture index, y = unused for Texture2D)
    DescriptorHandle<Texture2D> handle = DescriptorHandle<Texture2D>(uint2(index, 0));
    return *nonuniform(handle);
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
#define MAT_FLAG_TWO_SIDED     (1 << 1)
#define MAT_FLAG_EMISSIVE      (1 << 2)
#define MAT_FLAG_HAS_DETAIL    (1 << 3)
#define MAT_FLAG_HAS_NORMAL    (1 << 4)
#define MAT_FLAG_HAS_PBR       (1 << 5)
#define MAT_FLAG_TERRAIN       (1 << 6)
#define MAT_FLAG_HAS_PBR_LAYER (1 << 7)
#define MAT_FLAG_ALPHA_BLEND   (1 << 8)

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

// ═══════════════════════════════════════════════════════
//  BINDLESS BUFFERS
// ═══════════════════════════════════════════════════════

StructuredBuffer<MaterialData> g_Materials : register(t8);
StructuredBuffer<TerrainMaterialData> g_TerrainMaterials : register(t9);
StructuredBuffer<VariantTextureData> g_VariantTextures : register(t10);

// ═══════════════════════════════════════════════════════
//  SAMPLER STATE
// ═══════════════════════════════════════════════════════

SamplerState g_LinearSampler : register(s0);

// ═══════════════════════════════════════════════════════
//  VARIANT TEXTURE SAMPLING
// ═══════════════════════════════════════════════════════

float4 SampleVariantTexture(uint materialID, uint slot, float2 uv)
{
    uint texIdx = g_VariantTextures[materialID].tex[slot];
    if (texIdx == INVALID_TEXTURE_INDEX)
        return float4(0, 0, 0, 0);
    Texture2D tex = GetBindlessTexture(texIdx);
    return tex.Sample(g_LinearSampler, uv);
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
    return tex.Sample(g_LinearSampler, uv);
}

// ─────────────────────────────────────────────────────
//  NORMAL SAMPLING
// ─────────────────────────────────────────────────────

float3 SampleNormal(MaterialData mat, float2 uv)
{
    if (mat.normalIndex == INVALID_TEXTURE_INDEX)
        return float3(0.5, 0.5, 1.0);  // Flat normal

    Texture2D tex = GetBindlessTexture(mat.normalIndex);
    float4 sample = tex.Sample(g_LinearSampler, uv);

    // X-Ray normal map format: R=gloss, G=Z, B=Y, A=X
    float3 normal;
    normal.x = sample.a * 2.0 - 1.0;
    normal.y = sample.b * 2.0 - 1.0;
    normal.z = sample.g * 2.0 - 1.0;
    return normal;
}

// ─────────────────────────────────────────────────────
//  DETAIL SAMPLING
// ─────────────────────────────────────────────────────

float4 SampleDetail(MaterialData mat, float2 uv)
{
    if (mat.detailIndex == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 0.5);  // Neutral detail

    Texture2D tex = GetBindlessTexture(mat.detailIndex);
    return tex.Sample(g_LinearSampler, uv * mat.detailScale);
}

// ─────────────────────────────────────────────────────
//  PBR SAMPLING
// ─────────────────────────────────────────────────────

float3 SamplePBR(MaterialData mat, float2 uv)
{
    if (mat.pbrIndex == INVALID_TEXTURE_INDEX)
        return float3(0.0, 0.5, 1.0);  // Default: non-metallic, medium rough, full AO

    Texture2D tex = GetBindlessTexture(mat.pbrIndex);
    return tex.Sample(g_LinearSampler, uv).rgb;
}

#endif // BINDLESS_COMMON_H
