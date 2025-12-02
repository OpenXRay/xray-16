// xrRender/Bindless/BindlessTypes.h
// Simplified bindless rendering types
// Just 4 texture arrays (Diffuse, Normal, Detail, PBR), all RGBA8 format
#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  CONFIGURATION
// ═══════════════════════════════════════════════════════

// All textures are resized/padded to this common size
constexpr u32 ATLAS_TEXTURE_SIZE = 1024;

// Maximum textures per atlas array
constexpr u32 MAX_TEXTURES_PER_ATLAS = 2048;

// Maximum unique materials
constexpr u32 MAX_MATERIALS = 16384;

// ═══════════════════════════════════════════════════════
//  ATLAS TYPES
// ═══════════════════════════════════════════════════════
// Just 4 texture arrays - one per type, all same size and format

enum class AtlasType : u8 {
    Diffuse  = 0,   // Base color / albedo
    Normal   = 1,   // Normal maps
    Detail   = 2,   // Detail textures
    PBR      = 3,   // Metallic/Roughness/AO packed
    Count    = 4
};

constexpr u32 TOTAL_ATLAS_COUNT = static_cast<u32>(AtlasType::Count);  // 4

inline const char* GetAtlasTypeName(AtlasType type) {
    static const char* names[] = { "Diffuse", "Normal", "Detail", "PBR" };
    return names[static_cast<u8>(type)];
}

// ═══════════════════════════════════════════════════════
//  TEXTURE SLICE REFERENCE
// ═══════════════════════════════════════════════════════
// Simple encoding: atlas type (8 bits) + slice index (24 bits)

struct TextureSliceRef {
    u32 packed = 0xFFFFFFFF;  // Invalid by default

    TextureSliceRef() = default;

    TextureSliceRef(AtlasType type, u32 sliceIndex) {
        packed = (static_cast<u32>(type) << 24) | (sliceIndex & 0x00FFFFFF);
    }

    bool IsValid() const { return packed != 0xFFFFFFFF; }

    AtlasType GetType() const { return static_cast<AtlasType>(packed >> 24); }
    u32 GetSliceIndex() const { return packed & 0x00FFFFFF; }
};

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches HLSL MaterialData struct)
// ═══════════════════════════════════════════════════════
// GPU-side material representation - must match HLSL exactly!

struct alignas(16) MaterialData {
    // Texture slice references (type in high 8 bits, slice in low 24 bits)
    u32 diffuseSlice;
    u32 normalSlice;
    u32 detailSlice;
    u32 pbrSlice;

    // Material properties
    float detailScale;
    float alphaRef;
    u32 flags;
    float padding1;

    // UV scale factors for textures smaller than ATLAS_TEXTURE_SIZE
    // Original texture is placed at top-left, rest is black
    // uvScale = originalSize / ATLAS_TEXTURE_SIZE
    float diffuseUVScaleU;
    float diffuseUVScaleV;
    float padding2;
    float padding3;
};
static_assert(sizeof(MaterialData) == 48, "MaterialData must be 48 bytes for GPU alignment");

// Material flags (must match HLSL)
enum MaterialFlags : u32 {
    MAT_FLAG_ALPHA_TEST    = (1 << 0),
    MAT_FLAG_TWO_SIDED     = (1 << 1),
    MAT_FLAG_EMISSIVE      = (1 << 2),
    MAT_FLAG_HAS_DETAIL    = (1 << 3),
    MAT_FLAG_HAS_NORMAL    = (1 << 4),
    MAT_FLAG_HAS_PBR       = (1 << 5),
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
