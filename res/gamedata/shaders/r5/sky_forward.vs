// xrRender/Shaders/forward/sky_forward.vs
// Sky dome vertex shader for Forward+ rendering
//
// Transforms sky box vertices to clip space with z=w (infinite far plane)
// Passes through cubemap texture coordinates for sky sampling

#include "shared/common.h"

struct VS_INPUT {
    float4 position : POSITION;
    float4 color    : COLOR0;
    float3 tc0      : TEXCOORD0;
    float3 tc1      : TEXCOORD1;
};

struct VS_OUTPUT {
    float4 hpos     : SV_POSITION;
    float4 color    : COLOR0;      // RGB = sky color, A = blend factor
    float3 tc0      : TEXCOORD0;   // Cubemap UV for sky0
    float3 tc1      : TEXCOORD1;   // Cubemap UV for sky1
};

VS_OUTPUT main(VS_INPUT v) {
    VS_OUTPUT o;

    // Scale position by 1000 for distant sky dome (vanilla magic number)
    // CRITICAL: Only scale xyz, NOT w! w must remain 1.0 for correct transformation
    float4 tpos = float4(v.position.xyz * 1000.0, 1.0);

    // Transform to clip space
    o.hpos = mul(m_WVP, tpos);

    // For reverse-Z: far plane is z=0, but we use small epsilon to avoid clipping
    // This places sky at maximum depth (behind all geometry)
    o.hpos.z = o.hpos.w * 0.0001;

    // Pass through texture coordinates (cubemap directions)
    o.tc0 = v.tc0;
    o.tc1 = v.tc1;

    // Pass through color with HDR scaling
    // Note: Vanilla uses tonemap texture here, we'll apply exposure in PS
    o.color = v.color;

    return o;
}
