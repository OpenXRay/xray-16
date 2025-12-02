#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

constexpr u32 ATLAS_TEXTURE_SIZE = 1024;

constexpr u32 MAX_TEXTURES_PER_ATLAS = 2048;

constexpr u32 MAX_MATERIALS = 16384;

enum class AtlasType : u8 {
    Diffuse  = 0,
    Normal   = 1,
    Detail   = 2,
    PBR      = 3,
    Count    = 4
};

constexpr u32 TOTAL_ATLAS_COUNT = static_cast<u32>(AtlasType::Count);

inline const char* GetAtlasTypeName(AtlasType type) {
    static const char* names[] = { "Diffuse", "Normal", "Detail", "PBR" };
    return names[static_cast<u8>(type)];
}

struct TextureSliceRef {
    u32 packed = 0xFFFFFFFF;

    TextureSliceRef() = default;

    TextureSliceRef(AtlasType type, u32 sliceIndex) {
        packed = (static_cast<u32>(type) << 24) | (sliceIndex & 0x00FFFFFF);
    }

    bool IsValid() const { return packed != 0xFFFFFFFF; }

    AtlasType GetType() const { return static_cast<AtlasType>(packed >> 24); }
    u32 GetSliceIndex() const { return packed & 0x00FFFFFF; }
};

struct alignas(16) MaterialData {
    u32 diffuseSlice;
    u32 normalSlice;
    u32 detailSlice;
    u32 pbrSlice;

    float detailScale;
    float alphaRef;
    u32 flags;
    float padding1;

    float diffuseUVScaleU;
    float diffuseUVScaleV;
    float padding2;
    float padding3;
};
static_assert(sizeof(MaterialData) == 48, "MaterialData must be 48 bytes for GPU alignment");

enum MaterialFlags : u32 {
    MAT_FLAG_ALPHA_TEST    = (1 << 0),
    MAT_FLAG_TWO_SIDED     = (1 << 1),
    MAT_FLAG_EMISSIVE      = (1 << 2),
    MAT_FLAG_HAS_DETAIL    = (1 << 3),
    MAT_FLAG_HAS_NORMAL    = (1 << 4),
    MAT_FLAG_HAS_PBR       = (1 << 5),
};

}
