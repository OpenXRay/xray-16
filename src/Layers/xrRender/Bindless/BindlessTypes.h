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
    float padding;       // Pad to 32 bytes (16-byte aligned)
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
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
