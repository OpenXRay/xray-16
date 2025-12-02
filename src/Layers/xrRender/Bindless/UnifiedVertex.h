#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector2.h"
#include "xrCore/_vector3d.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

#pragma pack(push, 1)
struct UnifiedVertex {
    Fvector3 position;
    u32 normal;
    u32 tangent;
    u32 binormal;
    Fvector2 texcoord0;
    Fvector2 texcoord1;
    u32 color;
    u32 flags;
};
#pragma pack(pop)

static_assert(sizeof(UnifiedVertex) == 48, "UnifiedVertex must be 48 bytes");
static_assert(offsetof(UnifiedVertex, position) == 0, "position at offset 0");
static_assert(offsetof(UnifiedVertex, normal) == 12, "normal at offset 12");
static_assert(offsetof(UnifiedVertex, tangent) == 16, "tangent at offset 16");
static_assert(offsetof(UnifiedVertex, binormal) == 20, "binormal at offset 20");
static_assert(offsetof(UnifiedVertex, texcoord0) == 24, "texcoord0 at offset 24");
static_assert(offsetof(UnifiedVertex, texcoord1) == 32, "texcoord1 at offset 32");
static_assert(offsetof(UnifiedVertex, color) == 40, "color at offset 40");
static_assert(offsetof(UnifiedVertex, flags) == 44, "flags at offset 44");

enum class SourceVertexFormat : u8 {
    Unknown = 0,
    R1_Lmap,
    R1_Vert,
    MU_Model,
    R1_Lmap_Unpacked,
    R1_Vert_Unpacked,
    MU_Model_Unpacked,
    X_Vert,
};

inline SourceVertexFormat DetectVertexFormat(u32 stride, bool hasLightmap, bool hasColor) {
    switch (stride) {
        case 12: return SourceVertexFormat::X_Vert;
        case 28:
            return hasColor ? SourceVertexFormat::R1_Vert_Unpacked : SourceVertexFormat::MU_Model_Unpacked;
        case 32:
            if (hasLightmap) return SourceVertexFormat::R1_Lmap;
            if (hasColor) return SourceVertexFormat::R1_Vert;
            return SourceVertexFormat::MU_Model;
        default:
            return SourceVertexFormat::Unknown;
    }
}

constexpr u32 UNIFIED_VERTEX_STRIDE = 48;

}
