#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

constexpr u32 MAX_MATERIALS = 16384;

constexpr u32 INVALID_TEXTURE_INDEX = UINT32_MAX;

enum class TextureType : u8 {
    Diffuse  = 0,
    Normal   = 1,
    Detail   = 2,
    PBR      = 3,
    Count    = 4
};

constexpr u32 TOTAL_TEXTURE_TYPES = static_cast<u32>(TextureType::Count);

inline const char* GetTextureTypeName(TextureType type) {
    static const char* names[] = { "Diffuse", "Normal", "Detail", "PBR" };
    return names[static_cast<u8>(type)];
}

struct alignas(16) MaterialData {
    u32 diffuseIndex;
    u32 normalIndex;
    u32 detailIndex;
    u32 pbrIndex;

    float detailScale;
    float alphaRef;
    u32 flags;
    float padding;
};
static_assert(sizeof(MaterialData) == 32, "MaterialData must be 32 bytes for GPU alignment");

enum MaterialFlags : u32 {
    MAT_FLAG_ALPHA_TEST    = (1 << 0),
    MAT_FLAG_TWO_SIDED     = (1 << 1),
    MAT_FLAG_EMISSIVE      = (1 << 2),
    MAT_FLAG_HAS_DETAIL    = (1 << 3),
    MAT_FLAG_HAS_NORMAL    = (1 << 4),
    MAT_FLAG_HAS_PBR       = (1 << 5),
    MAT_FLAG_TERRAIN       = (1 << 6),
    MAT_FLAG_HAS_PBR_LAYER = (1 << 7),
};

constexpr u32 MAX_TERRAIN_MATERIALS = 512 * 4;

struct alignas(16) TerrainMaterialData {
    u32 baseAlbedoIndex;
    u32 blendMaskIndex;

    u32 detailR_Index;
    u32 detailG_Index;
    u32 detailB_Index;
    u32 detailA_Index;

    u32 normalR_Index;
    u32 normalG_Index;
    u32 normalB_Index;
    u32 normalA_Index;

    u32 pbrR_Index;
    u32 pbrG_Index;
    u32 pbrB_Index;
    u32 pbrA_Index;

    float detailScale;
    u32 flags;
};
static_assert(sizeof(TerrainMaterialData) == 64, "TerrainMaterialData must be 64 bytes for GPU alignment");

}
