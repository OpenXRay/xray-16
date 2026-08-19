#include "shared/common.h"

struct VS_INPUT {
    float4 position : POSITION;
    float4 color    : COLOR0;
    float3 tc0      : TEXCOORD0;
    float3 tc1      : TEXCOORD1;
};

struct VS_OUTPUT {
    float4 hpos     : SV_POSITION;
    float4 color    : COLOR0;
    float3 tc0      : TEXCOORD0;
    float3 tc1      : TEXCOORD1;
    float  elev     : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT v) {
    VS_OUTPUT o;

    float4 tpos = float4(v.position.xyz * 1000.0, 1.0);
    o.hpos = mul(m_WVP, tpos);
    o.hpos.z = o.hpos.w * 0.0001;

    o.tc0 = v.tc0;
    o.tc1 = v.tc1;
    o.elev = normalize(v.position.xyz).y;
    o.color = v.color;
    return o;
}
