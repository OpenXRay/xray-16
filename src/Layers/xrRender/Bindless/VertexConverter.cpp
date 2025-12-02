#include "stdafx.h"
#include "VertexConverter.h"

static_assert(sizeof(r1v_lmap) == 32, "r1v_lmap must be 32 bytes");
static_assert(sizeof(r1v_vert) == 32, "r1v_vert must be 32 bytes");
static_assert(sizeof(mu_model_vert) == 32, "mu_model_vert must be 32 bytes");
static_assert(sizeof(r1v_lmap_unpacked) == 32, "r1v_lmap_unpacked must be 32 bytes");
static_assert(sizeof(r1v_vert_unpacked) == 28, "r1v_vert_unpacked must be 28 bytes");
static_assert(sizeof(mu_model_vert_unpacked) == 28, "mu_model_vert_unpacked must be 28 bytes");
static_assert(sizeof(x_vert) == 12, "x_vert must be 12 bytes");

namespace xray::render::RENDER_NAMESPACE::bindless {

float VertexConverter::UnpackS24_UV(s16 primary, float alphaFrac)
{
    return (static_cast<float>(primary) + alphaFrac) * (32.0f / 32768.0f);
}

float VertexConverter::UnpackLmapUV(s16 value)
{
    return static_cast<float>(value) * (1.0f / 32768.0f);
}

float VertexConverter::UnpackMuModelUV(s16 value)
{
    return static_cast<float>(value) * (32.0f / 32768.0f);
}

Fvector3 VertexConverter::UnpackNormal(u32 packed)
{
    Fcolor c(packed);
    return Fvector3{
        c.r * 2.0f - 1.0f,
        c.g * 2.0f - 1.0f,
        c.b * 2.0f - 1.0f
    };
}

void VertexConverter::ConvertR1Lmap(const r1v_lmap& src, UnifiedVertex& dst)
{
    dst.position = src.P;

    dst.normal = src.N;
    dst.tangent = src.T;
    dst.binormal = src.B;

    Fcolor tColor(src.T);
    Fcolor bColor(src.B);

    dst.texcoord0.x = (src.tc0.x + tColor.a) * (32.f / 32768.f);
    dst.texcoord0.y = (src.tc0.y + bColor.a) * (32.f / 32768.f);

    dst.texcoord1.x = src.tc1.x * (1.f / 32768.f);
    dst.texcoord1.y = src.tc1.y * (1.f / 32768.f);

    dst.color = DEFAULT_COLOR;
    dst.flags = 0;
}

void VertexConverter::ConvertR1Vert(const r1v_vert& src, UnifiedVertex& dst)
{
    dst.position = src.P;

    dst.normal = src.N;
    dst.tangent = src.T;
    dst.binormal = src.B;

    Fcolor tColor(src.T);
    Fcolor bColor(src.B);

    static float s_minU = FLT_MAX, s_maxU = -FLT_MAX;
    static float s_minV = FLT_MAX, s_maxV = -FLT_MAX;
    static int s_debugCount = 0;
    float u = (src.tc.x + tColor.a) * (32.f / 32768.f);
    float v = (src.tc.y + bColor.a) * (32.f / 32768.f);
    s_minU = std::min(s_minU, u); s_maxU = std::max(s_maxU, u);
    s_minV = std::min(s_minV, v); s_maxV = std::max(s_maxV, v);

    if (s_debugCount < 5) {
        Msg("  [UV Debug] r1_vert: tc=(%d,%d), T.a=%.4f, B.a=%.4f -> uv=(%.4f,%.4f)",
            src.tc.x, src.tc.y, tColor.a, bColor.a, u, v);
        s_debugCount++;
    }
    static bool s_rangeLogged = false;
    if (!s_rangeLogged && s_debugCount >= 5) {
        Msg("  [UV Debug] r1_vert range: U=[%.2f, %.2f], V=[%.2f, %.2f]",
            s_minU, s_maxU, s_minV, s_maxV);
        s_rangeLogged = true;
    }

    dst.texcoord0.x = (src.tc.x + tColor.a) * (32.f / 32768.f);
    dst.texcoord0.y = (src.tc.y + bColor.a) * (32.f / 32768.f);

    dst.texcoord1.x = 0.0f;
    dst.texcoord1.y = 0.0f;

    dst.color = src.C;
    dst.flags = 0;
}

void VertexConverter::ConvertMuModel(const mu_model_vert& src, UnifiedVertex& dst)
{
    dst.position = src.P;

    dst.normal = src.N;
    dst.tangent = src.T;
    dst.binormal = src.B;

    float u = UnpackMuModelUV(src.misc.x);
    float v = UnpackMuModelUV(src.misc.y);

    static float s_minU = FLT_MAX, s_maxU = -FLT_MAX;
    static float s_minV = FLT_MAX, s_maxV = -FLT_MAX;
    static int s_debugCount = 0;
    s_minU = std::min(s_minU, u); s_maxU = std::max(s_maxU, u);
    s_minV = std::min(s_minV, v); s_maxV = std::max(s_maxV, v);

    if (s_debugCount < 10) {
        Msg("  [UV Debug] mu_model: misc=(%d,%d,%d,%d) -> uv=(%.4f,%.4f)",
            src.misc.x, src.misc.y, src.misc.z, src.misc.w, u, v);
        s_debugCount++;
    }
    static bool s_rangeLogged = false;
    if (!s_rangeLogged && s_debugCount >= 10) {
        Msg("  [UV Debug] mu_model UV range: U=[%.2f, %.2f], V=[%.2f, %.2f]",
            s_minU, s_maxU, s_minV, s_maxV);
        s_rangeLogged = true;
    }

    dst.texcoord0.x = u;
    dst.texcoord0.y = v;

    dst.texcoord1.x = 0.0f;
    dst.texcoord1.y = 0.0f;

    dst.color = DEFAULT_COLOR;
    dst.flags = 0;
}

void VertexConverter::ConvertR1LmapUnpacked(const r1v_lmap_unpacked& src, UnifiedVertex& dst)
{
    dst.position = src.P;
    dst.normal = src.N;
    dst.tangent = DEFAULT_TANGENT;
    dst.binormal = DEFAULT_BINORMAL;
    dst.texcoord0 = src.tc0;
    dst.texcoord1 = src.tc1;
    dst.color = DEFAULT_COLOR;
    dst.flags = 0;
}

void VertexConverter::ConvertR1VertUnpacked(const r1v_vert_unpacked& src, UnifiedVertex& dst)
{
    dst.position = src.P;
    dst.normal = src.N;
    dst.tangent = DEFAULT_TANGENT;
    dst.binormal = DEFAULT_BINORMAL;
    dst.texcoord0 = src.tc;
    dst.texcoord1.set(0.0f, 0.0f);
    dst.color = src.C;
    dst.flags = 0;
}

void VertexConverter::ConvertMuModelUnpacked(const mu_model_vert_unpacked& src, UnifiedVertex& dst)
{
    dst.position = src.P;
    dst.normal = src.N;
    dst.tangent = DEFAULT_TANGENT;
    dst.binormal = DEFAULT_BINORMAL;
    dst.texcoord0 = src.tc;
    dst.texcoord1.set(0.0f, 0.0f);
    dst.color = src.C;
    dst.flags = 0;
}

void VertexConverter::ConvertXVert(const x_vert& src, UnifiedVertex& dst)
{
    dst.position = src.P;
    dst.normal = DEFAULT_NORMAL;
    dst.tangent = DEFAULT_TANGENT;
    dst.binormal = DEFAULT_BINORMAL;
    dst.texcoord0.set(0.0f, 0.0f);
    dst.texcoord1.set(0.0f, 0.0f);
    dst.color = DEFAULT_COLOR;
    dst.flags = 0;
}

u32 VertexConverter::ConvertVertices(
    const void* srcData,
    u32 srcStride,
    u32 vertexCount,
    SourceVertexFormat format,
    UnifiedVertex* dstData)
{
    if (!srcData || !dstData || vertexCount == 0)
        return 0;

    const u8* src = static_cast<const u8*>(srcData);

    switch (format) {
        case SourceVertexFormat::R1_Lmap:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertR1Lmap(*reinterpret_cast<const r1v_lmap*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::R1_Vert:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertR1Vert(*reinterpret_cast<const r1v_vert*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::MU_Model:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertMuModel(*reinterpret_cast<const mu_model_vert*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::R1_Lmap_Unpacked:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertR1LmapUnpacked(*reinterpret_cast<const r1v_lmap_unpacked*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::R1_Vert_Unpacked:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertR1VertUnpacked(*reinterpret_cast<const r1v_vert_unpacked*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::MU_Model_Unpacked:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertMuModelUnpacked(*reinterpret_cast<const mu_model_vert_unpacked*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        case SourceVertexFormat::X_Vert:
            for (u32 i = 0; i < vertexCount; i++) {
                ConvertXVert(*reinterpret_cast<const x_vert*>(src), dstData[i]);
                src += srcStride;
            }
            break;

        default:
            Msg("! [VertexConverter] Unknown vertex format, cannot convert %u vertices", vertexCount);
            return 0;
    }

    return vertexCount;
}

u32 VertexConverter::ConvertVerticesAuto(
    const void* srcData,
    u32 srcStride,
    u32 vertexCount,
    UnifiedVertex* dstData,
    bool hasLightmapUV,
    bool hasVertexColor)
{
    SourceVertexFormat format = DetectVertexFormat(srcStride, hasLightmapUV, hasVertexColor);

    if (format == SourceVertexFormat::Unknown) {
        Msg("! [VertexConverter] Cannot auto-detect format for stride %u", srcStride);
        return 0;
    }

    return ConvertVertices(srcData, srcStride, vertexCount, format, dstData);
}

}
