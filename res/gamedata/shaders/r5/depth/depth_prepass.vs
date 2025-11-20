// xrRender/Shaders/depth/depth_prepass.vs.hlsl
// Phase 2.2: Depth Prepass Vertex Shader (stub for Phase 2)
//
// This shader will be implemented in Phase 2 for early-Z optimization
// For now, it's a placeholder to establish the directory structure

#include "../shared/common.h"

// ══════════════════════════════════════════════════════════
//  INPUT
// ══════════════════════════════════════════════════════════

struct VS_INPUT {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;  // For alpha testing in pixel shader
};

// ══════════════════════════════════════════════════════════
//  OUTPUT
// ══════════════════════════════════════════════════════════

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// ══════════════════════════════════════════════════════════
//  VERTEX SHADER (Minimal - depth only)
// ══════════════════════════════════════════════════════════

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;

    // Transform to clip space
    output.position = mul(float4(input.position, 1.0), m_WVP);

    // Pass through UVs for alpha testing
    output.texcoord = input.texcoord;

    return output;
}
