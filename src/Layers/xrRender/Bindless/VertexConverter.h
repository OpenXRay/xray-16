// xrRender/Bindless/VertexConverter.h

#pragma once

#include "UnifiedVertex.h"
#include "Layers/xrRender/VertexLayout.h"
#include "Common/OGF_GContainer_Vertices.hpp"

namespace xray::render::fg::bindless {


constexpr u16 SOURCE_ELEMENT_NONE = 0xFFFFu;
constexpr u8 SOURCE_TYPE_NONE = 0xFFu;

class SourceVertexLayout
{
public:
    u32 stride = 0;
    u16 positionOffset = SOURCE_ELEMENT_NONE;
    u16 normalOffset = SOURCE_ELEMENT_NONE;
    u16 tangentOffset = SOURCE_ELEMENT_NONE;
    u16 binormalOffset = SOURCE_ELEMENT_NONE;
    u16 colorOffset = SOURCE_ELEMENT_NONE;
    u16 uv0Offset = SOURCE_ELEMENT_NONE;
    u16 uv1Offset = SOURCE_ELEMENT_NONE;
    u8 normalType = SOURCE_TYPE_NONE;
    u8 uv0Type = SOURCE_TYPE_NONE;
    u8 uv1Type = SOURCE_TYPE_NONE;
    u8 valid = 0;

    bool IsValid() const;

    bool HasFloatBasis() const;

    u32 Signature() const;
};

SourceVertexLayout BuildSourceVertexLayout(const VertexElement* decl, u32 stride);

// ═══════════════════════════════════════════════════════════════════
//  VERTEX CONVERTER
// ═══════════════════════════════════════════════════════════════════

class VertexConverter {
public:
    static u32 ConvertVertices(
        const void* srcData,
        u32 vertexCount,
        const SourceVertexLayout& layout,
        UnifiedVertex* dstData,
        Fvector3* outFloatNormals
    );

    // ───────────────────────────────────────────────────────────────
    //  UV UNPACKING HELPERS
    // ───────────────────────────────────────────────────────────────

    // Unpack X-Ray's s24 UV encoding: (short + alpha_frac) * (32/32768)
    static float UnpackS24_UV(s16 primary, float alphaFrac);

    // Unpack lightmap UV: short * (1/32768)
    static float UnpackLmapUV(s16 value);

    // Unpack mu_model UV: short * (16/32768)
    static float UnpackMuModelUV(s16 value);

    // Unpack D3DCOLOR direction to float3
    static Fvector3 UnpackNormal(u32 packed);

    static u32 PackNormal(const Fvector3& n);

    // Default values for missing attributes
    static constexpr u32 DEFAULT_COLOR = 0xFFFFFFFF;  // White, full alpha
    static constexpr u32 DEFAULT_TANGENT = 0x8080FF80; // +X tangent
    static constexpr u32 DEFAULT_BINORMAL = 0x80FF8080; // +Y binormal
    static constexpr u32 DEFAULT_NORMAL = 0xFF808080;  // +Z normal
};

} // namespace xray::render::fg::bindless
