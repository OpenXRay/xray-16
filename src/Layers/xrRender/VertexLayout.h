#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
enum VertexFormat : u8
{
    VF_FLOAT1     = 0,
    VF_FLOAT2     = 1,
    VF_FLOAT3     = 2,
    VF_FLOAT4     = 3,
    VF_COLOR      = 4,
    VF_UBYTE4     = 5,
    VF_SHORT2     = 6,
    VF_SHORT4     = 7,
    VF_UBYTE4N    = 8,
    VF_SHORT2N    = 9,
    VF_SHORT4N    = 10,
    VF_USHORT2N   = 11,
    VF_USHORT4N   = 12,
    VF_UDEC3      = 13,
    VF_DEC3N      = 14,
    VF_FLOAT16_2  = 15,
    VF_FLOAT16_4  = 16,
    VF_UNUSED     = 17,
};

enum VertexSemantic : u8
{
    VS_POSITION     = 0,
    VS_BLENDWEIGHT  = 1,
    VS_BLENDINDICES = 2,
    VS_NORMAL       = 3,
    VS_PSIZE        = 4,
    VS_TEXCOORD     = 5,
    VS_TANGENT      = 6,
    VS_BINORMAL     = 7,
    VS_TESSFACTOR   = 8,
    VS_POSITIONT    = 9,
    VS_COLOR        = 10,
    VS_FOG          = 11,
    VS_DEPTH        = 12,
    VS_SAMPLE       = 13,
};

constexpr u32 XR_MAX_DECL_LENGTH = 64;

struct VertexElement
{
    u16 Stream;
    u16 Offset;
    u8  Type;
    u8  Method;
    u8  Usage;
    u8  UsageIndex;
};
static_assert(sizeof(VertexElement) == 8, "VertexElement OGF disk-format size");

#define XR_VERTEX_ELEMENT_END {0xFF, 0, ::xray::render::fg::VF_UNUSED, 0, 0, 0}

nvrhi::Format ToNvrhiFormat(u32 vertexFormat);
const char*   ToSemanticName(u32 vertexSemantic);
}
