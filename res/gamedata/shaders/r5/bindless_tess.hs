// bindless_tess.hs — Hull shader (screen-space + distance LOD)
//
// IMPORTANT (Slang → SPIR-V / MoltenVK):
// Do NOT put user varyings in the patch-constant struct. Slang does not emit
// the SPIR-V Patch decoration on them, so HS/DS locations diverge and
// createGraphicsPipelines fails with VK_ERROR_INITIALIZATION_FAILED.
// PN control points are recomputed in the domain shader from the 3 CPs.
#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct VS_CONTROL_POINT
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
    float hemi : TEXCOORD6;
};

struct HS_CONTROL_POINT
{
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
    float hemi : TEXCOORD6;
};

struct HS_CONSTANT_DATA
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
};

// Screen-space edge length drives factor; capped for Metal tess temp buffers.
float ComputeEdgeFactor(float3 p0, float3 p1)
{
    float3 mid = 0.5 * (p0 + p1);
    float dist = max(length(mid - eye_position), 0.5);

    float edgeLen = length(p0 - p1);
    float pixels = (edgeLen / dist) * 720.0;

    float f = saturate((pixels - 4.0) / 20.0);
    float distFade = saturate(1.0 - (dist - 8.0) / 25.0);
    // integer partitioning — emit whole factors only
    return max(1.0, round(lerp(1.0, 4.0, f * f * distFade)));
}

HS_CONSTANT_DATA PatchConstant(InputPatch<VS_CONTROL_POINT, 3> patch)
{
    HS_CONSTANT_DATA o;

    uint matId = patch[0].materialID;
    MaterialData mat = g_Materials[matId];
    // Classic: only model mats with TessMethod; never terrain (BmmD TessMethod=0)
    bool doTess = (mat.tessMethod != TESS_METHOD_OFF) &&
                  ((mat.flags & MAT_FLAG_WATER) == 0) &&
                  ((mat.flags & MAT_FLAG_TERRAIN) == 0);

    float f0 = 1.0, f1 = 1.0, f2 = 1.0;
    if (doTess)
    {
        f0 = ComputeEdgeFactor(patch[1].worldPos, patch[2].worldPos);
        f1 = ComputeEdgeFactor(patch[2].worldPos, patch[0].worldPos);
        f2 = ComputeEdgeFactor(patch[0].worldPos, patch[1].worldPos);
    }

    o.Edges[0] = f0;
    o.Edges[1] = f1;
    o.Edges[2] = f2;
    o.Inside = max(1.0, round((f0 + f1 + f2) / 3.0));
    return o;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstant")]
[maxtessfactor(4.0)]
HS_CONTROL_POINT main(InputPatch<VS_CONTROL_POINT, 3> patch, uint id : SV_OutputControlPointID)
{
    HS_CONTROL_POINT o;
    o.worldPos = patch[id].worldPos;
    o.texcoord = patch[id].texcoord;
    o.normal = patch[id].normal;
    o.tangent = patch[id].tangent;
    o.bitangent = patch[id].bitangent;
    o.materialID = patch[id].materialID;
    o.hemi = patch[id].hemi;
    return o;
}
