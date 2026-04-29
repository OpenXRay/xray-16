#pragma once

#include "UnifiedVertex.h"
#include "Common/OGF_GContainer_Vertices.hpp"

namespace xray::render::fg::bindless {

class VertexConverter {
public:

    static void ConvertR1Lmap(const r1v_lmap& src, UnifiedVertex& dst);

    static void ConvertR1Vert(const r1v_vert& src, UnifiedVertex& dst);

    static void ConvertMuModel(const mu_model_vert& src, UnifiedVertex& dst);

    static void ConvertR1LmapUnpacked(const r1v_lmap_unpacked& src, UnifiedVertex& dst);
    static void ConvertR1VertUnpacked(const r1v_vert_unpacked& src, UnifiedVertex& dst);
    static void ConvertMuModelUnpacked(const mu_model_vert_unpacked& src, UnifiedVertex& dst);

    static void ConvertXVert(const x_vert& src, UnifiedVertex& dst);

    static u32 ConvertVertices(
        const void* srcData,
        u32 srcStride,
        u32 vertexCount,
        SourceVertexFormat format,
        UnifiedVertex* dstData
    );

    static u32 ConvertVerticesAuto(
        const void* srcData,
        u32 srcStride,
        u32 vertexCount,
        UnifiedVertex* dstData,
        bool hasLightmapUV = false,
        bool hasVertexColor = false
    );

    static float UnpackS24_UV(s16 primary, float alphaFrac);

    static float UnpackLmapUV(s16 value);

    static float UnpackMuModelUV(s16 value);

    static Fvector3 UnpackNormal(u32 packed);

private:
    static constexpr u32 DEFAULT_COLOR = 0xFFFFFFFF;
    static constexpr u32 DEFAULT_TANGENT = 0x8080FF80;
    static constexpr u32 DEFAULT_BINORMAL = 0x80FF8080;
    static constexpr u32 DEFAULT_NORMAL = 0xFF808080;
};

}
