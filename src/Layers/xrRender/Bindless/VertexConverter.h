// xrRender/Bindless/VertexConverter.h
// Converts X-Ray vertex formats to UnifiedVertex for GPU-driven rendering

#pragma once

#include "UnifiedVertex.h"
#include "Common/OGF_GContainer_Vertices.hpp"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════════════════
//  VERTEX CONVERTER
// ═══════════════════════════════════════════════════════════════════
//
// Converts various X-Ray vertex formats to UnifiedVertex.
// All conversions unpack UVs to float2 but keep normals packed.

class VertexConverter {
public:
    // ───────────────────────────────────────────────────────────────
    //  SINGLE VERTEX CONVERSION
    // ───────────────────────────────────────────────────────────────

    // Convert r1v_lmap (lightmapped terrain/walls) to UnifiedVertex
    static void ConvertR1Lmap(const r1v_lmap& src, UnifiedVertex& dst);

    // Convert r1v_vert (vertex-lit objects) to UnifiedVertex
    static void ConvertR1Vert(const r1v_vert& src, UnifiedVertex& dst);

    // Convert mu_model_vert (trees/models) to UnifiedVertex
    static void ConvertMuModel(const mu_model_vert& src, UnifiedVertex& dst);

    // Convert already-unpacked formats
    static void ConvertR1LmapUnpacked(const r1v_lmap_unpacked& src, UnifiedVertex& dst);
    static void ConvertR1VertUnpacked(const r1v_vert_unpacked& src, UnifiedVertex& dst);
    static void ConvertMuModelUnpacked(const mu_model_vert_unpacked& src, UnifiedVertex& dst);

    // Convert position-only vertex
    static void ConvertXVert(const x_vert& src, UnifiedVertex& dst);

    // ───────────────────────────────────────────────────────────────
    //  BATCH CONVERSION
    // ───────────────────────────────────────────────────────────────

    // Convert array of vertices based on detected format
    // Returns number of vertices converted
    static u32 ConvertVertices(
        const void* srcData,
        u32 srcStride,
        u32 vertexCount,
        SourceVertexFormat format,
        UnifiedVertex* dstData
    );

    // Auto-detect format and convert
    // hints help disambiguation between same-stride formats
    static u32 ConvertVerticesAuto(
        const void* srcData,
        u32 srcStride,
        u32 vertexCount,
        UnifiedVertex* dstData,
        bool hasLightmapUV = false,
        bool hasVertexColor = false
    );

    // ───────────────────────────────────────────────────────────────
    //  UV UNPACKING HELPERS
    // ───────────────────────────────────────────────────────────────

    // Unpack X-Ray's s24 UV encoding: (short + alpha_frac) * (32/32768)
    static float UnpackS24_UV(s16 primary, float alphaFrac);

    // Unpack lightmap UV: short * (1/32768)
    static float UnpackLmapUV(s16 value);

    // Unpack mu_model UV: short * (32/32768)
    static float UnpackMuModelUV(s16 value);

    // ───────────────────────────────────────────────────────────────
    //  NORMAL UNPACKING (for reference - shader does this)
    // ───────────────────────────────────────────────────────────────

    // Unpack D3DCOLOR normal to float3 (for CPU-side use if needed)
    static Fvector3 UnpackNormal(u32 packed);

private:
    // Default values for missing attributes
    static constexpr u32 DEFAULT_COLOR = 0xFFFFFFFF;  // White, full alpha
    static constexpr u32 DEFAULT_TANGENT = 0x8080FF80; // +X tangent
    static constexpr u32 DEFAULT_BINORMAL = 0x80FF8080; // +Y binormal
    static constexpr u32 DEFAULT_NORMAL = 0xFF808080;  // +Z normal
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
