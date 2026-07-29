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
    o.hpos = mul(m_WVP, float4(v.position.xyz, 1.0));
    o.color = v.color;
    o.tc = v.tc;
    return o;
}
