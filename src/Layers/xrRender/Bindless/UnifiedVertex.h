// xrRender/Bindless/UnifiedVertex.h
// Unified vertex format for GPU-driven rendering
// All X-Ray vertex formats convert to this common format

#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector2.h"
#include "xrCore/_vector3d.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════════════════
//  UNIFIED VERTEX FORMAT (48 bytes)
// ═══════════════════════════════════════════════════════════════════
//
// This format can represent all X-Ray vertex types:
//   - r1v_lmap (lightmapped terrain/walls)
//   - r1v_vert (vertex-lit objects)
//   - mu_model_vert (trees/models)
//
// Design decisions:
//   - Normal/tangent/binormal kept as packed D3DCOLOR (shader unpacks: val * 2 - 1)
//   - UVs fully unpacked to float2 (eliminates complex s24 decoding)
//   - Lightmap UV slot (zeros for non-lightmapped)
//   - Vertex color slot (0xFFFFFFFF for non-vertex-lit)
//
// Memory layout matches shader input layout for zero-copy GPU upload

#pragma pack(push, 1)
struct UnifiedVertex {
    Fvector3 position;    // 12 bytes - world position
    u32 normal;           // 4 bytes  - packed D3DCOLOR normal (RGB=normal*0.5+0.5, A=hemi)
    u32 tangent;          // 4 bytes  - packed D3DCOLOR tangent
    u32 binormal;         // 4 bytes  - packed D3DCOLOR binormal
    Fvector2 texcoord0;   // 8 bytes  - base UV (fully unpacked)
    Fvector2 texcoord1;   // 8 bytes  - lightmap UV (or zeros)
    u32 color;            // 4 bytes  - vertex color RGBA (or 0xFFFFFFFF)
    u32 flags;            // 4 bytes  - reserved for future use
    // Total: 48 bytes (aligned)
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

// ═══════════════════════════════════════════════════════════════════
//  VERTEX FORMAT IDENTIFIERS
// ═══════════════════════════════════════════════════════════════════

enum class SourceVertexFormat : u8 {
    Unknown = 0,
    R1_Lmap,          // r1v_lmap - lightmapped (32 bytes)
    R1_Vert,          // r1v_vert - vertex-lit (32 bytes)
    MU_Model,         // mu_model_vert - trees/models (32 bytes)
    R1_Lmap_Unpacked, // r1v_lmap_unpacked (32 bytes)
    R1_Vert_Unpacked, // r1v_vert_unpacked (28 bytes)
    MU_Model_Unpacked,// mu_model_vert_unpacked (28 bytes)
    X_Vert,           // x_vert - position only (12 bytes)
};

// Get format from vertex stride (heuristic)
inline SourceVertexFormat DetectVertexFormat(u32 stride, bool hasLightmap, bool hasColor) {
    switch (stride) {
        case 12: return SourceVertexFormat::X_Vert;
        case 28:
            // Could be r1_vert_unpacked or mu_model_unpacked
            return hasColor ? SourceVertexFormat::R1_Vert_Unpacked : SourceVertexFormat::MU_Model_Unpacked;
        case 32:
            // Could be r1_lmap, r1_vert, mu_model, or r1_lmap_unpacked
            if (hasLightmap) return SourceVertexFormat::R1_Lmap;
            if (hasColor) return SourceVertexFormat::R1_Vert;
            return SourceVertexFormat::MU_Model;
        default:
            return SourceVertexFormat::Unknown;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  SHADER INPUT LAYOUT (matches UnifiedVertex)
// ═══════════════════════════════════════════════════════════════════
//
// HLSL struct:
//   struct VS_INPUT {
//       float3 position  : POSITION;    // offset 0
//       float4 normal    : NORMAL;      // offset 12 (BGRA8_UNORM)
//       float4 tangent   : TANGENT;     // offset 16 (BGRA8_UNORM)
//       float4 binormal  : BINORMAL;    // offset 20 (BGRA8_UNORM)
//       float2 texcoord0 : TEXCOORD0;   // offset 24 (FLOAT2)
//       float2 texcoord1 : TEXCOORD1;   // offset 32 (FLOAT2)
//       float4 color     : COLOR;       // offset 40 (BGRA8_UNORM)
//   };
//
// nvrhi input layout:
//   Position:  RGB32_FLOAT  at offset 0,  stride 48
//   Normal:    BGRA8_UNORM  at offset 12, stride 48
//   Tangent:   BGRA8_UNORM  at offset 16, stride 48
//   Binormal:  BGRA8_UNORM  at offset 20, stride 48
//   TexCoord0: RG32_FLOAT   at offset 24, stride 48
//   TexCoord1: RG32_FLOAT   at offset 32, stride 48
//   Color:     BGRA8_UNORM  at offset 40, stride 48

constexpr u32 UNIFIED_VERTEX_STRIDE = 48;

} // namespace xray::render::RENDER_NAMESPACE::bindless
