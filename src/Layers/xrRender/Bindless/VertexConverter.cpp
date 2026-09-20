// xrRender/Bindless/VertexConverter.cpp
#include "stdafx.h"
#include "VertexConverter.h"
#include "Layers/xrRender/ClusterBasis.h"

// Verify source vertex struct sizes match expected
static_assert(sizeof(r1v_lmap) == 32, "r1v_lmap must be 32 bytes");
static_assert(sizeof(r1v_vert) == 32, "r1v_vert must be 32 bytes");
static_assert(sizeof(mu_model_vert) == 32, "mu_model_vert must be 32 bytes");
static_assert(sizeof(r1v_lmap_unpacked) == 32, "r1v_lmap_unpacked must be 32 bytes");
static_assert(sizeof(r1v_vert_unpacked) == 28, "r1v_vert_unpacked must be 28 bytes");
static_assert(sizeof(mu_model_vert_unpacked) == 28, "mu_model_vert_unpacked must be 28 bytes");
static_assert(sizeof(x_vert) == 12, "x_vert must be 12 bytes");

namespace xray::render::fg::bindless {

namespace {

u32 ElementSize(u8 type)
{
    switch (type) {
    case VF_FLOAT1: return 4;
    case VF_FLOAT2: return 8;
    case VF_FLOAT3: return 12;
    case VF_FLOAT4: return 16;
    case VF_COLOR: return 4;
    case VF_SHORT2: return 4;
    case VF_SHORT4: return 8;
    default: return 0;
    }
}

bool AssignSlot(u16& slot, u8& typeSlot, const VertexElement& elem, u32 stride)
{
    const u32 size = ElementSize(elem.Type);
    if (size == 0)
        return false;
    if (u32(elem.Offset) + size > stride)
        return false;
    if (slot != SOURCE_ELEMENT_NONE)
        return false;
    slot = elem.Offset;
    typeSlot = elem.Type;
    return true;
}

float ReadFloat(const u8* base, u16 offset)
{
    float v;
    memcpy(&v, base + offset, sizeof(v));
    return v;
}

s16 ReadShort(const u8* base, u16 offset)
{
    s16 v;
    memcpy(&v, base + offset, sizeof(v));
    return v;
}

u32 ReadU32(const u8* base, u16 offset)
{
    u32 v;
    memcpy(&v, base + offset, sizeof(v));
    return v;
}

u64 Mix(u64 h, u32 value)
{
    h ^= value;
    h *= 1099511628211ull;
    return h;
}

} // namespace

u32 SourceVertexLayout::Signature() const
{
    if (!valid)
        return 0;
    u64 h = 14695981039346656037ull;
    h = Mix(h, stride);
    h = Mix(h, u32(positionOffset));
    h = Mix(h, u32(normalOffset) | (u32(normalType) << 16));
    h = Mix(h, u32(tangentOffset));
    h = Mix(h, u32(binormalOffset));
    h = Mix(h, u32(colorOffset));
    h = Mix(h, u32(uv0Offset) | (u32(uv0Type) << 16));
    h = Mix(h, u32(uv1Offset) | (u32(uv1Type) << 16));
    const u32 folded = u32(h ^ (h >> 32));
    return folded ? folded : 1u;
}

SourceVertexLayout BuildSourceVertexLayout(const VertexElement* decl, u32 stride)
{
    SourceVertexLayout layout;
    if (!decl || stride == 0 || stride > 1024)
        return layout;

    layout.stride = stride;

    u32 count = 0;
    for (const VertexElement* elem = decl; elem->Stream != 0xFF; ++elem) {
        if (++count > XR_MAX_DECL_LENGTH)
            return SourceVertexLayout();
        if (elem->Stream != 0)
            return SourceVertexLayout();

        switch (elem->Usage) {
        case VS_POSITION:
            if (elem->UsageIndex != 0 || elem->Type != VF_FLOAT3)
                return SourceVertexLayout();
            {
                u8 ignored = SOURCE_TYPE_NONE;
                if (!AssignSlot(layout.positionOffset, ignored, *elem, stride))
                    return SourceVertexLayout();
            }
            break;

        case VS_NORMAL:
            if (elem->UsageIndex != 0 || (elem->Type != VF_COLOR && elem->Type != VF_FLOAT3))
                return SourceVertexLayout();
            if (!AssignSlot(layout.normalOffset, layout.normalType, *elem, stride))
                return SourceVertexLayout();
            break;

        case VS_TANGENT:
            if (elem->UsageIndex != 0 || elem->Type != VF_COLOR)
                return SourceVertexLayout();
            {
                u8 ignored = SOURCE_TYPE_NONE;
                if (!AssignSlot(layout.tangentOffset, ignored, *elem, stride))
                    return SourceVertexLayout();
            }
            break;

        case VS_BINORMAL:
            if (elem->UsageIndex != 0 || elem->Type != VF_COLOR)
                return SourceVertexLayout();
            {
                u8 ignored = SOURCE_TYPE_NONE;
                if (!AssignSlot(layout.binormalOffset, ignored, *elem, stride))
                    return SourceVertexLayout();
            }
            break;

        case VS_COLOR:
            if (elem->UsageIndex != 0 || elem->Type != VF_COLOR)
                return SourceVertexLayout();
            {
                u8 ignored = SOURCE_TYPE_NONE;
                if (!AssignSlot(layout.colorOffset, ignored, *elem, stride))
                    return SourceVertexLayout();
            }
            break;

        case VS_TEXCOORD:
            if (elem->UsageIndex == 0) {
                if (elem->Type != VF_SHORT2 && elem->Type != VF_SHORT4 && elem->Type != VF_FLOAT2)
                    return SourceVertexLayout();
                if (!AssignSlot(layout.uv0Offset, layout.uv0Type, *elem, stride))
                    return SourceVertexLayout();
            }
            else if (elem->UsageIndex == 1) {
                if (elem->Type != VF_SHORT2 && elem->Type != VF_FLOAT2)
                    return SourceVertexLayout();
                if (!AssignSlot(layout.uv1Offset, layout.uv1Type, *elem, stride))
                    return SourceVertexLayout();
            }
            else {
                return SourceVertexLayout();
            }
            break;

        default:
            return SourceVertexLayout();
        }
    }

    if (layout.positionOffset == SOURCE_ELEMENT_NONE)
        return SourceVertexLayout();

    if (layout.uv0Type == VF_SHORT2 &&
        (layout.tangentOffset == SOURCE_ELEMENT_NONE || layout.binormalOffset == SOURCE_ELEMENT_NONE))
        return SourceVertexLayout();

    if (layout.normalType == VF_FLOAT3 &&
        (layout.tangentOffset != SOURCE_ELEMENT_NONE || layout.binormalOffset != SOURCE_ELEMENT_NONE))
        return SourceVertexLayout();

    layout.valid = 1;
    return layout;
}

// ═══════════════════════════════════════════════════════════════════
//  UV UNPACKING HELPERS
// ═══════════════════════════════════════════════════════════════════

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
    return static_cast<float>(value) * (16.0f / 32768.0f);
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

u32 VertexConverter::PackNormal(const Fvector3& n)
{
    return u8_vec4(Fvector().set(n.x, n.y, n.z), 0);
}

// ═══════════════════════════════════════════════════════════════════
//  BATCH CONVERSION
// ═══════════════════════════════════════════════════════════════════

u32 VertexConverter::ConvertVertices(
    const void* srcData,
    u32 vertexCount,
    const SourceVertexLayout& layout,
    UnifiedVertex* dstData,
    Fvector3* outFloatNormals)
{
    if (!srcData || !dstData || vertexCount == 0 || !layout.IsValid())
        return 0;

    const u8* src = static_cast<const u8*>(srcData);

    for (u32 i = 0; i < vertexCount; ++i, src += layout.stride) {
        UnifiedVertex& dst = dstData[i];

        dst.position.x = ReadFloat(src, layout.positionOffset);
        dst.position.y = ReadFloat(src, layout.positionOffset + 4);
        dst.position.z = ReadFloat(src, layout.positionOffset + 8);

        float tangentAlpha = 0.0f;
        float binormalAlpha = 0.0f;

        if (layout.normalType == VF_FLOAT3) {
            Fvector3 n;
            n.x = ReadFloat(src, layout.normalOffset);
            n.y = ReadFloat(src, layout.normalOffset + 4);
            n.z = ReadFloat(src, layout.normalOffset + 8);

            Fvector3 t;
            Fvector3 b;
            ClusterDeriveBasis(n, t, b);

            dst.normal = PackNormal(ClusterNormalizeBasisInput(n));
            dst.tangent = PackNormal(t);
            dst.binormal = PackNormal(b);
            dst.flags = UNIFIED_VERTEX_FLAG_FLOAT_BASIS;
            if (outFloatNormals)
                outFloatNormals[i] = n;
        }
        else {
            dst.normal = (layout.normalOffset != SOURCE_ELEMENT_NONE)
                ? ReadU32(src, layout.normalOffset) : DEFAULT_NORMAL;
            dst.tangent = (layout.tangentOffset != SOURCE_ELEMENT_NONE)
                ? ReadU32(src, layout.tangentOffset) : DEFAULT_TANGENT;
            dst.binormal = (layout.binormalOffset != SOURCE_ELEMENT_NONE)
                ? ReadU32(src, layout.binormalOffset) : DEFAULT_BINORMAL;
            dst.flags = 0;
            if (layout.tangentOffset != SOURCE_ELEMENT_NONE)
                tangentAlpha = Fcolor(dst.tangent).a;
            if (layout.binormalOffset != SOURCE_ELEMENT_NONE)
                binormalAlpha = Fcolor(dst.binormal).a;
            if (outFloatNormals)
                outFloatNormals[i] = UnpackNormal(dst.normal);
        }

        switch (layout.uv0Type) {
        case VF_SHORT2:
            dst.texcoord0.x = UnpackS24_UV(ReadShort(src, layout.uv0Offset), tangentAlpha);
            dst.texcoord0.y = UnpackS24_UV(ReadShort(src, layout.uv0Offset + 2), binormalAlpha);
            break;
        case VF_SHORT4:
            dst.texcoord0.x = UnpackMuModelUV(ReadShort(src, layout.uv0Offset));
            dst.texcoord0.y = UnpackMuModelUV(ReadShort(src, layout.uv0Offset + 2));
            break;
        case VF_FLOAT2:
            dst.texcoord0.x = ReadFloat(src, layout.uv0Offset);
            dst.texcoord0.y = ReadFloat(src, layout.uv0Offset + 4);
            break;
        default:
            dst.texcoord0.set(0.0f, 0.0f);
            break;
        }

        switch (layout.uv1Type) {
        case VF_SHORT2:
            dst.texcoord1.x = UnpackLmapUV(ReadShort(src, layout.uv1Offset));
            dst.texcoord1.y = UnpackLmapUV(ReadShort(src, layout.uv1Offset + 2));
            break;
        case VF_FLOAT2:
            dst.texcoord1.x = ReadFloat(src, layout.uv1Offset);
            dst.texcoord1.y = ReadFloat(src, layout.uv1Offset + 4);
            break;
        default:
            dst.texcoord1.set(0.0f, 0.0f);
            break;
        }

        dst.color = (layout.colorOffset != SOURCE_ELEMENT_NONE)
            ? ReadU32(src, layout.colorOffset) : DEFAULT_COLOR;
    }

    return vertexCount;
}

bool SourceVertexLayout::IsValid() const
{
    return valid != 0;
}

bool SourceVertexLayout::HasFloatBasis() const
{
    return normalType == VF_FLOAT3;
}
} // namespace xray::render::fg::bindless
