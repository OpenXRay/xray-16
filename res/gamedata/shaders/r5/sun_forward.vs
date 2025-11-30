// sun_forward.vs
// Sun billboard vertex shader for Forward+ rendering
//
// Transforms sun quad vertices (already in world space) to clip space

#include "shared/common.h"

struct VS_INPUT {
    float4 position : POSITION;
    float4 color    : COLOR0;
    float2 tc       : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 hpos     : SV_POSITION;
    float4 color    : COLOR0;
    float2 tc       : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT v) {
    VS_OUTPUT o;

    // Transform world position to clip space
    // Vertices are already in world space (billboard computed on CPU)
    o.hpos = mul(m_WVP, float4(v.position.xyz, 1.0));

    // Place sun at far plane (reverse-Z: far = 0)
    o.hpos.z = o.hpos.w * 0.0001;

    // Pass through color and UV
    o.color = v.color;
    o.tc = v.tc;

    return o;
}
