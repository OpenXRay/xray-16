// xrRender/Shaders/forward/forward_base.vs.hlsl
// Phase 1.2: Forward+ Base Vertex Shader (simplified from gbuffer.vs)
#include "../shared/common.h"

// ══════════════════════════════════════════════════════════
//  INPUT
// ══════════════════════════════════════════════════════════

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

// ══════════════════════════════════════════════════════════
//  OUTPUT
// ══════════════════════════════════════════════════════════

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    float3 worldTangent : TEXCOORD3;
    float3 worldBinormal : TEXCOORD4;
};

// ══════════════════════════════════════════════════════════
//  VERTEX SHADER
// ══════════════════════════════════════════════════════════

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;

    // Transform position to clip space using X-Ray's m_WVP
    output.position = mul(float4(input.position, 1.0), m_WVP);

    // Transform position to world space using X-Ray's m_W
    float4 worldPos4 = float4(input.position, 1.0);
    output.worldPos.x = dot(worldPos4, m_W[0]);
    output.worldPos.y = dot(worldPos4, m_W[1]);
    output.worldPos.z = dot(worldPos4, m_W[2]);

    // Transform normal to world space (3x4 matrix)
    output.worldNormal.x = dot(input.normal, m_W[0].xyz);
    output.worldNormal.y = dot(input.normal, m_W[1].xyz);
    output.worldNormal.z = dot(input.normal, m_W[2].xyz);
    output.worldNormal = normalize(output.worldNormal);

    // Transform tangent space to world space
    output.worldTangent.x = dot(input.tangent, m_W[0].xyz);
    output.worldTangent.y = dot(input.tangent, m_W[1].xyz);
    output.worldTangent.z = dot(input.tangent, m_W[2].xyz);
    output.worldTangent = normalize(output.worldTangent);

    output.worldBinormal.x = dot(input.binormal, m_W[0].xyz);
    output.worldBinormal.y = dot(input.binormal, m_W[1].xyz);
    output.worldBinormal.z = dot(input.binormal, m_W[2].xyz);
    output.worldBinormal = normalize(output.worldBinormal);

    // Pass through texture coordinates
    output.texcoord = input.texcoord;

    return output;
}
