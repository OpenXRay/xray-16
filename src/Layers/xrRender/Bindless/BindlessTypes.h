// xrRender/Bindless/BindlessTypes.h
// SM6 Bindless Rendering Types
// Uses D3D12 ResourceDescriptorHeap with simple u32 descriptor indices
#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  CONFIGURATION
// ═══════════════════════════════════════════════════════

// Maximum unique materials
constexpr u32 MAX_MATERIALS = 16384;

// Invalid texture index sentinel
constexpr u32 INVALID_TEXTURE_INDEX = UINT32_MAX;

// ═══════════════════════════════════════════════════════
//  TEXTURE TYPES
// ═══════════════════════════════════════════════════════

enum class TextureType : u8 {
    Diffuse  = 0,   // Base color / albedo
    Normal   = 1,   // Normal maps
    Detail   = 2,   // Detail textures
    PBR      = 3,   // Metallic/Roughness/AO packed
    Count    = 4
};

constexpr u32 TOTAL_TEXTURE_TYPES = static_cast<u32>(TextureType::Count);  // 4

inline const char* GetTextureTypeName(TextureType type) {
    static const char* names[] = { "Diffuse", "Normal", "Detail", "PBR" };
    return names[static_cast<u8>(type)];
}

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches HLSL MaterialData struct)
// ═══════════════════════════════════════════════════════
// GPU-side material representation - must match HLSL exactly!
// Uses SM6 bindless texture indices from ResourceDescriptorHeap
//
// Layout (32 bytes total):
//   Bytes 0-15:  Texture descriptor indices (4× u32)
//   Bytes 16-31: Material properties

struct alignas(16) MaterialData {
    // Descriptor heap indices (UINT32_MAX = invalid/not present)
    u32 diffuseIndex;    // Base color / albedo texture
    u32 normalIndex;     // Normal map texture
    u32 detailIndex;     // Detail texture
    u32 pbrIndex;        // Packed Metallic/Roughness/AO texture

    // Material properties
    float detailScale;   // Detail texture tiling multiplier
    float alphaRef;      // Alpha test threshold (0.5 typical)
    u32 flags;           // Material flags (see MaterialFlags)
    u32 shaderVariant;   // Index into ShaderVariantRegistry (0=default)
};
static_assert(sizeof(MaterialData) == 32, "MaterialData must be 32 bytes for GPU alignment");

// Material flags (must match HLSL)
enum MaterialFlags : u32 {
    MAT_FLAG_ALPHA_TEST    = (1 << 0),  // Enable alpha testing
    MAT_FLAG_TWO_SIDED     = (1 << 1),  // Disable backface culling
    MAT_FLAG_EMISSIVE      = (1 << 2),  // Has emissive component
    MAT_FLAG_HAS_DETAIL    = (1 << 3),  // Has detail texture (detailIndex valid)
    MAT_FLAG_HAS_NORMAL    = (1 << 4),  // Has normal map (normalIndex valid)
    MAT_FLAG_HAS_PBR       = (1 << 5),  // Has PBR textures (pbrIndex valid)
    MAT_FLAG_TERRAIN       = (1 << 6),  // Terrain 4-layer blending material
    MAT_FLAG_HAS_PBR_LAYER = (1 << 7),  // Terrain has PBR detail textures
    MAT_FLAG_ALPHA_BLEND   = (1 << 8),  // Transparent alpha blending
};

// ═══════════════════════════════════════════════════════
//  TERRAIN MATERIAL DATA (matches HLSL TerrainMaterialData)
// ═══════════════════════════════════════════════════════
// Terrain uses 4-layer detail blending with RGBA mask
// Each layer has: color, normal, and optional PBR textures
//
// Layout (64 bytes total):
//   Bytes 0-7:   Base and mask indices
//   Bytes 8-23:  Detail color indices (R/G/B/A)
//   Bytes 24-39: Detail normal indices (R/G/B/A)
//   Bytes 40-55: Detail PBR indices (R/G/B/A)
//   Bytes 56-63: Properties

constexpr u32 MAX_TERRAIN_MATERIALS = 512 * 4;

struct alignas(16) TerrainMaterialData {
    // Base textures
    u32 baseAlbedoIndex;     // Level terrain base texture (e.g., terrain\terrain_zaton)
    u32 blendMaskIndex;      // RGBA blend mask (e.g., terrain\terrain_zaton_mask)

    // Detail color textures (s_dt_r/g/b/a)
    u32 detailR_Index;       // Detail texture for mask.r channel
    u32 detailG_Index;       // Detail texture for mask.g channel
    u32 detailB_Index;       // Detail texture for mask.b channel
    u32 detailA_Index;       // Detail texture for mask.a channel

    // Detail normal textures (s_dn_r/g/b/a)
    u32 normalR_Index;       // Detail normal for mask.r channel
    u32 normalG_Index;       // Detail normal for mask.g channel
    u32 normalB_Index;       // Detail normal for mask.b channel
    u32 normalA_Index;       // Detail normal for mask.a channel

    // Detail PBR textures (s_pbr_r/g/b/a) - optional
    u32 pbrR_Index;          // PBR for mask.r channel (R=metal, G=rough, B=AO, A=parallax)
    u32 pbrG_Index;          // PBR for mask.g channel
    u32 pbrB_Index;          // PBR for mask.b channel
    u32 pbrA_Index;          // PBR for mask.a channel

    // Properties
    float detailScale;       // Uniform tiling scale for all 4 detail layers
    u32 flags;               // MAT_FLAG_TERRAIN, MAT_FLAG_HAS_PBR_LAYER
};
static_assert(sizeof(TerrainMaterialData) == 64, "TerrainMaterialData must be 64 bytes for GPU alignment");

} // namespace xray::render::RENDER_NAMESPACE::bindless
